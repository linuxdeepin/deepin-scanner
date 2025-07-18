// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
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

#include <DLabel>
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
    void onScanFinished(const QImage &image);
    void handleDeviceError(const QString &error);

private:
    void setupUI();
    void connectDeviceSignals(bool bind);
    void updateDeviceSettings();
    QImage convertToBlackWhite(const QImage &sourceImage);

    DeviceBase* m_device = nullptr;
    bool m_isScanner;

    QMutex m_previewMutex;

    // preview area
    DLabel *m_previewLabel;

    DLabel *m_modeLabel;
    QComboBox *m_modeCombo;
    QComboBox *m_resolutionCombo;
    QComboBox *m_colorCombo;
    QComboBox *m_formatCombo;

    QScopedPointer<ImageSettings> m_imageSettings;
    QPlainTextEdit *m_historyEdit;   // Scan history display box
    QString m_saveDir;               // Custom save directory
};

#endif   // SCANWIDGET_H
