#include "dircontentstreeview.h"
#include "filetreemodel.h"
#include "filetreeitem.h"
#include "sortfilterproxymodel.h"
#include "basearchengine.h"
#include <QMouseEvent>
#include <QApplication>
#include <QMimeData>
#include <QSortFilterProxyModel>

DirContentsTreeView::DirContentsTreeView(QWidget * parent) : BaseTreeView(parent) {
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setRootIsDecorated(false);
    QTreeView::setModel(new SortFilterProxyModel(this));

    current_index = -1;
}

bool DirContentsTreeView::isColumnVisible(int column) const {
    return (column < 7);
}

uint DirContentsTreeView::iconSize() const {
    return BaseArchEngine::defaultRightIconSize;
}

void DirContentsTreeView::setModel(QAbstractItemModel * model) {
    SortFilterProxyModel * proxyModel = (SortFilterProxyModel *)this->model();
    proxyModel->setSourceModel(model);
    history.clear();
    history.append(QModelIndex());
    current_index = 0;
}

void DirContentsTreeView::setRootIndex(const QModelIndex & index) {
    SortFilterProxyModel * proxyModel = (SortFilterProxyModel *)this->model();
    proxyModel->setRootIndex(index);
    root_index = index;
    BaseTreeView::setRootIndex(proxyModel->mapFromSource(index));
    resizeColumnToContents(0);
}

bool DirContentsTreeView::selectPrev(bool testing_only) {
    SortFilterProxyModel * proxyModel = (SortFilterProxyModel *)this->model();

    if (current_index <= 0) return false;

    if (!testing_only) {
        QModelIndex index = history.at(--current_index);
        if (!index.isValid()) index = proxyModel->index(0,0,QModelIndex());
        else index = proxyModel->mapFromSource(index);
        BaseTreeView::index_activated(index);
    }

    return true;
}

bool DirContentsTreeView::selectNext(bool testing_only) {
    SortFilterProxyModel * proxyModel = (SortFilterProxyModel *)this->model();

    if ((current_index + 1) >= history.count()) return false;

    if (!testing_only) BaseTreeView::index_activated(proxyModel->mapFromSource(history.at(++current_index)));

    return true;
}

void DirContentsTreeView::index_activated(const QModelIndex & index) {
    SortFilterProxyModel * proxyModel = (SortFilterProxyModel *)this->model();

    QModelIndex source_index = proxyModel->mapToSource(index);

    if (is_dir(index)) {
        if ((index.row() == 0) && !index.parent().isValid()) {
            source_index = QModelIndex();
            current_index = -1;
        }
        history.insert(current_index+1,source_index);
        current_index++;
        if (history.count() > (current_index + 1)) history.erase(history.begin() + current_index + 1,history.end());
    }

    BaseTreeView::index_activated(index);
}

bool DirContentsTreeView::selectUp(bool testing_only) {
    QModelIndex up_index = rootIndex().parent();
    if (!up_index.isValid()) return false;

    if (!testing_only) {
        SortFilterProxyModel * proxyModel = (SortFilterProxyModel *)this->model();
        QModelIndex index;
        if ((current_index > 0) && (!(index = history.at(current_index-1)).isValid() || (proxyModel->mapFromSource(index) == up_index))) {
            current_index--;
            BaseTreeView::index_activated(up_index);
        }
        else index_activated(up_index);
    }

    return true;
}

void DirContentsTreeView::selectRoot() {
    SortFilterProxyModel * proxyModel = (SortFilterProxyModel *)this->model();

    index_activated(proxyModel->index(0,0,QModelIndex()));
}

void DirContentsTreeView::rowsInserted(const QModelIndex & parent,int start,int end) {
    BaseTreeView::rowsInserted(parent,start,end);
    if (parent == rootIndex()) resizeColumnToContents(0);
}
