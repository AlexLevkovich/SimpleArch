#include "droparchengine.h"
#include "basearchengine.h"
#include "eventloop.h"
#include <QFileInfo>
#include "staticutils.h"


DropArchEngine::DropArchEngine(BaseArchEngine * engine,const QString & archiveDir,const QList<QUrl> & files,QObject * parent) : QObject(parent) {
    base_engine = engine;
    for (int i=0;i<files.count();i++) {
        if (!files[i].isLocalFile() || !QFileInfo(files[i].path()).exists()) continue;
        m_files.append(files[i].path());
    }
    m_archiveDir = archiveDir;
    m_with_full_path = false;
    m_password_protected = false;
}

DropArchEngine::DropArchEngine(BaseArchEngine * engine,const QString & archiveDir,const QStringList & files,bool with_full_path,bool password_protected,QObject * parent) : QObject(parent) {
    base_engine = engine;
    for (int i=0;i<files.count();i++) {
        if (!QFileInfo(files[i]).exists()) continue;
        m_files.append(files[i]);
    }
    m_archiveDir = archiveDir;
    m_with_full_path = with_full_path;
    m_password_protected = password_protected;
}

bool DropArchEngine::exec() {
    EventLoop pause;
    connect(base_engine,SIGNAL(create_password(bool)),this,SLOT(input_create_password(bool)));
    connect(base_engine,SIGNAL(update_ok()),&pause,SLOT(quit()));
    connect(base_engine,SIGNAL(error(const QString &)),&pause,SLOT(cancel()));
    base_engine->updateArchive(m_files,m_with_full_path,m_archiveDir,m_password_protected);
    return (pause.exec() == 0);
}

void DropArchEngine::input_create_password(bool verificate) {
    QString ret = StaticUtils::inputPasswordRequest(verificate);
    if (ret.isEmpty()) base_engine->terminateCommand();
    else base_engine->sendPassword(ret);
}
