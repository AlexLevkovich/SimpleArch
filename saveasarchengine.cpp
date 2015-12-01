#include "saveasarchengine.h"
#include "basearchengine.h"
#include "eventloop.h"
#include <QDirIterator>
#include "staticutils.h"

#define TMP_DIR_CNT_CREAT "Cannot create the temporary directory!!!"
#define CANT_FIND_ENGINE "Cannot find a suitable engine to open %1 file!"

SaveAsArchEngine::SaveAsArchEngine(BaseArchEngine * engine,const QString & output_file,bool password_protected) : QObject(NULL) {
    m_engine = engine;
    m_output_file = output_file;
    m_password_protected = password_protected;
}

QStringList SaveAsArchEngine::getTemporaryContents() const {
    QStringList ret;
    QDirIterator di(temp_dir.path(),QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    while (di.hasNext()) {
        di.next();
        ret.append(di.filePath());
    }
    return ret;
}

void SaveAsArchEngine::password_request(const QString & fileName) {
    QString ret = StaticUtils::inputPasswordRequest(fileName);
    if (ret.isEmpty()) m_engine->terminateCommand();
    else m_engine->sendPassword(ret);
}

bool SaveAsArchEngine::exec() {
    if (!temp_dir.isValid()) {
        QMetaObject::invokeMethod(m_engine,"error",Qt::QueuedConnection,Q_ARG(QString,tr(TMP_DIR_CNT_CREAT)));
        return false;
    }

    EventLoop pause;
    connect(m_engine,SIGNAL(extraction_ok()),&pause,SLOT(quit()));
    connect(m_engine,SIGNAL(error(const QString &)),&pause,SLOT(cancel()));
    connect(m_engine,SIGNAL(password_required(const QString &)),this,SLOT(password_request(const QString &)));
    m_engine->extractFiles(QStringList(),temp_dir.path(),true);
    if (pause.exec()) return false;

    BaseArchEngine * saveas_engine = BaseArchEngine::findEngine(m_output_file,this);
    if (saveas_engine == NULL) {
        QMetaObject::invokeMethod(m_engine,"error",Qt::QueuedConnection,Q_ARG(QString,tr(CANT_FIND_ENGINE)));
        return false;
    }

    connect(saveas_engine,SIGNAL(create_password(bool)),this,SLOT(input_create_password(bool)));
    connect(saveas_engine,SIGNAL(create_ok()),&pause,SLOT(quit()));
    connect(saveas_engine,SIGNAL(error(const QString &)),m_engine,SIGNAL(error(const QString &)));
    connect(saveas_engine,SIGNAL(error(const QString &)),&pause,SLOT(cancel()));
    saveas_engine->createArchive(getTemporaryContents(),false,m_password_protected);
    if (pause.exec()) {
        delete saveas_engine;
        return false;
    }

    delete saveas_engine;
    return true;
}

void SaveAsArchEngine::input_create_password(bool verificate) {
    BaseArchEngine * saveas_engine = (BaseArchEngine *)sender();
    QString ret = StaticUtils::inputPasswordRequest(verificate);
    if (ret.isEmpty()) saveas_engine->terminateCommand();
    else saveas_engine->sendPassword(ret);
}
