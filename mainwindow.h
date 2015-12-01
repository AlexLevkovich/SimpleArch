#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUrl>

class BaseTreeView;
class QAction;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString & input_file_name,QWidget *parent = 0);
    ~MainWindow();

private slots:
    void dirView_modelUpdated();
    void long_processing_started();
    void long_processing_completed(bool was_error = false);
    void on_actionDelete_files_triggered();
    void on_actionAppendFiles_triggered();
    void on_actionExtractFiles_triggered();
    void on_actionOpenArchive_triggered();
    void on_actionCreateNewArchive_triggered();
    void show_error(const QString & error);
    void post_modelUpdated();
    void on_actionSelect_all_triggered();
    void on_actionSelect_by_template_triggered();
    void on_actionCheck_integrity_triggered();
    void on_actionAbout_triggered();
    void on_actionProperties_triggered();
    void on_actionExit_triggered();
    void on_actionSettings_triggered();
    void on_actionGo_home_triggered();
    void on_actionGo_previous_triggered();
    void on_actionGo_next_triggered();
    void on_actionGo_up_triggered();
    void on_actionSave_as_triggered();
    void aboutToQuit();
    void on_actionOpenFile_triggered();

private:
    void openArchive(const QString & filePath,bool doTarExtract = true);
    BaseTreeView * focusedView();

    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
