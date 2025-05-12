// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef DEVICEBASE_H
#define DEVICEBASE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QImage>
#include <QSharedPointer>

class DeviceBase : public QObject
{
    Q_OBJECT

public:
    enum DeviceState {
        Disconnected,
        Initialized,
        Connected,
        Capturing
    };

    enum DeviceType {
        Scanner,
        Webcam
    };

    enum DeviceStatus {
        Idle,
        Offline
    };

    explicit DeviceBase(QObject *parent = nullptr);
    virtual ~DeviceBase() = default;

    // Device management
    virtual bool initialize() = 0;
    virtual QStringList getAvailableDevices() = 0;
    virtual bool openDevice(const QString &deviceName) = 0;
    virtual void closeDevice() = 0;

    // Capture control
    virtual void startCapture() = 0;
    virtual void stopCapture() = 0;
    virtual bool isCapturing() const = 0;

    // Device information
    virtual DeviceType getDeviceType() const = 0;
    virtual QString currentDeviceName() const = 0;
    DeviceState state() const { return m_state; }

signals:
    void deviceInitialized(bool success);
    void deviceOpened(bool success);
    void deviceClosed();
    void captureStarted();
    void imageCaptured(const QImage &image);
    void errorOccurred(const QString &error);
    // preview image signal
    void previewUpdated(const QImage &image);

protected:
    void setState(DeviceState state);
    QString m_currentDeviceName;

private:
    DeviceState m_state = Disconnected;
};

#endif   // DEVICEBASE_H