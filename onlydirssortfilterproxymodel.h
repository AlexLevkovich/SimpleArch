#ifndef ONLYDIRSSORTFILTERPROXYMODEL_H
#define ONLYDIRSSORTFILTERPROXYMODEL_H

#include "sortfilterproxymodel.h"

class OnlyDirsSortFilterProxyModel : public SortFilterProxyModel {
    Q_OBJECT
public:
    explicit OnlyDirsSortFilterProxyModel(QObject *parent = 0);

protected:
    bool filterAcceptsRow(int source_row,const QModelIndex & source_parent) const;
    bool hasChildren(const QModelIndex & parent = QModelIndex()) const;
};

#endif // ONLYDIRSSORTFILTERPROXYMODEL_H
