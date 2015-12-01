#include "ziparchengine.h"
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QDebug>
#include "filetreeitem.h"
#include "staticutils.h"

#define TMP_DIR_CNT_CREAT  "Cannot create the temporary directory!!!"
#define TMP_COPY_FILES_ERR "Cannot copy files from the temporary directory!!!"
#define ARCH_FILES_CNT_0   "Can't create archive without input files!!!"
#define ERR_PARSING        "Cannot parse file list record!!!"

DECLARE_ENGINE(ZipArchEngine)

ZipArchEngine::ZipArchEngine(const QString & fileName,QObject *parent) : BaseArchEngine(fileName,parent) {
    m_user = StaticUtils::currentUserName();
}

QString ZipArchEngine::list_contents_command() const {
    return QString("%1/bash -c \"%1/zipinfo -lT '%2'\"").arg(TOOLS_PATH).arg(fileName());
}

QString ZipArchEngine::processingSymbols() const {
    return "";
}

bool ZipArchEngine::startListContents_body() {
    is_error = false;
    return BaseArchEngine::startListContents_body();
}

FileTreeItem * ZipArchEngine::split_list_contents_record(const QString & data,bool & ignore) {
    QString path;
    QString link_to;
    QByteArray perms;
    QString user;
    QString group;
    qreal file_size;
    QDateTime date_time;
    bool ok;
    bool encripted = false;

    ignore = false;

    if (data.startsWith(QString("Archive:  %1").arg(fileName())) ||
        data.startsWith("Zip file size: ")) {
        ignore = true;
        return NULL;
    }

    if (data.startsWith(QString("[%1]").arg(fileName()))) {
        is_error = true;
        ignore = true;
        return NULL;
    }

    if (is_error) {
        addProcessErrorLine(data);
        ignore = true;
        return NULL;
    }

    QStringList parts = data.split(' ',QString::SkipEmptyParts);

    if (parts.count() < 2) return NULL;
    if (parts[1].endsWith(',')) {
        ignore = true;
        return NULL;
    }

    if (parts.count() < 9) return NULL;

    if (parts[4].length() != 2) return NULL;

    if (parts[4].at(1) == 'X') encripted = true;

    perms = parts[0].toLocal8Bit();

    file_size = parts[3].toDouble(&ok);
    if (!ok) return NULL;

    date_time = QDateTime::fromString(parts[7],"yyyyMMdd.hhmmss");
    if (!date_time.isValid()) return NULL;

    user = m_user;
    group = m_user;

    QString search_str = parts[6] + " " + parts[7] + " ";

    path = data.mid(data.indexOf(search_str)+search_str.length());
    if (path.contains(" -> ")) {
        parts = path.split(" -> ");
        path = parts[0];
        link_to = parts[1];
    }

    return new FileTreeItem(path,link_to,perms,user,group,file_size,date_time,encripted);
}

bool ZipArchEngine::supportsEncripting() const {
    return true;
}

bool ZipArchEngine::extractFiles_body(const QStringList & files,const QString & dir_path,bool with_full_path) {
    temp_dir = new QTemporaryDir();
    if (!temp_dir->isValid()) {
        delete temp_dir;
        post_error_signal(tr(TMP_DIR_CNT_CREAT));
        return false;
    }

    uint strip_cnt = 0;
    if ((files.count() > 1) && !with_full_path) {
        QString begin_str;
        QStringList parts = files[0].split("/",QString::SkipEmptyParts);
        for (int i=0;i<parts.count();i++) {
            begin_str += parts[i] + "/";
            bool do_strip = true;
            for (int j=1;j<files.count();j++) {
                if (!files[j].startsWith(begin_str)) {
                    do_strip = false;
                    break;
                }
            }
            if (do_strip) strip_cnt++;
        }
    }
    if ((files.count() == 1) && !with_full_path) {
        QStringList parts = files[0].split("/",QString::SkipEmptyParts);
        if (parts.count() > 1) strip_cnt = parts.count() - 1;
    }

    if (strip_cnt > 0) {
        m_dir_path = dir_path;
        m_files_dir = files.at(0);
        QStringList parts = m_files_dir.split("/",QString::SkipEmptyParts);
        m_files_dir.clear();
        for (uint i=0;i<strip_cnt;i++) {
            m_files_dir += (((uint)i == 0)?"":"/") + parts.at(i);
        }
    }
    else m_files_dir.clear();

    connect(this,SIGNAL(extraction_ok()),this,SLOT(extraction_ok()));
    connect(this,SIGNAL(error(const QString &)),this,SLOT(remove_temp_dir()));
    return BaseArchEngine::extractFiles_body(files,(with_full_path || (strip_cnt <= 0))?dir_path:temp_dir->path(),true);
}

void ZipArchEngine::extraction_ok() {
    if (!m_files_dir.isEmpty()) {
        if (!StaticUtils::copyPath(QDir(temp_dir->path()+"/"+m_files_dir),QDir(m_dir_path))) {
            post_error_signal(tr(TMP_COPY_FILES_ERR));
        }
    }

    remove_temp_dir();
}

void ZipArchEngine::remove_temp_dir() {
    if (temp_dir != NULL) delete temp_dir;
    temp_dir = NULL;
    disconnect(this,SIGNAL(update_ok()),this,SLOT(remove_temp_dir()));
    disconnect(this,SIGNAL(create_ok()),this,SLOT(remove_temp_dir()));
    disconnect(this,SIGNAL(extraction_ok()),this,SLOT(extraction_ok()));
    disconnect(this,SIGNAL(error(const QString &)),this,SLOT(remove_temp_dir()));
}

QString ZipArchEngine::uncompress_command(const QStringList & files,const QString & dir_path,bool) const {
    QString file_names;
    if (files.count() > 0) {
        for (int i=0;i<files.count();i++) {
            file_names += "'" + files[i] + (files[i].endsWith("/")?"*":"") + "' ";
        }
    }

    QString output_dir;
    QString redirection;
    if (dir_path.isEmpty()) {
        output_dir = "-p";
        redirection = " >/dev/null";
    }
    else output_dir = QString("-d '%1'").arg(dir_path);

    return QString("%1/bash -c \"%1/unzip %2 '%3' %4 %5\"").arg(TOOLS_PATH).arg(output_dir).arg(fileName()).arg(file_names).arg(redirection);
}

bool ZipArchEngine::createArchive_body(const QStringList & files,bool with_full_path,bool password_protected) {
    if (files.count() <= 0) {
        post_error_signal(tr(ARCH_FILES_CNT_0));
        return false;
    }

    if (with_full_path) {
        return BaseArchEngine::createArchive_body(files,with_full_path,password_protected);
    }

    temp_dir = new QTemporaryDir();
    if (!temp_dir->isValid()) {
        delete temp_dir;
        post_error_signal(tr(TMP_DIR_CNT_CREAT));
        return false;
    }

    QString out_name;
    for (int i=0;i<files.count();i++) {
        QFileInfo info(files.at(i));
        if (info.isDir()) {
            out_name = temp_dir->path()+"/"+info.fileName();
            if (!QDir().mkpath(out_name)) {
                delete temp_dir;
                post_error_signal(tr(TMP_DIR_CNT_CREAT));
                return false;
            }
            if (!StaticUtils::copyPath(QDir(files.at(i)),QDir(out_name))) {
                delete temp_dir;
                post_error_signal(tr(TMP_COPY_FILES_ERR));
                return false;
            }
        }
        else {
            if (!QFile::copy(files.at(i),temp_dir->path()+"/"+info.fileName())) {
                delete temp_dir;
                post_error_signal(tr(TMP_COPY_FILES_ERR));
                return false;
            }
        }
    }

    connect(this,SIGNAL(create_ok()),this,SLOT(remove_temp_dir()));
    connect(this,SIGNAL(error(const QString &)),this,SLOT(remove_temp_dir()));
    return BaseArchEngine::createArchive_body(files,with_full_path,password_protected);
}

QString ZipArchEngine::create_command(const QStringList & files,bool with_full_path,bool password_protected) const {
    if (files.count() <= 0) return "";

    QString file_names;
    for (int i=0;i<files.count();i++) {
        file_names += "'" + files[i] + "' ";
    }

    QString cd_command;
    if (!with_full_path) {
        cd_command = QString("cd '%1'; ").arg(temp_dir->path());
        file_names = "*";
    }

    return QString("%1/bash -c \"%2 %1/zip --symlinks -%3 -r %4 '%5' %6\"").arg(TOOLS_PATH).arg(cd_command).arg(compressionLevel()).arg(password_protected?"-e":"").arg(fileName()).arg(file_names);
}

bool ZipArchEngine::updateArchive_body(const QStringList & files,bool with_full_path,const QString & archiveDir,bool password_protected) {
    if (files.count() <= 0) {
        post_error_signal(tr(ARCH_FILES_CNT_0));
        return false;
    }

    if (with_full_path && archiveDir.isEmpty()) {
        return BaseArchEngine::updateArchive_body(files,with_full_path,archiveDir,password_protected);
    }

    temp_dir = new QTemporaryDir();
    if (!temp_dir->isValid()) {
        delete temp_dir;
        post_error_signal(tr(TMP_DIR_CNT_CREAT));
        return false;
    }

    if (!with_full_path) {
        QString out_name;
        if (!archiveDir.isEmpty()) {
            out_name = temp_dir->path()+"/"+archiveDir;
            if (!QDir().mkpath(out_name)) {
                delete temp_dir;
                post_error_signal(tr(TMP_DIR_CNT_CREAT));
                return false;
            }
        }
        for (int i=0;i<files.count();i++) {
            QFileInfo info(files.at(i));
            if (info.isDir()) {
                out_name = temp_dir->path()+"/"+(archiveDir.isEmpty()?"":(archiveDir+"/"))+info.fileName();
                if (!QDir().mkpath(out_name)) {
                    delete temp_dir;
                    post_error_signal(tr(TMP_DIR_CNT_CREAT));
                    return false;
                }
                if (!StaticUtils::copyPath(QDir(files.at(i)),QDir(out_name))) {
                    delete temp_dir;
                    post_error_signal(tr(TMP_COPY_FILES_ERR));
                    return false;
                }
            }
            else {
                if (!QFile::copy(files.at(i),temp_dir->path()+"/"+(archiveDir.isEmpty()?"":(archiveDir+"/"))+info.fileName())) {
                    delete temp_dir;
                    post_error_signal(tr(TMP_COPY_FILES_ERR));
                    return false;
                }
            }
        }
    }
    else if (with_full_path && !archiveDir.isEmpty()) {
        QString out_name;
        for (int i=0;i<files.count();i++) {
            QFileInfo info(files.at(i));
            if (info.isDir()) {
                out_name = temp_dir->path()+"/"+archiveDir+files.at(i);
                if (!QDir().mkpath(out_name)) {
                    delete temp_dir;
                    post_error_signal(tr(TMP_DIR_CNT_CREAT));
                    return false;
                }
                if (!StaticUtils::copyPath(QDir(files.at(i)),QDir(out_name))) {
                    delete temp_dir;
                    post_error_signal(tr(TMP_COPY_FILES_ERR));
                    return false;
                }
            }
            else {
                out_name = temp_dir->path()+"/"+archiveDir+"/"+info.path();
                if (!QDir().mkpath(out_name)) {
                    delete temp_dir;
                    post_error_signal(tr(TMP_DIR_CNT_CREAT));
                    return false;
                }
                if (!QFile::copy(files.at(i),out_name+"/"+info.fileName())) {
                    delete temp_dir;
                    post_error_signal(tr(TMP_COPY_FILES_ERR));
                    return false;
                }
            }
        }
    }

    connect(this,SIGNAL(update_ok()),this,SLOT(remove_temp_dir()));
    connect(this,SIGNAL(error(const QString &)),this,SLOT(remove_temp_dir()));
    return BaseArchEngine::updateArchive_body(files,with_full_path,archiveDir,password_protected);
}

QString ZipArchEngine::update_command(const QStringList & files,bool with_full_path,const QString & archiveDir,bool password_protected) const {
    if (files.count() <= 0) return "";

    QString file_names;
    for (int i=0;i<files.count();i++) {
        file_names += "'" + files[i] + "' ";
    }

    QString cd_command;
    if (!with_full_path || !archiveDir.isEmpty()) {
        cd_command = QString("cd '%1'; ").arg(temp_dir->path());
        file_names = "*";
    }

    return QString("%1/bash -c \"%2 %1/zip --symlinks -u %3 -r '%4' %5\"").arg(TOOLS_PATH).arg(cd_command).arg(password_protected?"-e":"").arg(fileName()).arg(file_names);
}

BaseArchEngine * ZipArchEngine::dupEngine(QObject * parent) {
    return new ZipArchEngine(fileName(),parent);
}

bool ZipArchEngine::isMultiFiles() const {
    return true;
}

bool ZipArchEngine::areExternalProgramsOK() {
    return (isProgramAvailable(QString("%1/bash").arg(TOOLS_PATH)) &&
            isProgramAvailable(QString("%1/zipinfo").arg(TOOLS_PATH)) &&
            isProgramAvailable(QString("%1/zip").arg(TOOLS_PATH)) &&
            isProgramAvailable(QString("%1/unzip").arg(TOOLS_PATH)));
}

QString ZipArchEngine::delete_command(const QStringList & files) const {
    if (files.count() <= 0) return "";

    QString file_names;
    for (int i=0;i<files.count();i++) {
        file_names += "\"" + files[i] + "\" ";
        if (files[i].endsWith("/")) file_names += "\"" + files[i] + "*\" ";
    }
    return QString("%1/zip -d \"%2\" %3").arg(TOOLS_PATH).arg(fileName()).arg(file_names);
}

const QStringList ZipArchEngine::suffixes() {
    return QStringList() << "zip" << "jar";
}

const QString ZipArchEngine::openFilter() {
    QStringList filters;
    QStringList suffs = suffixes();
    for (int i=0;i<suffs.count();i++) {
        filters << "*." + suffs[i];
    }
    return tr("Zip files") + " (" + filters.join(" ") + ")";
}

const QString ZipArchEngine::createFilter() {
    return openFilter();
}

bool ZipArchEngine::parsePasswordRequiring(const QString & _line,QString & fileName) {
    fileName.clear();
    QString line = _line.trimmed();
    QString search_str = "["+this->fileName()+"] ";
    if (line.startsWith(search_str) && line.endsWith(" password:")) {
        int last_index = line.lastIndexOf(" password:");
        if (last_index < 0) return false;

        int index = line.indexOf(search_str);
        if (index < 0) return false;

        index += search_str.length();
        fileNameForPassword = fileName = line.mid(index,last_index-index);
        return true;
    }
    if (line == "password incorrect--reenter:") {
        fileName = fileNameForPassword;
        return true;
    }
    return false;
}

bool ZipArchEngine::parseUpdatePassword(const QString & _line,bool & verificate) {
    qDebug() << _line;
    verificate = false;
    QString line = _line.trimmed();
    if (line.startsWith("Enter password:")) return true;
    if (line.startsWith("Verify password:")) {
        verificate = true;
        return true;
    }

    return false;
}

QList<int> ZipArchEngine::compressionLevels() const {
    QList<int> ret;
    return (ret << 0 << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9);
}

int ZipArchEngine::defaultCompressionLevel() const {
    return 6;
}
