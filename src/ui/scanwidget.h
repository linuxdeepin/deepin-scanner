// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SCANWIDGET_H
#define SCANWIDGET_H

#include <QWidget>
#include <QSplitter>
#include <QSharedPointer>
#include <QTimer>
#include <QMutex>
#include <QScopedPointer>
#include <QFutureWatcher>

#include <DLabel>
#include <DSpinner>
#include "device/devicebase.h"
#include "device/scannerdevice.h"
#include "device/webcamdevice.h"

DWIDGET_USE_NAMESPACE

class QComboBox;
class QPlainTextEdit;

struct ImageSettings
{
    int colorMode = 0;   // 0=COLOR, 1=GRAYSCALE, 2=BLACKWHITE
    int format = 0;   // 0=PNG, 1=JPG, 2=BMP, 3=TIFF, 4=PDF, 5=OFD
    int paperSize = 1;   // 0=AUTO, 1=A4, 2=A3, 3=A5, 4=A6, 5=B4, 6=B5
};

class ScanWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ScanWidget(QWidget *parent = nullptr);
    ~ScanWidget();

    void setupDeviceMode(DeviceBase* device, QString name);

    void startCameraPreview();
    void stopCameraPreview();

    // A camera reconfiguration runs on a worker thread and keeps using the device: the
    // owner has to wait for it before releasing the device.
    void waitForPendingCameraWork();

    QString getSaveDirectory();
    void setSaveDirectory(const QString &dir);

signals:
    void scanRequested();
    void scanFinished(const QImage &image);
    void saveRequested();
    void deviceSettingsChanged();

public slots:
    void startScanning();

private slots:
    void onUpdatePreview(const QImage &image);
    void onResolutionChanged(int index);
    void onColorModeChanged(int index);
    void onFormatChanged(int index);
    void onScanModeChanged(int index);
    void onPaperSizeChanged(int index);
    void onScanFinished(const QImage &image);
    void handleDeviceError(const QString &error);
    void onDeviceOpened();

private:
    // Everything the worker needs to process and save a scan; gathered on the UI thread
    // because the image settings and the device belong to it.
    struct ScanSaveRequest {
        QImage image;
        QString filePath;
        int formatIndex = 0;
        int colorMode = 0;
        ScannerDevice::PaperSize paperSize = ScannerDevice::PAPER_SIZE_AUTO;
        int dpi = 300;
    };

    // Result of saveScan().
    struct ScanSaveResult {
        QString filePath;
        bool success = false;
    };

    void setupUI();
    void connectDeviceSignals(bool bind);
    void updateDeviceSettings();
    void resetPreview();
    void hidePreviewSpinner();

    // Processing and saving run on a worker thread, so these must not touch widget state.
    static ScanSaveResult saveScan(const ScanSaveRequest &request);
    static bool renderPdf(const QImage &image, const QString &filePath, ScannerDevice::PaperSize paperSize);
    static QImage convertToBlackWhite(const QImage &sourceImage);

    // Paper size handling methods
    static ScannerDevice::PaperSize detectPaperSize(const QImage &image, int dpi);
    static QImage scaleToPaperSize(const QImage &image, ScannerDevice::PaperSize targetSize, int dpi);

    DeviceBase* m_device = nullptr;
    bool m_isScanner;
    // True while the camera is being reconfigured off the UI thread, so a scan cannot be
    // started against a device that is currently closing and re-opening.
    bool m_cameraBusy = false;
    // The reconfiguration task of the camera, or null when none is in flight. It has to be
    // waited for before the device it works on is destroyed, see waitForPendingCameraWork().
    QFutureWatcher<bool> *m_resolutionWatcher = nullptr;

    QMutex m_previewMutex;

    // preview area
    DLabel *m_previewLabel;
    // Small spinner centered over the preview area while the camera is being reconfigured.
    DSpinner *m_previewSpinner = nullptr;

    DLabel *m_modeLabel;
    QComboBox *m_modeCombo;
    QComboBox *m_resolutionCombo;
    QComboBox *m_colorCombo;
    QComboBox *m_formatCombo;
    
    // Paper size controls
    DLabel *m_paperSizeLabel;
    QComboBox *m_paperSizeCombo;

    QScopedPointer<ImageSettings> m_imageSettings;
    QPlainTextEdit *m_historyEdit;   // Scan history display box
    QString m_saveDir;               // Custom save directory
};

#endif   // SCANWIDGET_H
