#include "7zarchengine.h"
#include <QFileInfo>
#include <QDir>
#include "filetreeitem.h"
#include "staticutils.h"
#include "archiverprocess.h"
#include <QDebug>

#define TMP_DIR_CNT_CREAT  "Cannot create the temporary directory!!!"
#define TMP_COPY_FILES_ERR "Cannot copy files from the temporary directory!!!"
#define ENTR_PASSWORD_FIND "Enter password (will not be echoed) :"
#define VERF_PASSWORD_FIND "Verify password (will not be echoed) :"
#define ERR_STRING_MSG     "Error :"
#define ERR_FIND_MSG       "Archives with Errors:"
#define ARCH_FILES_CNT_0   "Can't create archive without input files!!!"


DECLARE_ENGINE(Z7ArchEngine)

Z7ArchEngine::Z7ArchEngine(const QString & fileName,QObject *parent) : BaseArchEngine(fileName,parent) {
    m_user = StaticUtils::currentUserName();
}

QString Z7ArchEngine::list_contents_command() const {
    return QString("%1/bash -c \"%1/7z l -slt '%2'\"").arg(TOOLS_PATH).arg(fileName());
}

QString Z7ArchEngine::processingSymbols() const {
    return "";
}

bool Z7ArchEngine::startListContents_body() {
    star_listing = false;
    encripted = false;
    err_messages.clear();
    return BaseArchEngine::startListContents_body();
}

FileTreeItem * Z7ArchEngine::split_list_contents_record(const QString & rec,bool & ignore) {
    bool ok = false;
    ignore = true;

    if ((rec == "--") && !star_listing) {
        err_messages.clear();
        return NULL;
    }

    if (!star_listing && (rec == "----------")) {
        star_listing = true;
        return NULL;
    }
    else if (!star_listing) {
        err_messages += rec + "\n";
        return NULL;
    }

    if (!star_listing) return NULL;

    if (rec.startsWith("Path = ")) {
        path = rec.mid(7);
    } else if (rec.isEmpty()) {
        FileTreeItem * item = NULL;
        if (!path.isEmpty()) item = new FileTreeItem(path,"",perms,m_user,m_user,file_size,date_time,encripted);

        ignore = false;
        encripted = false;
        perms.clear();
        file_size = 0;
        path.clear();

        if (item == NULL) {
            ignore = true;
            post_error_signal(err_messages);
            star_listing = false;
        }

        return item;
    } else if (rec.startsWith("Attributes = ")) {
        QString type = rec.mid(13);
        if (type.startsWith("D")) {
            perms = "drw-------";
            if (!path.endsWith("/")) path += "/";
        }
        else perms = "-rw-------";
    } else if (rec.startsWith("Modified = ")) {
        date_time = QDateTime::fromString(rec.mid(11),"yyyy-MM-dd hh:mm:ss");
        if (!date_time.isValid()) return NULL;
    } else if (rec.startsWith("Size = ")) {
        file_size = rec.mid(7).toDouble(&ok);
        if (!ok) return NULL;
    } else if (rec.startsWith("Encrypted = ")) {
        if (rec.mid(12) == "+") encripted = true;
    }

    return NULL;
}

bool Z7ArchEngine::supportsEncripting() const {
    return true;
}

bool Z7ArchEngine::extractFiles_body(const QStringList & files,const QString & dir_path,bool with_full_path) {
    err_messages.clear();

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

void Z7ArchEngine::extraction_ok() {
    if (!m_files_dir.isEmpty()) {
        if (!StaticUtils::copyPath(QDir(temp_dir->path()+"/"+m_files_dir),QDir(m_dir_path))) {
            post_error_signal(tr(TMP_COPY_FILES_ERR));
        }
    }

    remove_temp_dir();
}

void Z7ArchEngine::remove_temp_dir() {
    if (temp_dir != NULL) delete temp_dir;
    temp_dir = NULL;
    disconnect(this,SIGNAL(update_ok()),this,SLOT(remove_temp_dir()));
    disconnect(this,SIGNAL(create_ok()),this,SLOT(remove_temp_dir()));
    disconnect(this,SIGNAL(extraction_ok()),this,SLOT(extraction_ok()));
    disconnect(this,SIGNAL(error(const QString &)),this,SLOT(remove_temp_dir()));
}

QString Z7ArchEngine::uncompress_command(const QStringList & files,const QString & dir_path,bool) const {
    QString file_names;
    if (files.count() > 0) {
        for (int i=0;i<files.count();i++) {
            file_names += "'" + files[i] + (files[i].endsWith("/")?"*":"") + "' ";
        }
    }

    QString output_flag;
    QString redirection;
    if (dir_path.isEmpty()) {
        output_flag = "-so";
        redirection = " >/dev/null";
    }
    else {
        output_flag = "-o'" + dir_path + "'";
        redirection = "";
    }

    return QString("%1/bash -c \"%1/7z x %2 -mmt=on -y '%3' %4 %5\"").arg(TOOLS_PATH).arg(output_flag).arg(fileName()).arg(file_names).arg(redirection);
}

bool Z7ArchEngine::createArchive_body(const QStringList & files,bool with_full_path,bool password_protected) {
    if (files.count() <= 0) {
        post_error_signal(tr(ARCH_FILES_CNT_0));
        return false;
    }

    if (!with_full_path) {
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
            out_name = temp_dir->path()+"/"+files.at(i);
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
            out_name = temp_dir->path()+"/"+info.path();
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

    connect(this,SIGNAL(create_ok()),this,SLOT(remove_temp_dir()));
    connect(this,SIGNAL(error(const QString &)),this,SLOT(remove_temp_dir()));
    return BaseArchEngine::createArchive_body(files,with_full_path,password_protected);
}

QString Z7ArchEngine::create_command(const QStringList & files,bool with_full_path,bool password_protected) const {
    if (files.count() <= 0) return "";

    QString file_names;
    for (int i=0;i<files.count();i++) {
        file_names += "'" + files[i] + "' ";
    }

    QString cd_command;
    if (with_full_path) {
        cd_command = QString("cd '%1'; ").arg(temp_dir->path());
        file_names = "*";
    }

    return QString("%1/bash -c \"%2 %1/7z a %3 -r -mmt=on -mx=%4 '%5' %6\"").arg(TOOLS_PATH).arg(cd_command).arg(password_protected?"-P":"").arg(compressionLevel()).arg(fileName()).arg(file_names);
}

bool Z7ArchEngine::updateArchive_body(const QStringList & files,bool with_full_path,const QString & archiveDir,bool password_protected) {
    if (files.count() <= 0) {
        post_error_signal(tr(ARCH_FILES_CNT_0));
        return false;
    }

    if (!with_full_path && archiveDir.isEmpty()) {
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
    else {
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

QString Z7ArchEngine::update_command(const QStringList & files,bool with_full_path,const QString & archiveDir,bool password_protected) const {
    if (files.count() <= 0) return "";

    QString file_names;
    for (int i=0;i<files.count();i++) {
        file_names += "'" + files[i] + "' ";
    }

    QString cd_command;
    if (with_full_path || !archiveDir.isEmpty()) {
        cd_command = QString("cd '%1'; ").arg(temp_dir->path());
        file_names = "*";
    }

    return QString("%1/bash -c \"%2 %1/7z u %3 -mmt=on -r '%4' %5\"").arg(TOOLS_PATH).arg(cd_command).arg(password_protected?"-P":"").arg(fileName()).arg(file_names);
}

BaseArchEngine * Z7ArchEngine::dupEngine(QObject * parent) {
    return new Z7ArchEngine(fileName(),parent);
}

bool Z7ArchEngine::isMultiFiles() const {
    return true;
}

bool Z7ArchEngine::areExternalProgramsOK() {
    return (isProgramAvailable(QString("%1/bash").arg(TOOLS_PATH)) &&
            isProgramAvailable(QString("%1/7z").arg(TOOLS_PATH)));
}

QString Z7ArchEngine::delete_command(const QStringList & files) const {
    if (files.count() <= 0) return "";

    QString file_names;
    for (int i=0;i<files.count();i++) {
        file_names += "\"" + files[i] + "\" ";
    }
    return QString("%1/7z d -mmt=on \"%2\" %3").arg(TOOLS_PATH).arg(fileName()).arg(file_names);
}

const QStringList Z7ArchEngine::suffixes() {
    return QStringList() << "7z";
}

const QString Z7ArchEngine::openFilter() {
    QStringList filters;
    QStringList suffs = suffixes();
    for (int i=0;i<suffs.count();i++) {
        filters << "*." + suffs[i];
    }
    return tr("7z files") + " (" + filters.join(" ") + ")";
}

const QString Z7ArchEngine::createFilter() {
    return openFilter();
}

bool Z7ArchEngine::parsePasswordRequiring(const QString & line,QString & fileName) {
    fileName.clear();


    if (line.startsWith(ENTR_PASSWORD_FIND)) {
        fileName = tr("all files");
        return true;
    }

    if (line.contains(ERR_FIND_MSG)) {
        emit error_message(err_messages);
        return false;
    }

    if (line.contains(ERR_STRING_MSG)) {
        err_messages += line + "\n";
        return false;
    }

    return false;
}

BaseArchEngine::ChannelFlag Z7ArchEngine::parsePasswordChannel() const {
    return BaseArchEngine::StdoutChannel;
}

bool Z7ArchEngine::parseUpdatePassword(const QString & line,bool & verificate) {
    verificate = false;
    if (line.startsWith(ENTR_PASSWORD_FIND)) return true;
    if (line.startsWith(VERF_PASSWORD_FIND)) {
        verificate = true;
        return true;
    }

    if (line.startsWith("password verification failed")) {
        emit error_message(line);
        return false;
    }

    return false;
}

QList<int> Z7ArchEngine::compressionLevels() const {
    return QList<int>() << 0 << 1 << 3 << 5 << 7 << 9;
}

int Z7ArchEngine::defaultCompressionLevel() const {
    return 5;
}
