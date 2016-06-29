#ifndef UPDATEARCHENGINE_H
#define UPDATEARCHENGINE_H

#include <QObject>

class BaseArchEngine;

class UpdateArchEngine : public QObject {
    Q_OBJECT
public:
    UpdateArchEngine(BaseArchEngine * engine,const QStringList & files,bool with_full_path,const QString & archiveDir = QString(),bool password_protected = false,QObject *parent = 0);
    bool exec();

private slots:
    void input_create_password(bool verificate);

private:
    BaseArchEngine * base_engine;
    QStringList m_files;
    bool m_with_full_path;
    QString m_archiveDir;
    bool m_password_protected;
};

#endif // UPDATEARCHENGINE_H
