#ifndef WaitDialog_H
#define WaitDialog_H

#include <QProgressDialog>

class WaitDialog : public QProgressDialog {
    Q_OBJECT
public:
    explicit WaitDialog(const QString & labelText,QWidget *parent = 0);

private slots:
    void show_error(const QString & error);
};

#endif // WaitDialog_H
