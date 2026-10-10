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
#include <DLabel>
#include <DSpinner>

DWIDGET_USE_NAMESPACE

class ScannersWidget : public DWidget
{
    Q_OBJECT
public:
    explicit ScannersWidget(QWidget *parent = nullptr);

    void updateDeviceList(QSharedPointer<ScannerDevice> scanner, QSharedPointer<WebcamDevice> webcam);

    // Marks the SANE enumeration as still running: the title row then shows a spinner with a
    // "Loading devices..." hint and the refresh button is disabled. The feedback deliberately
    // does not sit in the list, so the devices that are already known (the local cameras) stay
    // on the first row and can be used while the scanners are still being enumerated.
    void setScannerSearching(bool searching);

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
    DIconButton *m_refreshButton = nullptr;
    DSpinner *m_searchSpinner = nullptr;
    DLabel *m_searchLabel = nullptr;
    QStringList m_scannerNames;
    QStringList m_webcamNames;
    // An enumeration is in flight: the device list is still unknown, so the empty state must
    // not be shown yet.
    bool m_scannerSearching = false;

    void setupUI();
    void rebuildList();
    void addPlaceholder(const QString &text);
    void addDeviceItem(const QString &name, const QString &model,
                       DeviceBase::DeviceStatus status, bool isScanner);
};

#endif   // SCANNERSWIDGET_H
