#include "openarchivedialog.h"
#include "complextararchengine.h"
#include "iconprovider.h"
#include <QDir>
#include <QBoxLayout>
#include <QCheckBox>
#include <QListView>
#include <QTreeView>
#include <QAbstractProxyModel>
#include <QFileSystemModel>

OpenArchiveDialog::OpenArchiveDialog(QWidget *parent) : QFileDialog(parent) {
    setWindowTitle(tr("Open archive"));
    setDirectory(BaseArchEngine::defaultOpenDir);
    setIconProvider(IconProvider::instance());
    QStringList filters = BaseArchEngine::findOpenFilters();
    filters.insert(0,tr("All files") + " (*.*)");
    setNameFilters(filters);
    setFileMode(QFileDialog::ExistingFile);
    setOption(QFileDialog::DontUseNativeDialog);
    setOption(QFileDialog::ReadOnly);

    QGridLayout *mainLayout = findChild<QGridLayout*>();
    Q_ASSERT(mainLayout);

    QHBoxLayout *hblayout = new QHBoxLayout(this);
    tarCheck = new QCheckBox(tr("Don't extract tar"),this);
    hblayout->addWidget(tarCheck);
    tarCheck->setChecked(!BaseArchEngine::defaultDoTarExtraction);
    tarCheck->setEnabled(false);
    int numRows = mainLayout->rowCount();
    mainLayout->addLayout(hblayout,numRows,0,1,-1);

    listview = findChild<QListView*>("listView");
    if (listview) {
        connect(listview->selectionModel(),SIGNAL(selectionChanged(const QItemSelection &,const QItemSelection &)),this,SLOT(onSelectionChanged(const QItemSelection &,const QItemSelection &)));
    }

    treeview = findChild<QTreeView*>();
    if (treeview) {
        connect(treeview->selectionModel(),SIGNAL(selectionChanged(const QItemSelection &,const QItemSelection &)),this,SLOT(onSelectionChanged(const QItemSelection &,const QItemSelection &)));
    }

    connect(this,SIGNAL(accepted()),this,SLOT(accepted()));
}

void OpenArchiveDialog::onSelectionChanged(const QItemSelection & sel,const QItemSelection &) {
    QItemSelectionModel  * selModel = (QItemSelectionModel  *)sender();
    QAbstractItemView * view = NULL;
    if ((listview != NULL) && (listview->selectionModel() == selModel)) view = listview;
    else if ((treeview != NULL) && (treeview->selectionModel() == selModel)) view = treeview;
    if (view == NULL) return;

    QAbstractItemModel  * proxyModel = view->model();
    if (proxyModel == NULL) return;

    QFileSystemModel * model = (proxyModel->inherits("QFileSystemModel"))?(QFileSystemModel *)proxyModel:(QFileSystemModel *)((QAbstractProxyModel *)proxyModel)->sourceModel();

    if (sel.indexes().count() > 0) {
        QModelIndex index = sel.indexes()[0];
        index = proxyModel->index(index.row(),0,index.parent());
        QModelIndex proxy_index = (proxyModel == model)?index:((QAbstractProxyModel *)proxyModel)->mapToSource(index);
        tarCheck->setEnabled(!model->isDir(proxy_index) && ComplexTarArchEngine::isRealTar(index.data().toString()));
    }
    else tarCheck->setEnabled(false);

    tarCheck->setChecked(!BaseArchEngine::defaultDoTarExtraction);
}

bool OpenArchiveDialog::doExtractTar() const {
    return !tarCheck->isChecked();
}

void OpenArchiveDialog::accepted() {
    BaseArchEngine::defaultDoTarExtraction = doExtractTar();
    BaseArchEngine::defaultOpenDir = QFileInfo(selectedFiles().at(0)).path();
}
