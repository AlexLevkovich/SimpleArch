#include "extractarchengine.h"
#include "basearchengine.h"
#include "eventloop.h"
#include "staticutils.h"

ExtractArchEngine::ExtractArchEngine(BaseArchEngine * engine,const QStringList & files,const QString & output_dir,bool use_full_path,QObject * parent) : QObject(parent) {
    base_engine = engine;
    m_files = files;
    m_use_full_path = use_full_path;
    m_output_dir = output_dir;
}

ExtractArchEngine::ExtractArchEngine(BaseArchEngine * engine,const QStringList & files,bool use_full_path,QObject * parent) : QObject(parent) {
    base_engine = engine;
    m_files = files;
    m_use_full_path = use_full_path;
}

bool ExtractArchEngine::exec() {
    EventLoop pause;
    connect(base_engine,SIGNAL(extraction_ok()),&pause,SLOT(quit()));
    connect(base_engine,SIGNAL(error(const QString &)),&pause,SLOT(cancel()));
    connect(base_engine,SIGNAL(password_required(const QString &)),this,SLOT(password_request(const QString &)));
    base_engine->extractFiles(m_files,m_output_dir,m_use_full_path);
    if (pause.exec() > 0) {
        return false;
    }

    return true;
}

void ExtractArchEngine::setOutputDir(const QString & dir) {
    m_output_dir = dir;
}

void ExtractArchEngine::password_request(const QString & fileName) {
    QString ret = StaticUtils::inputPasswordRequest(fileName);
    if (ret.isEmpty()) base_engine->terminateCommand();
    else base_engine->sendPassword(ret);
}
