#include "loadingdialog.h"
#include <QApplication>
#include <QScreen>
#include <QGuiApplication>
#include <DSpinner>

LoadingDialog::LoadingDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setModal(true);

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
}

LoadingDialog::~LoadingDialog()
{
    m_spinner->stop();
}

void LoadingDialog::showWithTimeout(int timeoutMs)
{
    m_timeoutTimer->start(timeoutMs);
    m_spinner->start();
    
    // Center dialog on parent or screen
    if (parentWidget()) {
        move(parentWidget()->mapToGlobal(parentWidget()->rect().center()) - rect().center());
    } else {
        QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
        if (screen) {
            move(screen->geometry().center() - rect().center());
        }
    }
    
    show();
}

void LoadingDialog::setText(const QString &text)
{
    m_textLabel->setText(text);
}
