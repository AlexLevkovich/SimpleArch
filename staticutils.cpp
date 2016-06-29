#include <gio/gio.h>
#include "staticutils.h"
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#include <QMainWindow>
#include <QInputDialog>
#include <QApplication>
#include <QProcess>
#include <QMessageBox>
#include "statusbar.h"
#include "createarchivedialog.h"
#include "basearchengine.h"
#include "createarchengine.h"
#include "waitdialog.h"
#include "folderchooser.h"
#include "extractarchengine.h"
#include "updatearchengine.h"
#include <QTemporaryFile>
#include <QUrl>

#define CANT_FIND_ENGINE "Cannot find a suitable engine to open %1 file!"
//#define DEV_SHM          "/dev/shm"

bool StaticUtils::copyPath(const QDir & src,const QDir & dst) {
    if (!src.exists() || !dst.exists()) return false;

    QString dst_path;
    foreach (QString d, src.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        dst_path = dst.path() + QDir::separator() + d;
        if (!src.mkpath(dst_path)) return false;
        if (!copyPath(QDir(src.path() + QDir::separator() + d), QDir(dst_path))) return false;
    }

    foreach (QString f, src.entryList(QDir::Files)) {
        if (!QFile::copy(src.path() + QDir::separator() + f, dst.path() + QDir::separator() + f)) return false;
    }

    return true;
}

const QString StaticUtils::currentUserName() {
    QString userName;
    register struct passwd *pw;

    pw = getpwuid(getuid());
    if (pw) userName = QString::fromLocal8Bit(pw->pw_name);
    return userName;
}

QMainWindow * StaticUtils::findMainWindow() {
    foreach(QWidget *widget, qApp->topLevelWidgets()) {
        if(widget->inherits("QMainWindow")) {
            return (QMainWindow *)widget;
        }
    }

    return NULL;
}

const QString StaticUtils::inputPasswordRequest(const QString & fileName) {
    bool ok = false;
    QString ret = QInputDialog::getText(findMainWindow(),QObject::tr("Password is required"),QObject::tr("to extract")+" "+fileName,QLineEdit::Password,QString(),&ok,0,Qt::ImhHiddenText);
    if (!ok) return "";
    return ret;
}

const QString StaticUtils::inputPasswordRequest(bool verify_request) {
    bool ok = false;
    QString ret = QInputDialog::getText(findMainWindow(),QObject::tr("Password request"),verify_request?QObject::tr("Verify password:"):QObject::tr("Enter password:"),QLineEdit::Password,QString(),&ok,0,Qt::ImhHiddenText);
    if (!ok) return "";
    return ret;
}

void StaticUtils::setAppWaitStatus(const QString & message) {
    QMainWindow * wnd = findMainWindow();
    if (wnd == NULL) return;
    StatusBar * statusBar = (StatusBar *)wnd->statusBar();
    if (statusBar == NULL) return;
    statusBar->setWaitStatus(true);
    statusBar->showMessage(message);
    QApplication::setOverrideCursor(Qt::WaitCursor);
}

void StaticUtils::restoreAppStatus() {
    QMainWindow * wnd = findMainWindow();
    if (wnd == NULL) return;
    StatusBar * statusBar = (StatusBar *)wnd->statusBar();
    if (statusBar == NULL) return;
    statusBar->setWaitStatus(false);
    statusBar->clearMessage();
    QApplication::restoreOverrideCursor();
}

const QString StaticUtils::simplifyLeft(const QString & str) {
    for (int i=0;i<str.length();i++) {
        if ((str.at(i) != '\n') &&
            (str.at(i) != ' ') &&
            (str.at(i) != '\r') &&
            (str.at(i) != '\t')) {
            return (i==0)?str:str.mid(i);
        }
    }
    return "";
}

void StaticUtils::simplifyRight(QString & str) {
    int len = str.length();
    int cnt = 0;
    for (int i=(len-1);i>=0;i--) {
        if ((str.at(i) == '\n') ||
            (str.at(i) == ' ') ||
            (str.at(i) == '\r') ||
            (str.at(i) == '\t')) {
            cnt++;
        }
        else break;
    }
    if (cnt > 0) str.chop(cnt);
}

bool StaticUtils::createArchive(const QStringList & items) {
    if (items.count() <= 0) return false;

    CreateArchiveDialog dlg(true,true,true);
    if (dlg.exec() == QDialog::Rejected) return false;

    BaseArchEngine * engine = BaseArchEngine::findEngine(dlg.selectedFile(),NULL);
    if (engine == NULL) {
        QMessageBox::critical(&dlg,QObject::tr("Error!!!"),QObject::tr(CANT_FIND_ENGINE).arg(engine->fileName()));
        return false;
    }

    WaitDialog wait_dlg(QObject::tr("Creating %1").arg(engine->fileName())+"...",&dlg);
    wait_dlg.show();

    QObject::connect(engine,SIGNAL(error(const QString &)),&wait_dlg,SLOT(show_error(const QString &)));
    bool ret = CreateArchEngine(engine,items,dlg.useFullPath(),dlg.useEncription()).exec();

    delete engine;
    return ret;
}

bool StaticUtils::addToArchive(const QString & fileName,const QStringList & items) {
    BaseArchEngine * engine = BaseArchEngine::findEngine(fileName,NULL);
    if (engine == NULL) {
        QMessageBox::critical(NULL,QObject::tr("Error!!!"),QObject::tr(CANT_FIND_ENGINE).arg(engine->fileName()));
        return false;
    }
    WaitDialog wait_dlg(QObject::tr("Appending the files to %1").arg(engine->fileName())+"...",NULL);
    wait_dlg.show();
    QObject::connect(engine,SIGNAL(error(const QString &)),&wait_dlg,SLOT(show_error(const QString &)));
    bool ret = UpdateArchEngine(engine,items,false).exec();

    delete engine;
    return ret;
}

bool StaticUtils::extractArchive(const QString & fileName,const QString & _toDir) {
    if (!QFileInfo(fileName).exists()) return false;

    QString toDir = _toDir;
    QFileInfo dir_info(toDir);
    if (!dir_info.exists() || !dir_info.isDir()) toDir.clear();

    if (toDir.isEmpty()) {
        FolderChooser folder_dlg(QDir::currentPath());
        if (folder_dlg.exec() == QDialog::Rejected) return false;

        toDir = folder_dlg.folderPath();
    }

    BaseArchEngine * engine = BaseArchEngine::findEngine(fileName,NULL);
    if (engine == NULL) {
        QMessageBox::critical(NULL,QObject::tr("Error!!!"),QObject::tr(CANT_FIND_ENGINE).arg(engine->fileName()));
        return false;
    }

    WaitDialog wait_dlg(QObject::tr("Extracting %1").arg(engine->fileName())+"...",NULL);
    wait_dlg.show();

    QObject::connect(engine,SIGNAL(error(const QString &)),&wait_dlg,SLOT(show_error(const QString &)));
    bool ret = ExtractArchEngine(engine,QStringList(),toDir,true).exec();

    delete engine;
    return ret;
}

const QString StaticUtils::urlOrLocalPath(const QString & url_or_path) {
    QUrl url(url_or_path);
    if (!url.isValid()) return url_or_path;
    return url.toLocalFile();
}

const QString StaticUtils::urlOrLocalPath(const char * url_or_path) {
    return urlOrLocalPath(QString::fromLocal8Bit(url_or_path));
}

const QString StaticUtils::tempFileName(bool autoRemove) {
    QTemporaryFile file;
    //if (QDir(DEV_SHM).exists()) file.setFileTemplate(QString(DEV_SHM)+"/simplearch_XXXXXX");
    file.setFileTemplate(QDir::tempPath()+"/simplearch_XXXXXX");
    file.setAutoRemove(autoRemove);
    if (!file.open()) return QString();
    return file.fileName();
}

class MimeProcess: public QProcess {
    Q_OBJECT
public:
    MimeProcess(const QString & filePath,QObject * parent = NULL) : QProcess(parent) {
        this->filePath = filePath;

        connect(this,SIGNAL(error(QProcess::ProcessError)),this,SLOT(slot_finished()));
        connect(this,SIGNAL(error(QProcess::ProcessError)),this,SLOT(deleteLater()));
        connect(this,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(slot_finished(int,QProcess::ExitStatus)));
        connect(this,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(deleteLater()));

        start(QString("xdg-open %1").arg(filePath));
    }

private slots:
    void slot_finished() {
        slot_finished(1,QProcess::NormalExit);
    }

    void slot_finished(int code,QProcess::ExitStatus) {
        if (code == 0) return;

        GError *error;
        GFileInfo *file_info = NULL;
        GAppInfo *app_info = NULL;
        GFile *file = g_file_new_for_path(filePath.toLocal8Bit().constData());
        if (file != NULL) {
            file_info = g_file_query_info(file,"standard::*",(GFileQueryInfoFlags)0,NULL,&error);
            if (file_info != NULL) {
                const char *content_type = g_file_info_get_content_type (file_info);
                if (content_type != NULL) {
                    app_info = g_app_info_get_default_for_type(content_type,FALSE);
                    if (app_info != NULL) {
                        QProcess::startDetached(QString::fromLocal8Bit(QByteArray(g_app_info_get_executable(app_info))) + " " + filePath);
                    }
                }
            }
        }
        if (file != NULL) g_object_unref(file);
        if (file_info != NULL) g_object_unref(file_info);
        if (app_info != NULL) g_object_unref(app_info);
    }

private:
    QString filePath;
};

void StaticUtils::mimeOpen(const QString & filePath) {
    new MimeProcess(filePath);
}

#include "staticutils.moc"
