// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <DMainWindow>
#include <DIconButton>
#include <QStackedLayout>
#include <QComboBox>   // <--- 添加 QComboBox 用于分辨率选择

#include "device/scannerdevice.h"
#include "device/webcamdevice.h"
#include "ui/scannerswidget.h"
#include "ui/scanwidget.h"
#include "ui/loadingdialog.h"

DWIDGET_USE_NAMESPACE

class MainWindow : public DMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QMap<QString, QSharedPointer<DeviceBase>> m_devices;

    // UI Components
    QStackedLayout *m_stackLayout;
    ScannersWidget *m_scannersWidget;
    ScanWidget *m_scanWidget;

    // Current device info
    QString m_currentDevice;
    bool m_isCurrentScanner;

private slots:
    void showScanView(const QString &device, bool isScanner);
    void showDeviceListView();
    void updateDeviceList();

    void showLoading(const QString &message = QString(), int timeoutMs = 10000);
    void hideLoading();
private:
    DIconButton *m_backBtn = nullptr;
    LoadingDialog *m_loadingDialog = nullptr;
};

#endif   // MAINWINDOW_H