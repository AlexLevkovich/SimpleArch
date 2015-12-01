#ifndef SaveAsArchEngine_H
#define SaveAsArchEngine_H

#include <QObject>
#if QT_VERSION < 0x050000
#include "qtemporarydir.h"
#else
#include <QTemporaryDir>
#endif

#include <QStringList>

class BaseArchEngine;

class SaveAsArchEngine : public QObject {
    Q_OBJECT
public:
    explicit SaveAsArchEngine(BaseArchEngine * engine,const QString & output_file,bool password_protected);
    bool exec();

private slots:
    void password_request(const QString & fileName);
    void input_create_password(bool verificate);

private:
    QStringList getTemporaryContents() const;

    QTemporaryDir temp_dir;
    BaseArchEngine * m_engine;
    QString m_output_file;
    bool m_password_protected;
};

#endif // SaveAsArchEngine_H
