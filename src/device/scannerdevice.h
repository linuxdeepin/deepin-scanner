// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SCANNERDEVICE_H
#define SCANNERDEVICE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QImage>
#include "devicebase.h"

// Only include and use SANE on non-Windows platforms
#ifndef _WIN32   // Or use Q_OS_WIN if preferred
#    include <sane/sane.h>
#    include <png.h>   // For the scan_it implementation details
#    include <stdio.h>   // For FILE* used in scan_it
#endif

// Forward declaration for Image struct if needed within scan_it
struct Image;

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

    // Helper structure used by scanning logic
    // You need to provide the definition for this based on your original code.
    struct Image
    {
        unsigned char *data;
        int width;
        int height;
        int x, y;
        int line_buffer_size;   // Example member, adjust as needed
    };

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
    bool advance(Image *im);

    // Scan mode related methods
    // Set scan mode (flatbed, ADF simplex, ADF duplex)
    bool setScanMode(ScanMode mode);
    
    // Get supported scan modes list
    QList<ScanMode> getSupportedScanModes();
    
    // Set scan resolution (DPI)
    bool setResolution(int dpi);
    
    // Get current resolution
    int getResolution() const;
    
    // Get supported resolutions list
    QList<int> getSupportedResolutions();

signals:
    // // Emitted when the scan successfully completes.
    // void scanCompleted(const QImage &scannedImage);

    // // Emitted if an error occurs during initialization, opening, or scanning.
    // void errorOccurred(const QString &errorMessage);

    // Emitted periodically during scanning to report progress (0-100).
    // Note: Requires modification in scan_it to emit this.
    void scanProgress(int percentage);

    // New signal
    void previewLineAvailable(const QImage &line);   // For real-time scan line preview

private:
#ifndef _WIN32
    // The core scanning function (refactored from your original code).
    // Returns SANE_STATUS_GOOD on success.
    SANE_Status scan_it(FILE *ofp);

    SANE_Handle m_device = nullptr;   // Handle for the currently open device
    bool m_saneInitialized = false;   // Flag indicating if sane_init was called
#endif
    bool m_deviceOpen = false;   // Flag indicating if a device is open
    bool m_usingTestDevice = false;

    ScanMode m_currentScanMode = SCAN_MODE_FLATBED; // Current scan mode
    int m_currentResolutionDPI = 300; // Current scan resolution (DPI)

    void generateTestImage(const QString &outputPath);
};

#endif   // SCANNERDEVICE_H