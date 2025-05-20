// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef WEBCAMDEVICE_H
#define WEBCAMDEVICE_H

#include <QObject>
#include <QWidget>
#include <QImage>
#include <QTimer>
#include <linux/videodev2.h>
#include <QMutex>
#include <QThread>
#include <QDebug>
#include "devicebase.h"

class WebcamDevice : public DeviceBase
{
    Q_OBJECT

public:
    explicit WebcamDevice(QObject *parent = nullptr);
    ~WebcamDevice() override;

    // DeviceBase interface implementation
    bool initialize() override;
    QStringList getAvailableDevices() override;
    bool openDevice(const QString &devicePath) override;
    void closeDevice() override;
    void startCapture() override;
    void stopCapture() override;
    bool isCapturing() const override;
    DeviceType getDeviceType() const override { return DeviceType::Webcam; }
    QString currentDeviceName() const override { return m_currentDeviceName; }

    // Extended webcam-specific interface
    void startPreview();
    void stopPreview();

    QSize getMaxResolution();
    bool setResolution(int width, int height);
    QList<QSize> getSupportedResolutions();
    QSize getResolution() const { return QSize(m_width, m_height); }
    QImage getLatestFrame();

    // 设置摄像头控制参数
    bool setCameraControl(uint32_t controlId, int value);
    void listCameraControls();
    bool adjustCommonCameraSettings();

    // 常用控制参数设置的快捷方法（可选）
    bool setCameraBrightness(int value);
    bool setCameraContrast(int value);
    bool setCameraExposure(int value);
    bool setCameraAutoExposure(bool enable);
    bool setCameraAutoFocus(bool enable);   // 自动对焦控制

signals:
    void captureStarted();
    // void scanCompleted(const QImage &image);
    // void captureError(const QString &error);
    // 新增信号
    void resolutionsChanged(const QList<QSize> &resolutions);

private slots:
    void updatePreview();

private:
    int m_fd;   // Device file descriptor
    void *m_buffers[4];   // Video buffers
    size_t m_bufferSizes[4];   // Buffer sizes
    int m_currentBuffer;   // Currently used buffer
    bool m_isInitialized;
    bool m_deviceSelected;
    QTimer m_previewTimer;
    int m_width;
    int m_height;
    int m_pixelFormat;
    QList<QSize> m_supportedResolutions;
    // 用于存储最新的预览帧
    QImage m_latestFrame;
    QMutex m_frameMutex;   // Mutex lock for protecting m_latestFrame

    bool initMmap();
    void uninitMmap();
    QImage frameToQImage(const void *data, int width, int height, int format);
    void captureImage();
    bool startCapturing();
    void stopCapturing();
    bool selectBestPixelFormat();
    void diagnoseCameraIssues();
    // 新增私有方法
    void enumerateSupportedResolutions();
};

#endif   // WEBCAMDEVICE_H
