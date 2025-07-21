// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SCANNERDEVICE_H
#define SCANNERDEVICE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QImage>
#include <QThread>
#include "devicebase.h"

#ifndef _WIN32
#    include <sane/sane.h>
#endif

class ScannerWorker;

class ScannerDevice : public DeviceBase
{
    Q_OBJECT

public:
    enum ScanMode {
        SCAN_MODE_FLATBED,    // Flatbed scan mode
        SCAN_MODE_ADF_SIMPLEX, // ADF simplex scan
        SCAN_MODE_ADF_DUPLEX   // ADF duplex scan
    };
    Q_ENUM(ScanMode)

    explicit ScannerDevice(QObject *parent = nullptr);
    ~ScannerDevice() override;

    // DeviceBase interface implementation
    bool initialize() override;
    QStringList getAvailableDevices() override;
    bool openDevice(const QString &deviceName) override;
    void closeDevice() override;
    void startCapture() override;
    void stopCapture() override;
    bool isCapturing() const override;
    DeviceType getDeviceType() const override { return DeviceType::Scanner; }
    QString currentDeviceName() const override { return m_currentDeviceName; }

    // Extended scanner-specific interface
    void startScan(const QString &tempOutputFilePath);
    void cancelScan();
    bool setScanMode(ScanMode mode);
    QList<ScanMode> getSupportedScanModes();
    bool setResolution(int dpi);
    int getResolution() const;
    QList<int> getSupportedResolutions();

signals:
    // Signals to trigger worker operations
    void triggerOpenDevice(const QString &deviceName);
    void triggerCloseDevice();
    void triggerStartScan(const QString &filePath, int dpi, ScanMode mode);
    void triggerCancelScan();

    // Forwarded signals from worker
    void scanProgress(int percentage);
    void deviceOpened();
    void deviceClosed();

private slots:
    // Slots to handle results from the worker thread
    void onWorkerError(const QString &errorMessage);
    void onDeviceOpened(const QList<int> &resolutions, const QList<ScannerDevice::ScanMode> &modes);
    void onDeviceClosed();
    void onCaptureCompleted(const QString &filePath);

private:
    QString m_currentDeviceName;
    bool m_isCapturing = false;
    bool m_deviceOpen = false;
    bool m_scanPending = false; // True if a scan was requested before device was open

    QThread m_workerThread;
    ScannerWorker *m_worker = nullptr;

    QList<int> m_supportedResolutions;
    QList<ScanMode> m_supportedScanModes;
    int m_currentResolutionDPI = 300;
    ScanMode m_currentScanMode = SCAN_MODE_FLATBED;
};

// --- Worker Class for background scanning ---
class ScannerWorker : public QObject
{
    Q_OBJECT

public:
    explicit ScannerWorker();
    ~ScannerWorker() override;

public slots:
    void doOpenDevice(const QString &deviceName);
    void doCloseDevice();
    void doStartScan(const QString &tempOutputFilePath, int dpi, ScannerDevice::ScanMode mode);
    void doCancelScan();

signals:
    void errorOccurred(const QString &errorMessage);
    void deviceOpened(const QList<int> &resolutions, const QList<ScannerDevice::ScanMode> &modes);
    void deviceClosed();
    void captureCompleted(const QString &filePath);
    void scanProgress(int percentage);

private:
#ifndef _WIN32
    struct Image;
    SANE_Status scan_it(FILE *ofp);
    void updateSupportedOptions();
    void doSetResolution(int dpi);
    void doSetScanMode(ScannerDevice::ScanMode mode);

    SANE_Handle m_device = nullptr;
    bool m_deviceOpen = false;
    volatile bool m_scanCancelled = false;
#endif
    bool m_usingTestDevice = false;
    void generateTestImage(const QString &outputPath);
};

#endif // SCANNERDEVICE_H