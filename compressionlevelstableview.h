#ifndef SETTINGSCOMBOTABLEVIEW_H
#define SETTINGSCOMBOTABLEVIEW_H

#include <QTableView>
#include <QMap>

class QStandardItemModel;

class CompressionLevelsTableView : public QTableView {
    Q_OBJECT
public:
    explicit CompressionLevelsTableView(QWidget *parent = 0);
    QMap<QString,int> levelsMap() const;

private:
    QStandardItemModel *model;
};

#endif // SETTINGSCOMBOTABLEVIEW_H
