#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QInputDialog>
#include <QAbstractItemModel>
#include <QFileInfo>
#include <QDateTime>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QDesktopServices>
#include "filesselectordialog.h"
#include "extractfolderchooser.h"
#include "extractarchengine.h"
#include "openarchivedialog.h"
#include "complextararchengine.h"
#include "createarchivedialog.h"
#include "createarchengine.h"
#include "byteshumanizer.h"
#include "saveasarchengine.h"
#include "archiverprocess.h"
#include "extracttotemparchengine.h"
#include "settingsdialog.h"
#include "scrollmessagebox.h"
#include "staticutils.h"

#define CANT_FIND_ENGINE "Cannot find a suitable engine to open %1 file!"
extern const char * simplearch_version;
#define File_name          "File name"
#define Path_to_file       "Path to file"
#define Type               "Type"
#define Modified           "Modified"
#define Packed_size        "Packed size"
#define Unpacked_size      "Unpacked size"
#define Count_of_the_files "Count of the files"
#define Packing_ratio      "Packing ratio"

MainWindow::MainWindow(const QString & input_file_name,QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    ui->progressWidget->setVisible(false);

    connect(ui->dirView,SIGNAL(selectionChanged(const QModelIndex &)),ui->filesView,SLOT(setRootIndex(const QModelIndex &)));
    connect(ui->dirView,SIGNAL(long_processing_started()),this,SLOT(long_processing_started()));
    connect(ui->dirView,SIGNAL(modelUpdated()),this,SLOT(dirView_modelUpdated()));
    connect(ui->dirView,SIGNAL(long_processing_completed(bool)),this,SLOT(long_processing_completed(bool)));
    connect(ui->dirView,SIGNAL(operationStatesUpdated()),this,SLOT(post_modelUpdated()));
    connect(ui->dirView,SIGNAL(selectionChanged(const QModelIndex &)),this,SLOT(post_modelUpdated()));
    connect(ui->filesView,SIGNAL(operationStatesUpdated()),this,SLOT(post_modelUpdated()));
    connect(ui->filesView,SIGNAL(dir_activated(const QModelIndex &)),ui->dirView,SLOT(select_dir(const QModelIndex &)));
    connect(ui->filesView,SIGNAL(dir_activated(const QModelIndex &)),this,SLOT(post_modelUpdated()));
    connect(qApp,SIGNAL(focusChanged(QWidget *,QWidget *)),this,SLOT(post_modelUpdated()));
    connect(qApp,SIGNAL(aboutToQuit()),this,SLOT(aboutToQuit()));

    setWindowTitle("SimpleArch");

    post_modelUpdated();

    if (!input_file_name.isEmpty()) {
        openArchive(input_file_name);
    }

    restoreGeometry(BaseArchEngine::settingsInstance()->value("mainWindowSizes").toByteArray());
    ui->mainSplitter->restoreState(BaseArchEngine::settingsInstance()->value("mainSplitterSizes").toByteArray());
    ui->splitter->restoreState(BaseArchEngine::settingsInstance()->value("splitterSizes").toByteArray());

    ui->filesView->addContextMenuAction(ui->actionOpenFile,BaseTreeView::openFile_enabler);
    ui->filesView->addContextMenuAction(NULL);
    ui->filesView->addContextMenuAction(ui->actionDelete_files);
    ui->filesView->addContextMenuAction(ui->actionAppendFiles);
    ui->filesView->addContextMenuAction(ui->actionExtractFiles);


    ui->dirView->addContextMenuAction(ui->actionDelete_files);
    ui->dirView->addContextMenuAction(ui->actionAppendFiles);
    ui->dirView->addContextMenuAction(ui->actionExtractFiles);

    ui->dirView->setVisible(BaseArchEngine::defaultDoShowFolderTree);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::dirView_modelUpdated() {
    if (BaseArchEngine::main_arch_engine == NULL) return;

    setWindowTitle("SimpleArch - "+BaseArchEngine::main_arch_engine->fileName());
    ui->filesView->setModel(((QSortFilterProxyModel *)ui->dirView->model())->sourceModel());
    QMetaObject::invokeMethod(this,"post_modelUpdated",Qt::QueuedConnection);
}

void MainWindow::long_processing_started() {
    ui->dirView->setVisible(false);
    ui->filesView->setVisible(false);
    ui->progressWidget->setVisible(true);
}

void MainWindow::long_processing_completed(bool) {
    ui->progressWidget->setVisible(false);
    ui->dirView->setVisible(true);
    ui->filesView->setVisible(true);
}

BaseTreeView * MainWindow::focusedView() {
    QWidget * focused_widget = qApp->focusWidget();
    if ((focused_widget != NULL) && ((focused_widget == ui->dirView) || (focused_widget == ui->filesView))) return (BaseTreeView *)focused_widget;
    return NULL;
}

void MainWindow::on_actionDelete_files_triggered() {
    BaseTreeView * focused_treeview = focusedView();
    if (focused_treeview != NULL) focused_treeview->deleteSelectedRows();
}

void MainWindow::on_actionAppendFiles_triggered() {
    if (BaseArchEngine::main_arch_engine == NULL) return;

    BaseTreeView * focused_treeview = focusedView();
    if (focused_treeview != NULL) {
        FilesSelectorDialog selector(BaseArchEngine::main_arch_engine->supportsEncripting(),this);
        if (selector.exec() == QDialog::Rejected) return;

        focused_treeview->insertFiles(selector.selectedFiles(),selector.useFullPath(),selector.useEncription());
    }
}

void MainWindow::on_actionExtractFiles_triggered() {
    if (BaseArchEngine::main_arch_engine == NULL) return;

    BaseTreeView * focused_treeview = focusedView();
    if (focused_treeview != NULL) {
        ExtractFolderChooser folderChooser(this);
        if (folderChooser.exec() == QDialog::Rejected) return;

        focused_treeview->extractFiles(folderChooser.folderPath(),folderChooser.useFullPath(),folderChooser.extractSelectedItemsOnly());
    }
}

void MainWindow::openArchive(const QString & filePath,bool doTarExtract) {
    if (BaseArchEngine::main_arch_engine != NULL) delete BaseArchEngine::main_arch_engine;
    BaseArchEngine::main_arch_engine = BaseArchEngine::findEngine(filePath,this);
    if (BaseArchEngine::main_arch_engine != NULL) {
        connect(BaseArchEngine::main_arch_engine,SIGNAL(destroyed()),this,SLOT(post_modelUpdated()));
        connect(BaseArchEngine::main_arch_engine,SIGNAL(error(const QString &)),this,SLOT(show_error(const QString &)));
        connect(BaseArchEngine::main_arch_engine,SIGNAL(error_message(const QString &)),this,SLOT(show_error(const QString &)));
        if (BaseArchEngine::main_arch_engine->inherits("ComplexTarArchEngine")) {
            ComplexTarArchEngine * tarEngine = (ComplexTarArchEngine *) BaseArchEngine::main_arch_engine;
            tarEngine->setNotTarForceFlag(!doTarExtract);
        }
        setWindowTitle("SimpleArch");
        ui->dirView->updateView();
    }
    else QMessageBox::critical(this,tr("Error!!!"),tr(CANT_FIND_ENGINE).arg(filePath));
}

void MainWindow::on_actionOpenArchive_triggered() {
    OpenArchiveDialog openDlg(this);
    if (openDlg.exec() == QDialog::Rejected) return;

    openArchive(openDlg.selectedFiles().at(0),openDlg.doExtractTar());
}

void MainWindow::show_error(const QString & error) {
    long_processing_completed(true);
    QObject * obj = (QObject *)sender();
    if (error.isEmpty()) return;
    if (obj->inherits("BaseArchEngine")) ScrollMessageBox::critical(this,tr("Error !!!"),tr("for %1").arg(((BaseArchEngine *)obj)->fileName()),error);
    else ScrollMessageBox::critical(this,tr("Error !!!"),"",error);
}

void MainWindow::on_actionCreateNewArchive_triggered() {
    CreateArchiveDialog createDlg(false,false,false,this);
    if (createDlg.exec() == QDialog::Rejected) return;

    QString fileName = createDlg.selectedFile();
    BaseArchEngine * new_engine = BaseArchEngine::findEngine(fileName,this);
    if (new_engine == NULL) {
        QMessageBox::critical(this,tr("Error!!!"),tr(CANT_FIND_ENGINE).arg(fileName));
        return;
    }

    FilesSelectorDialog selector(new_engine->supportsEncripting(),this);
    if (selector.exec() == QDialog::Rejected) {
        delete new_engine;
        return;
    }

    if (BaseArchEngine::main_arch_engine != NULL) delete BaseArchEngine::main_arch_engine;
    BaseArchEngine::main_arch_engine = new_engine;

    connect(BaseArchEngine::main_arch_engine,SIGNAL(error(const QString &)),this,SLOT(show_error(const QString &)));
    connect(BaseArchEngine::main_arch_engine,SIGNAL(destroyed()),this,SLOT(post_modelUpdated()));

    long_processing_started();
    if (!CreateArchEngine(BaseArchEngine::main_arch_engine,selector.selectedFiles(),selector.useFullPath(),selector.useEncription()).exec()) {
        if (BaseArchEngine::main_arch_engine != NULL) delete BaseArchEngine::main_arch_engine;
    }
    ui->dirView->updateView();
}

void MainWindow::post_modelUpdated() {
    BaseTreeView * focused_treeview = focusedView();
    ui->actionAppendFiles->setEnabled((focused_treeview != NULL) && focused_treeview->isFilesInsertionPossible() && (ArchiverProcess::active_processes.count() == 0));
    ui->actionDelete_files->setEnabled((focused_treeview != NULL) && focused_treeview->isRowsDeletionPossible() && (ArchiverProcess::active_processes.count() == 0));
    ui->actionExtractFiles->setEnabled((focused_treeview != NULL) && focused_treeview->isFilesExtractionPossible() && (ArchiverProcess::active_processes.count() == 0));
    ui->actionSelect_all->setEnabled((BaseArchEngine::main_arch_engine != NULL) && (focused_treeview != NULL) && (ArchiverProcess::active_processes.count() == 0));
    ui->actionSelect_by_template->setEnabled((BaseArchEngine::main_arch_engine != NULL) && (focused_treeview != NULL) && (ArchiverProcess::active_processes.count() == 0));
    ui->actionCheck_integrity->setEnabled((BaseArchEngine::main_arch_engine != NULL) && (ArchiverProcess::active_processes.count() == 0));
    ui->actionProperties->setEnabled((BaseArchEngine::main_arch_engine != NULL) && QFileInfo(BaseArchEngine::main_arch_engine->fileName()).exists() && (ArchiverProcess::active_processes.count() == 0));
    ui->actionSave_as->setEnabled((BaseArchEngine::main_arch_engine != NULL) && QFileInfo(BaseArchEngine::main_arch_engine->fileName()).exists() && (ArchiverProcess::active_processes.count() == 0));
    ui->actionGo_previous->setEnabled((BaseArchEngine::main_arch_engine != NULL) && ui->filesView->selectPrev(true) && (ArchiverProcess::active_processes.count() == 0));
    ui->actionGo_next->setEnabled((BaseArchEngine::main_arch_engine != NULL) && ui->filesView->selectNext(true) && (ArchiverProcess::active_processes.count() == 0));
    ui->actionGo_up->setEnabled((BaseArchEngine::main_arch_engine != NULL) && ui->filesView->selectUp(true) && (ArchiverProcess::active_processes.count() == 0));
    ui->actionGo_home->setEnabled((BaseArchEngine::main_arch_engine != NULL) && (ArchiverProcess::active_processes.count() == 0));
    ui->actionCreateNewArchive->setEnabled(ArchiverProcess::active_processes.count() == 0);
    ui->actionOpenArchive->setEnabled(ArchiverProcess::active_processes.count() == 0);
}

void MainWindow::on_actionSelect_all_triggered() {
    if (BaseArchEngine::main_arch_engine == NULL) return;

    BaseTreeView * focused_treeview = focusedView();
    if (focused_treeview != NULL) {
        StaticUtils::setAppWaitStatus(tr("Selecting all items..."));
        focused_treeview->selectAllItems();
        StaticUtils::restoreAppStatus();
    }
}

void MainWindow::on_actionSelect_by_template_triggered() {
    if (BaseArchEngine::main_arch_engine == NULL) return;

    BaseTreeView * focused_treeview = focusedView();
    bool ok = false;
    QString ret = QInputDialog::getText(this,tr("Selection by template"),tr("Enter any Qt-like regular expression:"),QLineEdit::Normal,QString(),&ok);
    if (!ok) return;

    if (focused_treeview != NULL) {
        StaticUtils::setAppWaitStatus(tr("Selecting the items..."));
        focused_treeview->selectItemsByWildcard(ret);
        StaticUtils::restoreAppStatus();
    }
}

void MainWindow::on_actionCheck_integrity_triggered() {
    if (BaseArchEngine::main_arch_engine == NULL) return;

    long_processing_started();
    bool ret = ExtractArchEngine(BaseArchEngine::main_arch_engine,QStringList(),"",true,this).exec();
    long_processing_completed();
    if (ret) QMessageBox::information(this,tr("Information..."),tr("The checking of archive integrity has completed ok!!!"));
}

void MainWindow::on_actionAbout_triggered() {
    QMessageBox::about(this,tr("About FileFinder..."),tr("<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">SimpleArch</span> is simple archiver.</p>"
                                                      "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">SimpleArch version is %1.</p>"
                                                      "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Developer: Alex Levkovich (<a href=\"mailto:alevkovich@tut.by\"><span style=\" text-decoration: underline; color:#0057ae;\">alevkovich@tut.by</span></a>)</p>"
                                                      "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">License: GPL</p>").arg(simplearch_version));
}

void MainWindow::on_actionProperties_triggered() {
    if (BaseArchEngine::main_arch_engine == NULL) return;

    qreal uncompressed_size;
    qreal files_count;
    if (!ui->dirView->calcAdditionalInfo(uncompressed_size,files_count)) return;

    QString str = "<table border=\"0\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px;\" width=\"100%\" cellspacing=\"8\" cellpadding=\"0\">"
                  "<tr>"
                  "<td style=\" vertical-align:top;\">"
                  "<p align=\"right\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">File name:</span></p></td>"
                  "<td style=\" vertical-align:top;\">"
                  "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">%1</p></td></tr>"
                  "<tr>"
                  "<td style=\" vertical-align:top;\">"
                  "<p align=\"right\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Path to file:</span></p></td>"
                  "<td style=\" vertical-align:top;\">"
                  "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">%2</p></td></tr>"
                  "<tr>"
                  "<td style=\" vertical-align:top;\">"
                  "<p align=\"right\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Type:</span></p></td>"
                  "<td style=\" vertical-align:top;\">"
                  "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">%3</p></td></tr>"
                  "<tr>"
                  "<td style=\" vertical-align:top;\">"
                  "<p align=\"right\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Modified:</span></p></td>"
                  "<td style=\" vertical-align:top;\">"
                  "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">%4</p></td></tr>"
                  "<tr>"
                  "<td style=\" vertical-align:top;\">"
                  "<p align=\"right\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Packed size:</span></p></td>"
                  "<td style=\" vertical-align:top;\">"
                  "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">%5</p></td></tr>"
                  "<tr>"
                  "<td style=\" vertical-align:top;\">"
                  "<p align=\"right\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Unpacked size:</span></p></td>"
                  "<td style=\" vertical-align:top;\">"
                  "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">%6</p></td></tr>"
                  "<tr>"
                  "<td style=\" vertical-align:top;\">"
                  "<p align=\"right\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Count of the files:</span></p></td>"
                  "<td style=\" vertical-align:top;\">"
                  "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">%7</p></td></tr>"
                  "<tr>"
                  "<td style=\" vertical-align:top;\">"
                  "<p align=\"right\" style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\"><span style=\" font-weight:600;\">Packing ratio:</span></p></td>"
                  "<td style=\" vertical-align:top;\">"
                  "<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">%8  </p></td></tr></table>";
    str.replace(File_name,tr(File_name));
    str.replace(Path_to_file,tr(Path_to_file));
    str.replace(Type,tr(Type));
    str.replace(Modified,tr(Modified));
    str.replace(Packed_size,tr(Packed_size));
    str.replace(Unpacked_size,tr(Unpacked_size));
    str.replace(Count_of_the_files,tr(Count_of_the_files));
    str.replace(Packing_ratio,tr(Packing_ratio));
    QFileInfo info(BaseArchEngine::main_arch_engine->fileName());
    str.replace("%1",info.fileName());
    str.replace("%2",info.path());
    str.replace("%3",ComplexTarArchEngine::fullSuffix(info));
    str.replace("%4",info.lastModified().toString());
    str.replace("%5",BytesHumanizer(info.size()).toString());
    str.replace("%6",BytesHumanizer(uncompressed_size).toString());
    str.replace("%7",QString::number((uint)files_count));
    str.replace("%8",QString::number(uncompressed_size/info.size()));

    QMessageBox::information(this,tr("Archive information..."),str);
}

void MainWindow::on_actionExit_triggered() {
    qApp->quit();
}

void MainWindow::on_actionGo_home_triggered() {
    ui->filesView->selectRoot();
}

void MainWindow::on_actionGo_previous_triggered() {
    ui->filesView->selectPrev();
}

void MainWindow::on_actionGo_next_triggered() {
    ui->filesView->selectNext();
}

void MainWindow::on_actionGo_up_triggered() {
    ui->filesView->selectUp();
}

void MainWindow::on_actionSave_as_triggered() {
    if (BaseArchEngine::main_arch_engine == NULL) return;

    CreateArchiveDialog createDlg(true,false,false,this);
    if (createDlg.exec() == QDialog::Rejected) return;

    long_processing_started();
    if (!SaveAsArchEngine(BaseArchEngine::main_arch_engine,createDlg.selectedFile(),createDlg.useEncription()).exec()) {
        long_processing_completed();
        return;
    }

    openArchive(createDlg.selectedFile(),true);
}

void MainWindow::aboutToQuit() {
    BaseArchEngine::settingsInstance()->setValue("mainSplitterSizes",ui->mainSplitter->saveState());
    BaseArchEngine::settingsInstance()->setValue("splitterSizes",ui->splitter->saveState());
    BaseArchEngine::settingsInstance()->setValue("mainWindowSizes",saveGeometry());
}

void MainWindow::on_actionOpenFile_triggered() {
    if (BaseArchEngine::main_arch_engine == NULL) return;

    QString file_path = ui->filesView->selectedPath();
    if (file_path.isEmpty()) return;

    ExtractToTempArchEngine extract_engine(BaseArchEngine::main_arch_engine,QStringList() << file_path,this);
    StaticUtils::setAppWaitStatus(tr("Extracting the selected items..."));
    if (!extract_engine.exec()) return;
    StaticUtils::restoreAppStatus();

    QString suffix = QFileInfo(file_path).suffix();
    if (BaseArchEngine::userMimes.contains(suffix)) QProcess::startDetached(BaseArchEngine::userMimes[suffix] + QLatin1Char(' ') + extract_engine.outputPaths().at(0).toLocalFile());
    else if (BaseArchEngine::defaultUseSystemMimes) StaticUtils::mimeOpen(extract_engine.outputPaths().at(0).toLocalFile());
    else QMessageBox::critical(this,tr("Error for %1").arg(file_path),tr("No file association is defined to open this file!"));
}

void MainWindow::on_actionSettings_triggered() {
    if (SettingsDialog(this).exec() == QDialog::Rejected) return;

    if (ArchiverProcess::active_processes.count() == 0) {
        ui->dirView->setVisible(BaseArchEngine::defaultDoShowFolderTree);
        ui->dirView->setIconSize(QSize(BaseArchEngine::defaultLeftIconSize,BaseArchEngine::defaultLeftIconSize));
        ui->filesView->setIconSize(QSize(BaseArchEngine::defaultRightIconSize,BaseArchEngine::defaultRightIconSize));
    }

    if (!BaseArchEngine::defaultHighlightFocusedView) {
        ui->dirView->setStyleSheet("");
        ui->filesView->setStyleSheet("");
    }
}
