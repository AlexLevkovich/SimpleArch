#include "sortfilterproxymodel.h"
#include "filetreeitem.h"
#include "filetreemodel.h"

SortFilterProxyModel::SortFilterProxyModel(QObject * parent) : QSortFilterProxyModel(parent) {
    setDynamicSortFilter(true);
    m_rootIndex = QModelIndex();
}

bool SortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const {
    FileTreeItem * leftData = (FileTreeItem * )left.internalPointer();
    FileTreeItem * rightData = (FileTreeItem * )right.internalPointer();

    if (!left.isValid()) return false;
    if (!right.isValid()) return true;

    if(left.column() == 5) {
        return leftData->file_size() < rightData->file_size();
    }
    else if (left.column() == 0) {
        if ((leftData->is_dir() && rightData->is_dir()) ||
            (!leftData->is_dir() && !rightData->is_dir())) {
            return (leftData->name().compare(rightData->name(),Qt::CaseInsensitive) < 0);
        }
        else if (leftData->is_dir() && !rightData->is_dir()) return true;
        else if (!leftData->is_dir() && rightData->is_dir()) return false;
    }
    else if (left.column() == 6) {
        return leftData->date() < rightData->date();
    }

    return QSortFilterProxyModel::lessThan(left, right);
}

void SortFilterProxyModel::setSourceModel(QAbstractItemModel * sourceModel) {
    if (sourceModel == this->sourceModel()) return;
    if (!sourceModel->inherits("FileTreeModel")) return;
    QSortFilterProxyModel::setSourceModel(sourceModel);
    emit sourceModelChanged();
}

bool SortFilterProxyModel::hasChildren(const QModelIndex & parent) const {
    if (!parent.isValid()) return true;
    if (!m_rootIndex.isValid()) return true;
    return (parent == mapFromSource(m_rootIndex));
}

void SortFilterProxyModel::setRootIndex(const QModelIndex & rootIndex) {
    m_rootIndex = rootIndex;
}

