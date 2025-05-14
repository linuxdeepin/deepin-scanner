// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
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

signals:
    void deviceSelected(const QString &deviceName, bool isScanner);
    void updateDeviceListRequested();

private:
    DListWidget *deviceList;

    void setupUI();
    void addDeviceItem(const QString &name, const QString &model,
                       DeviceBase::DeviceStatus status, bool isScanner);
};

#endif   // SCANNERSWIDGET_H
