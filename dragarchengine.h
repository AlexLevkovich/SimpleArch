#ifndef DragArchEngine_H
#define DragArchEngine_H

#include <QObject>
#if QT_VERSION < 0x050000
#include "qtemporarydir.h"
#else
#include <QTemporaryDir>
#endif

class BaseArchEngine;
class QMimeData;

class DragArchEngine : public QObject {
    Q_OBJECT
public:
    explicit DragArchEngine(BaseArchEngine * engine,const QStringList & files,QObject * parent = NULL);
    QMimeData * exec();
    QMimeData * mimeData();
    QStringList files() const;

private slots:
    void password_request(const QString & fileName);

private:
    QTemporaryDir temp_dir;
    BaseArchEngine * base_engine;
    QStringList m_files;
};

#endif // DragArchEngine_H
