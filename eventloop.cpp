#include "eventloop.h"
#include "staticutils.h"
#include <QEvent>
#include <QApplication>
#include <QMainWindow>

EventLoop::EventLoop(QObject *parent) : QEventLoop(parent) {
    QMainWindow * wnd = StaticUtils::findMainWindow();
    if (wnd != NULL) {
        wnd->installEventFilter(this);

        QObjectList childs = wnd->children();
        for (int i=0;i<childs.count();i++) {
            if (childs.at(i)->inherits("QWidget")) childs.at(i)->installEventFilter(this);
        }
    }
}

EventLoop::~EventLoop() {
    QMainWindow * wnd = StaticUtils::findMainWindow();
    if (wnd != NULL) {
        wnd->removeEventFilter(this);
        QObjectList childs = wnd->children();
        for (int i=0;i<childs.count();i++) {
            if (childs.at(i)->inherits("QWidget")) childs.at(i)->removeEventFilter(this);
        }
    }
}

void EventLoop::cancel() {
    exit(1);
}

bool EventLoop::eventFilter(QObject *obj, QEvent *event) {
    if ((event->type() == QEvent::KeyPress) ||
        (event->type() == QEvent::KeyRelease) ||
        (event->type() == QEvent::MouseButtonDblClick) ||
        (event->type() == QEvent::MouseButtonPress) ||
        (event->type() == QEvent::MouseButtonRelease) ||
        (event->type() == QEvent::MouseMove)) {
        if (!obj->inherits("QLineEdit") &&
            !obj->inherits("QPushButton") &&
            !obj->inherits("QScrollBar") &&
            !obj->inherits("QScrollArea")) return true;
    }

    if ((event->type() == QEvent::Show) && obj->inherits("QDialog")) {
        if (QApplication::overrideCursor() != NULL) QApplication::changeOverrideCursor(Qt::ArrowCursor);
    }

    if ((event->type() == QEvent::Hide) && obj->inherits("QDialog")) {
        if (QApplication::overrideCursor() != NULL) QApplication::changeOverrideCursor(Qt::WaitCursor);
    }

    if (event->type() == QEvent::ChildAdded) {
        QChildEvent* ce = static_cast<QChildEvent*>(event);
        ce->child()->installEventFilter(this);
    }

    if (event->type() == QEvent::ChildRemoved) {
        QChildEvent* ce = static_cast<QChildEvent*>(event);
        ce->child()->removeEventFilter(this);
    }

    return QEventLoop::eventFilter(obj, event);
}
