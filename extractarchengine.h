#ifndef ExtractArchEngine_H
#define ExtractArchEngine_H

#include <QObject>
#include <QStringList>

class BaseArchEngine;

class ExtractArchEngine : public QObject {
    Q_OBJECT
public:
    ExtractArchEngine(BaseArchEngine * engine,const QStringList & files,const QString & output_dir,bool use_full_path = false,QObject * parent = NULL);
    bool exec();

protected:
    ExtractArchEngine(BaseArchEngine * engine,const QStringList & files,bool use_full_path = false,QObject * parent = NULL);
    void setOutputDir(const QString & dir);
    QStringList m_files;

private slots:
    void password_request(const QString & fileName);

private:
    BaseArchEngine * base_engine;
    QString m_output_dir;
    bool m_use_full_path;
};

#endif // ExtractArchEngine_H
