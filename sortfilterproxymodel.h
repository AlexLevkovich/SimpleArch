#ifndef SortFilterProxyModel_H
#define SortFilterProxyModel_H

#include <QSortFilterProxyModel>

class SortFilterProxyModel: public QSortFilterProxyModel {
    Q_OBJECT
public:
    SortFilterProxyModel(QObject * parent = NULL);
    void setSourceModel(QAbstractItemModel * sourceModel);
    void setRootIndex(const QModelIndex & rootIndex);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;
    bool hasChildren(const QModelIndex & parent = QModelIndex()) const;

signals:
    void sourceModelChanged();

private:
    QModelIndex m_rootIndex;
};

#endif // SortFilterProxyModel_H
