#include "waitdialog.h"
#include <QIcon>
#include "scrollmessagebox.h"

WaitDialog::WaitDialog(const QString & labelText,QWidget *parent) : QProgressDialog(parent) {
    setWindowIcon(QIcon(":/pics/utilities-file-archiver.png"));
    setWindowTitle(tr("Please wait..."));
    setLabelText(labelText);
    setMinimumDuration(500);
    setRange(0,0);
}

void WaitDialog::show_error(const QString & error) {
    ScrollMessageBox::critical(this,tr("Error !!!"),"",error);
}

