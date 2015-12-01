#include "rararchengine.h"
#include <QFileInfo>
#include <QDir>
#include "filetreeitem.h"
#include "staticutils.h"
#include "archiverprocess.h"

#define TMP_DIR_CNT_CREAT  "Cannot create the temporary directory!!!"
#define TMP_COPY_FILES_ERR "Cannot copy files from the temporary directory!!!"
#define ENTR_PASSWORD_FIND "Enter password (will not be echoed) for "
#define WRONG_PASSWORD_MSG "Corrupt file or wrong password."


DECLARE_ENGINE(RarArchEngine)

RarArchEngine::RarArchEngine(const QString & fileName,QObject *parent) : BaseArchEngine(fileName,parent) {
    m_user = StaticUtils::currentUserName();
}

QString RarArchEngine::list_contents_command() const {
    return QString("%1/bash -c \"%1/unrar lta '%2'\"").arg(TOOLS_PATH).arg(fileName());
}

QString RarArchEngine::processingSymbols() const {
    return "";
}

FileTreeItem * RarArchEngine::split_list_contents_record(const QString & data,bool & ignore) {
    bool ok = false;
    ignore = true;

    QString rec = StaticUtils::simplifyLeft(data);
    if (rec.startsWith("Name: ")) {
        FileTreeItem * item = NULL;
        if (!path.isEmpty()) item = new FileTreeItem(path,"",perms,m_user,m_user,file_size,date_time,encripted);

        encripted = false;
        perms.clear();
        file_size = 0;
        path = rec.mid(6);
        if (item != NULL) {
            ignore = false;
            return item;
        }
    } else if (rec.startsWith("Service: EOF")) {
        FileTreeItem * item = NULL;
        if (!path.isEmpty()) item = new FileTreeItem(path,"",perms,m_user,m_user,file_size,date_time,encripted);

        encripted = false;
        perms.clear();
        file_size = 0;
        path.clear();

        if (item != NULL) {
            ignore = false;
            return item;
        }
    } else if (rec.startsWith("Type: ")) {
        QString type = rec.mid(6);
        if (type == "Directory") {
            perms = "drw-------";
            if (!path.endsWith("/")) path += "/";
        }
        else if (type == "Unix symbolic link") perms = "lrw-------";
        else perms = "-rw-------";
    } else if (rec.startsWith("mtime: ")) {
        date_time = QDateTime::fromString(rec.mid(7),"yyyy-MM-dd hh:mm:ss,zzz");
        if (!date_time.isValid()) return NULL;
    } else if (rec.startsWith("Size: ")) {
        file_size = rec.mid(6).toDouble(&ok);
        if (!ok) return NULL;
    } else if (rec.startsWith("Flags: ")) {
        if (rec.mid(7).contains("encrypted")) encripted = true;
    } else if (rec.startsWith("ERROR: ")) {
        post_error_signal(rec.mid(7));
    }

    return NULL;
}

bool RarArchEngine::supportsEncripting() const {
    return true;
}

bool RarArchEngine::extractFiles_body(const QStringList & files,const QString & dir_path,bool with_full_path) {
    use_prev_password = true;

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

void RarArchEngine::extraction_ok() {
    if (!m_files_dir.isEmpty()) {
        if (!StaticUtils::copyPath(QDir(temp_dir->path()+"/"+m_files_dir),QDir(m_dir_path))) {
            post_error_signal(tr(TMP_COPY_FILES_ERR));
        }
    }

    remove_temp_dir();
}

void RarArchEngine::remove_temp_dir() {
    if (temp_dir != NULL) delete temp_dir;
    temp_dir = NULL;
    disconnect(this,SIGNAL(update_ok()),this,SLOT(remove_temp_dir()));
    disconnect(this,SIGNAL(create_ok()),this,SLOT(remove_temp_dir()));
    disconnect(this,SIGNAL(extraction_ok()),this,SLOT(extraction_ok()));
    disconnect(this,SIGNAL(error(const QString &)),this,SLOT(remove_temp_dir()));
}

QString RarArchEngine::uncompress_command(const QStringList & files,const QString & dir_path,bool) const {
    QString file_names;
    if (files.count() > 0) {
        for (int i=0;i<files.count();i++) {
            file_names += "'" + files[i] + (files[i].endsWith("/")?"*":"") + "' ";
        }
    }

    QString output_flag;
    QString redirection;
    if (dir_path.isEmpty()) {
        output_flag = "p";
        redirection = " >/dev/null";
    }
    else {
        output_flag = 'x';
        redirection = "'" + dir_path + "'";
    }

    return QString("%1/bash -c \"%1/unrar %2 -o+ '%3' %4 %5\"").arg(TOOLS_PATH).arg(output_flag).arg(fileName()).arg(file_names).arg(redirection);
}

QString RarArchEngine::create_command(const QStringList &,bool,bool) const {
    return "";
}

QString RarArchEngine::update_command(const QStringList &,bool,const QString &,bool) const {
    return "";
}

BaseArchEngine * RarArchEngine::dupEngine(QObject * parent) {
    return new RarArchEngine(fileName(),parent);
}

bool RarArchEngine::isMultiFiles() const {
    return true;
}

bool RarArchEngine::areExternalProgramsOK() {
    return (isProgramAvailable(QString("%1/bash").arg(TOOLS_PATH)) &&
            isProgramAvailable(QString("%1/unrar").arg(TOOLS_PATH)));
}

QString RarArchEngine::delete_command(const QStringList &) const {
    return "";
}

const QStringList RarArchEngine::suffixes() {
    return QStringList() << "rar";
}

const QString RarArchEngine::openFilter() {
    QStringList filters;
    QStringList suffs = suffixes();
    for (int i=0;i<suffs.count();i++) {
        filters << "*." + suffs[i];
    }
    return tr("Rar files") + " (" + filters.join(" ") + ")";
}

const QString RarArchEngine::createFilter() {
    return openFilter();
}

bool RarArchEngine::parsePasswordRequiring(const QString & _line,QString & fileName) {
    fileName.clear();

    QString line = _line;
    StaticUtils::simplifyRight(line);

    if (line.startsWith(ENTR_PASSWORD_FIND)) {
        fileName = line.mid(strlen(ENTR_PASSWORD_FIND));
        if (fileName.endsWith(":")) fileName.chop(1);
        return true;
    }

    if (line.endsWith("OK") && line.startsWith("Extracting ")) {
        use_prev_password = true;
        return false;
    }

    if (line.endsWith(WRONG_PASSWORD_MSG)) {
        use_prev_password = false;
        emit error_message(tr(WRONG_PASSWORD_MSG));
        return false;
    }

    if (line.contains("use current password ?")) {
        sendPassword(use_prev_password?"a":"n");
        return false;
    }

    return false;
}

bool RarArchEngine::parseUpdatePassword(const QString &,bool &) {
    return false;
}

QList<int> RarArchEngine::compressionLevels() const {
    return QList<int>();
}

int RarArchEngine::defaultCompressionLevel() const {
    return -1;
}
