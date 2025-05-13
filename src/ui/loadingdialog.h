#ifndef LOADINGDIALOG_H
#define LOADINGDIALOG_H

#include <QTimer>
#include <QLabel>
#include <QVBoxLayout>
#include <QDialog>

#include <DSpinner>

DWIDGET_USE_NAMESPACE
/**
 * @brief The LoadingDialog class provides a modal loading dialog with timeout
 */
class LoadingDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LoadingDialog(QWidget *parent = nullptr);
    ~LoadingDialog();

    void showWithTimeout(int timeoutMs = 10000);
    void setText(const QString &text);

private:
    QTimer *m_timeoutTimer;
    QLabel *m_loadingLabel;
    QLabel *m_textLabel;
    DSpinner *m_spinner;
};

#endif // LOADINGDIALOG_H
