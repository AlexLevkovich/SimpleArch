#include "extracttotemparchengine.h"
#include <QFile>

QTemporaryDir ExtractToTempArchEngine::temp_dir;

#define TMP_DIR_CNT_CREAT "Cannot create the temporary directory!!!"

ExtractToTempArchEngine::ExtractToTempArchEngine(BaseArchEngine * engine,const QStringList & files,QObject * parent) : ExtractArchEngine(engine,files,true,parent) {
    setOutputDir(temp_dir.path());

    for (int i=0;i<m_files.count();i++) {
        QFile::remove(temp_dir.path()+"/"+m_files[i]);
    }
}

QList<QUrl> ExtractToTempArchEngine::outputPaths() const {
    QList<QUrl> ret;

    for (int i=0;i<m_files.count();i++) {
        ret.append(QUrl::fromLocalFile(temp_dir.path()+"/"+m_files[i]));
    }

    return ret;
}
