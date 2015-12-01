#ifndef STATUSBAR_H
#define STATUSBAR_H

#include <QStatusBar>

class QProgressBar;
class QEvent;

class StatusBar : public QStatusBar {
    Q_OBJECT
public:
    explicit StatusBar(QWidget *parent = 0);
    void setWaitStatus(bool status);

private:
    QProgressBar * progressBar;
};

#endif // STATUSBAR_H
