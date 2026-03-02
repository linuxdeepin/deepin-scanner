// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
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

    // --- Connect signals from worker back to this (UI) thread's ddslots ---
    connect(m_worker, &ScannerWorker::errorOccurred, this, &ScannerDevice::onWorkerError);
    connect(m_worker, &ScannerWorker::deviceOpened, this, &ScannerDevice::onDeviceOpened);
    connect(m_worker, &ScannerWorker::deviceClosed, this, &ScannerDevice::onDeviceClosed);
    connect(m_worker, &ScannerWorker::captureCompleted, this, &ScannerDevice::onCaptureCompleted);
    connect(m_worker, &ScannerWorker::scanProgress, this, &ScannerDevice::scanProgress); // Forward signal

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
    // This is synchronous for now, but could be moved to the worker if it proves slow.
#ifndef _WIN32
    const SANE_Device **device_list;
    QStringList deviceNames;

    SANE_Status status = sane_get_devices(&device_list, SANE_FALSE);
    if (status != SANE_STATUS_GOOD) {
        emit errorOccurred(tr("Failed to get device list: %1").arg(sane_strstatus(status)));
        return deviceNames;
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

    return deviceNames;
#else
    if (ADD_TEST_DEVICE) {
        return QStringList() << "test:0";
    }
    return QStringList();
#endif
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
        // Device is still opening, queue the scan request.
        qCDebug(app) << "Scan requested before device was open. Setting pending flag.";
        m_scanPending = true;
        // Optionally set a state to indicate connecting/opening
        setState(Connected); // Use 'Connected' as a transitional state
        return;
    }

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
    // Set SANE_DEBUG env vars for more info
    qputenv("SANE_DEBUG_DLL", "2");
    qputenv("SANE_DEBUG_NET", "2");
    qputenv("SANE_DEBUG_HPAIO", "2");
    qputenv("SANE_DEBUG_USB", "2");
}

ScannerWorker::~ScannerWorker()
{
    if (m_deviceOpen) {
        doCloseDevice();
    }
    sane_exit();
    qCInfo(app) << "SANE backend exited from worker destructor.";
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

    QByteArray deviceNameBytes = deviceName.toUtf8();
    SANE_Status status = sane_open(deviceNameBytes.constData(), &m_device);
    
    if (status != SANE_STATUS_GOOD) {
        emit errorOccurred(tr("Failed to open SANE device '%1': %2").arg(deviceName).arg(sane_strstatus(status)));
        m_device = nullptr;
        m_deviceOpen = false;
        return;
    }

    m_deviceOpen = true;
    updateSupportedOptions();
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
    qCDebug(app) << "Worker: Closed SANE device.";
#else
    m_deviceOpen = false;
    m_usingTestDevice = false;
#endif
    emit deviceClosed();
}

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

    doSetScanMode(mode);
    doSetColorMode(colorMode);
    doSetResolution(dpi);

    if (mode != ScannerDevice::SCAN_MODE_FLATBED) {
        QSizeF pageSize = ScannerDevice::getPaperSizeDimensions(paperSize);
        doSetPageSize(pageSize.width(), pageSize.height());
    }
    
    m_scanCancelled = false;

    // Start the scan
    SANE_Status status = sane_start(m_device);
    if (status != SANE_STATUS_GOOD) {
        emit errorOccurred(tr("Failed to start scan: %1").arg(sane_strstatus(status)));
        return;
    }

    QByteArray tempPathBytes = tempOutputFilePath.toLocal8Bit();
    FILE *ofp = fopen(tempPathBytes.constData(), "wb");
    if (!ofp) {
        sane_cancel(m_device);
        emit errorOccurred(tr("Failed to open temporary output file."));
        return;
    }

    status = scan_it(ofp); // The blocking call
    fclose(ofp);

    if (m_scanCancelled) {
        QFile::remove(tempOutputFilePath);
        // Don't emit error, as it's a user action
        qCDebug(app) << "Scan was cancelled.";
        // a cancel implies the device is no longer capturing
        emit errorOccurred(tr("Scan canceled by user"));
        return;
    }

    if (status != SANE_STATUS_GOOD && status != SANE_STATUS_EOF) {
        emit errorOccurred(tr("Scan failed during read: %1").arg(sane_strstatus(status)));
        QFile::remove(tempOutputFilePath);
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
SANE_Status ScannerWorker::scan_it(FILE *ofp)
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

    int total_lines = parm.lines > 0 ? parm.lines : 1000; // Guess if not provided
    int lines_read = 0;

    do {
        if (m_scanCancelled) {
            status = SANE_STATUS_CANCELLED;
            break;
        }

        status = sane_read(m_device, buffer, parm.bytes_per_line, &len);
        
        if (status == SANE_STATUS_GOOD) {
            png_write_row(png_ptr, buffer);
            lines_read++;
            if (parm.lines > 0) {
                emit scanProgress((lines_read * 100) / total_lines);
            }
        }
    } while (status == SANE_STATUS_GOOD);

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
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
