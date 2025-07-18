// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
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

#include <DFrame>
#include <DPushButton>
#include <DIconButton>
#include <DLabel>

using namespace DDLog;

static const QStringList FORMATS = { "PNG", "JPG", "BMP", "TIFF", "PDF", "OFD" };

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
    previewArea->setMinimumSize(480, 360);
    QVBoxLayout *previewLayout = new QVBoxLayout(previewArea);

    m_previewLabel = new DLabel();
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    previewLayout->addWidget(m_previewLabel);

    mainLayout->addWidget(previewArea, 9);

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

    updateDeviceSettings();

    connectDeviceSignals(true);
}

void ScanWidget::connectDeviceSignals(bool bind)
{
    if (!m_device) return;

    if (bind) {
        connect(m_device, &DeviceBase::previewUpdated, this, &ScanWidget::onUpdatePreview);
        connect(m_device, &DeviceBase::imageCaptured, this, &ScanWidget::onScanFinished);
        connect(m_device, &ScannerDevice::errorOccurred, this, &ScanWidget::handleDeviceError);

        connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScanWidget::onScanModeChanged);
        connect(m_resolutionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScanWidget::onResolutionChanged);
        connect(m_colorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScanWidget::onColorModeChanged);
        connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScanWidget::onFormatChanged);
    } else {
        disconnect(m_device, &DeviceBase::previewUpdated, this, &ScanWidget::onUpdatePreview);
        disconnect(m_device, &DeviceBase::imageCaptured, this, &ScanWidget::onScanFinished);
        disconnect(m_device, &ScannerDevice::errorOccurred, this, &ScanWidget::handleDeviceError);

        disconnect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScanWidget::onScanModeChanged);
        disconnect(m_resolutionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScanWidget::onResolutionChanged);
        disconnect(m_colorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScanWidget::onColorModeChanged);
        disconnect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ScanWidget::onFormatChanged);
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
            webcam->stopPreview();
            QTimer::singleShot(100, [webcam]() {
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

void ScanWidget::onUpdatePreview(const QImage &image)
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
    qCWarning(app) << "Device error:" << error;
    m_previewLabel->setText(error);
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
    QString scanDir = getSaveDirectory();

    // generate a file name with timestamp
    QString fileName = QString("%1.%2").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"))
                               .arg(FORMATS[m_imageSettings->format].toLower());

    // handle color mode conversion
    QImage processedImage = image;
    // handle color mode conversion
    if (m_imageSettings->colorMode == 1) {   // GRAYSCALE
        processedImage = image.convertToFormat(QImage::Format_Grayscale8);
    } else if (m_imageSettings->colorMode == 2) {   // BLACKWHITE
        // 使用改进的黑白转换算法
        processedImage = convertToBlackWhite(image);
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
            qCWarning(app) << "Failed to create OFD file";
        }
    }

    if (saveSuccess) {
        qCDebug(app) << "Scan saved to:" << filePath;
        // add the saved file path to the top of the history box
        m_historyEdit->moveCursor(QTextCursor::Start);
        m_historyEdit->insertPlainText(filePath + "\n");
    } else {
        qCWarning(app) << "Failed to save scan to:" << filePath;
    }
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

