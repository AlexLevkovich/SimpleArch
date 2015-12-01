#ifndef UpdateTarArchEngine_H
#define UpdateTarArchEngine_H

#include <QObject>
#if QT_VERSION < 0x050000
#include "qtemporarydir.h"
#else
#include <QTemporaryDir>
#endif

class ComplexTarArchEngine;
class TarArchEngine;

class UpdateTarArchEngine : public QObject {
    Q_OBJECT
public:
    explicit UpdateTarArchEngine(ComplexTarArchEngine * engine,const QStringList & files,bool with_full_path,const QString & archiveDir = QString());
    ~UpdateTarArchEngine();

private slots:
    void update_extraction_ok();
    void tar_update_ok();
    void start();

signals:
    void error(const QString & line);
    void update_ok();

private:
    QTemporaryDir temp_dir;
    TarArchEngine * tar_engine;
    ComplexTarArchEngine * dup_engine;
    QStringList m_files;
    bool m_with_full_path;
    QString m_archiveDir;
};

#endif // UpdateTarArchEngine_H
