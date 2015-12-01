#ifndef EXTRACTTOTEMPARCHENGINE_H
#define EXTRACTTOTEMPARCHENGINE_H

#include "extractarchengine.h"

#if QT_VERSION < 0x050000
#include "qtemporarydir.h"
#else
#include <QTemporaryDir>
#endif

#include <QList>
#include <QUrl>

class ExtractToTempArchEngine : public ExtractArchEngine {
public:
    ExtractToTempArchEngine(BaseArchEngine * engine,const QStringList & files,QObject * parent);
    QList<QUrl> outputPaths() const;

private:
    static QTemporaryDir temp_dir;
};

#endif // EXTRACTTOTEMPARCHENGINE_H
