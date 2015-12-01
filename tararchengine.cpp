#include "tararchengine.h"
#include <QFileInfo>
#include <QDateTime>
#include "filetreeitem.h"

DECLARE_ENGINE(TarArchEngine)

TarArchEngine::TarArchEngine(const QString & fileName,QObject *parent) : BaseArchEngine(fileName,parent) {
}

QString TarArchEngine::list_contents_command() const {
    return QString("%1/tar %2 -tvf \"%3\"").arg(TOOLS_PATH).arg(processingSymbols()).arg(fileName());
}

QString TarArchEngine::processingSymbols() const {
    return "";
}

FileTreeItem * TarArchEngine::split_list_contents_record(const QString & data,bool & ignore) {
    QString path;
    QString link_to;
    QByteArray perms;
    QString user;
    QString group;
    qreal file_size;
    QDateTime date_time;
    bool ok;

    ignore = false;

    QStringList parts = data.split(' ',QString::SkipEmptyParts);
    if (parts.count() < 6) return NULL;

    perms = parts[0].toLocal8Bit();
    if (perms.length() != 10) return NULL;

    file_size = parts[2].toDouble(&ok);
    if (!ok) return NULL;

    date_time = QDateTime::fromString(parts[3]+parts[4],"yyyy-MM-ddhh:mm");
    if (!date_time.isValid()) return NULL;

    QString search_str = parts[3] + " " + parts[4] + " ";

    QString user_group = parts[1];
    parts = user_group.split("/");
    if (parts.count() > 0) {
        user = parts[0];
        group = parts[1];
    }
    else return NULL;

    path = data.mid(data.indexOf(search_str)+search_str.length());
    if (path.contains(" -> ")) {
        parts = path.split(" -> ");
        path = parts[0];
        link_to = parts[1];
    }

    return new FileTreeItem(path,link_to,perms,user,group,file_size,date_time);
}

QString TarArchEngine::uncompress_command(const QStringList & files,const QString & dir_path,bool with_full_path) const {
    QString file_names;
    QString strip_parm;
    if ((files.count() > 1) && !with_full_path) {
        int strip_cnt = 0;
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
        if (strip_cnt > 0) strip_parm = QString("--strip=%1").arg(strip_cnt);
    }
    if (files.count() > 0) {
        if ((files.count() == 1) && !with_full_path) {
            QStringList parts = files[0].split("/",QString::SkipEmptyParts);
            if (parts.count() > 1) strip_parm = QString("--strip=%1").arg(parts.count() - 1);
        }
        if ((files.count() == 1) && !files[0].endsWith("/")) {
            strip_parm += " --occurrence=1";
        }

        for (int i=0;i<files.count();i++) {
            file_names += "'" + files[i] + "' ";
        }
    }

    QString output_dir;
    QString redirection;
    if (dir_path.isEmpty()) {
        output_dir = "--to-stdout";
        redirection = " >/dev/null";
    }
    else output_dir = QString("--directory='%1'").arg(dir_path);

    return QString("%1/bash -c \"%1/tar %2 %3 %4 -xf '%5' %6%7\"").arg(TOOLS_PATH).arg(output_dir).arg(processingSymbols()).arg(strip_parm).arg(fileName()).arg(file_names).arg(redirection);
}

QString TarArchEngine::create_command(const QStringList & files,bool with_full_path,bool) const {
    if (files.count() <= 0) return "";

    QString file_names;
    for (int i=0;i<files.count();i++) {
        QFileInfo info(files[i]);
        file_names += (with_full_path?" ":QString("-C \"%1\" ").arg(info.path())) + "\"" + (with_full_path?files[i]:info.fileName()) + "\" ";
    }
    return QString("%1/tar %2 -cf \"%3\" %4").arg(TOOLS_PATH).arg(processingSymbols()).arg(fileName()).arg(file_names);
}

QString TarArchEngine::update_command(const QStringList & files,bool with_full_path,const QString & archiveDir,bool) const {
    if (files.count() <= 0) return "";

    QString file_names;
    for (int i=0;i<files.count();i++) {
        QFileInfo info(files[i]);
        file_names += (with_full_path?" ":QString("-C '%1' ").arg(info.path())) + "'" + (with_full_path?files[i]:info.fileName()) + "' ";
    }
    QString transform;
    if (!archiveDir.isEmpty()) {
        transform = QString("--transform='s,^,%1/,S'").arg(archiveDir);
    }
    return QString("%1/bash -c \"%1/tar %2 -uf '%3' %4\"").arg(TOOLS_PATH).arg(transform).arg(fileName()).arg(file_names);
}

BaseArchEngine * TarArchEngine::dupEngine(QObject * parent) {
    return new TarArchEngine(fileName(),parent);
}

bool TarArchEngine::isMultiFiles() const {
    return true;
}

bool TarArchEngine::areExternalProgramsOK() {
    return (isProgramAvailable(QString("%1/bash").arg(TOOLS_PATH)) &&
            isProgramAvailable(QString("%1/tar").arg(TOOLS_PATH)));
}

QString TarArchEngine::delete_command(const QStringList & files) const {
    if (files.count() <= 0) return "";

    QString file_names;
    for (int i=0;i<files.count();i++) {
        file_names += "\"" + files[i] + "\" ";
    }
    return QString("%1/tar --delete -f \"%2\" %3").arg(TOOLS_PATH).arg(fileName()).arg(file_names);
}

const QStringList TarArchEngine::suffixes() {
    return QStringList() << "tar";
}

const QString TarArchEngine::openFilter() {
    QStringList filters;
    QStringList suffs = suffixes();
    for (int i=0;i<suffs.count();i++) {
        filters << "*." + suffs[i];
    }
    return tr("Tar files") + " (" + filters.join(" ") + ")";
}

const QString TarArchEngine::createFilter() {
    return openFilter();
}

bool TarArchEngine::parsePasswordRequiring(const QString &,QString &) {
    return false;
}

bool TarArchEngine::parseUpdatePassword(const QString &,bool &) {
    return false;
}

QList<int> TarArchEngine::compressionLevels() const {
    return QList<int>();
}

int TarArchEngine::defaultCompressionLevel() const {
    return -1;
}
