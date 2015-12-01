#ifndef RemoveArchEngine_H
#define RemoveArchEngine_H

#include <QObject>
#include <QStringList>

class BaseArchEngine;

class RemoveArchEngine : public QObject {
    Q_OBJECT
public:
    explicit RemoveArchEngine(BaseArchEngine * engine,const QStringList & files,QObject * parent = NULL);
    bool exec();

private:
    BaseArchEngine * base_engine;
    QStringList m_files;
};

#endif // RemoveArchEngine_H
