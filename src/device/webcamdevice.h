// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
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
#include <QThreadPool>
#include <QDebug>
#include <atomic>
#include <mutex>
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
    // S_FMT/REQBUFS/STREAMON, which takes about a second on a 1080p camera. The caller
    // decides on which thread it runs; it does nothing when the stream is already on.
    bool startStream();
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
    // Emitted when the device turned out to be gone (e.g. it was unplugged), carrying a
    // message that can be shown to the user as is.
    void deviceUnavailable(const QString &deviceName, const QString &message);

private slots:
    void updatePreview();

private:
    int m_fd;   // Device file descriptor
    void *m_buffers[4];   // Video buffers
    size_t m_bufferSizes[4];   // Buffer sizes
    int m_currentBuffer;   // Currently used buffer
    std::atomic<bool> m_isInitialized = false;
    bool m_deviceSelected;
    // Opening, closing and reconfiguring all close and re-open the descriptor, and they can
    // be asked for from the UI thread and from worker threads at the same time. Recursive
    // because openDevice() and setResolution() close the previous device as their first step.
    std::recursive_mutex m_deviceMutex;
    // Whether the V4L2 stream is on, so that startStream() does not restart an active one.
    std::atomic<bool> m_streaming = false;
    // Set once a broken stream has been reported, so a device that is gone is reported
    // only once instead of on every preview tick.
    std::atomic<bool> m_disconnectReported = false;
    // Dequeuing a frame, waiting for one and converting it are all too slow for the UI
    // thread, so they run here: a single thread that keeps the preview and the capture
    // from touching the device at the same time.
    QThreadPool m_ioPool;
    std::atomic<bool> m_frameTaskRunning = false;   // a preview frame is being processed
    std::atomic<bool> m_captureRunning = false;   // a capture is in flight
    std::atomic<bool> m_captureAbort = false;   // a capture in flight has to give up
    std::atomic<bool> m_previewRunning = false;   // the preview timer is running
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
    // captureImage() and dequeueFrame() run on the I/O thread, see m_ioPool.
    void captureImage();
    QImage dequeueFrame();
    // The preview timer belongs to the UI thread, so the I/O thread reaches it through these.
    void pausePreviewTimer();
    void resumePreviewTimer();
    // Gives up the capture in flight and waits for the I/O thread to go idle, so that the
    // device can be closed, re-opened or reconfigured safely.
    void stopIoWork();
    bool startCapturing();
    void stopCapturing();
    bool selectBestPixelFormat();
    void diagnoseCameraIssues();
    // 新增私有方法
    void enumerateSupportedResolutions();
};

#endif   // WEBCAMDEVICE_H
