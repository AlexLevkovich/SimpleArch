#ifndef DeleteTarArchEngine_H
#define DeleteTarArchEngine_H

#include <QObject>
#if QT_VERSION < 0x050000
#include "qtemporarydir.h"
#else
#include <QTemporaryDir>
#endif

class ComplexTarArchEngine;
class TarArchEngine;

class DeleteTarArchEngine : public QObject {
    Q_OBJECT
public:
    explicit DeleteTarArchEngine(ComplexTarArchEngine * engine,const QStringList & files);
    ~DeleteTarArchEngine();

private slots:
    void update_extraction_ok();
    void tar_delete_ok();
    void start();

signals:
    void error(const QString & line);
    void delete_ok();

private:
    QTemporaryDir temp_dir;
    TarArchEngine * tar_engine;
    ComplexTarArchEngine * dup_engine;
    QStringList m_files;
};

#endif // DeleteTarArchEngine_H
