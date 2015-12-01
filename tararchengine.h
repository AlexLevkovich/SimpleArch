#ifndef TARARCHENGINE_H
#define TARARCHENGINE_H

#include "basearchengine.h"

class TarArchEngine : public BaseArchEngine {
    Q_OBJECT
public:
    Q_INVOKABLE TarArchEngine(const QString & fileName,QObject *parent = 0);
    virtual BaseArchEngine * dupEngine(QObject * parent = NULL);
    virtual bool isMultiFiles() const;
    virtual QList<int> compressionLevels() const;
    virtual int defaultCompressionLevel() const;
    inline virtual QString engineName() const {
        return "Tar";
    }
    inline bool readOnly() const {
        return false;
    }

protected slots:
    virtual bool areExternalProgramsOK();
    virtual const QStringList suffixes();
    virtual const QString openFilter();
    virtual const QString createFilter();

protected:
    virtual QString list_contents_command() const;
    virtual QString uncompress_command(const QStringList & files,const QString & dir_path,bool with_full_path) const;
    virtual QString create_command(const QStringList & files,bool with_full_path,bool password_protected = false) const;
    virtual QString update_command(const QStringList & files,bool with_full_path,const QString & archiveDir = QString(),bool password_protected = false) const;
    virtual QString processingSymbols() const;
    virtual QString delete_command(const QStringList & files) const;
    bool parsePasswordRequiring(const QString & line,QString & fileName);
    bool parseUpdatePassword(const QString & line,bool & verificate);
    FileTreeItem * split_list_contents_record(const QString & rec,bool & ignore);
};

#endif // TARARCHENGINE_H
