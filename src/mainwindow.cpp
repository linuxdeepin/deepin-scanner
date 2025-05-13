// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindow.h"
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

MainWindow::MainWindow(QWidget *parent)
    : DMainWindow(parent)
{
    // --- Initialize Devices ---
    QSharedPointer<ScannerDevice> scannerDevice(new ScannerDevice(this));
    QSharedPointer<WebcamDevice> webcamDevice(new WebcamDevice(this));

    // Store devices in map
    m_devices["scanner"] = scannerDevice;
    m_devices["webcam"] = webcamDevice;

    // Initialize SANE (Scanner)
    if (!scannerDevice->initializeSane()) {
        QMessageBox::critical(this, tr("Scanner error"), tr("Failed to initialize SANE backend.\nPlease ensure SANE libraries (e.g. sane-backends) are installed and you may need to configure permissions (e.g. add user to 'scanner' or 'saned' group).\nScanner functionality will be unavailable."));
    }

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
    qDebug() << "Updating device list...";
    showLoading(tr("Loading devices..."));

    // 检查设备是否初始化
    if (!m_devices["scanner"] || !m_devices["webcam"]) {
        qDebug() << "Error: Devices not initialized";
        return;
    }

    // 使用类型安全的指针转换
    auto scanner = qSharedPointerCast<ScannerDevice>(m_devices["scanner"]);
    auto webcam = qSharedPointerCast<WebcamDevice>(m_devices["webcam"]);

    if (scanner && webcam) {
        // 更新ScannersWidget中的设备列表
        m_scannersWidget->updateDeviceList(scanner, webcam);
    } else {
        qDebug() << "Error: Failed to cast device pointers";
    }


    QTimer::singleShot(500, this, &MainWindow::hideLoading);
}

void MainWindow::showScanView(const QString &device, bool isScanner)
{
    m_currentDevice = device;
    m_isCurrentScanner = isScanner;

    showLoading(tr("Opening device..."));

    // 设置当前设备指针
    auto devicePtr = isScanner ? m_devices["scanner"] : m_devices["webcam"];
    qDebug() << "Current device: " << m_currentDevice;
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
        qDebug() << "Failed to open device" << m_currentDevice;
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
