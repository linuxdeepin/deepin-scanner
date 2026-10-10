// SPDX-FileCopyrightText: 2025 - 2026 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef LOADINGDIALOG_H
#define LOADINGDIALOG_H

#include <QTimer>
#include <QLabel>
#include <QVBoxLayout>
#include <QDialog>

#include <DSpinner>

DWIDGET_USE_NAMESPACE
/**
 * @brief The LoadingDialog class provides a loading overlay on the main window, with timeout
 *
 * It is a child widget of the window it belongs to, not a second window: the window stays
 * movable while loading, and the overlay follows it because they share the same surface.
 */
class LoadingDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LoadingDialog(QWidget *parent = nullptr);
    ~LoadingDialog();

    void showWithTimeout(int timeoutMs = 10000);
    void setText(const QString &text);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void centerOnParent();

    QTimer *m_timeoutTimer;
    QLabel *m_loadingLabel;
    QLabel *m_textLabel;
    DSpinner *m_spinner;
};

#endif // LOADINGDIALOG_H
