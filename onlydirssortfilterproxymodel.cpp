#include "onlydirssortfilterproxymodel.h"
#include "filetreeitem.h"

OnlyDirsSortFilterProxyModel::OnlyDirsSortFilterProxyModel(QObject *parent) : SortFilterProxyModel(parent) {

}

bool OnlyDirsSortFilterProxyModel::filterAcceptsRow(int source_row,const QModelIndex & source_parent) const {
    QAbstractItemModel * model = (QAbstractItemModel *)sourceModel();
    if (model == NULL) return false;

    FileTreeItem * parent_item = static_cast<FileTreeItem*>(source_parent.internalPointer());
    if (parent_item == NULL) return true;

    FileTreeItem * child_item = parent_item->child(source_row);
    if (child_item == NULL) return false;

    return SortFilterProxyModel::filterAcceptsRow(source_row,source_parent) && child_item->is_dir();
}

bool OnlyDirsSortFilterProxyModel::hasChildren(const QModelIndex & parent) const {
    return QSortFilterProxyModel::hasChildren(parent);
}
