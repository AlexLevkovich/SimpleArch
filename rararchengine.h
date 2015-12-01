#ifndef RarArchEngine_H
#define RarArchEngine_H

#include <QStringList>
#include "basearchengine.h"
#if QT_VERSION < 0x050000
#include "qtemporarydir.h"
#else
#include <QTemporaryDir>
#endif
#include <QDateTime>

class RarArchEngine : public BaseArchEngine {
    Q_OBJECT
public:
    Q_INVOKABLE RarArchEngine(const QString & fileName,QObject *parent = 0);
    virtual BaseArchEngine * dupEngine(QObject * parent = NULL);
    virtual bool isMultiFiles() const;
    bool supportsEncripting() const;
    QList<int> compressionLevels() const;
    int defaultCompressionLevel() const;
    inline QString engineName() const {
        return "Rar";
    }
    inline bool readOnly() const {
        return true;
    }


protected slots:
    bool areExternalProgramsOK();
    const QStringList suffixes();
    const QString openFilter();
    const QString createFilter();

protected:
    virtual QString list_contents_command() const;
    virtual QString uncompress_command(const QStringList & files,const QString & dir_path,bool with_full_path) const;
    virtual QString create_command(const QStringList & files,bool with_full_path,bool password_protected = false) const;
    virtual QString update_command(const QStringList & files,bool with_full_path,const QString & archiveDir = QString(),bool password_protected = false) const;
    virtual QString processingSymbols() const;
    virtual QString delete_command(const QStringList & files) const;
    FileTreeItem * split_list_contents_record(const QString & rec,bool & ignore);
    bool extractFiles_body(const QStringList & files,const QString & dir_path,bool with_full_path);
    bool parsePasswordRequiring(const QString & line,QString & fileName);
    bool parseUpdatePassword(const QString & line,bool & verificate);

private slots:
    void remove_temp_dir();
    void extraction_ok();

private:
    QTemporaryDir * temp_dir;
    QString m_dir_path;
    QString m_files_dir;
    QString m_user;
    bool use_prev_password;

    QString path;
    QByteArray perms;
    qreal file_size;
    QDateTime date_time;
    bool encripted;
};

#endif // RarArchEngine_H
