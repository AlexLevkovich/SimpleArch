#ifndef MIMESTABLE_H
#define MIMESTABLE_H

#include <QTableWidget>
#include <QMap>

class MimesTable : public QTableWidget {
    Q_OBJECT
public:
    explicit MimesTable(QWidget *parent = 0);
    void addUserMime(const QString & ext,const QString & path);
    void addUserMimes(const QMap<QString,QString> & mimes);
    bool userMime(int row,QString & ext,QString & path);
    QMap<QString,QString> userMimes() const;

private slots:
    void cellChanged(int row,int col);
};

#endif // MIMESTABLE_H
