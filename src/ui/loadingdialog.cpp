// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "loadingdialog.h"
#include <QEvent>
#include <DSpinner>

LoadingDialog::LoadingDialog(QWidget *parent)
    : QDialog(parent)
{
    // A child widget of the parent window, not a window of its own: it does not take the
    // input away from the window (Qt::Dialog would, which made the title bar of the main
    // window undraggable while loading) and it moves together with the window.
    setWindowFlags(Qt::Widget);
    // setWindowFlags() clears the hidden state, so a child overlay would otherwise become
    // visible together with the parent window: it must stay hidden until showWithTimeout().
    hide();
    setAttribute(Qt::WA_TranslucentBackground);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(10);

    m_spinner = new DSpinner(this);
    m_spinner->setFixedSize(48, 48);
    m_spinner->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_spinner->setFocusPolicy(Qt::NoFocus);

    m_textLabel = new QLabel(tr("Loading..."), this);
    m_textLabel->setAlignment(Qt::AlignCenter);

    layout->addWidget(m_spinner, 0, Qt::AlignCenter);
    layout->addSpacing(15);
    layout->addWidget(m_textLabel, 0, Qt::AlignCenter);

    setFixedSize(220, 220);

    m_timeoutTimer = new QTimer(this);
    connect(m_timeoutTimer, &QTimer::timeout, this, &LoadingDialog::close);

    // Keep the overlay centered when the window is resized while loading.
    if (parent) {
        parent->installEventFilter(this);
    }
}

LoadingDialog::~LoadingDialog()
{
    m_spinner->stop();
}

void LoadingDialog::showWithTimeout(int timeoutMs)
{
    m_timeoutTimer->start(timeoutMs);
    m_spinner->start();

    centerOnParent();
    show();
    raise();   // stay above the device list or scan page underneath
}

void LoadingDialog::setText(const QString &text)
{
    m_textLabel->setText(text);
}

void LoadingDialog::centerOnParent()
{
    QWidget *parent = parentWidget();
    if (parent) {
        move((parent->width() - width()) / 2, (parent->height() - height()) / 2);
    }
}

bool LoadingDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (isVisible() && watched == parentWidget() && event->type() == QEvent::Resize) {
        centerOnParent();
    }
    return QDialog::eventFilter(watched, event);
}
