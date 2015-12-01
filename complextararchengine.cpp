#include "complextararchengine.h"
#include <QDateTime>
#include <QVariant>
#include "updatetararchengine.h"
#include "deletetararchengine.h"
#include "filetreeitem.h"

ComplexTarArchEngine::ComplexTarArchEngine(const QString & fileName,QObject *parent,bool force_tobe_nottar) : TarArchEngine(fileName,parent) {
    m_force_tobe_nottar = force_tobe_nottar;
}

QString ComplexTarArchEngine::processingSymbols() const {
    if (isTar() && (lastCommand() == "createArchive")) return QString("\"--use-compress-program=%1 -%2\"").arg(archiverPath()).arg(compressionLevel());
    return QString("\"--use-compress-program=%1\"").arg(archiverPath());
}

const QString ComplexTarArchEngine::firstSuffix(const QString & fileName) {
    QStringList parts = QFileInfo(fileName).completeSuffix().toLower().split(".");
    if (parts.count() > 1) {
        return parts[parts.count()-2];
    }

    return "";
}

const QString ComplexTarArchEngine::fullSuffix(const QString & fileName) {
    QStringList parts = QFileInfo(fileName).completeSuffix().toLower().split(".");
    if (parts.count() > 1) {
        return parts[parts.count()-2]+"."+parts[parts.count()-1];
    }

    return parts[0];
}

const QString ComplexTarArchEngine::fullSuffix(const QFileInfo & info) {
    QStringList parts = info.completeSuffix().toLower().split(".");
    if (parts.count() > 1) {
        return parts[parts.count()-2]+"."+parts[parts.count()-1];
    }

    return parts[0];
}

bool ComplexTarArchEngine::isRealTar(const QString & fileName) {
    QString suffix = QFileInfo(fileName).suffix().toLower();
    QString f_suffix = firstSuffix(fileName);
    return !((suffix!= "tar") && (f_suffix.isEmpty() || (f_suffix != "tar"))) ||
           (suffix == "tbz2") ||
           (suffix == "tgz") ||
           (suffix == "tlz") ||
           (suffix == "tlz4") ||
           (suffix == "txz");
}

bool ComplexTarArchEngine::isTar() const {
    if (m_force_tobe_nottar) return false;

    return isRealTar(fileName());
}

bool ComplexTarArchEngine::updateArchive_body(const QStringList & files,bool with_full_path,const QString & archiveDir,bool password_protected) {
    if (!isTar()) {
        return BaseArchEngine::updateArchive_body(files,with_full_path,archiveDir,password_protected);
    }

    UpdateTarArchEngine * updateEngine = new UpdateTarArchEngine(this,files,with_full_path,archiveDir);
    connect(updateEngine,SIGNAL(update_ok()),this,SIGNAL(update_ok()));
    connect(updateEngine,SIGNAL(error(const QString &)),this,SIGNAL(error(const QString &)));
    connect(updateEngine,SIGNAL(update_ok()),updateEngine,SLOT(deleteLater()));
    connect(updateEngine,SIGNAL(error(const QString &)),updateEngine,SLOT(deleteLater()));

    return true;
}

bool ComplexTarArchEngine::deleteFiles_body(const QStringList & files) {
    if (!isTar()) {
        return BaseArchEngine::deleteFiles_body(files);
    }

    DeleteTarArchEngine * deleteEngine = new DeleteTarArchEngine(this,files);
    connect(deleteEngine,SIGNAL(delete_ok()),this,SIGNAL(delete_ok()));
    connect(deleteEngine,SIGNAL(error(const QString &)),this,SIGNAL(error(const QString &)));
    connect(deleteEngine,SIGNAL(delete_ok()),deleteEngine,SLOT(deleteLater()));
    connect(deleteEngine,SIGNAL(error(const QString &)),deleteEngine,SLOT(deleteLater()));

    return true;
}

QString ComplexTarArchEngine::list_contents_command() const {
    QFileInfo info(fileName());
    QString f_suffix = firstSuffix(fileName());
    if (!isTar()) {
        return QString("%1/echo %2 %3/%4 %5 %6 %7").arg(TOOLS_PATH).arg(permissions()).arg(info.owner()).arg(info.group()).arg(info.size()).arg(info.lastModified().toString("yyyy-MM-dd hh:mm")).arg(info.baseName()+(f_suffix.isEmpty()?"":"."+f_suffix));
    }
    return TarArchEngine::list_contents_command();
}

QString ComplexTarArchEngine::permissions() const {
    return FileTreeItem::permissions(QFileInfo(fileName()));
}

QString ComplexTarArchEngine::tarName() const {
    QFileInfo info(fileName());
    QString file_name = info.fileName();
    file_name.chop(info.suffix().toLower().length()+1);
    if (!isRealTar(fileName())) return file_name;
    if (QFileInfo(file_name).suffix().toLower() != "tar") return file_name + ".tar";

    return file_name;
}

bool ComplexTarArchEngine::nottar_was_forced() const {
    return m_force_tobe_nottar;
}

void ComplexTarArchEngine::setNotTarForceFlag(bool flag) {
    m_force_tobe_nottar = flag;
}

BaseArchEngine * ComplexTarArchEngine::dupEngine(QObject *) {
    return NULL;
}

bool ComplexTarArchEngine::isMultiFiles() const {
    return isTar();
}

QList<int> ComplexTarArchEngine::compressionLevels() const {
    QList<int> ret;
    return (ret << 1 << 2 << 3 << 4 << 5 << 6 << 7 << 8 << 9);
}

int ComplexTarArchEngine::defaultCompressionLevel() const {
    return 6;
}
