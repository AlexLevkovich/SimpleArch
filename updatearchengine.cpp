#include "updatearchengine.h"
#include "basearchengine.h"
#include "eventloop.h"
#include "staticutils.h"

UpdateArchEngine::UpdateArchEngine(BaseArchEngine * engine,const QStringList & files,bool with_full_path,const QString & archiveDir,bool password_protected,QObject *parent) : QObject(parent) {
    m_files = files;
    m_with_full_path = with_full_path;
    m_archiveDir = archiveDir;
    m_password_protected = password_protected;
    base_engine = engine;
}

bool UpdateArchEngine::exec() {
    EventLoop pause;
    connect(base_engine,SIGNAL(update_ok()),&pause,SLOT(quit()));
    connect(base_engine,SIGNAL(error(const QString &)),&pause,SLOT(cancel()));
    connect(base_engine,SIGNAL(create_password(bool)),this,SLOT(input_create_password(bool)));
    base_engine->updateArchive(m_files,m_with_full_path,m_archiveDir,m_password_protected);
    return (pause.exec() == 0);
}

void UpdateArchEngine::input_create_password(bool verificate) {
    QString ret = StaticUtils::inputPasswordRequest(verificate);
    if (ret.isEmpty()) base_engine->terminateCommand();
    else base_engine->sendPassword(ret);
}
