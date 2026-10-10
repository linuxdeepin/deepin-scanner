// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "scannerswidget.h"

#include <QSharedPointer>
#include <QHBoxLayout>
#include <QListWidgetItem>

#include <DLabel>
#include <DFrame>
#include <DIcon>
#include <DPushButton>

ScannersWidget::ScannersWidget(QWidget *parent) : DWidget(parent)
{
    setupUI();
}

void ScannersWidget::setupUI()
{
    DFrame *mainFrame = new DFrame();
    QVBoxLayout *mainLayout = new QVBoxLayout(mainFrame);
    mainLayout->setContentsMargins(0, 10, 0, 10);

    // Title bar with refresh button
    QHBoxLayout *titleLayout = new QHBoxLayout();
    DLabel *titleLabel = new DLabel(tr("Scanner Devices"));
    QFont font = titleLabel->font();
    font.setBold(true);
    font.setPointSize(font.pointSize() + 5);
    titleLabel->setFont(font);

    DIconButton *refreshButton = new DIconButton();
    refreshButton->setIcon(QIcon::fromTheme("refresh"));
    refreshButton->setIconSize(QSize(40, 40));
    refreshButton->setFixedSize(50, 50);
    m_refreshButton = refreshButton;

    // Searching feedback, centered between the title and the refresh button. The SANE
    // enumeration takes seconds on network backends, so it is worth showing what is going
    // on - but not in the device list: the cameras are already known and must stay usable.
    m_searchSpinner = new DSpinner();
    m_searchSpinner->setFixedSize(20, 20);
    m_searchLabel = new DLabel(tr("Loading devices..."));
    setScannerSearching(false);

    titleLayout->addSpacing(20);
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(m_searchSpinner);
    titleLayout->addWidget(m_searchLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(refreshButton);
    titleLayout->addSpacing(20);

    mainLayout->addLayout(titleLayout);

    // Device list
    deviceList = new DListWidget();
    deviceList->setSpacing(5);
    deviceList->setSelectionMode(QAbstractItemView::NoSelection);
    deviceList->setFrameShape(QFrame::NoFrame);
    deviceList->setContentsMargins(0, 0, 0, 0);

    mainLayout->addWidget(deviceList);

    QVBoxLayout *outerLayout = new QVBoxLayout(this);
    outerLayout->addWidget(mainFrame);
    outerLayout->setContentsMargins(10, 10, 10, 10);

    connect(refreshButton, &DIconButton::clicked, this, [this]() {
        emit updateDeviceListRequested();
    });
}

void ScannersWidget::updateDeviceList(QSharedPointer<ScannerDevice> scanner, QSharedPointer<WebcamDevice> webcam)
{
    // Both calls are cheap here: the scanner returns the list the worker has already
    // enumerated (see ScannerDevice::requestAvailableDevices()) and the cameras are a
    // local /dev scan.
    m_scannerNames = scanner->getAvailableDevices();
    m_webcamNames = webcam->getAvailableDevices();
    rebuildList();
}

void ScannersWidget::updateWebcamDevices(const QStringList &names)
{
    m_webcamNames = names;
    rebuildList();
}

void ScannersWidget::removeDeviceItem(const QString &name)
{
    if (m_scannerNames.removeOne(name) || m_webcamNames.removeOne(name)) {
        rebuildList();
    }
}

void ScannersWidget::setScannerSearching(bool searching)
{
    m_scannerSearching = searching;
    m_searchSpinner->setVisible(searching);
    m_searchLabel->setVisible(searching);
    m_refreshButton->setEnabled(!searching);

    if (searching) {
        m_searchSpinner->start();
    } else {
        m_searchSpinner->stop();
    }
}

void ScannersWidget::rebuildList()
{
    deviceList->clear();

    for (const QString &name : m_scannerNames) {
        addDeviceItem(name, tr("Scanner"), DeviceBase::DeviceStatus::Idle, true);
    }

    for (const QString &name : m_webcamNames) {
        addDeviceItem(name, tr("Webcam"), DeviceBase::DeviceStatus::Idle, false);
    }

    // Update UI based on device availability. An enumeration that is still in flight means the
    // list is not known yet, so saying "no devices found" at the same time as the title row
    // says "Loading devices..." would contradict it.
    if (deviceList->count() == 0 && !m_scannerSearching) {
        addPlaceholder(tr("No devices found"));
    }
}

void ScannersWidget::addPlaceholder(const QString &text)
{
    DLabel *label = new DLabel(text);
    label->setAlignment(Qt::AlignCenter);
    deviceList->addItem(new QListWidgetItem());
    deviceList->setItemWidget(deviceList->item(deviceList->count() - 1), label);
}

void ScannersWidget::addDeviceItem(const QString &name, const QString &model,
                                   const DeviceBase::DeviceStatus status, bool isScanner)
{
    QListWidgetItem *item = new QListWidgetItem(deviceList);
    item->setData(Qt::UserRole, name);
    item->setData(Qt::UserRole + 1, isScanner);

    DFrame *itemWidget = new DFrame();
    itemWidget->setBackgroundRole(QPalette::Window);

    QHBoxLayout *layout = new QHBoxLayout(itemWidget);

    // Device icon
    DLabel *iconLabel = new DLabel();
    // TODO: Replace with actual device icon
    iconLabel->setFixedSize(120, 120);
    QIcon icon;
    if (isScanner) {
        icon = QIcon::fromTheme("ic_scanner");
    } else {
        icon = QIcon::fromTheme("ic_hicam");
    }
    iconLabel->setPixmap(icon.pixmap(120, 120));
    layout->addWidget(iconLabel);

    // Device info
    QVBoxLayout *infoLayout = new QVBoxLayout();
    DLabel *nameLabel = new DLabel(name);
    DLabel *modelLabel = new DLabel();
    DLabel *statusLabel = new DLabel();
    infoLayout->addWidget(nameLabel);
    infoLayout->addWidget(modelLabel);
    infoLayout->addWidget(statusLabel);
    infoLayout->setContentsMargins(10, 10, 10, 10);
    layout->addLayout(infoLayout);
    layout->addStretch();

    // Scan button
    DPushButton *scanButton = new DPushButton(tr("Scan"));
    scanButton->setProperty("deviceName", name);
    scanButton->setProperty("isScanner", isScanner);
    scanButton->setFixedWidth(200);
    scanButton->setFixedHeight(50);
    layout->addWidget(scanButton);
    layout->setContentsMargins(10, 10, 10, 10);

    connect(scanButton, &DPushButton::clicked, this, [this, name, isScanner]() {
        emit deviceSelected(name, isScanner);
    });

    // Convert status enum to display text
    QString statusText;
    switch (status) {
    case DeviceBase::DeviceStatus::Idle:
        statusText = tr("Idle");
        scanButton->setEnabled(true);
        break;
    case DeviceBase::DeviceStatus::Offline:
        statusText = tr("Offline");
        scanButton->setEnabled(false);
        break;
    }
    modelLabel->setText(tr("Model: %1").arg(model));
    statusLabel->setText(tr("Status: %1").arg(statusText));

    // itemWidget->setLayout(layout);
    item->setSizeHint(itemWidget->sizeHint());
    deviceList->setItemWidget(item, itemWidget);
}
