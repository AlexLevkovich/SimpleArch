#ifndef WAITVIEW_H
#define WAITVIEW_H

#include <QGraphicsView>
#include "busyindicator.h"
#include <QGraphicsScene>
#include <QTimer>

class WaitView : public QGraphicsView {
    Q_OBJECT
public:
    explicit WaitView(QWidget *parent = 0);

protected:
    void showEvent(QShowEvent * event);
    void hideEvent(QHideEvent * event);

private slots:
    void rotateSpinner();

private:
    QGraphicsScene m_scene;
    BusyIndicator m_busyIndicator;
    QTimer m_timer;
    int angle;
};

#endif // WAITVIEW_H
