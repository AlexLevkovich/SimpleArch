#include "lzip4archengine.h"
#include <QFileInfo>

DECLARE_ENGINE(Lzip4ArchEngine)

Lzip4ArchEngine::Lzip4ArchEngine(const QString & fileName,QObject *parent,bool force_tobe_nottar) : ComplexTarArchEngine(fileName,parent,force_tobe_nottar) {
}

QString Lzip4ArchEngine::uncompress_command(const QStringList & files,const QString & dir_path,bool with_full_path) const {
    if (!isTar()) {
        return QString("%1/bash -c \"%2 -d --stdout -k '%3' > '%4/%5'\"").arg(TOOLS_PATH).arg(archiverPath()).arg(fileName()).arg(dir_path.isEmpty()?"/dev":dir_path).arg(dir_path.isEmpty()?"null":tarName());
    }
    return TarArchEngine::uncompress_command(files,dir_path,with_full_path);
}

QString Lzip4ArchEngine::create_command(const QStringList & files,bool with_full_path,bool password_protected) const {
    if (files.count() <= 0) return "";

    if (!isTar()) {
        if (files.count() > 1) return "";
        if (QFileInfo(files[0]).isDir()) return "";

        return QString("%1/bash -c \"%2 -%3 -z -k --stdout '%4' > '%5'\"").arg(TOOLS_PATH).arg(archiverPath()).arg(compressionLevel()).arg(files[0]).arg(fileName());
    }
    return TarArchEngine::create_command(files,with_full_path,password_protected);
}

QString Lzip4ArchEngine::update_command(const QStringList & files,bool,const QString &,bool) const {
    if (files.count() <= 0) return "";

    if (!isTar()) {
        if (files.count() > 1) return "";
        if (QFileInfo(files[0]).isDir()) return "";

        return QString("%1/bash -c \"%2 -z -k --stdout '%3' > '%4'\"").arg(TOOLS_PATH).arg(archiverPath()).arg(files[0]).arg(fileName());
    }
    return "";
}

BaseArchEngine * Lzip4ArchEngine::dupEngine(QObject * parent) {
    return new Lzip4ArchEngine(fileName(),parent,nottar_was_forced());
}

QString Lzip4ArchEngine::archiverPath() const {
    return QString("%1/lz4").arg(TOOLS_PATH);
}

bool Lzip4ArchEngine::areExternalProgramsOK() {
    return  isProgramAvailable(QString("%1/lz4").arg(TOOLS_PATH)) &&
            isProgramAvailable(QString("%1/bash").arg(TOOLS_PATH)) &&
            isProgramAvailable(QString("%1/tar").arg(TOOLS_PATH));
}

QString Lzip4ArchEngine::delete_command(const QStringList &) const {
    return "";
}

const QStringList Lzip4ArchEngine::suffixes() {
    return QStringList() << "tlz4" << "lz4" << "lzip4";
}

const QString Lzip4ArchEngine::openFilter() {
    QStringList filters;
    QStringList suffs = suffixes();
    for (int i=0;i<suffs.count();i++) {
        filters << "*." + suffs[i];
    }
    return tr("Lzip4 files") + " (" + filters.join(" ") + ")";
}

const QString Lzip4ArchEngine::createFilter() {
    QStringList filters;
    filters << "*.tar.lz4";
    filters << "*.tar.lzip4";
    QStringList suffs = suffixes();
    for (int i=0;i<suffs.count();i++) {
        filters << "*." + suffs[i];
    }
    return tr("Lzip4 files") + " (" + filters.join(" ") + ")";
}

QList<int> Lzip4ArchEngine::compressionLevels() const {
    QList<int> ret;
    return (ret << 1 << 9);
}

int Lzip4ArchEngine::defaultCompressionLevel() const {
    return 1;
}
