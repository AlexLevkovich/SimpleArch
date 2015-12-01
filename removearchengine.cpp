#include "removearchengine.h"
#include "basearchengine.h"
#include "eventloop.h"


RemoveArchEngine::RemoveArchEngine(BaseArchEngine * engine,const QStringList & files,QObject * parent) : QObject(parent) {
    base_engine = engine;
    m_files = files;
}

bool RemoveArchEngine::exec() {
    EventLoop pause;
    connect(base_engine,SIGNAL(delete_ok()),&pause,SLOT(quit()));
    connect(base_engine,SIGNAL(error(const QString &)),&pause,SLOT(cancel()));
    base_engine->deleteFiles(m_files);
    return (pause.exec() == 0);
}
