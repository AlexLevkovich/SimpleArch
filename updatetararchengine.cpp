#include "updatetararchengine.h"
#include "complextararchengine.h"
#include <QDir>

#define TMP_DIR_CNT_CREAT "Cannot create the temporary directory!!!"

UpdateTarArchEngine::UpdateTarArchEngine(ComplexTarArchEngine * engine,const QStringList & files,bool with_full_path,const QString & archiveDir) : QObject(NULL) {
    tar_engine = NULL;
    dup_engine = NULL;
    m_files = files;
    m_with_full_path = with_full_path;
    m_archiveDir = archiveDir;

    if (!temp_dir.isValid()) {
        QMetaObject::invokeMethod(this,"error",Qt::QueuedConnection,Q_ARG(QString,tr(TMP_DIR_CNT_CREAT)));
        return;
    }

    dup_engine = (ComplexTarArchEngine *)engine->dupEngine(this);
    dup_engine->setNotTarForceFlag(true);
    connect(dup_engine,SIGNAL(extraction_ok()),this,SLOT(update_extraction_ok()));
    connect(dup_engine,SIGNAL(error(const QString &)),this,SIGNAL(error(const QString &)));
    QMetaObject::invokeMethod(this,"start",Qt::QueuedConnection);
}

UpdateTarArchEngine::~UpdateTarArchEngine() {
    if (tar_engine != NULL) delete tar_engine;
    if (dup_engine != NULL) delete dup_engine;
}

void UpdateTarArchEngine::start() {
    dup_engine->extractFiles(QStringList(),temp_dir.path(),true);
}

void UpdateTarArchEngine::update_extraction_ok() {
    tar_engine = new TarArchEngine(temp_dir.path()+"/"+dup_engine->tarName(),this);
    connect(tar_engine,SIGNAL(update_ok()),this,SLOT(tar_update_ok()));
    connect(tar_engine,SIGNAL(error(const QString &)),this,SIGNAL(error(const QString &)));
    tar_engine->updateArchive(m_files,m_with_full_path,m_archiveDir);
}

void UpdateTarArchEngine::tar_update_ok() {
    QStringList files;
    files << tar_engine->fileName();
    connect(dup_engine,SIGNAL(update_ok()),this,SIGNAL(update_ok()));
    dup_engine->updateArchive(files,true,m_archiveDir);
}
