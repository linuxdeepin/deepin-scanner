// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
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
    m_devices.clear();
}

void MainWindow::updateDeviceList()
{
    qDebug(app) << "Updating device list...";
    showLoading(tr("Loading devices..."));

    // 检查设备是否初始化
    if (!m_devices["scanner"] || !m_devices["webcam"]) {
        qDebug(app) << "Error: Devices not initialized";
        return;
    }

    // 使用类型安全的指针转换
    auto scanner = qSharedPointerCast<ScannerDevice>(m_devices["scanner"]);
    auto webcam = qSharedPointerCast<WebcamDevice>(m_devices["webcam"]);

    if (scanner && webcam) {
        // 给网络设备发现预留更多时间（特别是首次启动）
        static bool firstRun = true;
        int delay = firstRun ? 3000 : 500; // 首次启动等待3秒，后续等待0.5秒
        
        if (firstRun) {
            qCInfo(app) << "First device list update, allowing extra time for network device discovery...";
            firstRun = false;
        }
        
        // 使用定时器延迟更新设备列表
        QTimer::singleShot(delay, this, [this]() {
            // 重新获取设备指针，避免捕获过期指针
            auto scanner = qSharedPointerCast<ScannerDevice>(m_devices["scanner"]);
            auto webcam = qSharedPointerCast<WebcamDevice>(m_devices["webcam"]);
            
            if (scanner && webcam) {
                m_scannersWidget->updateDeviceList(scanner, webcam);
            }
            QTimer::singleShot(500, this, &MainWindow::hideLoading);
        });
    } else {
        qDebug(app) << "Error: Failed to cast device pointers";
        QTimer::singleShot(500, this, &MainWindow::hideLoading);
    }
}

void MainWindow::showScanView(const QString &device, bool isScanner)
{
    m_currentDevice = device;
    m_isCurrentScanner = isScanner;

    showLoading(tr("Opening device..."));

    // 设置当前设备指针
    auto devicePtr = isScanner ? m_devices["scanner"] : m_devices["webcam"];
    qDebug(app) << "Current device: " << m_currentDevice;
    // open the device first via concurrent thread
    QFuture<bool> future = QtConcurrent::run([=]() {
        return devicePtr->openDevice(m_currentDevice);
    });

    // 等待任务完成
    QFutureWatcher<bool> watcher;
    QEventLoop loop;
    QObject::connect(&watcher, &QFutureWatcher<bool>::finished, &loop, &QEventLoop::quit);
    watcher.setFuture(future);
    loop.exec();
    if (!watcher.result()) {
        QTimer::singleShot(500, this, &MainWindow::hideLoading);
        // TODO: show error message
        qDebug(app) << "Failed to open device" << m_currentDevice;
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
