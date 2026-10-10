// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "scannerdevice.h"
#include "ddlog.h"
#include <sane/saneopts.h>
#include <png.h>  // For PNG writing

#include <QFile>
#include <QApplication>   // For applicationDirPath
#include <QDir>   // For separator
#include <QProcess>   // For QProcess
#include <QPainter>
#include <QDateTime>
#include <QStandardPaths>
#include <QMetaType>
#include <QElapsedTimer>

using namespace DDLog;

#define ADD_TEST_DEVICE 0

// =================================================================
//  ScannerDevice Implementation (UI Thread)
// =================================================================

ScannerDevice::ScannerDevice(QObject *parent)
    : DeviceBase(parent)
{
    // Register custom types for cross-thread signal/slot connections
    qRegisterMetaType<ScannerDevice::ScanMode>("ScanMode");
    qRegisterMetaType<QList<int>>("QList<int>");
    qRegisterMetaType<QList<ScannerDevice::ScanMode>>("QList<ScannerDevice::ScanMode>");

    m_worker = new ScannerWorker();
    m_worker->moveToThread(&m_workerThread);

    // --- Connect signals from this (UI) thread to worker's slots ---
    connect(&m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &ScannerDevice::triggerOpenDevice, m_worker, &ScannerWorker::doOpenDevice);
    connect(this, &ScannerDevice::triggerCloseDevice, m_worker, &ScannerWorker::doCloseDevice);
    connect(this, &ScannerDevice::triggerStartScan, m_worker, &ScannerWorker::doStartScan);
    connect(this, &ScannerDevice::triggerCancelScan, m_worker, &ScannerWorker::doCancelScan);
    connect(this, &ScannerDevice::triggerGetAvailableDevices, m_worker, &ScannerWorker::doGetAvailableDevices);

    // --- Connect signals from worker back to this (UI) thread's ddslots ---
    connect(m_worker, &ScannerWorker::errorOccurred, this, &ScannerDevice::onWorkerError);
    connect(m_worker, &ScannerWorker::deviceOpened, this, &ScannerDevice::onDeviceOpened);
    connect(m_worker, &ScannerWorker::deviceClosed, this, &ScannerDevice::onDeviceClosed);
    connect(m_worker, &ScannerWorker::deviceDisconnected, this, [this](const QString &deviceName) {
        emit errorOccurred(tr("Scanner has been disconnected"));
        emit deviceUnavailable(deviceName);
    });
    connect(m_worker, &ScannerWorker::deviceUnavailable, this, &ScannerDevice::deviceUnavailable);
    connect(m_worker, &ScannerWorker::availableDevicesReady, this, &ScannerDevice::onAvailableDevicesReady);
    connect(m_worker, &ScannerWorker::captureCompleted, this, &ScannerDevice::onCaptureCompleted);
    connect(m_worker, &ScannerWorker::captureCompletedImage, this, &ScannerDevice::onCaptureCompletedImage);

    m_workerThread.start();
    qCInfo(app) << "Scanner worker thread started.";
}

ScannerDevice::~ScannerDevice()
{
    m_workerThread.quit();
    m_workerThread.wait();
    qCInfo(app) << "Scanner worker thread stopped.";
}

bool ScannerDevice::initialize()
{
#ifndef _WIN32
    SANE_Int version_code;
    SANE_Status status = sane_init(&version_code, NULL);
    if (status != SANE_STATUS_GOOD) {
        QString errorMsg = QString("Failed to initialize SANE: %1").arg(sane_strstatus(status));
        emit errorOccurred(errorMsg);
        return false;
    }
    setState(Initialized);
    qCInfo(app) << "SANE backend initialized successfully (version:" << version_code << ")";
    return true;
#else
    emit errorOccurred("SANE not available on this platform.");
    return false;
#endif
}

QStringList ScannerDevice::getAvailableDevices()
{
    // The SANE enumeration runs in the worker thread, see requestAvailableDevices(): it can
    // block for seconds on network backends, which must not happen on the UI thread.
    // Callers get the last list reported by the worker.
    return m_availableDevices;
}

void ScannerDevice::requestAvailableDevices()
{
    emit triggerGetAvailableDevices();
}

void ScannerDevice::onAvailableDevicesReady(const QStringList &deviceNames)
{
    m_availableDevices = deviceNames;
    emit availableDevicesReady(deviceNames);
}

bool ScannerDevice::openDevice(const QString &deviceName)
{
    m_currentDeviceName = deviceName;
    emit triggerOpenDevice(deviceName);
    return true; // Asynchronous operation, success is reported via signal
}

void ScannerDevice::closeDevice()
{
    emit triggerCloseDevice();
}

void ScannerDevice::startCapture()
{
    if (!m_deviceOpen) {
        if (m_currentDeviceName.isEmpty()) {
            // No open is in flight, so the device is not available any more (it was
            // disconnected): report it instead of queueing a scan that nothing will
            // ever start.
            emit errorOccurred(tr("Scanner has been disconnected"));
            return;
        }
        // Device is still opening, queue the scan request.
        qCDebug(app) << "Scan requested before device was open. Setting pending flag.";
        m_scanPending = true;
        // Optionally set a state to indicate connecting/opening
        setState(Connected); // Use 'Connected' as a transitional state
        return;
    }

    // Device availability is checked in the worker right before the scan
    // (ScannerWorker::reopenDevice): re-opening the device proves it is still there
    // and yields a handle that no device enumeration can have invalidated.
    if (m_isCapturing) {
        emit errorOccurred(tr("A scan is already in progress."));
        return;
    }
    
    QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) +
                       "/scan_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + ".png";
    
    m_isCapturing = true;
    setState(Capturing);
    emit triggerStartScan(tempPath, m_currentResolutionDPI, m_currentScanMode, m_currentColorMode, m_currentPaperSize);
}

void ScannerDevice::stopCapture()
{
    if (m_isCapturing) {
        emit triggerCancelScan();
    }
}

bool ScannerDevice::isCapturing() const
{
    return m_isCapturing;
}

void ScannerDevice::startScan(const QString &tempOutputFilePath)
{
    // This is now just a wrapper for startCapture
    startCapture();
    Q_UNUSED(tempOutputFilePath);
}

void ScannerDevice::cancelScan()
{
    stopCapture();
}

bool ScannerDevice::setScanMode(ScanMode mode)
{
    m_currentScanMode = mode;
    return true;
}

QList<ScannerDevice::ScanMode> ScannerDevice::getSupportedScanModes()
{
    return m_supportedScanModes;
}

bool ScannerDevice::setResolution(int dpi)
{
    m_currentResolutionDPI = dpi;
    return true;
}

int ScannerDevice::getResolution() const
{
    return m_currentResolutionDPI;
}

QList<int> ScannerDevice::getSupportedResolutions()
{
    return m_supportedResolutions;
}

// Paper size methods
void ScannerDevice::setPaperSize(PaperSize size)
{
    m_currentPaperSize = size;
}

ScannerDevice::PaperSize ScannerDevice::getPaperSize() const
{
    return m_currentPaperSize;
}

void ScannerDevice::setColorMode(ColorMode mode)
{
    m_currentColorMode = mode;
}

ScannerDevice::ColorMode ScannerDevice::getColorMode() const
{
    return m_currentColorMode;
}

QSizeF ScannerDevice::getPaperSizeDimensions(PaperSize size)
{
    switch(size) {
        case PAPER_SIZE_A3:
            return QSizeF(297, 420);  // A3: 297×420mm (standard ISO size)
        case PAPER_SIZE_A4:
            return QSizeF(210, 297);  // A4: 210×297mm (standard ISO size)
        case PAPER_SIZE_A5:
            return QSizeF(148, 210);  // A5: 148×210mm (standard ISO size)
        case PAPER_SIZE_A6:
            return QSizeF(105, 148);  // A6: 105×148mm (standard ISO size)
        case PAPER_SIZE_B4:
            return QSizeF(250, 353);  // B4: 250×353mm (standard ISO size)
        case PAPER_SIZE_B5:
            return QSizeF(176, 250);  // B5: 176×250mm (standard ISO size)
        case PAPER_SIZE_AUTO:
        default:
            return QSizeF(210, 297);  // Default to A4
    }
}

// --- SLOTS to handle results from worker ---

void ScannerDevice::onWorkerError(const QString &errorMessage)
{
    m_isCapturing = false;
    if (!m_deviceOpen) {
        // The error came from an open attempt: no open is in flight any more, so drop
        // the queued scan request together with the device name that marks it.
        m_scanPending = false;
        m_currentDeviceName.clear();
    }
    setState(m_deviceOpen ? Connected : Initialized);
    emit errorOccurred(errorMessage);
}

void ScannerDevice::onDeviceOpened(const QList<int> &resolutions, const QList<ScannerDevice::ScanMode> &modes)
{
    m_deviceOpen = true;
    m_supportedResolutions = resolutions;
    m_supportedScanModes = modes;

    // --- Validate and set a correct default resolution ---
    if (!resolutions.isEmpty()) {
        if (!resolutions.contains(m_currentResolutionDPI)) {
            qCWarning(app) << "Current default resolution" << m_currentResolutionDPI << "is not supported by the device.";
            // Try to find a sensible default, like 300, or fall back to the first available one.
            if (resolutions.contains(300)) {
                m_currentResolutionDPI = 300;
            } else {
                m_currentResolutionDPI = resolutions.first();
            }
            qCInfo(app) << "Setting new default resolution to" << m_currentResolutionDPI;
        }
    }

    // --- Validate and set a correct default scan mode ---
    if (!modes.isEmpty()) {
        if (!modes.contains(m_currentScanMode)) {
            qCWarning(app) << "Current default scan mode" << m_currentScanMode << "is not supported by the device.";
            qCWarning(app) << "  Supported modes:" << modes;
            m_currentScanMode = modes.first();
            qCInfo(app) << "Setting new default scan mode to" << m_currentScanMode;
        }
    }
    
    // Defaults are now set synchronously within doStartScan
    setState(Connected);
    emit deviceOpened();

    // If a scan was requested while the device was opening, start it now.
    if (m_scanPending) {
        qCDebug(app) << "Device is now open, processing pending scan request.";
        m_scanPending = false;
        startCapture();
    }
}

void ScannerDevice::onDeviceClosed()
{
    m_deviceOpen = false;
    m_isCapturing = false;
    m_scanPending = false;
    m_currentDeviceName.clear();
    m_supportedResolutions.clear();
    m_supportedScanModes.clear();
    setState(Initialized);
    emit deviceClosed();
}

void ScannerDevice::onCaptureCompleted(const QString &filePath)
{
    m_isCapturing = false;
    setState(Connected);
    
    QImage scannedImage;
    if (scannedImage.load(filePath)) {
        emit imageCaptured(scannedImage);
    } else {
        emit errorOccurred(tr("Failed to load scanned image from temp file."));
    }
    QFile::remove(filePath);
}


void ScannerDevice::onCaptureCompletedImage(const QImage &image)
{
    m_isCapturing = false;
    setState(Connected);

    emit imageCaptured(image);
}

// =================================================================
//  ScannerWorker Implementation (Worker Thread)
// =================================================================

#ifndef _WIN32
struct ScannerWorker::Image
{
    unsigned char *data;
    int width;
    int height;
    int x, y;
    int line_buffer_size;
};
#endif

ScannerWorker::ScannerWorker() : QObject(nullptr)
{
}

ScannerWorker::~ScannerWorker()
{
    if (m_deviceOpen) {
        doCloseDevice();
    }
    sane_exit();
    qCInfo(app) << "SANE backend exited from worker destructor.";
}


void ScannerWorker::doGetAvailableDevices()
{
#ifndef _WIN32
    const SANE_Device **device_list = nullptr;
    QStringList deviceNames;

    SANE_Status status = sane_get_devices(&device_list, SANE_FALSE);
    if (status != SANE_STATUS_GOOD) {
        emit errorOccurred(tr("Failed to get device list: %1").arg(sane_strstatus(status)));
        emit availableDevicesReady(deviceNames);
        return;
    }

    if (device_list) {
        for (int i = 0; device_list[i] != nullptr; ++i) {
            if (device_list[i]->name) {
                deviceNames.append(QString::fromUtf8(device_list[i]->name));
            }
        }
    }

    if (deviceNames.isEmpty() && ADD_TEST_DEVICE) {
        deviceNames.append("test:0");
    }

    emit availableDevicesReady(deviceNames);
#else
    emit availableDevicesReady(ADD_TEST_DEVICE ? QStringList() << "test:0" : QStringList());
#endif
}

void ScannerWorker::doOpenDevice(const QString &deviceName)
{
#ifndef _WIN32
    if (deviceName.startsWith("test:")) {
        m_usingTestDevice = true;
        m_deviceOpen = true;
        qCDebug(app) << "Worker: Opened virtual test device.";
        emit deviceOpened({100, 200, 300, 600}, {ScannerDevice::SCAN_MODE_FLATBED});
        return;
    }
    
    if (m_deviceOpen) {
        doCloseDevice();
    }

    m_deviceName = deviceName;

    QElapsedTimer openTimer;
    openTimer.start();

    QByteArray deviceNameBytes = deviceName.toUtf8();
    SANE_Status status = sane_open(deviceNameBytes.constData(), &m_device);
    const qint64 openMs = openTimer.elapsed();
    
    if (status != SANE_STATUS_GOOD) {
        emit errorOccurred(tr("Failed to open SANE device '%1': %2").arg(deviceName).arg(sane_strstatus(status)));
        m_device = nullptr;
        m_deviceOpen = false;
        // A device that cannot be opened is dropped from the device list, unless it is
        // only temporarily unusable (busy) or present but not accessible.
        if (status != SANE_STATUS_DEVICE_BUSY && status != SANE_STATUS_ACCESS_DENIED) {
            emit deviceUnavailable(deviceName);
        }
        return;
    }

    m_deviceOpen = true;
    openTimer.restart();
    updateSupportedOptions();
    qCInfo(app) << "Worker: Device" << deviceName << "opened in" << openMs << "ms, options read in" << openTimer.elapsed() << "ms";
    qCDebug(app) << "Worker: Device" << deviceName << "opened successfully.";
#else
    m_usingTestDevice = true;
    m_deviceOpen = true;
    emit deviceOpened({100, 200, 300, 600}, {ScannerDevice::SCAN_MODE_FLATBED});
#endif
}

void ScannerWorker::doCloseDevice()
{
#ifndef _WIN32
    if (m_deviceOpen && m_device) {
        sane_close(m_device);
    }
    m_device = nullptr;
    m_deviceOpen = false;
    m_usingTestDevice = false;
    m_deviceName.clear();
    qCDebug(app) << "Worker: Closed SANE device.";
#else
    m_deviceOpen = false;
    m_usingTestDevice = false;
#endif
    emit deviceClosed();
}

#ifndef _WIN32
// Re-open the device before every scan: re-opening is both the availability check
// and the only way to be sure the handle is usable, because enumerating devices
// rebuilds the device cache of some backends (the deepin cloud scan backend does)
// and thereby invalidates every handle obtained before the enumeration.
bool ScannerWorker::reopenDevice()
{
    if (m_deviceOpen && m_device) {
        sane_close(m_device);
    }
    m_device = nullptr;
    m_deviceOpen = false;

    QElapsedTimer openTimer;
    openTimer.start();

    QByteArray deviceNameBytes = m_deviceName.toUtf8();
    SANE_Status status = sane_open(deviceNameBytes.constData(), &m_device);
    if (status != SANE_STATUS_GOOD) {
        qCWarning(app) << "Worker: Failed to re-open device" << m_deviceName << "after" << openTimer.elapsed() << "ms:" << sane_strstatus(status);
        m_device = nullptr;
        m_deviceOpen = false;
        return false;
    }

    m_deviceOpen = true;
    qCInfo(app) << "Worker: Device" << m_deviceName << "re-opened in" << openTimer.elapsed() << "ms";
    return true;
}

void ScannerWorker::reportDeviceDisconnected()
{
    // The name has to be read before closing, doCloseDevice() clears it.
    const QString deviceName = m_deviceName;
    doCloseDevice();
    emit deviceDisconnected(deviceName);
}

// Report a failed scan: a device that is no longer enumerated, or that cannot be
// re-opened, has been disconnected; any other failure keeps the status the backend
// reported. The handle is released before the enumeration below, because that
// enumeration invalidates it (see reopenDevice()).
void ScannerWorker::reportScanFailure(const QString &errorMessage)
{
    if (m_deviceOpen && m_device) {
        sane_close(m_device);
    }
    m_device = nullptr;
    m_deviceOpen = false;

    const SANE_Device **deviceList = nullptr;
    bool devicePresent = true; // a failed enumeration is not a disconnect
    if (sane_get_devices(&deviceList, SANE_FALSE) == SANE_STATUS_GOOD) {
        devicePresent = false;
        for (int i = 0; deviceList && deviceList[i] != nullptr; ++i) {
            if (deviceList[i]->name && m_deviceName == QString::fromUtf8(deviceList[i]->name)) {
                devicePresent = true;
                break;
            }
        }
    }

    if (!devicePresent) {
        qCWarning(app) << "Worker: Device" << m_deviceName << "is no longer available.";
    } else if (reopenDevice()) {
        emit errorOccurred(errorMessage);
        return;
    }

    reportDeviceDisconnected();
}
#endif

void ScannerWorker::doStartScan(const QString &tempOutputFilePath, int dpi, ScannerDevice::ScanMode mode, ScannerDevice::ColorMode colorMode, ScannerDevice::PaperSize paperSize)
{
#ifndef _WIN32
    if (m_usingTestDevice) {
        generateTestImage(tempOutputFilePath);
        emit captureCompleted(tempOutputFilePath);
        return;
    }

    if (!m_deviceOpen || !m_device) {
        emit errorOccurred(tr("Scanner not opened"));
        return;
    }

    if (!reopenDevice()) {
        reportDeviceDisconnected();
        return;
    }

    QElapsedTimer scanTimer;
    scanTimer.start();

    doSetScanMode(mode);
    doSetColorMode(colorMode);
    doSetResolution(dpi);

    if (mode != ScannerDevice::SCAN_MODE_FLATBED) {
        QSizeF pageSize = ScannerDevice::getPaperSizeDimensions(paperSize);
        doSetPageSize(pageSize.width(), pageSize.height());
    }
    
    m_scanCancelled = false;

    const qint64 optionsMs = scanTimer.elapsed();
    scanTimer.restart();

    // Start the scan
    SANE_Status status = sane_start(m_device);
    const qint64 startMs = scanTimer.elapsed();
    scanTimer.restart();
    if (status != SANE_STATUS_GOOD) {
        reportScanFailure(tr("Failed to start scan: %1").arg(sane_strstatus(status)));
        return;
    }

    QByteArray tempPathBytes = tempOutputFilePath.toLocal8Bit();
    FILE *ofp = fopen(tempPathBytes.constData(), "wb");
    if (!ofp) {
        sane_cancel(m_device);
        emit errorOccurred(tr("Failed to open temporary output file."));
        return;
    }

    QImage scannedImage;
    status = scan_it(ofp, scannedImage); // The blocking call
    const qint64 readMs = scanTimer.elapsed();
    fclose(ofp);

    qCInfo(app) << "Worker: scan timing: options" << optionsMs << "ms, sane_start" << startMs << "ms, read" << readMs << "ms";

    if (m_scanCancelled) {
        QFile::remove(tempOutputFilePath);
        // Don't emit error, as it's a user action
        qCDebug(app) << "Scan was cancelled.";
        // a cancel implies the device is no longer capturing
        emit errorOccurred(tr("Scan canceled by user"));
        return;
    }

    if (status != SANE_STATUS_GOOD && status != SANE_STATUS_EOF) {
        QFile::remove(tempOutputFilePath);
        reportScanFailure(tr("Scan failed during read: %1").arg(sane_strstatus(status)));
    } else if (!scannedImage.isNull()) {
        // The page is already an image, the temp file stayed empty.
        QFile::remove(tempOutputFilePath);
        emit captureCompletedImage(scannedImage);
    } else {
        emit captureCompleted(tempOutputFilePath);
    }
#else
    generateTestImage(tempOutputFilePath);
    emit captureCompleted(tempOutputFilePath);
#endif
}


void ScannerWorker::doCancelScan()
{
#ifndef _WIN32
    if (m_device && !m_scanCancelled) {
        m_scanCancelled = true;
        sane_cancel(m_device);
        qCDebug(app) << "Worker: Sent sane_cancel().";
    }
#endif
}

void ScannerWorker::doSetResolution(int dpi)
{
#ifndef _WIN32
    if (!m_device) return;

    int opt_index = 0;
    const SANE_Option_Descriptor *opt_desc = nullptr;
    while ((opt_desc = sane_get_option_descriptor(m_device, opt_index))) {
        if (opt_desc && opt_desc->name && strcmp(opt_desc->name, SANE_NAME_SCAN_RESOLUTION) == 0) {
            if (opt_desc->cap & SANE_CAP_INACTIVE) {
                qCWarning(app) << "Resolution option is INACTIVE";
                return;
            }

            if (!(opt_desc->cap & SANE_CAP_SOFT_SELECT)) {
                qCWarning(app) << "Resolution option is not settable";
                return;
            }

            SANE_Status status;
            SANE_Int info;
            int value_to_set = dpi;
            status = sane_control_option(m_device, opt_index, SANE_ACTION_SET_VALUE, &value_to_set, &info);
            if (status != SANE_STATUS_GOOD) {
                qCWarning(app) << "Failed to set resolution:" << sane_strstatus(status);
            }
            return;
        }
        opt_index++;
    }
    qCWarning(app) << "Could not find resolution option";
#endif
}

void ScannerWorker::doSetScanMode(ScannerDevice::ScanMode mode)
{
#ifndef _WIN32
    if (!m_device) return;
    QString sourceValue = "Flatbed";
    if (mode == ScannerDevice::SCAN_MODE_ADF_SIMPLEX) sourceValue = "ADF Front";
    if (mode == ScannerDevice::SCAN_MODE_ADF_DUPLEX) sourceValue = "ADF Duplex";
    
    int opt_index = 0;
    const SANE_Option_Descriptor *opt_desc = nullptr;
    while ((opt_desc = sane_get_option_descriptor(m_device, opt_index++))) {
        if (opt_desc && opt_desc->name && strcmp(opt_desc->name, SANE_NAME_SCAN_SOURCE) == 0) {
            if (opt_desc->cap & SANE_CAP_INACTIVE) {
                qCWarning(app) << "Source option is INACTIVE";
                return;
            }

            if (!(opt_desc->cap & SANE_CAP_SOFT_SELECT)) {
                qCWarning(app) << "Source option is not settable";
                return;
            }

            QByteArray sourceBytes = sourceValue.toUtf8();
            SANE_Int info = 0;
            SANE_Status status = sane_control_option(m_device, opt_index - 1, SANE_ACTION_SET_VALUE, 
                                                      (void*)sourceBytes.constData(), &info);
            if (status != SANE_STATUS_GOOD) {
                qCWarning(app) << "Failed to set scan source:" << sane_strstatus(status);
            }
            return;
        }
    }
    qCWarning(app) << "Could not find scan source option";
#endif
}

void ScannerWorker::doSetColorMode(ScannerDevice::ColorMode colorMode)
{
#ifndef _WIN32
    if (!m_device) return;

    QString modeValue = "Color";
    if (colorMode == ScannerDevice::COLOR_MODE_GRAYSCALE) modeValue = "Gray";
    if (colorMode == ScannerDevice::COLOR_MODE_LINEART) modeValue = "Lineart";

    qCInfo(app) << "=== Setting Color Mode ===";
    qCInfo(app) << "Target mode:" << modeValue;

    int opt_index = 0;
    const SANE_Option_Descriptor *opt_desc = nullptr;
    while ((opt_desc = sane_get_option_descriptor(m_device, opt_index++))) {
        if (opt_desc && opt_desc->name && strcmp(opt_desc->name, SANE_NAME_SCAN_MODE) == 0) {
            qCInfo(app) << "Found color mode option at index" << opt_index - 1;
            qCInfo(app) << "  Option cap:" << opt_desc->cap;
            qCInfo(app) << "  Option type:" << opt_desc->type;

            if (opt_desc->cap & SANE_CAP_INACTIVE) {
                qCWarning(app) << "  Color mode option is INACTIVE!";
                return;
            }

            QByteArray modeBytes = modeValue.toUtf8();
            SANE_Int info = 0;
            SANE_Status status = sane_control_option(m_device, opt_index - 1, SANE_ACTION_SET_VALUE, 
                                                      (void*)modeBytes.constData(), &info);
            if (status != SANE_STATUS_GOOD) {
                qCWarning(app) << "  Failed to set color mode:" << sane_strstatus(status);
                if (opt_desc->constraint_type == SANE_CONSTRAINT_STRING_LIST) {
                    qCWarning(app) << "  Available color modes:";
                    for (int i = 0; opt_desc->constraint.string_list[i] != NULL; ++i) {
                        qCWarning(app) << "    - [" << opt_desc->constraint.string_list[i];
                    }
                }
            } else {
                qCInfo(app) << "  Color mode set to" << modeValue << "(info:" << info << ")";
            }
            return;
        }
    }
    qCWarning(app) << "Could not find color mode option (" << SANE_NAME_SCAN_MODE << ").";
#endif
}

void ScannerWorker::doSetPageSize(double widthMM, double heightMM)
{
#ifndef _WIN32
    if (!m_device) return;

    qCInfo(app) << "=== Setting Page Size ===";
    qCInfo(app) << "Target: width=" << widthMM << "mm, height=" << heightMM << "mm";

    int opt_index = 0;
    const SANE_Option_Descriptor *opt_desc = nullptr;
    bool widthSet = false;
    bool heightSet = false;

    while ((opt_desc = sane_get_option_descriptor(m_device, opt_index++))) {
        if (opt_desc && opt_desc->name) {
            if (strcmp(opt_desc->name, "page-width") == 0 || strcmp(opt_desc->name, "tl-x") == 0) {
                if (opt_desc->cap & SANE_CAP_INACTIVE) {
                    qCWarning(app) << "  Page width option is INACTIVE";
                } else {
                    SANE_Word value = static_cast<SANE_Word>(widthMM);
                    SANE_Int info = 1;
                    SANE_Status status = sane_control_option(m_device, opt_index - 1, SANE_ACTION_SET_VALUE, &value, &info);
                    if (status != SANE_STATUS_GOOD) {
                        qCWarning(app) << "  Failed to set page-width:" << sane_strstatus(status);
                    } else {
                        qCInfo(app) << "  Page width set to" << value << "mm (info:" << info << ")";
                        widthSet = true;
                    }
                }
            }
            if (strcmp(opt_desc->name, "page-height") == 0 || strcmp(opt_desc->name, "tl-y") == 0 || strcmp(opt_desc->name, "br-y") == 0) {
                if (opt_desc->cap & SANE_CAP_INACTIVE) {
                    qCWarning(app) << "  Page height option is INACTIVE";
                } else {
                    SANE_Word value = static_cast<SANE_Word>(heightMM);
                    SANE_Int info = 1;
                    SANE_Status status = sane_control_option(m_device, opt_index - 1, SANE_ACTION_SET_VALUE, &value, &info);
                    if (status != SANE_STATUS_GOOD) {
                        qCWarning(app) << "  Failed to set page-height:" << sane_strstatus(status);
                    } else {
                        qCInfo(app) << "  Page height set to" << value << "mm (info:" << info << ")";
                        heightSet = true;
                    }
                }
            }
        }
    }

    if (!widthSet) qCWarning(app) << "Could not find page-width option!";
    if (!heightSet) qCWarning(app) << "Could not find page-height option!";
    qCInfo(app) << "=== End Page Size ===";
#endif
}


#ifndef _WIN32
// The QImage format that matches the SANE frame, or Format_Invalid when the page has to go
// through the PNG temp file: lineart is bit packed and JPEG/G42D are not raw pixels.
static QImage::Format imageFormatFor(const SANE_Parameters &parm)
{
    if (parm.format == SANE_FRAME_GRAY) {
        if (parm.depth == 8) {
            return QImage::Format_Grayscale8;
        }
        if (parm.depth == 16) {
            return QImage::Format_Grayscale16;
        }
        return QImage::Format_Invalid;
    }
    if (parm.format == SANE_FRAME_RGB && parm.depth == 8) {
        return QImage::Format_RGB888;
    }
    return QImage::Format_Invalid;
}

// Reads the page. When the frame format can be handed over as an image, `image` is filled
// and nothing is written to `ofp`; otherwise the page is written there as PNG, which is
// also what happens when the backend does not tell the line count in advance.
SANE_Status ScannerWorker::scan_it(FILE *ofp, QImage &image)
{
    SANE_Parameters parm;
    SANE_Status status;
    int len;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;

    status = sane_get_parameters(m_device, &parm);
    if (status != SANE_STATUS_GOOD) {
        return status;
    }

    unsigned char *buffer = new unsigned char[parm.bytes_per_line];

    QImage::Format imageFormat = QImage::Format_Invalid;
    if (parm.lines > 0) {
        imageFormat = imageFormatFor(parm);
        if (imageFormat != QImage::Format_Invalid) {
            image = QImage(parm.pixels_per_line, parm.lines, imageFormat);
            if (image.isNull()) {
                imageFormat = QImage::Format_Invalid;
            }
        }
    }

    const bool writePng = imageFormat == QImage::Format_Invalid;
    if (writePng) {
        // Dummy PNG write header function.
        // Replace with your actual implementation if you have one.
        int bit_depth = (parm.depth == 1) ? 8 : parm.depth;
        png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        info_ptr = png_create_info_struct(png_ptr);
        png_init_io(png_ptr, ofp);
        png_set_IHDR(png_ptr, info_ptr, parm.pixels_per_line, parm.lines, bit_depth,
                     parm.format == SANE_FRAME_RGB ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_GRAY,
                     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        png_write_info(png_ptr, info_ptr);
        // End dummy png header
    }

    QElapsedTimer readTimer;
    readTimer.start();
    int linesRead = 0;

    do {
        if (m_scanCancelled) {
            status = SANE_STATUS_CANCELLED;
            break;
        }

        if (!writePng && linesRead >= parm.lines) {
            break;   // the image is complete
        }

        status = sane_read(m_device, buffer, parm.bytes_per_line, &len);
        
        if (status == SANE_STATUS_GOOD) {
            if (writePng) {
                png_write_row(png_ptr, buffer);
            } else {
                memcpy(image.scanLine(linesRead), buffer, qMin(len, image.bytesPerLine()));
            }
            linesRead++;
        }
    } while (status == SANE_STATUS_GOOD);

    if (!writePng && linesRead > 0 && linesRead < parm.lines) {
        // The backend stopped early, keep only the lines that arrived.
        image = image.copy(0, 0, image.width(), linesRead);
    }

    qCInfo(app) << "Worker: read" << linesRead << "of" << parm.lines << "lines in" << readTimer.elapsed() << "ms"
                << (writePng ? "(via PNG temp file)" : "(direct image)");

    if (writePng) {
        png_write_end(png_ptr, info_ptr);
        png_destroy_write_struct(&png_ptr, &info_ptr);
    }
    delete[] buffer;

    if (status != SANE_STATUS_EOF) {
        // If not a normal end-of-scan, cancel to be safe
        sane_cancel(m_device);
        return status;
    }

    return SANE_STATUS_GOOD;
}

void ScannerWorker::updateSupportedOptions() {
    QList<int> resolutions;
    QList<ScannerDevice::ScanMode> modes;

    if (!m_device) {
        emit deviceOpened(resolutions, modes);
        return;
    }

    int opt_index = 0;
    const SANE_Option_Descriptor *opt;

    qCInfo(app) << "=== Scanner Device Options Discovery ===";
    
    while ((opt = sane_get_option_descriptor(m_device, opt_index++))) {
        if (opt->name && strcmp(opt->name, SANE_NAME_SCAN_RESOLUTION) == 0) {
            qCInfo(app) << "Found resolution option, cap:" << opt->cap << "type:" << opt->type;
            if (opt->constraint_type == SANE_CONSTRAINT_RANGE) {
                qCInfo(app) << "  Resolution range:" << opt->constraint.range->min << "-" << opt->constraint.range->max;
                for (int i = opt->constraint.range->min; i <= opt->constraint.range->max; i += 100) {
                   resolutions.append(i);
                }
            } else if (opt->constraint_type == SANE_CONSTRAINT_WORD_LIST) {
                qCInfo(app) << "  Resolution list:" << opt->constraint.word_list[0] << "values";
                for (int i = 1; i <= opt->constraint.word_list[0]; ++i) {
                    resolutions.append(opt->constraint.word_list[i]);
                }
            }
        }
        if (opt->name && strcmp(opt->name, SANE_NAME_SCAN_SOURCE) == 0) {
            qCInfo(app) << "Found source option, cap:" << opt->cap << "type:" << opt->type;
            if(opt->constraint_type == SANE_CONSTRAINT_STRING_LIST){
                 qCInfo(app) << "  Available sources:";
                 for (int i = 0; opt->constraint.string_list[i] != NULL; ++i) {
                     QString mode(opt->constraint.string_list[i]);
                     qCInfo(app) << "    [" << i << "]" << mode;
                     if(mode.compare("Flatbed", Qt::CaseInsensitive) == 0) modes.append(ScannerDevice::SCAN_MODE_FLATBED);
                     else if(mode.compare("ADF", Qt::CaseInsensitive) == 0 || 
                             mode.contains("ADF Front", Qt::CaseInsensitive) ||
                             mode.compare("Feeder", Qt::CaseInsensitive) == 0 ||
                             mode.compare("Auto", Qt::CaseInsensitive) == 0) {
                         modes.append(ScannerDevice::SCAN_MODE_ADF_SIMPLEX);
                         qCInfo(app) << "      -> Mapped to ADF_SIMPLEX";
                     }
                     else if(mode.compare("ADF Duplex", Qt::CaseInsensitive) == 0 ||
                             mode.contains("ADF Back", Qt::CaseInsensitive) ||
                             mode.compare("Duplex", Qt::CaseInsensitive) == 0) {
                         modes.append(ScannerDevice::SCAN_MODE_ADF_DUPLEX);
                         qCInfo(app) << "      -> Mapped to ADF_DUPLEX";
                     }
                 }
            }
        }
        if (opt->name && strcmp(opt->name, SANE_NAME_SCAN_MODE) == 0) {
            qCInfo(app) << "Found scan mode option, cap:" << opt->cap;
            if(opt->constraint_type == SANE_CONSTRAINT_STRING_LIST){
                 qCInfo(app) << "  Available color modes:";
                 for (int i = 0; opt->constraint.string_list[i] != NULL; ++i) {
                     qCInfo(app) << "    [" << i << "]" << opt->constraint.string_list[i];
                 }
            }
        }
        if (opt->name && (strcmp(opt->name, "page-width") == 0 || strcmp(opt->name, "page-height") == 0)) {
            qCInfo(app) << "Found" << opt->name << "option, cap:" << opt->cap;
        }
    }

    qCInfo(app) << "=== End Options Discovery ===";
    
    if (resolutions.isEmpty()) resolutions << 100 << 300 << 600;
    if (modes.isEmpty()) modes << ScannerDevice::SCAN_MODE_FLATBED;

    emit deviceOpened(resolutions, modes);
}
#endif

void ScannerWorker::generateTestImage(const QString &outputPath)
{
    QImage image(600, 800, QImage::Format_RGB32);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 50));
    painter.drawText(image.rect(), Qt::AlignCenter, "Test Scan");
    painter.end();
    image.save(outputPath, "PNG");
    qCDebug(app) << "Generated test image at:" << outputPath;
}
