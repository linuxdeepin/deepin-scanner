// SPDX-FileCopyrightText: 2025 UnionTech Software Technology Co., Ltd.
//
// SPDX-Liteense-Identifier: GPL-3.0-or-later

#include "mainwindow.h"
#include "logger.h"

#include <DApplication>
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

    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
