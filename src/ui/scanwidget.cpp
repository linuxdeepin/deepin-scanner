// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "scanwidget.h"

#include <ofd/ofd_writer.h>

#include <QVBoxLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QGraphicsDropShadowEffect>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QPlainTextEdit>
#include <QDesktopServices>
#include <QPrinter>

#include <DFrame>
#include <DPushButton>
#include <DIconButton>
#include <DLabel>

static const QStringList FORMATS = { "PNG", "JPG", "BMP", "TIFF", "PDF", "OFD" };

// fixed width for label and combo box
static const int SETTING_LABEL_WIDTH = 120;
static const int SETTING_COMBO_WIDTH = 220;

ScanWidget::ScanWidget(QWidget *parent) : QWidget(parent),
                                          m_isScanner(false),
                                          m_previewTimer(this),
                                          m_imageSettings(new ImageSettings())
{
    setupUI();

    // 设置预览更新定时器
    m_previewTimer.setInterval(100);   // 10 FPS
    connect(&m_previewTimer, &QTimer::timeout, this, &ScanWidget::updatePreview);
}

ScanWidget::~ScanWidget()
{
    m_imageSettings.reset();
}

void ScanWidget::setupUI()
{
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

    // Preview area
    DFrame *previewArea = new DFrame();
    previewArea->setMinimumSize(480, 360);
    QVBoxLayout *previewLayout = new QVBoxLayout(previewArea);
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(previewArea);
    shadow->setBlurRadius(10);
    shadow->setOffset(2, 2);
    previewArea->setGraphicsEffect(shadow);
    previewArea->setBackgroundRole(QPalette::Window);

    m_previewLabel = new DLabel();
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    previewLayout->addWidget(m_previewLabel);

    splitter->addWidget(previewArea);

    // Settings area
    QWidget *settingsArea = new QWidget();
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
    settingsGroup->setMinimumWidth(350);

    // Device mode options
    m_modeLabel = new DLabel();
    m_modeLabel->setFixedWidth(SETTING_LABEL_WIDTH);
    m_modeCombo = new QComboBox();
    m_modeCombo->setMaximumWidth(SETTING_COMBO_WIDTH);
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScanWidget::onScanModeChanged);
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
    connect(m_resolutionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScanWidget::onResolutionChanged);
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
    connect(m_colorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScanWidget::onColorModeChanged);
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
    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ScanWidget::onFormatChanged);
    QHBoxLayout *formatLayout = new QHBoxLayout();
    formatLayout->setSpacing(20);
    formatLayout->addWidget(formatLabel);
    formatLayout->addWidget(m_formatCombo);
    groupLayout->addLayout(formatLayout);

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

    settingsLayout->addSpacing(20);
    settingsLayout->addWidget(m_historyEdit);

    connect(scanButton, &DPushButton::clicked, this, &ScanWidget::startScanning);
    connect(viewButton, &DPushButton::clicked, this, [this]() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(getSaveDirectory()));
    });

    splitter->addWidget(settingsArea);

    // make splitter default to right-side compressed
    splitter->setSizes({ 9, 1 });
    splitter->setStretchFactor(0, 1);   // left side stretchable
    splitter->setStretchFactor(1, 0);   // right side not stretchable

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(splitter);
}

void ScanWidget::setupDeviceMode(DeviceBase* device, QString name)
{
    if (!device) return;

    // disconnect device signals
    connectDeviceSignals(false);
    // 释放之前的设备
    if (m_device) {
        m_device->closeDevice();
        m_device = nullptr;
    }

    // 设置新设备
    auto deviceType = device->getDeviceType();
    m_isScanner = device->getDeviceType() == DeviceBase::Scanner;

    if (!device->openDevice(name)) {
        qWarning() << "无法打开设备:" << name;
        return;
    }
    // clear all combo boxes
    m_modeCombo->clear();
    m_resolutionCombo->clear();
    m_colorCombo->clear();
    m_formatCombo->clear();

    m_device = device;
    if (m_isScanner) {
        m_modeLabel->setText(tr("Scan Mode"));
        const QStringList scanModes = { tr("Flatbed") }; //, tr("ADF"), tr("Duplex")
        m_modeCombo->addItems(scanModes);
    } else {
        m_modeLabel->setText(tr("Video Format"));
        m_modeCombo->addItems({ "MJPG" });
    }

    const QStringList colorModes = { tr("Color"), tr("Grayscale"), tr("Black White") };
    m_colorCombo->addItems(colorModes);
    m_formatCombo->addItems(FORMATS);

    // 设置初始选中项
    m_colorCombo->setCurrentIndex(m_imageSettings->colorMode);
    m_formatCombo->setCurrentIndex(m_imageSettings->format);

    connectDeviceSignals(true);
}

void ScanWidget::connectDeviceSignals(bool bind)
{
    if (!m_device) return;

    if (bind) {
        connect(m_device, &DeviceBase::imageCaptured, this, &ScanWidget::onScanFinished);
        connect(m_device, &ScannerDevice::errorOccurred, this, &ScanWidget::handleDeviceError);
    } else {
        disconnect(m_device, &DeviceBase::imageCaptured, this, &ScanWidget::onScanFinished);
        disconnect(m_device, &ScannerDevice::errorOccurred, this, &ScanWidget::handleDeviceError);
    }
}

void ScanWidget::startScanning()
{
    if (!m_device) {
        handleDeviceError(tr("Device not initialized"));
        return;
    }

    m_device->startCapture();
}

void ScanWidget::updateDeviceSettings()
{
    // 更新分辨率选项
    m_resolutionCombo->clear();
    if (m_isScanner) {
        auto scanner = dynamic_cast<ScannerDevice*>(m_device);
        if (scanner) {
            m_resolutionCombo->addItems({ "300", "600", "1200" });
        }
    } else {
        auto webcam = dynamic_cast<WebcamDevice*>(m_device);
        if (webcam) {
            for (const auto &res : webcam->getSupportedResolutions()) {
                m_resolutionCombo->addItem(QString("%1x%2").arg(res.width()).arg(res.height()));
            }
        }
    }
}

void ScanWidget::startCameraPreview()
{
    if (!m_device) return;

    updateDeviceSettings();

    if (m_isScanner) {
        QIcon icon = QIcon::fromTheme("blank_doc");
        QPixmap pixmap = icon.pixmap(200, 200);
        m_previewLabel->setPixmap(pixmap);
    } else {
        auto webcam = dynamic_cast<WebcamDevice*>(m_device);
        if (webcam) {
            m_previewLabel->setText(tr("Initializing preview..."));
            webcam->stopPreview();
            QTimer::singleShot(100, [webcam]() {
                webcam->startPreview();
            });
            m_previewTimer.start();
        } else {
            m_previewLabel->setText(tr("Device preview not available"));
        }
    }
}

void ScanWidget::stopCameraPreview()
{
    if (!m_device) return;

    m_previewTimer.stop();

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

void ScanWidget::updatePreview()
{
    if (!m_device || m_isScanner) return;

    auto webcam = dynamic_cast<WebcamDevice*>(m_device);
    if (webcam) {
        // 获取最新帧并更新预览
        QImage frame = webcam->getLatestFrame();
        if (!frame.isNull()) {
            setPreviewImage(frame);
        }
    }
}

void ScanWidget::setPreviewImage(const QImage &image)
{
    if (image.isNull()) {
        m_previewLabel->setText(tr("No preview image"));
        return;
    }

    QMutexLocker locker(&m_previewMutex);
    QPixmap pixmap = QPixmap::fromImage(image);
    QPixmap scaled = pixmap.scaled(m_previewLabel->size(),
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
    m_previewLabel->setPixmap(scaled);
    m_previewLabel->setAlignment(Qt::AlignCenter);
}

void ScanWidget::handleDeviceError(const QString &error)
{
    qWarning() << "Device error:" << error;
    m_previewLabel->setText(error);
}

// 以下为参数变更处理函数
void ScanWidget::onResolutionChanged(int index)
{
    if (!m_device) return;

    if (m_isScanner) {
        auto scanner = dynamic_cast<ScannerDevice*>(m_device);
        if (scanner) {
            scanner->setResolution(m_resolutionCombo->itemText(index).toInt());
        }
    } else {
        auto webcam = dynamic_cast<WebcamDevice*>(m_device);
        if (webcam) {
            QString res = m_resolutionCombo->itemText(index);
            QStringList parts = res.split('x');
            if (parts.size() == 2) {
                webcam->setResolution(parts[0].toInt(), parts[1].toInt());
            }
        }
    }

    emit deviceSettingsChanged();
}

void ScanWidget::onColorModeChanged(int index)
{
    m_imageSettings->colorMode = index;
    emit deviceSettingsChanged();
}

void ScanWidget::onFormatChanged(int index)
{
    m_imageSettings->format = index;
    emit deviceSettingsChanged();
}

QString ScanWidget::getSaveDirectory()
{
    if (m_saveDir.isEmpty()) {
        // Set default save directory: Documents/scan
        QDir documentsDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation));
        m_saveDir = documentsDir.filePath("scan");
        if (!documentsDir.mkpath("scan")) {
            qWarning() << "Failed to create scan directory:" << m_saveDir;
        }
    }
    return m_saveDir;
}

void ScanWidget::setSaveDirectory(const QString &dir)
{
    if (dir.isEmpty()) {
        qWarning() << "Empty directory path provided";
        return;
    }
    
    QDir saveDir(dir);
    if (!saveDir.exists()) {
        if (!saveDir.mkpath(".")) {
            qWarning() << "Failed to create directory:" << dir;
            return;
        }
    }
    
    m_saveDir = dir;
    qDebug() << "Save directory set to:" << m_saveDir;
}

void ScanWidget::onScanFinished(const QImage &image)
{
    QString scanDir = getSaveDirectory();
    QString prefix = "scan";
    if (m_device) {
        QString deviceName = m_device->currentDeviceName();
        // get the first part of the device name before the first space
        // e.g. "Canon LiDE 300" -> "Canon"
        int spacePos = deviceName.indexOf(' ');
        prefix = spacePos > 0 ? deviceName.left(spacePos) : deviceName;
    }

    // generate a file name with timestamp
    QString fileName = QString("%1_%2.%3").arg(prefix)
                               .arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"))
                               .arg(FORMATS[m_imageSettings->format].toLower());

    // handle color mode conversion
    QImage processedImage = image;
    // handle color mode conversion
    if (m_imageSettings->colorMode == 1) {   // GRAYSCALE
        processedImage = image.convertToFormat(QImage::Format_Grayscale8);
    } else if (m_imageSettings->colorMode == 2) {   // BLACKWHITE
        processedImage = image.convertToFormat(QImage::Format_Mono);
    }

    QString filePath = QDir(scanDir).filePath(fileName);
    bool saveSuccess = false;

    if (m_imageSettings->format < 4) {   // PNG/JPG/BMP/TIFF
        saveSuccess = processedImage.save(filePath, FORMATS[m_imageSettings->format].toLatin1().constData());
    } else if (m_imageSettings->format == 4) {   // PDF
        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filePath);
        printer.setPageSize(QPageSize(QPageSize::A4));
        
        QPainter painter;
        if (painter.begin(&printer)) {
            QRect rect = painter.viewport();
            QSize size = processedImage.size();
            size.scale(rect.size(), Qt::KeepAspectRatio);
            painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
            painter.setWindow(processedImage.rect());
            painter.drawImage(0, 0, processedImage);
            painter.end();
            saveSuccess = true;
        }
    } else {
        // Create vector of images
        QVector<QImage> images;   
        images.append(processedImage);

        // Write OFD file
        ofd::Writer writer;
        saveSuccess = writer.createFromImages(filePath, images, 0, 0);
        if (!saveSuccess) {
            qWarning() << "Failed to create OFD file";
        }
    }

    if (saveSuccess) {
        qDebug() << "Scan saved to:" << filePath;
        // add the saved file path to the top of the history box
        m_historyEdit->moveCursor(QTextCursor::Start);
        m_historyEdit->insertPlainText(filePath + "\n");
    } else {
        qWarning() << "Failed to save scan to:" << filePath;
    }
}

void ScanWidget::onScanModeChanged(int index)
{
    if (m_isScanner) {
        // "平板", "ADF", "双面"
    } else {
        // "MJPG", "YUYV", "H264"
    }
    // 实现扫描模式变更逻辑
    emit deviceSettingsChanged();
}
