#include "dragarchengine.h"
#include "basearchengine.h"
#include <QDir>
#include <QUrl>
#include <QList>
#include <QMimeData>
#include <QDirIterator>
#include "eventloop.h"
#include "staticutils.h"

#define TMP_DIR_CNT_CREAT "Cannot create the temporary directory!!!"

DragArchEngine::DragArchEngine(BaseArchEngine * engine,const QStringList & files,QObject * parent) : QObject(parent) {
    base_engine = engine;
    m_files = files;

    if (!temp_dir.isValid()) {
        QMetaObject::invokeMethod(engine,"error",Qt::QueuedConnection,Q_ARG(QString,tr(TMP_DIR_CNT_CREAT)));
        return;
    }
}

QMimeData * DragArchEngine::exec() {
    EventLoop pause;
    connect(base_engine,SIGNAL(extraction_ok()),&pause,SLOT(quit()));
    connect(base_engine,SIGNAL(error(const QString &)),&pause,SLOT(cancel()));
    connect(base_engine,SIGNAL(password_required(const QString &)),this,SLOT(password_request(const QString &)));
    base_engine->extractFiles(m_files,temp_dir.path(),false);
    if (pause.exec() > 0) return NULL;

    return mimeData();
}

void DragArchEngine::password_request(const QString & fileName) {
    QString ret = StaticUtils::inputPasswordRequest(fileName);
    if (ret.isEmpty()) base_engine->terminateCommand();
    else base_engine->sendPassword(ret);
}

QStringList DragArchEngine::files() const {
    return m_files;
}

QMimeData * DragArchEngine::mimeData() {
    QList<QUrl> urls;
    QDirIterator di(temp_dir.path(), QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    while (di.hasNext()) {
        di.next();
        urls.append(QUrl::fromLocalFile(di.filePath()));
    }
    if (urls.count() <= 0) return NULL;

    QMimeData * mime_data = new QMimeData();
    mime_data->setUrls(urls);
    return mime_data;
}
