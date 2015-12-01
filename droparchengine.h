#ifndef DropArchEngine_H
#define DropArchEngine_H

#include <QObject>
#include <QList>
#include <QStringList>
#include <QUrl>

class BaseArchEngine;

class DropArchEngine : public QObject {
    Q_OBJECT
public:
    DropArchEngine(BaseArchEngine * engine,const QString & archiveDir,const QList<QUrl> & files,QObject * parent = NULL);
    DropArchEngine(BaseArchEngine * engine,const QString & archiveDir,const QStringList & files,bool with_full_path,bool password_protected = false,QObject * parent = NULL);
    bool exec();

private slots:
    void input_create_password(bool verificate);

private:
    BaseArchEngine * base_engine;
    QStringList m_files;
    QString m_archiveDir;
    bool m_with_full_path;
    bool m_password_protected;
};

#endif // DropArchEngine_H
