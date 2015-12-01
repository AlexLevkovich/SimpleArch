#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <QEventLoop>

class QEvent;

class EventLoop : public QEventLoop {
    Q_OBJECT
public:
    EventLoop(QObject *parent = 0);
    ~EventLoop();

protected:
    bool eventFilter(QObject *obj, QEvent *event);

public slots:
    void cancel();
};

#endif // EVENTLOOP_H
