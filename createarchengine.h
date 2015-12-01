#ifndef CreateArchEngine_H
#define CreateArchEngine_H

#include <QObject>
#include <QStringList>

class BaseArchEngine;

class CreateArchEngine : public QObject {
    Q_OBJECT
public:
    CreateArchEngine(BaseArchEngine * engine,const QStringList & files,bool with_full_path,bool password_protected,QObject * parent = NULL);
    bool exec();

private slots:
    void input_create_password(bool verificate);

private:
    BaseArchEngine * base_engine;
    QStringList m_files;
    bool m_with_full_path;
    bool m_password_protected;
};

#endif // CreateArchEngine_H
