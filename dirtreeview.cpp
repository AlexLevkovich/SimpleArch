#include "dirtreeview.h"
#include "filetreemodel.h"
#include "onlydirssortfilterproxymodel.h"
#include "basearchengine.h"

DirTreeView::DirTreeView(QWidget * parent) : BaseTreeView(parent) {
    temp_model = NULL;

    setSelectionMode(QAbstractItemView::SingleSelection);
    setRootIsDecorated(true);
    setModel(new OnlyDirsSortFilterProxyModel(this));

    connect(selectionModel(),SIGNAL(selectionChanged(const QItemSelection &,const QItemSelection &)),this,SLOT(slot_selectionChanged(const QItemSelection &,const QItemSelection &)));
}

void DirTreeView::updateView() {
    SortFilterProxyModel * proxyModel = (SortFilterProxyModel *)this->model();

    if (BaseArchEngine::main_arch_engine == NULL) {
        FileTreeModel * model = (FileTreeModel *)proxyModel->sourceModel();
        if (model != NULL) model->removeRow(0);
        return;
    }

    emit long_processing_started();

    if (temp_model != NULL) {
        disconnect(BaseArchEngine::main_arch_engine,SIGNAL(error(const QString &)),this,SLOT(onError()));
        disconnect(BaseArchEngine::main_arch_engine,SIGNAL(list_contents_record(FileTreeItem *)),temp_model,SLOT(importModelItem(FileTreeItem *)));
        disconnect(BaseArchEngine::main_arch_engine,SIGNAL(list_contents_ok()),this,SLOT(update()));
    }

    temp_model = new FileTreeModel(BaseArchEngine::main_arch_engine->fileName(),proxyModel);

    connect(temp_model,SIGNAL(rowsInserted(const QModelIndex &,int,int)),this,SLOT(rowsCountChanged(const QModelIndex &,int,int)));
    connect(temp_model,SIGNAL(rowsRemoved(const QModelIndex &,int,int)),this,SLOT(rowsCountChanged(const QModelIndex &,int,int)));
    connect(BaseArchEngine::main_arch_engine,SIGNAL(error(const QString &)),this,SLOT(onError()));
    connect(BaseArchEngine::main_arch_engine,SIGNAL(list_contents_record(FileTreeItem *)),temp_model,SLOT(importModelItem(FileTreeItem *)));
    connect(BaseArchEngine::main_arch_engine,SIGNAL(list_contents_ok()),this,SLOT(update()));
    BaseArchEngine::main_arch_engine->startListContents();
}

void DirTreeView::onError() {
    disconnect(BaseArchEngine::main_arch_engine,SIGNAL(error(const QString &)),this,SLOT(onError()));
    QMetaObject::invokeMethod(this,"post_onError",Qt::QueuedConnection);
}

void DirTreeView::post_onError() {
    if (BaseArchEngine::main_arch_engine->lastCommand() == "startListContents") {
        if (BaseArchEngine::main_arch_engine != NULL) delete BaseArchEngine::main_arch_engine;
    }
    else if (BaseArchEngine::main_arch_engine->lastCommand() == "extractFiles") return;

    updateView();
}

bool DirTreeView::isColumnVisible(int column) const {
    return (column == 0);
}

uint DirTreeView::iconSize() const {
    return BaseArchEngine::defaultLeftIconSize;
}

void DirTreeView::update() {
    SortFilterProxyModel * proxyModel = (SortFilterProxyModel *)this->model();
    FileTreeModel * prev_model = (FileTreeModel *)proxyModel->sourceModel();
    proxyModel->setSourceModel(temp_model);
    if (prev_model != NULL) delete prev_model;

    emit modelUpdated();
    select_root_dir();
    emit long_processing_completed(false);
}

void DirTreeView::slot_selectionChanged(const QItemSelection & selected,const QItemSelection &) {
    SortFilterProxyModel * proxyModel = (SortFilterProxyModel *)this->model();
    QModelIndexList sel_indexes = selected.indexes();
    if (sel_indexes.count() <= 0) return;

    emit selectionChanged(proxyModel->mapToSource(sel_indexes.at(0)));
}

void DirTreeView::select_dir(const QModelIndex & index) {
    SortFilterProxyModel * _proxyModel = (SortFilterProxyModel *)this->model();
    FileTreeModel * _model = (FileTreeModel *)_proxyModel->sourceModel();
    if (_model == NULL) return;

    QModelIndex topleft = _proxyModel->mapFromSource(_model->index(((FileTreeItem *)index.internalPointer()),index.row(),0));
    QItemSelection selection(topleft,_proxyModel->mapFromSource(_model->index(((FileTreeItem *)index.internalPointer()),index.row(),_model->columnCount()-1)));
    selectionModel()->select(selection,QItemSelectionModel::ClearAndSelect);
    scrollTo(topleft);
}

void DirTreeView::rowsCountChanged(const QModelIndex & parent,int,int) {
    SortFilterProxyModel * proxyModel = (SortFilterProxyModel *)this->model();

    QModelIndex rootIndex = proxyModel->index(0,0,QModelIndex());
    if ((parent.data().toString() == rootIndex.data().toString()) && parent.data(Qt::UserRole).toBool()) {
        collapse(rootIndex);
        QMetaObject::invokeMethod(this,"expand",Qt::QueuedConnection,Q_ARG(QModelIndex,rootIndex));
    }
}

void DirTreeView::selectRoot() {
    select_root_dir();
}

void DirTreeView::index_activated(const QModelIndex &) {
}
