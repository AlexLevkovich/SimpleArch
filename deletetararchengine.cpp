#include "deletetararchengine.h"
#include "complextararchengine.h"
#include <QDir>

#define TMP_DIR_CNT_CREAT "Cannot create the temporary directory!!!"

DeleteTarArchEngine::DeleteTarArchEngine(ComplexTarArchEngine * engine,const QStringList & files) : QObject(NULL) {
    tar_engine = NULL;
    dup_engine = NULL;
    m_files = files;

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

DeleteTarArchEngine::~DeleteTarArchEngine() {
    if (tar_engine != NULL) delete tar_engine;
    if (dup_engine != NULL) delete dup_engine;
}

void DeleteTarArchEngine::start() {
    dup_engine->extractFiles(QStringList(),temp_dir.path(),true);
}

void DeleteTarArchEngine::update_extraction_ok() {
    tar_engine = new TarArchEngine(temp_dir.path()+"/"+dup_engine->tarName(),this);
    connect(tar_engine,SIGNAL(delete_ok()),this,SLOT(tar_delete_ok()));
    connect(tar_engine,SIGNAL(error(const QString &)),this,SIGNAL(error(const QString &)));
    tar_engine->deleteFiles(m_files);
}

void DeleteTarArchEngine::tar_delete_ok() {
    QStringList files;
    files << tar_engine->fileName();
    connect(dup_engine,SIGNAL(update_ok()),this,SIGNAL(delete_ok()));
    dup_engine->updateArchive(files,true,"");
}

