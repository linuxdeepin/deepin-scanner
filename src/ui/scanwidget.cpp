// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "scanwidget.h"
#include "ddlog.h"

#include <ofd/ofd_writer.h>

#include <QVBoxLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QPlainTextEdit>
#include <QDesktopServices>
#include <QPrinter>
#include <QTransform>
#include <QPainter>
#include <QMap>
#include <QFutureWatcher>
#include <QElapsedTimer>
#include <QtConcurrent>
#include <qmath.h>
#include <limits>

#include <DFrame>
#include <DPushButton>
#include <DIconButton>
#include <DLabel>
#include <DMessageManager>

using namespace DDLog;

static const QStringList FORMATS = { "PNG", "JPEG", "BMP", "TIFF", "PDF", "OFD" };

// fixed width for label and combo box
static const int SETTING_LABEL_WIDTH = 80;
static const int SETTING_COMBO_WIDTH = 160;

ScanWidget::ScanWidget(QWidget *parent) : QWidget(parent),
                                          m_isScanner(false),
                                          m_imageSettings(new ImageSettings())
{
    setupUI();
}

ScanWidget::~ScanWidget()
{
    m_imageSettings.reset();
}

void ScanWidget::setupUI()
{
    DFrame *mainFrame = new DFrame();
    QHBoxLayout *mainLayout = new QHBoxLayout(mainFrame);

    // Preview area
    DFrame *previewArea = new DFrame();
    previewArea->setMinimumSize(300, 200);
    QVBoxLayout *previewLayout = new QVBoxLayout(previewArea);

    m_previewLabel = new DLabel();
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    previewLayout->addWidget(m_previewLabel);

    mainLayout->addWidget(previewArea, 9);

    // Settings area
    QWidget *settingsArea = new QWidget();
    settingsArea->setMinimumWidth(276);
    QVBoxLayout *settingsLayout = new QVBoxLayout(settingsArea);
    settingsLayout->addSpacing(30);

    DLabel *titleLabel = new DLabel(tr("Scan Settings"));
    QFont font = titleLabel->font();
    font.setBold(true);
    font.setPointSize(font.pointSize() + 5);
    titleLabel->setFont(font);
    settingsLayout->addWidget(titleLabel);

    // Device settings group
    QGroupBox *settingsGroup = new QGroupBox();
    QVBoxLayout *groupLayout = new QVBoxLayout(settingsGroup);
    groupLayout->setSpacing(10);
    settingsGroup->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    settingsGroup->setMinimumWidth(260);

    // Device mode options
    m_modeLabel = new DLabel();
    m_modeLabel->setFixedWidth(SETTING_LABEL_WIDTH);
    m_modeCombo = new QComboBox();
    m_modeCombo->setMaximumWidth(SETTING_COMBO_WIDTH);
    QHBoxLayout *modeLayout = new QHBoxLayout();
    modeLayout->setSpacing(20);
    modeLayout->addWidget(m_modeLabel);
    modeLayout->addWidget(m_modeCombo);
    groupLayout->addLayout(modeLayout);

    // Resolution options
    DLabel *resLabel = new DLabel(tr("Resolution"));
    resLabel->setFixedWidth(SETTING_LABEL_WIDTH);
    m_resolutionCombo = new QComboBox();
    m_resolutionCombo->setMaximumWidth(SETTING_COMBO_WIDTH);
    QHBoxLayout *resolutionLayout = new QHBoxLayout();
    resolutionLayout->setSpacing(20);
    resolutionLayout->addWidget(resLabel);
    resolutionLayout->addWidget(m_resolutionCombo);
    groupLayout->addLayout(resolutionLayout);

    // Color mode options
    DLabel *colorLabel = new DLabel(tr("Color Mode"));
    colorLabel->setFixedWidth(SETTING_LABEL_WIDTH);
    m_colorCombo = new QComboBox();
    m_colorCombo->setMaximumWidth(SETTING_COMBO_WIDTH);
    QHBoxLayout *colorLayout = new QHBoxLayout();
    colorLayout->setSpacing(20);
    colorLayout->addWidget(colorLabel);
    colorLayout->addWidget(m_colorCombo);
    groupLayout->addLayout(colorLayout);

    // Format options
    DLabel *formatLabel = new DLabel(tr("Image Format"));
    formatLabel->setFixedWidth(SETTING_LABEL_WIDTH);
    m_formatCombo = new QComboBox();
    m_formatCombo->setMaximumWidth(SETTING_COMBO_WIDTH);
    QHBoxLayout *formatLayout = new QHBoxLayout();
    formatLayout->setSpacing(20);
    formatLayout->addWidget(formatLabel);
    formatLayout->addWidget(m_formatCombo);
    groupLayout->addLayout(formatLayout);

    // Paper size options
    m_paperSizeLabel = new DLabel(tr("Paper Size"));
    m_paperSizeLabel->setFixedWidth(SETTING_LABEL_WIDTH);
    m_paperSizeCombo = new QComboBox();
    m_paperSizeCombo->setMaximumWidth(SETTING_COMBO_WIDTH);
    QHBoxLayout *paperSizeLayout = new QHBoxLayout();
    paperSizeLayout->setSpacing(20);
    paperSizeLayout->addWidget(m_paperSizeLabel);
    paperSizeLayout->addWidget(m_paperSizeCombo);
    groupLayout->addLayout(paperSizeLayout);

    settingsLayout->addWidget(settingsGroup);
    settingsLayout->addSpacing(20);

    QWidget *bottomWidget = new QWidget();
    QVBoxLayout *buttonLayout = new QVBoxLayout(bottomWidget);

    // Action buttons
    DIconButton *scanButton = new DIconButton();
    scanButton->setToolTip(tr("Scan"));
    scanButton->setFocusPolicy(Qt::NoFocus);
    scanButton->setIcon(QIcon::fromTheme("btn_scan"));
    scanButton->setIconSize(QSize(36, 36));
    scanButton->setFixedSize(200, 50);
    scanButton->setBackgroundRole(DPalette::Highlight);

    DPushButton *viewButton = new DPushButton(tr("View Scanned Image"));
    viewButton->setFixedSize(200, 50);
    // viewButton->setFlat(true);
    viewButton->setFocusPolicy(Qt::NoFocus);

    buttonLayout->addWidget(scanButton, 0, Qt::AlignHCenter);
    buttonLayout->addSpacing(20);
    buttonLayout->addWidget(viewButton, 0, Qt::AlignHCenter);

    settingsLayout->addWidget(bottomWidget);
    settingsLayout->setAlignment(bottomWidget, Qt::AlignHCenter);

    m_historyEdit = new QPlainTextEdit();
    m_historyEdit->setReadOnly(true);
    m_historyEdit->setPlaceholderText(tr("Scan history will be shown here"));
    m_historyEdit->hide();

    settingsLayout->addSpacing(20);
    settingsLayout->addWidget(m_historyEdit);
    settingsLayout->setAlignment(Qt::AlignTop);

    connect(scanButton, &DPushButton::clicked, this, [this]() {
        // Throttle rapid clicks to avoid duplicate scanning
        static QTimer throttle;
        if(throttle.isActive()) return;

        throttle.setSingleShot(true);
        throttle.start(500); // set 500ms delay for single click

        startScanning();
    });
    connect(viewButton, &DPushButton::clicked, this, [this]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(getSaveDirectory()));
    });

    mainLayout->addWidget(settingsArea, 1);

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->addWidget(mainFrame);
    outerLayout->setContentsMargins(0, 0, 0, 0);
}

void ScanWidget::setupDeviceMode(DeviceBase* device, QString name)
{
    if (!device) return;

    // disconnect device signals
    connectDeviceSignals(false);

    // 设置新设备
    auto deviceType = device->getDeviceType();
    m_isScanner = device->getDeviceType() == DeviceBase::Scanner;

    // clear all combo boxes
    m_modeCombo->clear();
    m_resolutionCombo->clear();
    m_colorCombo->clear();
    m_formatCombo->clear();
    m_paperSizeCombo->clear();

    m_device = device;
    if (m_isScanner) {
        m_modeLabel->setText(tr("Scan Mode"));

        auto scanner = dynamic_cast<ScannerDevice*>(m_device);
        QStringList scanModes;
        if (scanner) {
            auto supportedModes = scanner->getSupportedScanModes();
            qCInfo(app) << "Device supported scan modes:" << supportedModes;
            
            for (auto mode : supportedModes) {
                switch (mode) {
                    case ScannerDevice::SCAN_MODE_FLATBED:
                        scanModes << tr("Flatbed");
                        break;
                    case ScannerDevice::SCAN_MODE_ADF_SIMPLEX:
                        scanModes << tr("ADF");
                        break;
                    case ScannerDevice::SCAN_MODE_ADF_DUPLEX:
                        scanModes << tr("Duplex");
                        break;
                }
            }
            
            if (scanModes.isEmpty()) {
                scanModes << tr("Flatbed");
            }
        } else {
            scanModes << tr("Flatbed");
        }
        m_modeCombo->addItems(scanModes);
    } else {
        m_modeLabel->setText(tr("Video Format"));
        m_modeCombo->addItems({ "MJPG" });
    }

    const QStringList colorModes = { tr("Color"), tr("Grayscale"), tr("Black White") };
    m_colorCombo->addItems(colorModes);
    m_formatCombo->addItems(FORMATS);
    
    // Add paper size options
    const QStringList paperSizes = { 
        tr("Auto"), 
        "A4 (210×297mm)", 
        "A3 (297×420mm)",  
        "A5 (148×210mm)", 
        "A6 (105×148mm)",
        "B4 (250×353mm)",
        "B5 (176×250mm)"
    };
    m_paperSizeCombo->addItems(paperSizes);

    // 设置初始选中项
    m_colorCombo->setCurrentIndex(m_imageSettings->colorMode);
    m_formatCombo->setCurrentIndex(m_imageSettings->format);
    m_paperSizeCombo->setCurrentIndex(m_imageSettings->paperSize);

    updateDeviceSettings();

    connectDeviceSignals(true);
}

void ScanWidget::connectDeviceSignals(bool bind)
{
    if (!m_device) return;

    if (bind) {
        connect(m_device, &DeviceBase::previewUpdated, this, &ScanWidget::onUpdatePreview);
        connect(m_device, &DeviceBase::imageCaptured, this, &ScanWidget::onScanFinished);
        connect(m_device, &DeviceBase::errorOccurred, this, &ScanWidget::handleDeviceError);

        if (m_isScanner) {
            auto scanner = dynamic_cast<ScannerDevice*>(m_device);
            if (scanner) {
                connect(scanner, &ScannerDevice::deviceOpened, this, &ScanWidget::onDeviceOpened);
            }
        }

        connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScanWidget::onScanModeChanged);
        connect(m_resolutionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScanWidget::onResolutionChanged);
        connect(m_colorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScanWidget::onColorModeChanged);
        connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScanWidget::onFormatChanged);
        connect(m_paperSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScanWidget::onPaperSizeChanged);
    } else {
        disconnect(m_device, &DeviceBase::previewUpdated, this, &ScanWidget::onUpdatePreview);
        disconnect(m_device, &DeviceBase::imageCaptured, this, &ScanWidget::onScanFinished);
        disconnect(m_device, &DeviceBase::errorOccurred, this, &ScanWidget::handleDeviceError);

        if (m_isScanner) {
            auto scanner = dynamic_cast<ScannerDevice*>(m_device);
            if (scanner) {
                disconnect(scanner, &ScannerDevice::deviceOpened, this, &ScanWidget::onDeviceOpened);
            }
        }

        disconnect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScanWidget::onScanModeChanged);
        disconnect(m_resolutionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScanWidget::onResolutionChanged);
        disconnect(m_colorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScanWidget::onColorModeChanged);
        disconnect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScanWidget::onFormatChanged);
        disconnect(m_paperSizeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScanWidget::onPaperSizeChanged);
    }
}

void ScanWidget::startScanning()
{
    if (!m_device) {
        handleDeviceError(tr("Device not initialized"));
        return;
    }

    if (m_cameraBusy) {
        // The camera is still being reconfigured, a capture now would race with it.
        return;
    }

    if (auto *webcam = dynamic_cast<WebcamDevice *>(m_device)) {
        if (webcam->isCapturing()) {
            // The capture in flight is still waiting for its frame; a second one would only
            // queue up behind it.
            return;
        }
    }

    resetPreview();
    m_device->startCapture();
}

void ScanWidget::updateDeviceSettings()
{
    // 更新分辨率选项
    m_resolutionCombo->clear();
    QStringList resolutions;
    int defaultIndex = 0;

    if (m_isScanner) {
        auto scanner = dynamic_cast<ScannerDevice*>(m_device);
        if (scanner) {
            auto dpis = scanner->getSupportedResolutions();
            auto currentDpi = scanner->getResolution();
            for (const auto &res : dpis) {
                resolutions << QString("%1 DPI").arg(res);
                if (res == currentDpi) {
                    defaultIndex = resolutions.size() - 1;
                }
            }
        }
    } else {
        auto webcam = dynamic_cast<WebcamDevice*>(m_device);
        if (webcam) {
            auto resols = webcam->getSupportedResolutions();
            auto currentRes = webcam->getResolution();
            for (const auto &res : resols) {
                resolutions << QString("%1x%2").arg(res.width()).arg(res.height());
                if (res == currentRes) {
                    defaultIndex = resolutions.size() - 1;
                }
            }
        }
    }
    m_resolutionCombo->addItems(resolutions);
    m_resolutionCombo->setCurrentIndex(defaultIndex);
}

void ScanWidget::startCameraPreview()
{
    if (!m_device) return;

    if (m_isScanner) {
        QIcon icon = QIcon::fromTheme("blank_doc");
        QPixmap pixmap = icon.pixmap(200, 200);
        m_previewLabel->setPixmap(pixmap);
    } else {
        auto webcam = dynamic_cast<WebcamDevice*>(m_device);
        if (webcam) {
            m_previewLabel->setText(tr("Initializing preview..."));
            // The stream is already open: openDevice() starts it on the thread that opens
            // the camera, so stopping it here would only restart it for nothing. The camera
            // is the context object, so a device that went away in the meantime is not
            // started by a stale timer.
            QTimer::singleShot(100, webcam, [webcam]() {
                webcam->startPreview();
            });
        } else {
            m_previewLabel->setText(tr("Device preview not available"));
        }
    }
}

void ScanWidget::stopCameraPreview()
{
    if (!m_device) return;

    if (m_isScanner) {
        auto scanner = dynamic_cast<ScannerDevice*>(m_device);
        if (scanner) {
            scanner->cancelScan();
        }
    } else {
        auto webcam = dynamic_cast<WebcamDevice*>(m_device);
        if (webcam) {
            webcam->stopPreview();
        }
    }
}

void ScanWidget::waitForPendingCameraWork()
{
    if (m_resolutionWatcher) {
        m_resolutionWatcher->waitForFinished();
    }
}

void ScanWidget::onUpdatePreview(const QImage &image)
{
    if (image.isNull()) {
        m_previewLabel->setText(tr("No preview image"));
        return;
    }

    QMutexLocker locker(&m_previewMutex);
    // Scale the image before converting it to a pixmap: converting a full camera frame
    // (up to 8 MP) to a pixmap and scaling that is several times more expensive.
    const QImage scaledImage = image.scaled(m_previewLabel->size(),
                                           Qt::KeepAspectRatio,
                                           Qt::FastTransformation);
    m_previewLabel->setPixmap(QPixmap::fromImage(scaledImage));
    m_previewLabel->setAlignment(Qt::AlignCenter);
}

void ScanWidget::handleDeviceError(const QString &error)
{
    qCWarning(app) << "Device error:" << error;
    m_previewLabel->setText(error);
}

void ScanWidget::onDeviceOpened()
{
    qCDebug(app) << "Device opened signal received, updating UI settings.";
    updateDeviceSettings();
}
void ScanWidget::resetPreview()
{
    QIcon icon = QIcon::fromTheme("blank_doc");
    QPixmap pixmap = icon.pixmap(200, 200);
    m_previewLabel->setPixmap(pixmap);
    // m_previewLabel->setText(tr("Click 'Scan' to begin"));
    m_previewLabel->setAlignment(Qt::AlignCenter);
}

// 以下为参数变更处理函数
void ScanWidget::onResolutionChanged(int index)
{
    if (!m_device) return;

    if (m_isScanner) {
        auto scanner = dynamic_cast<ScannerDevice*>(m_device);
        if (scanner) {
            // "300 DPI" -> DPI
            QString dpiStr = m_resolutionCombo->itemText(index);
            int dpi = dpiStr.left(dpiStr.indexOf(' ')).toInt();
            scanner->setResolution(dpi);
        }
    } else {
        auto webcam = dynamic_cast<WebcamDevice*>(m_device);
        if (webcam) {
            QString res = m_resolutionCombo->itemText(index);
            QStringList parts = res.split('x');
            if (parts.size() == 2) {
                if (m_cameraBusy || webcam->isCapturing()) {
                    // The device is busy being reconfigured or read: a second reconfiguration
                    // would close it underneath the first one.
                    return;
                }

                // Reconfiguring the camera (close, re-open, set the format, map the buffers
                // and start the stream) takes about a second, so it runs off the UI thread.
                // The preview is stopped here, on this thread, and restarted when the new
                // resolution is up.
                m_cameraBusy = true;
                m_previewLabel->setText(tr("Initializing preview..."));
                webcam->stopPreview();

                const int width = parts[0].toInt();
                const int height = parts[1].toInt();
                // The watcher belongs to this widget, not to the camera: the task keeps using
                // the device from a worker thread, so it has to be waited for before the
                // device is released (see waitForPendingCameraWork()).
                auto *watcher = new QFutureWatcher<bool>(this);
                m_resolutionWatcher = watcher;
                connect(watcher, &QFutureWatcher<bool>::finished, this, [this, webcam, watcher]() {
                    const bool changed = watcher->result();
                    watcher->deleteLater();
                    if (m_resolutionWatcher == watcher) {
                        m_resolutionWatcher = nullptr;
                    }
                    m_cameraBusy = false;
                    if (changed) {
                        webcam->startPreview();
                    }
                });
                watcher->setFuture(QtConcurrent::run(&WebcamDevice::setResolution, webcam, width, height));
            }
        }
    }

    emit deviceSettingsChanged();
}

void ScanWidget::onColorModeChanged(int index)
{
    m_imageSettings->colorMode = index;

    if (m_device && m_isScanner) {
        auto scanner = dynamic_cast<ScannerDevice*>(m_device);
        if (scanner) {
            scanner->setColorMode(static_cast<ScannerDevice::ColorMode>(index));
        }
    }

    emit deviceSettingsChanged();
}

void ScanWidget::onFormatChanged(int index)
{
    m_imageSettings->format = index;
    emit deviceSettingsChanged();
}

void ScanWidget::onPaperSizeChanged(int index)
{
    m_imageSettings->paperSize = index;
    
    // Update scanner device paper size
    if (m_device && m_isScanner) {
        auto scanner = dynamic_cast<ScannerDevice*>(m_device);
        if (scanner) {
            scanner->setPaperSize(static_cast<ScannerDevice::PaperSize>(index));
        }
    }
    
    emit deviceSettingsChanged();
}

QString ScanWidget::getSaveDirectory()
{
    if (m_saveDir.isEmpty()) {
        // Set default save directory: Documents/scan
        QDir documentsDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        m_saveDir = documentsDir.filePath("scan");
        if (!documentsDir.mkpath("scan")) {
            qCWarning(app) << "Failed to create scan directory:" << m_saveDir;
        }
    }
    return m_saveDir;
}

void ScanWidget::setSaveDirectory(const QString &dir)
{
    if (dir.isEmpty()) {
        qCWarning(app) << "Empty directory path provided";
        return;
    }
    
    QDir saveDir(dir);
    if (!saveDir.exists()) {
        if (!saveDir.mkpath(".")) {
            qCWarning(app) << "Failed to create directory:" << dir;
            return;
        }
    }
    
    m_saveDir = dir;
    qCDebug(app) << "Save directory set to:" << m_saveDir;
}

void ScanWidget::onScanFinished(const QImage &image)
{
    // Processing a full page (black and white conversion, paper size detection and scaling)
    // and writing the file takes seconds, so it runs on a worker thread. The widget state is
    // read here, because it belongs to the UI thread.
    ScanSaveRequest request;
    request.image = image;
    // The path is claimed here, on the UI thread, before the save starts: two scans that end
    // within the same second would otherwise build the same name and overwrite each other,
    // as both saves run in parallel.
    request.filePath = QDir(getSaveDirectory())
                           .filePath(QString("%1.%2").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmsszzz"))
                                         .arg(FORMATS[m_imageSettings->format].toLower()));
    request.formatIndex = m_imageSettings->format;
    request.colorMode = m_imageSettings->colorMode;
    request.paperSize = static_cast<ScannerDevice::PaperSize>(m_imageSettings->paperSize);
    if (m_device && m_isScanner) {
        auto scanner = dynamic_cast<ScannerDevice*>(m_device);
        if (scanner) {
            request.dpi = scanner->getResolution();
        }
    }

    auto *watcher = new QFutureWatcher<ScanSaveResult>(this);
    connect(watcher, &QFutureWatcher<ScanSaveResult>::finished, this, [this, watcher]() {
        const ScanSaveResult result = watcher->result();
        watcher->deleteLater();

        if (result.success) {
            qCDebug(app) << "Scan saved to:" << result.filePath;
            // add the saved file path to the top of the history box
            m_historyEdit->moveCursor(QTextCursor::Start);
            m_historyEdit->insertPlainText(result.filePath + "\n");
            DMessageManager::instance()->sendMessage(window(), QIcon::fromTheme("dialog-ok"),
                                                     tr("Scan completed, saved to: %1").arg(result.filePath));
        } else {
            qCWarning(app) << "Failed to save scan to:" << result.filePath;
            DMessageManager::instance()->sendMessage(window(), QIcon::fromTheme("dialog-warning"),
                                                     tr("Failed to save scan."));
        }
    });
    watcher->setFuture(QtConcurrent::run(&ScanWidget::saveScan, request));
}

// Runs on a worker thread: it must not touch any widget state.
ScanWidget::ScanSaveResult ScanWidget::saveScan(const ScanSaveRequest &request)
{
    ScanSaveResult result;
    result.filePath = request.filePath;

    QElapsedTimer saveTimer;
    saveTimer.start();

    QImage processedImage = request.image;
    if (request.colorMode == 1) {   // GRAYSCALE
        processedImage = processedImage.convertToFormat(QImage::Format_Grayscale8);
    } else if (request.colorMode == 2) {   // BLACKWHITE
        // 使用改进的黑白转换算法
        processedImage = convertToBlackWhite(processedImage);
    }

    // Handle paper size detection and scaling
    if (request.paperSize == ScannerDevice::PAPER_SIZE_AUTO) {
        // Auto-detect paper size
        const ScannerDevice::PaperSize detectedSize = detectPaperSize(processedImage, request.dpi);
        qDebug() << "Auto-detected paper size:" << detectedSize;

        // Scale to detected size if needed
        if (detectedSize != ScannerDevice::PAPER_SIZE_AUTO) {
            processedImage = scaleToPaperSize(processedImage, detectedSize, request.dpi);
        }
    } else {
        // Use manually selected paper size
        qDebug() << "Using selected paper size:" << request.paperSize;
        processedImage = scaleToPaperSize(processedImage, request.paperSize, request.dpi);
    }

    const qint64 processMs = saveTimer.elapsed();
    saveTimer.restart();

    if (request.formatIndex < 4) {   // PNG/JPG/BMP/TIFF
        result.success = processedImage.save(result.filePath, FORMATS[request.formatIndex].toLatin1().constData());
    } else if (request.formatIndex == 4) {   // PDF
        result.success = renderPdf(processedImage, result.filePath, request.paperSize);
    } else {
        // Create vector of images
        QVector<QImage> images;
        images.append(processedImage);

        // Write OFD file
        ofd::Writer writer;

        // Get paper size dimensions in mm for OFD
        const QSizeF paperSizeMM = ScannerDevice::getPaperSizeDimensions(request.paperSize);

        result.success = writer.createFromImages(result.filePath, images, 0, 0,
                                                paperSizeMM.width(), paperSizeMM.height());
        if (!result.success) {
            qCWarning(app) << "Failed to create OFD file";
        }
    }

    qCInfo(app) << "Scan save timing: process" << processMs << "ms, write" << saveTimer.elapsed() << "ms, format" << FORMATS[request.formatIndex];
    return result;
}

// Runs on a worker thread: QPrinter is created and used there, which Qt allows for the
// PDF output format (a QPrinter must be created in the thread that paints on it).
bool ScanWidget::renderPdf(const QImage &image, const QString &filePath, ScannerDevice::PaperSize paperSize)
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);

    // Set page size based on selected paper size
    QPageSize::PageSizeId pageSizeId = QPageSize::A4;  // Default
    switch (paperSize) {
        case ScannerDevice::PAPER_SIZE_A3:
            pageSizeId = QPageSize::A3;
            break;
        case ScannerDevice::PAPER_SIZE_A4:
            pageSizeId = QPageSize::A4;
            break;
        case ScannerDevice::PAPER_SIZE_A5:
            pageSizeId = QPageSize::A5;
            break;
        case ScannerDevice::PAPER_SIZE_A6:
            pageSizeId = QPageSize::A6;
            break;
        case ScannerDevice::PAPER_SIZE_B4:
            pageSizeId = QPageSize::B4;
            break;
        case ScannerDevice::PAPER_SIZE_B5:
            pageSizeId = QPageSize::B5;
            break;
        case ScannerDevice::PAPER_SIZE_AUTO:
        default:
            // For auto mode, use the detected paper size or default to A4
            // Note: the image has already been scaled to the correct size
            pageSizeId = QPageSize::A4;
            break;
    }
    printer.setPageSize(QPageSize(pageSizeId));

    QPainter painter;
    if (!painter.begin(&printer)) {
        return false;
    }

    QRect rect = painter.viewport();
    QSize size = image.size();
    size.scale(rect.size(), Qt::KeepAspectRatio);
    painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
    painter.setWindow(image.rect());
    painter.drawImage(0, 0, image);
    painter.end();
    return true;
}


void ScanWidget::onScanModeChanged(int index)
{
    if (!m_device) return;

    if (m_isScanner) {
        // "平板", "ADF", "双面"
        auto scanner = dynamic_cast<ScannerDevice*>(m_device);
        if (scanner) {
            ScannerDevice::ScanMode mode = static_cast<ScannerDevice::ScanMode>(index);
            scanner->setScanMode(mode);
        }
    } else {
        // "MJPG", "YUYV", "H264"
        auto webcam = dynamic_cast<WebcamDevice*>(m_device);
        if (webcam) {
            // TODO: 实现视频格式变更逻辑
        }
    }
    // 实现扫描模式变更逻辑
    emit deviceSettingsChanged();
}

QImage ScanWidget::convertToBlackWhite(const QImage &sourceImage)
{
    // 首先转换为灰度图
    QImage grayImage = sourceImage.convertToFormat(QImage::Format_Grayscale8);

    const int width = grayImage.width();
    const int height = grayImage.height();

    qDebug() << "Starting hybrid threshold binarization...";

    // 步骤1: 计算全局阈值 (Otsu方法的简化版本)
    int histogram[256] = {0};

    // 构建直方图
    for (int y = 0; y < height; ++y) {
        const uchar* line = reinterpret_cast<const uchar*>(grayImage.scanLine(y));
        for (int x = 0; x < width; ++x) {
            histogram[line[x]]++;
        }
    }

    // 计算全局阈值 (Otsu方法)
    int totalPixels = width * height;
    long long sum = 0;
    for (int i = 0; i < 256; ++i) {
        sum += i * histogram[i];
    }

    int globalThreshold = 128; // 默认值
    double maxVariance = 0;

    long long sumB = 0;
    int wB = 0;

    for (int t = 0; t < 256; ++t) {
        wB += histogram[t];
        if (wB == 0) continue;
        
        int wF = totalPixels - wB;
        if (wF == 0) break;
        
        sumB += t * histogram[t];
        
        double mB = (double)sumB / wB;
        double mF = (double)(sum - sumB) / wF;
        
        double betweenVar = (double)wB * wF * (mB - mF) * (mB - mF);
        
        if (betweenVar > maxVariance) {
            maxVariance = betweenVar;
            globalThreshold = t;
        }
    }

    qDebug() << "Global threshold (Otsu):" << globalThreshold;
    
    // 步骤2: 创建全局阈值结果
    QImage globalResult(width, height, QImage::Format_Mono);
    for (int y = 0; y < height; ++y) {
        const uchar* grayLine = reinterpret_cast<const uchar*>(grayImage.scanLine(y));
        for (int x = 0; x < width; ++x) {
            globalResult.setPixel(x, y, (grayLine[x] < globalThreshold) ? 0 : 1);
        }
    }

    // 步骤3: 局部自适应阈值处理
    QImage localResult(width, height, QImage::Format_Mono);
    const int windowSize = 21; // 局部窗口大小
    
    for (int y = 0; y < height; ++y) {
        const uchar* grayLine = reinterpret_cast<const uchar*>(grayImage.scanLine(y));
        
        for (int x = 0; x < width; ++x) {
            // 计算局部窗口内的平均值
            int sum = 0;
            int count = 0;
            
            int startY = qMax(0, y - windowSize / 2);
            int endY = qMin(height - 1, y + windowSize / 2);
            int startX = qMax(0, x - windowSize / 2);
            int endX = qMin(width - 1, x + windowSize / 2);
            
            for (int wy = startY; wy <= endY; wy += 2) { // 跳步采样加速
                const uchar* windowLine = reinterpret_cast<const uchar*>(grayImage.scanLine(wy));
                for (int wx = startX; wx <= endX; wx += 2) {
                    sum += windowLine[wx];
                    count++;
                }
            }
            
            int localThreshold = (count > 0) ? (sum / count) * 0.9 : globalThreshold;
            localResult.setPixel(x, y, (grayLine[x] < localThreshold) ? 0 : 1);
        }
    }

    // 步骤4: 混合两种结果
    QImage hybridResult(width, height, QImage::Format_Mono);
    
    // 分析图像特征，决定混合权重
    int brightPixels = 0;
    for (int y = 0; y < height; ++y) {
        const uchar* grayLine = reinterpret_cast<const uchar*>(grayImage.scanLine(y));
        for (int x = 0; x < width; ++x) {
            if (grayLine[x] > 180) brightPixels++;
        }
    }

    double brightRatio = (double)brightPixels / totalPixels;
    qDebug() << "Bright pixel ratio:" << brightRatio;

    // 根据图像特征选择策略
    for (int y = 0; y < height; ++y) {
        const uchar* grayLine = reinterpret_cast<const uchar*>(grayImage.scanLine(y));
        
        for (int x = 0; x < width; ++x) {
            int currentPixel = grayLine[x];
            int globalPixel = globalResult.pixelIndex(x, y);
            int localPixel = localResult.pixelIndex(x, y);
            
            int finalPixel;
            
            if (brightRatio > 0.3) {
                // 高亮度图像：偏向局部阈值，保护文字细节
                if (currentPixel > 160) {
                    // 亮区域使用局部阈值
                    finalPixel = localPixel;
                } else if (currentPixel < 80) {
                    // 暗区域使用全局阈值
                    finalPixel = globalPixel;
                } else {
                    // 中等亮度：权重融合
                    if (globalPixel == localPixel) {
                        finalPixel = globalPixel;
                    } else {
                        // 倾向于保留文字（黑色）
                        finalPixel = (globalPixel == 0 || localPixel == 0) ? 0 : 1;
                    }
                }
            } else {
                // 正常或低亮度图像：偏向全局阈值
                if (currentPixel > 140 && currentPixel < 200) {
                    // 中等亮度区域使用局部阈值增强细节
                    finalPixel = localPixel;
                } else {
                    // 其他区域使用全局阈值
                    finalPixel = globalPixel;
                }
            }
            
            hybridResult.setPixel(x, y, finalPixel);
        }
    }

    // 步骤5: 后处理 - 形态学操作
    QImage finalResult = hybridResult.copy();
    
    // 简单的去噪和连接
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            int blackCount = 0;
            
            // 统计3x3邻域的黑色像素
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (hybridResult.pixelIndex(x + dx, y + dy) == 0) {
                        blackCount++;
                    }
                }
            }
            
            int currentPixel = hybridResult.pixelIndex(x, y);
            
            // 去除孤立噪点
            if (currentPixel == 0 && blackCount <= 2) {
                finalResult.setPixel(x, y, 1);
            }
            // 填充小空洞
            else if (currentPixel == 1 && blackCount >= 6) {
                finalResult.setPixel(x, y, 0);
            }
        }
    }

    // // 保存调试图像
    // QString debugDir = getSaveDirectory();
    // globalResult.convertToFormat(QImage::Format_RGB32).save(QDir(debugDir).filePath("debug_global_threshold.png"));
    // localResult.convertToFormat(QImage::Format_RGB32).save(QDir(debugDir).filePath("debug_local_threshold.png"));
    // hybridResult.convertToFormat(QImage::Format_RGB32).save(QDir(debugDir).filePath("debug_hybrid_result.png"));
    
    qDebug() << "Hybrid threshold binarization completed.";
    
    return finalResult;
}

ScannerDevice::PaperSize ScanWidget::detectPaperSize(const QImage &image, int dpi)
{
    if (image.isNull()) {
        return ScannerDevice::PAPER_SIZE_A4;  // Default to A4
    }

    // Calculate physical dimensions in millimeters
    double widthMM = (image.width() * 25.4) / dpi;
    double heightMM = (image.height() * 25.4) / dpi;
    
    qDebug() << "Detected image size:" << widthMM << "x" << heightMM << "mm";
    
    // Define paper size tolerances (in mm)
    const double tolerance = 15.0;  // 15mm tolerance for detection
    
    // Map of paper sizes to their dimensions
    QMap<ScannerDevice::PaperSize, QSizeF> paperSizes = {
        {ScannerDevice::PAPER_SIZE_A3, QSizeF(297, 420)},
        {ScannerDevice::PAPER_SIZE_A4, QSizeF(210, 297)},
        {ScannerDevice::PAPER_SIZE_A5, QSizeF(148, 210)},
        {ScannerDevice::PAPER_SIZE_A6, QSizeF(105, 148)},
        {ScannerDevice::PAPER_SIZE_B4, QSizeF(250, 353)},
        {ScannerDevice::PAPER_SIZE_B5, QSizeF(176, 250)}
    };
    
    // Find the best matching paper size
    ScannerDevice::PaperSize bestMatch = ScannerDevice::PAPER_SIZE_A4;
    double minError = std::numeric_limits<double>::max();
    
    for (auto it = paperSizes.begin(); it != paperSizes.end(); ++it) {
        QSizeF paperSize = it.value();
        
        // Check both orientations
        double error1 = qAbs(widthMM - paperSize.width()) + qAbs(heightMM - paperSize.height());
        double error2 = qAbs(widthMM - paperSize.height()) + qAbs(heightMM - paperSize.width());
        
        double minErrorForThisSize = qMin(error1, error2);
        
        if (minErrorForThisSize < minError && minErrorForThisSize <= tolerance * 2) {
            minError = minErrorForThisSize;
            bestMatch = it.key();
        }
    }
    
    qDebug() << "Detected paper size:" << bestMatch;
    return bestMatch;
}

QImage ScanWidget::scaleToPaperSize(const QImage &image, ScannerDevice::PaperSize targetSize, int dpi)
{
    if (image.isNull() || targetSize == ScannerDevice::PAPER_SIZE_AUTO) {
        return image;
    }
    
    // Get target paper dimensions in mm
    QSizeF targetSizeMM = ScannerDevice::getPaperSizeDimensions(targetSize);
    
    // Convert target dimensions to pixels at given DPI
    int targetWidth = qRound((targetSizeMM.width() * dpi) / 25.4);
    int targetHeight = qRound((targetSizeMM.height() * dpi) / 25.4);
    
    QSize targetPixelSize(targetWidth, targetHeight);
    
    // Check if scaling is needed
    // If image is smaller than target size, don't scale up (avoid quality loss)
    if (image.width() <= targetWidth && image.height() <= targetHeight) {
        qDebug() << "Image is smaller than target size, no scaling applied";
        return image;
    }
    
    // Determine if we need to consider orientation
    bool needsRotation = false;
    QSize scaledSize = image.size();
    
    // Calculate scaling factors for both orientations
    double scaleNormal = qMin((double)targetWidth / image.width(), 
                              (double)targetHeight / image.height());
    double scaleRotated = qMin((double)targetWidth / image.height(), 
                               (double)targetHeight / image.width());
    
    // Choose the better scaling (allows rotation if it provides better fit)
    if (scaleRotated > scaleNormal * 1.1) {  // 10% threshold for preferring rotation
        needsRotation = true;
        scaledSize = QSize(qRound(image.height() * scaleRotated), 
                          qRound(image.width() * scaleRotated));
    } else {
        scaledSize = QSize(qRound(image.width() * scaleNormal), 
                          qRound(image.height() * scaleNormal));
    }
    
    qDebug() << "Scaling from" << image.size() << "to" << scaledSize;
    
    // Perform scaling with high quality
    QImage scaledImage = image.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    
    // If rotation is needed, rotate the image
    if (needsRotation) {
        QTransform transform;
        transform.rotate(90);
        scaledImage = scaledImage.transformed(transform, Qt::SmoothTransformation);
    }
    
    // Create final image with exact target dimensions, centered
    QImage finalImage(targetWidth, targetHeight, image.format());
    finalImage.fill(Qt::white);  // White background for letter/paper images
    
    QPainter painter(&finalImage);
    int x = (targetWidth - scaledImage.width()) / 2;
    int y = (targetHeight - scaledImage.height()) / 2;
    painter.drawImage(x, y, scaledImage);
    painter.end();
    
    qDebug() << "Final image size:" << finalImage.size();
    return finalImage;
}

