#include "statusbar.h"
#include <QProgressBar>
#include <QApplication>

#define PROGRESS_WIDTH 100

StatusBar::StatusBar(QWidget *parent) : QStatusBar(parent) {
    progressBar = new QProgressBar(this);
    progressBar->setMaximumWidth(PROGRESS_WIDTH);
    progressBar->setMinimumWidth(PROGRESS_WIDTH);
    progressBar->setRange(0,100);
    progressBar->setVisible(false);
    addPermanentWidget(progressBar);
}

void StatusBar::setWaitStatus(bool status) {
    if (status) {
        progressBar->setRange(0,0);
        progressBar->setVisible(true);
    }
    else {
        progressBar->setVisible(false);
        progressBar->setRange(0,100);
    }
}
