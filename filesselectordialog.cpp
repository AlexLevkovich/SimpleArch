#include "filesselectordialog.h"
#include "basearchengine.h"
#include "iconprovider.h"
#include <QBoxLayout>
#include <QCheckBox>
#include <QListView>
#include <QTreeView>
#include <QComboBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QAbstractProxyModel>
#include <QLineEdit>
#include <QFileSystemModel>

FilesSelectorDialog::FilesSelectorDialog(bool encriptCheckBoxVisible,QWidget *parent) : QFileDialog(parent) {
    setDirectory(BaseArchEngine::defaultSelectDir);
    setAcceptMode(QFileDialog::AcceptOpen);
    setOption(QFileDialog::ReadOnly);
    setOption(QFileDialog::DontUseNativeDialog);
    setFileMode(QFileDialog::Directory);
    setLabelText(QFileDialog::Accept,tr("Insert"));
    setIconProvider(IconProvider::instance());
    setWindowTitle(tr("Select the files or folders to insert"));
    setNameFilter(tr("Folders and files (*.*)"));

    lineEdit = findChild<QLineEdit *>();
    Q_ASSERT(lineEdit);

    QDialogButtonBox *buttons = findChild<QDialogButtonBox *>();
    Q_ASSERT(buttons);
    okButton = buttons->button(QDialogButtonBox::Open);
    Q_ASSERT(okButton);
    okButton->setEnabled(false);

    QGridLayout *mainLayout = findChild<QGridLayout*>();
    Q_ASSERT(mainLayout);

    QHBoxLayout *hblayout = new QHBoxLayout(this);
    fullPathCheck = new QCheckBox(tr("Insert into archive with the full path"),this);
    hblayout->addWidget(fullPathCheck);
    encriptCheck = new QCheckBox(tr("Encrypt the files"),this);
    hblayout->addWidget(encriptCheck);
    encriptCheck->setChecked(false);
    encriptCheck->setVisible(encriptCheckBoxVisible);
    //encriptCheck->setVisible((BaseArchEngine::main_arch_engine != NULL) && BaseArchEngine::main_arch_engine->supportsEncripting());
    fullPathCheck->setChecked(BaseArchEngine::defaultUseFullPath);
    int numRows = mainLayout->rowCount();
    mainLayout->addLayout(hblayout,numRows,0,1,-1);

    listview = findChild<QListView*>("listView");
    if (listview) {
        listview->setSelectionMode(QAbstractItemView::ExtendedSelection);
        connect(listview->selectionModel(),SIGNAL(selectionChanged(const QItemSelection &,const QItemSelection &)),this,SLOT(onSelectionChanged()));
    }
    treeview = findChild<QTreeView*>();
    if (treeview) {
        treeview->setSelectionMode(QAbstractItemView::ExtendedSelection);
        connect(treeview->selectionModel(),SIGNAL(selectionChanged(const QItemSelection &,const QItemSelection &)),this,SLOT(onSelectionChanged()));
    }

    connect(this,SIGNAL(accepted()),this,SLOT(accepted()));
}

bool FilesSelectorDialog::useFullPath() const {
    return fullPathCheck->isChecked();
}

void FilesSelectorDialog::accepted() {
    BaseArchEngine::defaultUseFullPath = useFullPath();
    BaseArchEngine::defaultSelectDir = QFileInfo(selectedFiles().at(0)).path();
}

void FilesSelectorDialog::onSelectionChanged() {
    m_selected_files.clear();

    QItemSelectionModel  * selModel = (QItemSelectionModel  *)sender();
    QAbstractItemView * view = NULL;
    if ((listview != NULL) && (listview->selectionModel() == selModel)) view = listview;
    else if ((treeview != NULL) && (treeview->selectionModel() == selModel)) view = treeview;
    if (view == NULL) return;

    QAbstractItemModel  * proxyModel = view->model();
    if (proxyModel == NULL) return;

    QFileSystemModel * model = (proxyModel->inherits("QFileSystemModel"))?(QFileSystemModel *)proxyModel:(QFileSystemModel *)((QAbstractProxyModel *)proxyModel)->sourceModel();
    QModelIndexList indexes = view->selectionModel()->selectedRows();

    if (indexes.count() > 0) {
        QStringList allFiles;
        for (int i = 0; i < indexes.count(); ++i) {
            QModelIndex index = indexes.at(i);
            index = proxyModel->index(index.row(),0,index.parent());
            QModelIndex source_index = (proxyModel == model)?index:((QAbstractProxyModel *)proxyModel)->mapToSource(index);
            allFiles.append(index.data().toString());
            m_selected_files.append(model->filePath(source_index));
        }
        if (allFiles.count() > 1) {
            for (int i = 0; i < allFiles.count(); ++i) {
                allFiles.replace(i, QString(QLatin1Char('"') + allFiles.at(i) + QLatin1Char('"')));
            }
        }

        QString finalFiles = allFiles.join(" ");
        lineEdit->setText(finalFiles);
        okButton->setEnabled(true);
    }
    else okButton->setEnabled(false);
}

QStringList FilesSelectorDialog::selectedFiles() const {
    return m_selected_files;
}

void FilesSelectorDialog::accept() {
    QDialog::accept();
}

bool FilesSelectorDialog::useEncription() const {
    return encriptCheck->isChecked();
}
