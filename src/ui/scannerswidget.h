// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SCANNERSWIDGET_H
#define SCANNERSWIDGET_H

#include "device/scannerdevice.h"
#include "device/webcamdevice.h"

#include <QVBoxLayout>
#include <QSharedPointer>

#include <DWidget>
#include <DIconButton>

DWIDGET_USE_NAMESPACE

class ScannersWidget : public DWidget
{
    Q_OBJECT
public:
    explicit ScannersWidget(QWidget *parent = nullptr);

    void updateDeviceList(QSharedPointer<ScannerDevice> scanner, QSharedPointer<WebcamDevice> webcam);

    // Updates the camera entries only, used for the cheap local re-enumeration when the
    // device list is shown again.
    void updateWebcamDevices(const QStringList &names);

    // Drops a device that is known to be unusable from the list, without re-enumerating
    // the devices.
    void removeDeviceItem(const QString &name);

signals:
    void deviceSelected(const QString &deviceName, bool isScanner);
    void updateDeviceListRequested();

private:
    DListWidget *deviceList;
    QStringList m_scannerNames;
    QStringList m_webcamNames;

    void setupUI();
    void rebuildList();
    void addDeviceItem(const QString &name, const QString &model,
                       DeviceBase::DeviceStatus status, bool isScanner);
};

#endif   // SCANNERSWIDGET_H
