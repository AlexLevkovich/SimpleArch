#include "folderchooser.h"
#include "ui_folderchooser.h"
#include <QFileSystemModel>
#include <QDir>
#include <QPushButton>
#include <QDebug>

FolderChooser::FolderChooser(const QString & start_folder,QWidget *parent) : QDialog(parent), ui(new Ui::FolderChooser) {
    ui->setupUi(this);

    QFileSystemModel *model = new QFileSystemModel(ui->folderView);
    model->setFilter(QDir::Dirs|QDir::Drives|QDir::NoDotAndDotDot);
    ui->folderView->setModel(model);
    QModelIndex root_index = model->setRootPath(start_folder);
    if (root_index.isValid()) {
        QItemSelection selection(root_index,model->index(root_index.row(),model->columnCount()-1,root_index.parent()));
        ui->folderView->selectionModel()->select(selection,QItemSelectionModel::ClearAndSelect);
        ui->folderView->scrollTo(root_index);
        ui->folderView->expand(root_index);
    }

    connect(ui->folderView->selectionModel(),SIGNAL(selectionChanged(const QItemSelection&,const QItemSelection&)),this,SLOT(selectionChanged()));

#if QT_VERSION >= 0x050000
    ui->folderView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
#else
    ui->folderView->header()->setResizeMode(0, QHeaderView::ResizeToContents);
#endif

    ui->folderView->header()->hideSection(1);
    ui->folderView->header()->hideSection(2);
    ui->folderView->header()->hideSection(3);

    selectionChanged();
}

FolderChooser::~FolderChooser() {
    delete ui;
}

QString FolderChooser::folderPath() const {
    return ((QFileSystemModel *)ui->folderView->model())->filePath(ui->folderView->selectionModel()->selectedIndexes().at(0));
}

void FolderChooser::selectionChanged() {
    QItemSelectionModel * selection_model = ui->folderView->selectionModel();
    QModelIndexList indexes = selection_model->selectedRows();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled((indexes.count() > 0) && ((QFileSystemModel *)ui->folderView->model())->isDir(indexes.at(0)));
}

