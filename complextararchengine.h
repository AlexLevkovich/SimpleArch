#ifndef COMPLEXTARARCHENGINE_H
#define COMPLEXTARARCHENGINE_H

#include "tararchengine.h"
#include <QFileInfo>

class ComplexTarArchEngine : public TarArchEngine {
    Q_OBJECT
public:
    bool isTar() const;
    QString tarName() const;
    bool nottar_was_forced() const;
    void setNotTarForceFlag(bool flag);
    virtual BaseArchEngine * dupEngine(QObject * parent = NULL);
    bool isMultiFiles() const;
    static bool isRealTar(const QString & fileName);
    static const QString firstSuffix(const QString & fileName);
    static const QString fullSuffix(const QString & fileName);
    static const QString fullSuffix(const QFileInfo & fileName);
    virtual QList<int> compressionLevels() const;
    virtual int defaultCompressionLevel() const;

protected:
    ComplexTarArchEngine(const QString & fileName,QObject *parent = 0,bool force_tobe_nottar = false);
    QString list_contents_command() const;
    virtual QString archiverPath() const = 0;
    bool updateArchive_body(const QStringList & files,bool with_full_path,const QString & archiveDir = QString(),bool password_protected = false);
    bool deleteFiles_body(const QStringList & files);
    QString processingSymbols() const;

private slots:

private:
    QString permissions() const;

    bool m_force_tobe_nottar;
};

#endif // COMPLEXTARARCHENGINE_H
