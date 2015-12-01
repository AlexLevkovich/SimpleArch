#include "mimestable.h"
#include <QHeaderView>

MimesTable::MimesTable(QWidget *parent) : QTableWidget(parent) {
    setColumnCount(2);
    setHorizontalHeaderItem(0,new QTableWidgetItem(tr("Extension")));
    setHorizontalHeaderItem(1,new QTableWidgetItem(tr("Program path")));
    setRowCount(1);
    setItem(0,0,new QTableWidgetItem());
    setItem(0,1,new QTableWidgetItem());
    horizontalHeader()->setStretchLastSection(true);
    verticalHeader()->setVisible(false);

    connect(this,SIGNAL(cellChanged(int,int)),this,SLOT(cellChanged(int,int)));
}

void MimesTable::cellChanged(int row,int) {
    if ((row + 1) == rowCount()) {
        bool isEmpty = false;
        for (int i=0;i<columnCount();i++) {
            if (item(row,i)->text().isEmpty()) {
                isEmpty = true;
                break;
            }
        }
        if (!isEmpty) {
            blockSignals(true);
            setRowCount(rowCount()+1);
            setItem(rowCount()-1,0,new QTableWidgetItem());
            setItem(rowCount()-1,1,new QTableWidgetItem());
            blockSignals(false);
            return;
        }
    }

    bool isLastRow = false;
    if ((isLastRow = ((row + 1) == rowCount())) ||
        ((row + 2) == rowCount())) {
        int second_row = isLastRow?row-1:row+1;
        if ((second_row >= 0) && (second_row < rowCount())) {
            bool allEmpty = true;
            for (int i=0;i<columnCount();i++) {
                if (!item(row,i)->text().isEmpty()) {
                    allEmpty = false;
                    break;
                }
            }
            if (allEmpty) {
                for (int i=0;i<columnCount();i++) {
                    if (!item(second_row,i)->text().isEmpty()) {
                        allEmpty = false;
                        break;
                    }
                }
                if (allEmpty) {
                    removeRow(isLastRow?row:second_row);
                    return;
                }
            }
        }
    }

    if (row < (rowCount()-1)) {
        int second_row = row+1;
        bool allEmpty = true;
        for (int i=0;i<columnCount();i++) {
            if (!item(row,i)->text().isEmpty()) {
                allEmpty = false;
                break;
            }
        }
        if (allEmpty) {
            for (int i=0;i<columnCount();i++) {
                if (!item(second_row,i)->text().isEmpty()) {
                    allEmpty = false;
                    break;
                }
            }
            if (!allEmpty) {
                removeRow(row);
                return;
            }
        }
    }
}

void MimesTable::addUserMime(const QString & ext,const QString & path) {
    item(rowCount()-1,0)->setText(ext);
    item(rowCount()-1,1)->setText(path);
}

void MimesTable::addUserMimes(const QMap<QString,QString> & mimes) {
    QMapIterator<QString,QString> i(mimes);
    while (i.hasNext()) {
        i.next();
        addUserMime(i.key(),i.value());
    }
}

bool MimesTable::userMime(int row,QString & ext,QString & path) {
    if (row < 0 || row >= rowCount()) return false;

    ext = item(row,0)->text();
    path = item(row,1)->text();

    return true;
}

QMap<QString,QString> MimesTable::userMimes() const {
    QMap<QString,QString> ret;
    QString ext;
    QString path;
    for (int i=0;i<rowCount();i++) {
        ((MimesTable *)this)->userMime(i,ext,path);
        if (ext.isEmpty() || path.isEmpty()) continue;

        ret[ext] = path;
    }

    return ret;
}
