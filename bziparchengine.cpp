#include "bziparchengine.h"
#include <QFileInfo>

DECLARE_ENGINE(BzipArchEngine)

BzipArchEngine::BzipArchEngine(const QString & fileName,QObject *parent,bool force_tobe_nottar) : ComplexTarArchEngine(fileName,parent,force_tobe_nottar) {
}

QString BzipArchEngine::uncompress_command(const QStringList & files,const QString & dir_path,bool with_full_path) const {
    if (!isTar()) {
        return QString("%1/bash -c \"%2 -d --stdout -k '%3' > '%4/%5'\"").arg(TOOLS_PATH).arg(archiverPath()).arg(fileName()).arg(dir_path.isEmpty()?"/dev":dir_path).arg(dir_path.isEmpty()?"null":tarName());
    }
    return TarArchEngine::uncompress_command(files,dir_path,with_full_path);
}

QString BzipArchEngine::create_command(const QStringList & files,bool with_full_path,bool password_protected) const {
    if (files.count() <= 0) return "";

    if (!isTar()) {
        if (files.count() > 1) return "";
        if (QFileInfo(files[0]).isDir()) return "";

        return QString("%1/bash -c \"%2 -%3 -z -k --stdout '%4' > '%5'\"").arg(TOOLS_PATH).arg(archiverPath()).arg(compressionLevel()).arg(files[0]).arg(fileName());
    }
    return TarArchEngine::create_command(files,with_full_path,password_protected);
}

QString BzipArchEngine::update_command(const QStringList & files,bool,const QString &,bool) const {
    if (files.count() <= 0) return "";

    if (!isTar()) {
        if (files.count() > 1) return "";
        if (QFileInfo(files[0]).isDir()) return "";

        return QString("%1/bash -c \"%2 -z -k --stdout '%3' > '%4'\"").arg(TOOLS_PATH).arg(archiverPath()).arg(files[0]).arg(fileName());
    }
    return "";
}

BaseArchEngine * BzipArchEngine::dupEngine(QObject * parent) {
    return new BzipArchEngine(fileName(),parent,nottar_was_forced());
}

QString BzipArchEngine::archiverPath() const {
    QString pgm = QString("%1/pbzip2").arg(TOOLS_PATH);
    if (isProgramAvailable(pgm)) return pgm;
    return QString("%1/bzip2").arg(TOOLS_PATH);
}

bool BzipArchEngine::areExternalProgramsOK() {
    return (isProgramAvailable(QString("%1/pbzip2").arg(TOOLS_PATH)) ||
            isProgramAvailable(QString("%1/bzip2").arg(TOOLS_PATH))) &&
            isProgramAvailable(QString("%1/bash").arg(TOOLS_PATH)) &&
            isProgramAvailable(QString("%1/tar").arg(TOOLS_PATH));
}

QString BzipArchEngine::delete_command(const QStringList &) const {
    return "";
}

const QStringList BzipArchEngine::suffixes() {
    return QStringList() << "tbz2" << "bz2";
}

const QString BzipArchEngine::openFilter() {
    QStringList filters;
    QStringList suffs = suffixes();
    for (int i=0;i<suffs.count();i++) {
        filters << "*." + suffs[i];
    }
    return tr("Bzip2 files") + " (" + filters.join(" ") + ")";
}

const QString BzipArchEngine::createFilter() {
    QStringList filters;
    filters << "*.tar.bz2";
    QStringList suffs = suffixes();
    for (int i=0;i<suffs.count();i++) {
        filters << "*." + suffs[i];
    }
    return tr("Bzip2 files") + " (" + filters.join(" ") + ")";
}
