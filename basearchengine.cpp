#include "basearchengine.h"
#include <QCoreApplication>
#include <QFileInfo>
#include <QDir>
#include <QVariant>
#include <QSettings>
#include "tararchengine.h"
#include "archiverprocess.h"
#include "bziparchengine.h"
#include "gziparchengine.h"
#include "xziparchengine.h"
#include "lziparchengine.h"
#include "lzip4archengine.h"
#include "ziparchengine.h"
#include "filetreeitem.h"
#include <QMetaMethod>

#define FILE_DNT_EXIST " file does not exist!!!"
#define INPUT_FILES_ER "Input list is empty or too much files!!!"
#define DELETE_FILES_ER "Deleting is not supported!!!"
#define ANOTHER_COMMAND_STARTED "Another command still is active!!!"

BaseArchEngine * BaseArchEngine::main_arch_engine = NULL;
QString BaseArchEngine::defaultSaveFilter = "";
bool BaseArchEngine::defaultDoTarExtraction = true;
bool BaseArchEngine::defaultUseFullPath = false;
QString BaseArchEngine::defaultOpenDir;
QString BaseArchEngine::defaultSaveDir;
QString BaseArchEngine::defaultSelectDir;
QMap<QString,QString> BaseArchEngine::userMimes;
bool BaseArchEngine::defaultUseSystemMimes = true;
bool BaseArchEngine::defaultDoFileNamesSorting = true;
bool BaseArchEngine::defaultDoShowFolderTree = true;
bool BaseArchEngine::defaultUseSelectedFilesOnly = false;
bool BaseArchEngine::defaultHighlightFocusedView = false;
uint BaseArchEngine::defaultLeftIconSize = 16;
uint BaseArchEngine::defaultRightIconSize = 16;
QSettings * BaseArchEngine::p_settings = NULL;
BaseArchEngine::DateFormat BaseArchEngine::defaultDateFormat = BaseArchEngine::ISO;

QList<QMetaObject> BaseArchEngine::engine_objects;

QSettings * BaseArchEngine::settingsInstance() {
    if (p_settings == NULL) p_settings = new QSettings(QSettings::IniFormat,QSettings::UserScope,QCoreApplication::organizationName(),QCoreApplication::applicationName(),qApp);
    return p_settings;
}

void BaseArchEngine::initDefaults() {
    settingsInstance();

    defaultSaveFilter = p_settings->value("defaultSaveFilter","").toString();
    defaultDoTarExtraction = p_settings->value("defaultDoTarExtraction",true).toBool();
    defaultUseFullPath = p_settings->value("defaultUseFullPath",false).toBool();
    defaultOpenDir = p_settings->value("defaultOpenDir",QDir::homePath()).toString();
    defaultSaveDir = p_settings->value("defaultSaveDir",QDir::homePath()).toString();
    defaultSelectDir = p_settings->value("defaultSelectDir",QDir::homePath()).toString();
    QStringList userMimes_keys = p_settings->value("userMimes_keys").toStringList();
    QStringList userMimes_values = p_settings->value("userMimes_values").toStringList();
    if (userMimes_keys.count() == userMimes_values.count()) {
        for (int i=0;i<userMimes_keys.count();i++) {
            userMimes[userMimes_keys.at(i)] = userMimes_values.at(i);
        }
    }
    defaultUseSystemMimes = p_settings->value("defaultUseSystemMimes",true).toBool();
    defaultDoShowFolderTree = p_settings->value("defaultDoShowFolderTree",true).toBool();
    defaultDoFileNamesSorting = p_settings->value("defaultDoFileNamesSorting",true).toBool();
    defaultUseSelectedFilesOnly = p_settings->value("defaultUseSelectedFilesOnly",false).toBool();
    defaultHighlightFocusedView = p_settings->value("defaultHighlightFocusedView",false).toBool();
    defaultLeftIconSize = p_settings->value("defaultLeftIconSize",16).toUInt();
    defaultRightIconSize = p_settings->value("defaultRightIconSize",16).toUInt();
    defaultDateFormat = (BaseArchEngine::DateFormat)p_settings->value("defaultDateFormat",BaseArchEngine::ISO).toUInt();
}

void BaseArchEngine::saveDefaults() {
    if (p_settings == NULL) return;

    p_settings->setValue("defaultSaveFilter",defaultSaveFilter);
    p_settings->setValue("defaultDoTarExtraction",defaultDoTarExtraction);
    p_settings->setValue("defaultUseFullPath",defaultUseFullPath);
    p_settings->setValue("defaultOpenDir",defaultOpenDir);
    p_settings->setValue("defaultSaveDir",defaultSaveDir);
    p_settings->setValue("defaultSelectDir",defaultSelectDir);
    p_settings->setValue("userMimes_keys",(QStringList)userMimes.keys());
    p_settings->setValue("userMimes_values",(QStringList)userMimes.values());
    p_settings->setValue("defaultUseSystemMimes",defaultUseSystemMimes);
    p_settings->setValue("defaultDoShowFolderTree",defaultDoShowFolderTree);
    p_settings->setValue("defaultDoFileNamesSorting",defaultDoFileNamesSorting);
    p_settings->setValue("defaultHighlightFocusedView",defaultHighlightFocusedView);
    p_settings->setValue("defaultLeftIconSize",defaultLeftIconSize);
    p_settings->setValue("defaultRightIconSize",defaultRightIconSize);
    p_settings->setValue("defaultUseSelectedFilesOnly",defaultUseSelectedFilesOnly);
    p_settings->setValue("defaultDateFormat",(uint)defaultDateFormat);

    p_settings->sync();
}

BaseArchEngine::BaseArchEngine(const QString & fileName,QObject * parent) : QObject(parent) {
    m_fileName = fileName;
    current_process = NULL;
}

BaseArchEngine::~BaseArchEngine() {
    if (main_arch_engine == this) main_arch_engine = NULL;
    if (current_process != NULL) {
        current_process->kill();
        current_process->waitForFinished();
    }
}

bool BaseArchEngine::supportsEncripting() const {
    return false;
}

QString BaseArchEngine::fileName() const {
    return m_fileName;
}

QString BaseArchEngine::lastCommand() const {
    return m_lastCommand;
}

bool BaseArchEngine::another_command_active_error() {
    if (current_process != NULL) {
        post_error_signal(fileName() + ": " + tr(ANOTHER_COMMAND_STARTED));
        return true;
    }
    return false;
}

int BaseArchEngine::compressionLevel() const {
    return readOnly()?-1:settingsInstance()->value(QString("defaultCompressionLevel_")+metaObject()->className(),defaultCompressionLevel()).toInt();
}

int BaseArchEngine::compressionLevel(const QMetaObject & metaObject,QString & engineName,QList<int> & allLevels) {
    QObject * obj = metaObject.newInstance(Q_ARG(QString,""),Q_ARG(QObject *,NULL));
    if (obj == NULL) return -1;

    int level = -1;
    if (!QMetaObject::invokeMethod(obj,"compressionLevel",Q_RETURN_ARG(int,level))) {
        delete obj;
        return -1;
    }
    if (!QMetaObject::invokeMethod(obj,"engineName",Q_RETURN_ARG(QString,engineName))) {
        delete obj;
        return -1;
    }
    if (!QMetaObject::invokeMethod(obj,"compressionLevels",Q_RETURN_ARG(QList<int>,allLevels))) {
        delete obj;
        return -1;
    }

    delete obj;
    return level;
}

const QMap<QString,QPair<QList<int>,int> > BaseArchEngine::enginesCompressionLevels() {
    QMap<QString,QPair<QList<int>,int> > ret;
    QString engineName;
    QList<int> allLevels;
    for (int i=0;i<engine_objects.count();i++) {
        int level = compressionLevel(engine_objects.at(i),engineName,allLevels);
        if (level < 0 || (allLevels.count() <= 0)) continue;
        ret[engineName] = QPair<QList<int>,int>(allLevels,level);
    }

    return ret;
}

bool BaseArchEngine::setEnginesCompressionLevels(const QMap<QString,int> & levels) {
    QString engineName;
    bool ok;
    QMapIterator<QString, int> map_i(levels);
    while (map_i.hasNext()) {
        map_i.next();
        for (int i=0;i<engine_objects.count();i++) {
            QMetaObject metaObject = engine_objects.at(i);
            QObject * obj = metaObject.newInstance(Q_ARG(QString,""),Q_ARG(QObject *,NULL));
            if (obj == NULL) return false;

            if (!QMetaObject::invokeMethod(obj,"engineName",Q_RETURN_ARG(QString,engineName))) {
                delete obj;
                return false;
            }

            if (engineName == map_i.key()) {
                if (!QMetaObject::invokeMethod(obj,"setCompressionLevel",Q_RETURN_ARG(bool,ok),Q_ARG(int,map_i.value()))) {
                    delete obj;
                    return false;
                }
                if (!ok) {
                    delete obj;
                    return false;
                }

                delete obj;
                break;
            }
            delete obj;
        }
    }

    return true;
}

bool BaseArchEngine::setCompressionLevel(int level,const QMetaObject & metaObject) {
    QObject * obj = metaObject.newInstance(Q_ARG(QString,""),Q_ARG(QObject *,NULL));
    if (obj == NULL) return false;

    bool ok = false;
    QMetaObject::invokeMethod(obj,"setCompressionLevel",Q_RETURN_ARG(bool,ok),Q_ARG(int,level));

    delete obj;
    return ok;
}

bool BaseArchEngine::setCompressionLevel(int level) {
    if (readOnly()) return false;
    if (!compressionLevels().contains(level)) return false;
    settingsInstance()->setValue(QString("defaultCompressionLevel_")+metaObject()->className(),level);
    return true;
}

bool BaseArchEngine::startListContents() {
    if (another_command_active_error()) return false;
    if (!QFileInfo(fileName()).exists()) {
        post_error_signal(fileName() + tr(FILE_DNT_EXIST));
        return false;
    }

    m_lastCommand = "startListContents";
    return startListContents_body();
}

void BaseArchEngine::post_error_signal(const QString & line) {
    QMetaObject::invokeMethod(this,"error",Qt::QueuedConnection,Q_ARG(QString,line));
}

bool BaseArchEngine::startListContents_body() {
    current_process = new ArchiverProcess(this);

    connect(current_process,SIGNAL(finished()),this,SLOT(any_process_finished()));
    connect(current_process,SIGNAL(error(const QString &)),this,SLOT(any_process_finished()));
    connect(current_process,SIGNAL(finished()),this,SIGNAL(list_contents_ok()));
    connect(current_process,SIGNAL(error(const QString &)),this,SIGNAL(error(const QString &)));
    connect(current_process,SIGNAL(readyStandardOutputLine(const QString &)),this,SLOT(list_contents_split_record(const QString &)));

    current_process->start(list_contents_command(),QIODevice::ReadOnly);

    return true;
}

void BaseArchEngine::any_process_finished() {
    current_process = NULL;
}

void BaseArchEngine::list_contents_split_record(const QString & rec) {
    FileTreeItem * item = NULL;
    bool ignore = false;
    if (((item = split_list_contents_record(rec,ignore)) == NULL) && !ignore) {
        current_process->kill();
        emit error(tr("Cannot split the input record! Incorrect format!")+"\n"+rec);
        return;
    }
    if (!ignore) emit list_contents_record(item);
}

void BaseArchEngine::terminateCommand() {
    if (current_process != NULL) current_process->kill();
}

bool BaseArchEngine::extractFiles(const QStringList & files,const QString & dir_path,bool with_full_path) {
    if (another_command_active_error()) return false;
    if (!QFileInfo(fileName()).exists()) {
        post_error_signal(fileName() + ": " + tr(FILE_DNT_EXIST));
        return false;
    }

    m_lastCommand = "extractFiles";
    return extractFiles_body(files,dir_path,with_full_path);
}

BaseArchEngine::ChannelFlag BaseArchEngine::parsePasswordChannel() const {
    return BaseArchEngine::StderrChannel;
}

bool BaseArchEngine::extractFiles_body(const QStringList & files,const QString & dir_path,bool with_full_path) {
    current_process = new ArchiverProcess(this);

    connect(current_process,SIGNAL(finished()),this,SLOT(any_process_finished()));
    connect(current_process,SIGNAL(error(const QString &)),this,SLOT(any_process_finished()));
    if (parsePasswordChannel() == BaseArchEngine::StderrChannel) {
        connect(current_process,SIGNAL(readyStandardErrorLine(const QString &)),this,SLOT(extraction_readyStandardLine(const QString &)));
        connect(current_process,SIGNAL(peekStandardErrorTail(const QString &)),this,SLOT(extraction_readyStandardTail(const QString &)));
    }
    else {
        connect(current_process,SIGNAL(readyStandardOutputLine(const QString &)),this,SLOT(extraction_readyStandardLine(const QString &)));
        connect(current_process,SIGNAL(peekStandardOutputTail(const QString &)),this,SLOT(extraction_readyStandardTail(const QString &)));
    }
    connect(current_process,SIGNAL(finished()),this,SIGNAL(extraction_ok()));
    connect(current_process,SIGNAL(error(const QString &)),this,SIGNAL(error(const QString &)));

    current_process->start(uncompress_command(files,dir_path,with_full_path),QProcess::ReadWrite);

    return true;
}

void BaseArchEngine::sendPassword(const QString & password) {
    if (current_process != NULL) {
        if (current_process->write(password.toLocal8Bit()+'\n') != -1) {
            current_process->waitForBytesWritten(-1);
        }
    }
}

void BaseArchEngine::extraction_readyStandardLine(const QString & line) {
    QString fileName;
    if (parsePasswordRequiring(line,fileName)) emit password_required(fileName);
}

void BaseArchEngine::extraction_readyStandardTail(const QString & line) {
    QString fileName;
    if (parsePasswordRequiring(line,fileName)) {
        if (parsePasswordChannel() == BaseArchEngine::StderrChannel) current_process->clearStderrTail();
        else current_process->clearStdoutTail();
        emit password_required(fileName);
    }
}

bool BaseArchEngine::isProgramAvailable(const QString & program_path) {
    return QFile::exists(program_path);
}

BaseArchEngine * BaseArchEngine::findEngine(const QString & fileName,QObject * parent) {
    QString suffix = QFileInfo(fileName).suffix().toLower();

    for (int i=0;i<engine_objects.count();i++) {
        QMetaObject meta_obj = engine_objects.at(i);
        QObject * obj = meta_obj.newInstance(Q_ARG(QString,fileName),Q_ARG(QObject *,parent));
        if (obj == NULL) continue;

        QStringList suffixes;
        if (QMetaObject::invokeMethod(obj,"suffixes",Q_RETURN_ARG(QStringList,suffixes))) {
            bool ok;
            if (QMetaObject::invokeMethod(obj,"areExternalProgramsOK",Q_RETURN_ARG(bool,ok))) {
                if (suffixes.contains(suffix) && ok) return (BaseArchEngine *)obj;
            }

        }
        delete obj;
    }

    return NULL;
}

const QStringList BaseArchEngine::findOpenFilters() {
    QStringList filters;

    for (int i=0;i<engine_objects.count();i++) {
        QMetaObject meta_obj = engine_objects.at(i);
        QObject * obj = meta_obj.newInstance(Q_ARG(QString,""),Q_ARG(QObject *,NULL));
        if (obj == NULL) continue;

        bool ok;
        if (QMetaObject::invokeMethod(obj,"areExternalProgramsOK",Q_RETURN_ARG(bool,ok))) {
            if (ok) {
                QString filter;
                if (QMetaObject::invokeMethod(obj,"openFilter",Q_RETURN_ARG(QString,filter))) {
                    filters << filter;
                }
            }
        }
        delete obj;
    }

    return filters;
}

const QStringList BaseArchEngine::findCreateFilters() {
    QStringList filters;

    for (int i=0;i<engine_objects.count();i++) {
        QMetaObject meta_obj = engine_objects.at(i);
        QObject * obj = meta_obj.newInstance(Q_ARG(QString,""),Q_ARG(QObject *,NULL));
        if (obj == NULL) continue;

        bool ok;
        if (QMetaObject::invokeMethod(obj,"areExternalProgramsOK",Q_RETURN_ARG(bool,ok))) {
            if (ok) {
                QString filter;
                if (QMetaObject::invokeMethod(obj,"createFilter",Q_RETURN_ARG(QString,filter))) {
                    filters << filter;
                }
            }
        }
        delete obj;
    }

    return filters;
}

bool BaseArchEngine::createArchive(const QStringList & files,bool with_full_path,bool password_protected) {
    if (another_command_active_error()) return false;
    if (readOnly()) return false;

    m_lastCommand = "createArchive";
    return createArchive_body(files,with_full_path,password_protected);
}

bool BaseArchEngine::createArchive_body(const QStringList & files,bool with_full_path,bool password_protected) {
    current_process = new ArchiverProcess(this);

    if (parsePasswordChannel() == BaseArchEngine::StderrChannel) {
        connect(current_process,SIGNAL(readyStandardErrorLine(const QString &)),this,SLOT(update_readyStandardLine(const QString &)));
        connect(current_process,SIGNAL(peekStandardErrorTail(const QString &)),this,SLOT(update_readyStandardTail(const QString &)));
    }
    else {
        connect(current_process,SIGNAL(readyStandardOutputLine(const QString &)),this,SLOT(update_readyStandardLine(const QString &)));
        connect(current_process,SIGNAL(peekStandardOutputTail(const QString &)),this,SLOT(update_readyStandardTail(const QString &)));
    }
    connect(current_process,SIGNAL(finished()),this,SLOT(any_process_finished()));
    connect(current_process,SIGNAL(error(const QString &)),this,SLOT(any_process_finished()));
    connect(current_process,SIGNAL(finished()),this,SIGNAL(create_ok()));
    connect(current_process,SIGNAL(error(const QString &)),this,SIGNAL(error(const QString &)));

    QString command = create_command(files,with_full_path,password_protected);
    if (command.isEmpty()) {
        post_error_signal(tr(INPUT_FILES_ER));
        return false;
    }
    current_process->start(command,QIODevice::ReadWrite);

    return true;
}

bool BaseArchEngine::updateArchive(const QStringList & files,bool with_full_path,const QString & archiveDir,bool password_protected) {
    if (another_command_active_error()) return false;
    if (readOnly()) return false;
    if (!QFileInfo(fileName()).exists()) {
        post_error_signal(fileName() + tr(FILE_DNT_EXIST));
        return false;
    }

    m_lastCommand = "updateArchive";
    return updateArchive_body(files,with_full_path,archiveDir,password_protected);
}

void BaseArchEngine::update_readyStandardLine(const QString & line) {
    bool verificate;
    if (parseUpdatePassword(line,verificate)) emit create_password(verificate);
}

void BaseArchEngine::update_readyStandardTail(const QString & line) {
    bool verificate;
    if (parseUpdatePassword(line,verificate)) {
        if (parsePasswordChannel() == BaseArchEngine::StderrChannel) current_process->clearStderrTail();
        else current_process->clearStdoutTail();
        emit create_password(verificate);
    }
}

bool BaseArchEngine::updateArchive_body(const QStringList & files,bool with_full_path,const QString & archiveDir,bool password_protected) {
    current_process = new ArchiverProcess(this);

    if (parsePasswordChannel() == BaseArchEngine::StderrChannel) {
        connect(current_process,SIGNAL(readyStandardErrorLine(const QString &)),this,SLOT(update_readyStandardLine(const QString &)));
        connect(current_process,SIGNAL(peekStandardErrorTail(const QString &)),this,SLOT(update_readyStandardTail(const QString &)));
    }
    else {
        connect(current_process,SIGNAL(readyStandardOutputLine(const QString &)),this,SLOT(update_readyStandardLine(const QString &)));
        connect(current_process,SIGNAL(peekStandardOutputTail(const QString &)),this,SLOT(update_readyStandardTail(const QString &)));
    }
    connect(current_process,SIGNAL(finished()),this,SLOT(any_process_finished()));
    connect(current_process,SIGNAL(error(const QString &)),this,SLOT(any_process_finished()));
    connect(current_process,SIGNAL(finished()),this,SIGNAL(update_ok()));
    connect(current_process,SIGNAL(error(const QString &)),this,SIGNAL(error(const QString &)));

    QString command = update_command(files,with_full_path,archiveDir,password_protected);
    if (command.isEmpty()) {
        post_error_signal(tr(INPUT_FILES_ER));
        return false;
    }
    current_process->start(command,QIODevice::ReadWrite);

    return true;
}

bool BaseArchEngine::deleteFiles(const QStringList & files) {
    if (another_command_active_error()) return false;
    if (readOnly()) return false;
    if (!QFileInfo(fileName()).exists()) {
        post_error_signal(fileName() + tr(FILE_DNT_EXIST));
        return false;
    }

    m_lastCommand = "deleteFiles";
    return deleteFiles_body(files);
}

bool BaseArchEngine::deleteFiles_body(const QStringList & files) {
    current_process = new ArchiverProcess(this);

    connect(current_process,SIGNAL(finished()),this,SLOT(any_process_finished()));
    connect(current_process,SIGNAL(error(const QString &)),this,SLOT(any_process_finished()));
    connect(current_process,SIGNAL(finished()),this,SIGNAL(delete_ok()));
    connect(current_process,SIGNAL(error(const QString &)),this,SIGNAL(error(const QString &)));

    QString command = delete_command(files);
    if (command.isEmpty()) {
        post_error_signal(tr(DELETE_FILES_ER));
        return false;
    }
    current_process->start(command,QIODevice::ReadOnly);

    return true;
}

void BaseArchEngine::addProcessErrorLine(const QString & error) {
    if (current_process != NULL) current_process->addErrorLine(error);
}
