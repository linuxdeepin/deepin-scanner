// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "mainwindow.h"
#include "logger.h"

#include <DApplication>
#include <DGuiApplicationHelper>
#include <QTimer>
#include <QToolTip>
#include <unistd.h>

DGUI_USE_NAMESPACE
DWIDGET_USE_NAMESPACE

int main(int argc, char *argv[])
{
    // 日志处理要放在app之前，否则QApplication 内部可能进行了日志打印，导致环境变量设置不生效
    MLogger();

    DApplication app(argc, argv);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    app.setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    // set single instance
    if (!app.setSingleInstance("deepin-scanner")) {
        qWarning() << "set single instance failed!I (pid:" << getpid() << ") will exit.";
        return -1;
    }

    app.loadTranslator();
    // set application information
    app.setOrganizationName("deepin");
    app.setApplicationDisplayName(app.translate("Application", "Deepin Scanner"));
    app.setApplicationVersion(APP_VERSION);
    app.setProductIcon(QIcon::fromTheme("deepin-scanner"));
    app.setApplicationAcknowledgementPage("https://www.deepin.org/acknowledgments/");
    app.setApplicationDescription(app.translate("Application", "Scanner Manager is a scanner tool that supports a variety of scanning devices"));

    // Initialize logging system
    MLogger::initLogger();

    // Sync QToolTip palette with application palette so that tooltip text
    // follows the current theme (e.g. inverted in dark mode). DTK updates
    // QGuiApplication::palette() on theme switch but does not propagate the
    // change to QToolTip's standalone global palette.
    //
    // The initial sync is deferred to the next event-loop iteration because
    // DTK initializes the platform theme and applies the correct palette via
    // queued signal deliveries — at this point (before app.exec()) the
    // application palette may still hold the default light-mode colors.
    auto syncToolTipPalette = []() {
        QPalette tipPalette = QToolTip::palette();
        const QPalette &appPalette = qApp->palette();
        tipPalette.setColor(QPalette::ToolTipBase, appPalette.color(QPalette::ToolTipBase));
        tipPalette.setColor(QPalette::ToolTipText, appPalette.color(QPalette::ToolTipText));
        QToolTip::setPalette(tipPalette);
    };
    QTimer::singleShot(0, qApp, syncToolTipPalette);
    QObject::connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::applicationPaletteChanged,
                     qApp, syncToolTipPalette);

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
