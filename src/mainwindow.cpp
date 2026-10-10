// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindow.h"
#include "ddlog.h"

#include <QSharedPointer>
#include <QMessageBox>
#include <QFileDialog>
#include <QDebug>
#include <QFileInfo>
#include <QTimer>
#include <QApplication>
#include <QThread>
#include <QProcess>
#include <QScreen>
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrent>

#include <DTitlebar>
#include <DMessageManager>

using namespace DDLog;

MainWindow::MainWindow(QWidget *parent)
    : DMainWindow(parent)
{
    // --- Initialize Devices ---
    // Note: Don't pass 'this' as parent when using QSharedPointer to avoid double-deletion
    QSharedPointer<ScannerDevice> scannerDevice(new ScannerDevice(nullptr));
    scannerDevice->initialize();
    QSharedPointer<WebcamDevice> webcamDevice(new WebcamDevice(nullptr));
    webcamDevice->initialize();

    // Store devices in map
    m_devices["scanner"] = scannerDevice;
    m_devices["webcam"] = webcamDevice;

    // --- 创建主界面 ---
    setWindowTitle(tr("Scanner Manager"));
    QRect screenRect = QGuiApplication::primaryScreen()->geometry();
    resize(screenRect.width() / 2, screenRect.height() / 2);
    move((screenRect.width() - width()) / 2, (screenRect.height() - height()) / 2);

    // 创建中心部件和堆叠布局
    DWidget *centralWidget = new DWidget(this);
    m_stackLayout = new QStackedLayout(centralWidget);

    // 初始化两个界面
    m_scannersWidget = new ScannersWidget();
    m_scanWidget = new ScanWidget();

    // 添加到堆叠布局
    m_stackLayout->addWidget(m_scannersWidget);
    m_stackLayout->addWidget(m_scanWidget);

    // 默认显示设备列表界面
    m_stackLayout->setCurrentWidget(m_scannersWidget);

    setCentralWidget(centralWidget);

    // 连接信号槽
    connect(m_scannersWidget, &ScannersWidget::deviceSelected,
            this, &MainWindow::showScanView);
    connect(m_scannersWidget, &ScannersWidget::updateDeviceListRequested,
            this, [this]() { updateDeviceList(); });

    // A device that is known to be unusable (it was disconnected, or it cannot be
    // opened) is dropped from the list instead of staying there as a dead entry.
    connect(scannerDevice.data(), &ScannerDevice::deviceUnavailable,
            m_scannersWidget, &ScannersWidget::removeDeviceItem);

    // A camera that turned out to be gone (it was unplugged) is dropped from the list and
    // reported to the user.
    connect(webcamDevice.data(), &WebcamDevice::deviceUnavailable, this,
            [this](const QString &deviceName, const QString &message) {
                m_scannersWidget->removeDeviceItem(deviceName);
                DMessageManager::instance()->sendMessage(this, QIcon::fromTheme("dialog-warning"), message);
            });

    // The device list is rebuilt once the worker has enumerated the scanners.
    connect(scannerDevice.data(), &ScannerDevice::availableDevicesReady, this,
            [this](const QStringList &) {
                auto scanner = qSharedPointerCast<ScannerDevice>(m_devices["scanner"]);
                auto webcam = qSharedPointerCast<WebcamDevice>(m_devices["webcam"]);

                // A network scanner can still be getting discovered while the first
                // enumeration runs, so ask once more before leaving the list without it. The
                // list stays in the searching state until that retry is back, otherwise it
                // would claim "no devices found" while the answer is still coming.
                static bool retriedOnce = false;
                const bool retry = !retriedOnce && scanner && scanner->getAvailableDevices().isEmpty();
                if (retry) {
                    retriedOnce = true;
                    QTimer::singleShot(3000, this, [this]() {
                        auto scanner = qSharedPointerCast<ScannerDevice>(m_devices["scanner"]);
                        if (scanner) {
                            scanner->requestAvailableDevices();
                        }
                    });
                }

                if (scanner && webcam) {
                    m_scannersWidget->setScannerSearching(retry);
                    m_scannersWidget->updateDeviceList(scanner, webcam);
                }
            });

    // 设置标题栏logo
    auto titleBar = titlebar();
    // titleBar->setIcon(QIcon(":/resources/logo.svg"));
    titleBar->setIcon(QIcon::fromTheme("deepin-scanner"));

    m_backBtn = new DIconButton();
    m_backBtn->setIcon(QIcon::fromTheme("go-previous"));
    m_backBtn->setVisible(false);   // 初始隐藏
    titleBar->addWidget(m_backBtn, Qt::AlignLeft);


    // 连接返回按钮信号
    connect(m_backBtn, &DIconButton::clicked,
            this, &MainWindow::showDeviceListView);

    m_loadingDialog = new LoadingDialog(this);

    updateDeviceList();
}

MainWindow::~MainWindow()
{
    // A camera reconfiguration started from the UI keeps using the device on a worker
    // thread: let it finish before the devices are released here.
    m_scanWidget->waitForPendingCameraWork();
    m_devices.clear();
}

void MainWindow::updateDeviceList()
{
    qDebug(app) << "Updating device list...";

    // 使用类型安全的指针转换
    auto scanner = qSharedPointerCast<ScannerDevice>(m_devices["scanner"]);
    auto webcam = qSharedPointerCast<WebcamDevice>(m_devices["webcam"]);

    if (!scanner || !webcam) {
        qDebug(app) << "Error: Failed to cast device pointers";
        return;
    }

    // The cameras are a local enumeration and the scanner list arrives later, so the list is
    // shown right away with the scanner section marked as still searching. No overlay here:
    // the SANE enumeration blocks for seconds when a network backend is slow, and waiting for
    // it would only hide the cameras that are already known.
    m_scannersWidget->setScannerSearching(true);
    m_scannersWidget->updateDeviceList(scanner, webcam);

    scanner->requestAvailableDevices();
}

void MainWindow::showScanView(const QString &device, bool isScanner)
{
    if (m_loadingDialog->isVisible()) {
        // Something is already loading. The window stays movable while loading, so a second
        // click on a device has to be ignored instead of opening the device a second time.
        return;
    }

    m_currentDevice = device;
    m_isCurrentScanner = isScanner;

    showLoading(tr("Opening device..."));

    // 设置当前设备指针
    auto devicePtr = isScanner ? m_devices["scanner"] : m_devices["webcam"];
    qDebug(app) << "Current device: " << m_currentDevice;

    bool opened = false;
    if (isScanner) {
        // The scanner only queues the request, the SANE call itself happens in the worker
        // thread, so it is started here: that keeps the scanner state owned by the UI
        // thread.
        opened = devicePtr->openDevice(m_currentDevice);
    } else {
        // The camera is opened synchronously (V4L2), so it is opened off the UI thread.
        QFuture<bool> future = QtConcurrent::run([=]() {
            return devicePtr->openDevice(m_currentDevice);
        });

        // 等待任务完成
        QFutureWatcher<bool> watcher;
        QEventLoop loop;
        QObject::connect(&watcher, &QFutureWatcher<bool>::finished, &loop, &QEventLoop::quit);
        watcher.setFuture(future);
        loop.exec();
        opened = watcher.result();
    }
    if (!opened) {
        QTimer::singleShot(500, this, &MainWindow::hideLoading);
        qDebug(app) << "Failed to open device" << m_currentDevice;
        // The device could not be opened, so it is not there any more: drop it from the
        // list and tell the user instead of leaving a dead entry behind.
        m_scannersWidget->removeDeviceItem(m_currentDevice);
        DMessageManager::instance()->sendMessage(this, QIcon::fromTheme("dialog-warning"),
                                                 tr("Failed to open the device."));
        return;
    }
    m_scanWidget->setupDeviceMode(devicePtr.data(), m_currentDevice);
    m_stackLayout->setCurrentWidget(m_scanWidget);
    m_scanWidget->startCameraPreview();

    m_backBtn->setVisible(true);

    QTimer::singleShot(500, this, &MainWindow::hideLoading);
}

void MainWindow::showDeviceListView()
{
    m_backBtn->setVisible(false);
    // 切换到设备列表界面
    m_stackLayout->setCurrentWidget(m_scannersWidget);

    // Refresh the camera entries: enumerating /dev/video* is a local operation, so a
    // camera that was unplugged does not stay in the list as a clickable dead entry.
    // SANE devices are deliberately not re-enumerated here, that can block for seconds
    // on network devices and stays with the refresh button.
    auto webcam = qSharedPointerCast<WebcamDevice>(m_devices["webcam"]);
    if (webcam) {
        // The preview keeps the camera open; stop it before enumerating so that the
        // enumeration does not have to open a camera that this process still holds.
        webcam->stopPreview();
        m_scannersWidget->updateWebcamDevices(webcam->getAvailableDevices());
    }
}

void MainWindow::showLoading(const QString &message, int timeoutMs)
{
    if (!message.isEmpty()) {
        m_loadingDialog->setText(message);
    }
    m_loadingDialog->showWithTimeout(timeoutMs);
}

void MainWindow::hideLoading()
{
    m_loadingDialog->close();
}
