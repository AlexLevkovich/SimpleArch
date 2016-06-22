#include "basetreeview.h"
#include "sortfilterproxymodel.h"
#include <QMessageBox>
#include <QMenu>
#include <QContextMenuEvent>
#include "filetreemodel.h"
#include "extractarchengine.h"
#include "scrollmessagebox.h"
#include "basearchengine.h"
#include <QHeaderView>
#include "staticutils.h"

MyProxyStyle * BaseTreeView::no_item_current_selection_style = new MyProxyStyle();

void MyProxyStyle::drawPrimitive(PrimitiveElement element, const QStyleOption * option,
                           QPainter * painter, const QWidget * widget) const {
    if (element != QStyle::PE_FrameFocusRect) {
        if ((element == QStyle::PE_Frame) && BaseArchEngine::defaultHighlightFocusedView) {
            QStyleOption new_opt(*option);
            if (widget->hasFocus()) new_opt.palette.setColor(QPalette::Window,widget->style()->standardPalette().color(QPalette::Highlight));
            QProxyStyle::drawPrimitive(element,&new_opt,painter,widget);
        }
        else QProxyStyle::drawPrimitive(element,option,painter,widget);
    }
}

BaseTreeView::BaseTreeView(QWidget *parent) : QTreeView(parent) {
    setSortingEnabled(true);
    setDragEnabled(true);
    setAcceptDrops(true);
    setStyle(no_item_current_selection_style);

    connect(this,SIGNAL(activated(const QModelIndex &)),this,SLOT(index_activated(const QModelIndex &)));
    QMetaObject::invokeMethod(this,"post_init",Qt::QueuedConnection);
}

void BaseTreeView::post_init() {
    connect(model(),SIGNAL(sourceModelChanged()),this,SLOT(sourceModelChanged()));
}

void BaseTreeView::sourceModelChanged() {
    for (int i=0;i<model()->columnCount();i++) setColumnHidden(i,!isColumnVisible(i));
    setIconSize(QSize(iconSize(),iconSize()));
    if (BaseArchEngine::defaultDoFileNamesSorting) {
        model()->sort(0);
        header()->setSortIndicator(0,Qt::AscendingOrder);
    }
}

void BaseTreeView::deleteSelectedRows() {
    if (BaseArchEngine::main_arch_engine == NULL) return;

    QSortFilterProxyModel * proxyModel = (QSortFilterProxyModel *)this->model();
    if (proxyModel == NULL) return;
    QAbstractItemModel * model = (QAbstractItemModel *)proxyModel->sourceModel();
    if (model == NULL) return;

    QModelIndexList indexes = selectionModel()->selectedRows();
    QModelIndexList source_indexes;
    QString text;
    for (int i=0;i<indexes.count();i++) {
        if (indexes[i].parent().isValid()) {
            source_indexes.append(proxyModel->mapToSource(indexes[i]));
            text += proxyModel->index(indexes[i].row(),7,indexes[i].parent()).data().toString() + indexes[i].data().toString() + "\n";
        }
    }

    if (ScrollMessageBox::question(this,tr("Question..."),tr("Do you want to delete the items below?"),text) == QMessageBox::No) return;

    qSort(source_indexes.begin(),source_indexes.end());
    for (int i=(source_indexes.count()-1);i>=0;i--) {
        model->removeRow(source_indexes[i].row(),source_indexes[i].parent());
    }
}

void BaseTreeView::insertFiles(const QStringList & files,bool use_full_path,bool password_protected) {
    if (BaseArchEngine::main_arch_engine == NULL) return;

    QSortFilterProxyModel * proxyModel = (QSortFilterProxyModel *)this->model();
    if (proxyModel == NULL) return;
    FileTreeModel * model = (FileTreeModel *)proxyModel->sourceModel();
    if (model == NULL) return;

    QModelIndex parent;
    QModelIndexList indexes = selectionModel()->selectedRows();
    if (indexes.count() <= 0 || indexes.count() > 1 || !isLeftView()) {
        parent = rootIndex();
        if (!parent.isValid()) return;
        parent = proxyModel->mapToSource(parent);
    }
    else {
        parent = proxyModel->mapToSource(indexes[0]);
    }

    model->insertRows(parent,files,use_full_path,password_protected);
}

void BaseTreeView::extractFiles(const QString & folderPath,bool use_full_path,bool extract_selected_only) {
    if (BaseArchEngine::main_arch_engine == NULL) return;

    QSortFilterProxyModel * proxyModel = (QSortFilterProxyModel *)this->model();
    if (proxyModel == NULL) return;
    FileTreeModel * model = (FileTreeModel *)proxyModel->sourceModel();
    if (model == NULL) return;

    QStringList selectedFiles;
    QModelIndexList indexes = selectionModel()->selectedRows();
    QModelIndex parent;
    if (indexes.count() <= 0) {
        parent = rootIndex();
        if (!parent.isValid()) return;
        QString path = proxyModel->index(parent.row(),8,parent.parent()).data().toString();
        if (!path.isEmpty()) {
            selectedFiles << path;
        }
    }
    else {
        if ((indexes.count() != 1) || (indexes[0] != proxyModel->index(0,0,QModelIndex()))) {
            for (int i=0;i<indexes.count();i++) {
                selectedFiles << proxyModel->index(indexes[i].row(),8,indexes[i].parent()).data().toString();
            }
        }
    }

    ExtractArchEngine engine(BaseArchEngine::main_arch_engine,extract_selected_only?selectedFiles:QStringList(),folderPath,use_full_path,this);
    StaticUtils::setAppWaitStatus(tr("Extracting the selected items..."));
    engine.exec();
    StaticUtils::restoreAppStatus();
}

bool BaseTreeView::isRowsDeletionPossible() const {
    if (BaseArchEngine::main_arch_engine == NULL) return false;
    if (BaseArchEngine::main_arch_engine->readOnly()) return false;
    if (!BaseArchEngine::main_arch_engine->isMultiFiles()) return false;
    QModelIndexList indexes = selectionModel()->selectedRows();
    int count = 0;
    for (int i=0;i<indexes.count();i++) {
        if (indexes[i].parent().isValid()) count++;
    }

    return (count > 0);
}

bool BaseTreeView::isFilesInsertionPossible() const {
    if (BaseArchEngine::main_arch_engine == NULL) return false;
    if (BaseArchEngine::main_arch_engine->readOnly()) return false;
    QModelIndexList indexes = selectionModel()->selectedRows();
    if (indexes.count() <= 0 || indexes.count() > 1) {
        if (!rootIndex().isValid()) return false;
    }

    return true;
}

bool BaseTreeView::isFilesExtractionPossible() const {
    if (BaseArchEngine::main_arch_engine == NULL) return false;

    if (selectionModel()->selectedRows().count() <= 0) {
        if (!rootIndex().isValid()) return false;
    }

    return true;
}

void BaseTreeView::selectionChanged(const QItemSelection & selected,const QItemSelection & deselected) {
    QTreeView::selectionChanged(selected,deselected);
    emit operationStatesUpdated();
}

void BaseTreeView::selectAllItems() {
    if (rootIndex().isValid()) select_all_items();
    else select_root_dir();
}

void BaseTreeView::select_root_dir() {
    QSortFilterProxyModel * model = (QSortFilterProxyModel *)this->model();
    if (model == NULL) return;

    QModelIndex index = model->index(0,0,QModelIndex());
    QItemSelection selection(index,model->index(index.row(),model->columnCount()-1,QModelIndex()));
    selectionModel()->select(selection,QItemSelectionModel::ClearAndSelect);
    scrollTo(index);
    expand(index);
}

void BaseTreeView::select_all_items() {
    QSortFilterProxyModel * model = (QSortFilterProxyModel *)this->model();
    QModelIndex rootIndex = this->rootIndex();
    uint childCount = model->rowCount(rootIndex);
    if (childCount <= 0) return;

    QItemSelection selection(model->index(0,0,rootIndex),model->index(childCount-1,model->columnCount()-1,rootIndex));
    selectionModel()->select(selection,QItemSelectionModel::ClearAndSelect);
}

void BaseTreeView::selectItemsByWildcard(const QString & exp) {
    QSortFilterProxyModel * model = (QSortFilterProxyModel *)this->model();
    if (model == NULL) return;

    QModelIndex start_index = isLeftView()?model->index(0,0,QModelIndex()):rootIndex();

    QModelIndexList	indexes;
    if (model->hasChildren(start_index)) {
        start_index = start_index.child(0,0);
        indexes = model->match(start_index,Qt::DisplayRole,exp,-1,Qt::MatchRecursive | Qt::MatchWildcard | Qt::MatchContains | Qt::MatchWrap);
    }

    if (indexes.count() <= 0) {
        QMessageBox::information(this,tr("Information..."),tr("Nothing found!!!"));
        return;
    }

    if (isLeftView() && (indexes.count() > 1)) indexes = (QModelIndexList() << indexes[0]);
    if (indexes.count() > 1) qSort(indexes.begin(),indexes.end());

    QModelIndex last_index = indexes[indexes.count()-1];
    QItemSelection selection(indexes[0],model->index(last_index.row(),model->columnCount()-1,last_index.parent()));
    selectionModel()->select(selection,QItemSelectionModel::ClearAndSelect);
    if (isLeftView()) scrollTo(indexes[0]);
}

bool BaseTreeView::calcAdditionalInfo(qreal & all_files_size,qreal & files_count) {
    if (BaseArchEngine::main_arch_engine == NULL) return false;

    SortFilterProxyModel * proxyModel = (SortFilterProxyModel *)this->model();
    if (proxyModel == NULL) return false;
    FileTreeModel * model = (FileTreeModel *)proxyModel->sourceModel();
    if (model == NULL) return false;

    QModelIndexList indexes = model->match(proxyModel->mapToSource(proxyModel->index(0,0,QModelIndex())),Qt::DisplayRole,files_only_compare,-1);
    if (indexes.count() <= 0) return false;

    all_files_size = 0;
    files_count = indexes.count();
    for (int i=0;i<indexes.count();i++) {
        all_files_size += model->data(model->index(indexes[i].row(),5,indexes[i].parent()),Qt::UserRole).value<qreal>();
    }

    return true;
}

bool BaseTreeView::files_only_compare(const QModelIndex & index) {
    if (index.column() != 0) return false;
    if (index == index.model()->index(0,0,QModelIndex())) return false;

    return (!index.model()->index(index.row(),2,index.parent()).data().toString().startsWith("d"));
}

void BaseTreeView::select_index(const QModelIndex & index) {
    QSortFilterProxyModel * model = (QSortFilterProxyModel *)this->model();
    if (model == NULL) return;

    QItemSelection selection(index,model->index(index.row(),model->columnCount()-1,QModelIndex()));
    selectionModel()->select(selection,QItemSelectionModel::ClearAndSelect);
    scrollTo(index);
}

void BaseTreeView::select_items(const QItemSelection & selection) {
    QSortFilterProxyModel * model = (QSortFilterProxyModel *)this->model();
    if (model == NULL) return;

    if (selection.indexes().count() > 0) {
        QModelIndex index = selection.indexes().at(0);
        selectionModel()->select(selection,QItemSelectionModel::ClearAndSelect);
        scrollTo(index);
    }
}

char BaseTreeView::type(const QModelIndex & _index) {
    SortFilterProxyModel * proxyModel = (SortFilterProxyModel *)this->model();
    if (proxyModel == NULL) return ' ';
    FileTreeModel * model = (FileTreeModel *)proxyModel->sourceModel();
    if (model == NULL) return ' ';

    QModelIndex index = proxyModel->mapToSource(_index);
    QByteArray m_type = model->index(index.row(),2,index.parent()).data().toByteArray();
    return m_type.isEmpty()?' ':m_type.at(0);
}

bool BaseTreeView::is_dir(const QModelIndex & _index) {
    return type(_index) == 'd';
}

bool BaseTreeView::is_link(const QModelIndex & _index) {
    return type(_index) == 'l';
}

void BaseTreeView::index_activated(const QModelIndex & index) {
    QModelIndex _index = ((SortFilterProxyModel *)model())->mapToSource(index);
    if (is_dir(index)) emit dir_activated(_index);
    else {
        if (is_link(index)) return;
        QList<QAction *> actions = menu.actions();
        for (int i=0;i<actions.count();i++) {
            QAction * action = actions.at(i);
            if (action->text() == tr("Open with external editor")) {
                if (enablers_map.contains(action)) action->setEnabled(enablers_map[action](this));
                if (action->isEnabled()) action->trigger();
                break;
            }
        }
    }
}

void BaseTreeView::contextMenuEvent(QContextMenuEvent * event) {
    QTreeView::contextMenuEvent(event);
    if (menu.actions().count() <= 0) return;

    QList<QAction *> actions = menu.actions();
    for (int i=0;i<actions.count();i++) {
        if (enablers_map.contains(actions[i])) actions[i]->setEnabled(enablers_map[actions[i]](this));
    }

    menu.exec(event->globalPos());
}

void BaseTreeView::addContextMenuAction(const QAction * action) {
    if (action == NULL) menu.addSeparator();
    else menu.addAction((QAction *)action);
}

void BaseTreeView::addContextMenuAction(const QAction * action,bool (*enabler)(BaseTreeView *)) {
    if (action == NULL) menu.addSeparator();
    else {
        menu.addAction((QAction *)action);
        if (enabler != NULL) enablers_map[(QAction *)action] = enabler;
    }
}

bool BaseTreeView::openFile_enabler(BaseTreeView * view) {
    if (BaseArchEngine::main_arch_engine == NULL) return false;

    QModelIndexList indexes = view->selectionModel()->selectedRows();
    return (indexes.count() == 1) && !view->is_dir(indexes[0]);
}

QString BaseTreeView::selectedPath() const {
    if (!openFile_enabler((BaseTreeView *)this)) return "";

    SortFilterProxyModel * proxyModel = (SortFilterProxyModel *)this->model();
    QModelIndexList indexes = selectionModel()->selectedRows();
    return proxyModel->index(indexes[0].row(),8,indexes[0].parent()).data().toString();
}
