#ifndef LZIPARCHENGINE_H
#define LZIPARCHENGINE_H

#include "complextararchengine.h"

class LzipArchEngine : public ComplexTarArchEngine {
    Q_OBJECT
public:
    Q_INVOKABLE LzipArchEngine(const QString & fileName,QObject *parent = 0,bool force_tobe_nottar = false);
    BaseArchEngine * dupEngine(QObject * parent = NULL);
    inline QString engineName() const {
        return "Lzip";
    }

protected slots:
    bool areExternalProgramsOK();
    const QStringList suffixes();
    const QString openFilter();
    const QString createFilter();

protected:
    QString uncompress_command(const QStringList & files,const QString & dir_path,bool with_full_path) const;
    QString create_command(const QStringList & files,bool with_full_path,bool password_protected = false) const;
    QString update_command(const QStringList & files,bool with_full_path,const QString & archiveDir = QString(),bool password_protected = false) const;
    QString archiverPath() const;
    QString delete_command(const QStringList & files) const;
};

#endif // LZIPARCHENGINE_H
