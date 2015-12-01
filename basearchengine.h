#ifndef BASEARCHENGINE_H
#define BASEARCHENGINE_H

#include <QObject>
#include <QProcess>
#include <QMap>
#include <QPair>

class QSettings;
class FileTreeItem;
class ArchiverProcess;

class BaseArchEngine : public QObject {
    Q_OBJECT
    template<class Type> friend class EngineType;
public:
    enum DateFormat {
        ISO = 0,
        System = 1
    };

    enum ChannelFlag {
        StdoutChannel = 1,
        StderrChannel = 2
    };

    ~BaseArchEngine();

    static BaseArchEngine * main_arch_engine;

    //------------------default parameters
    static QString defaultSaveFilter;
    static bool defaultDoTarExtraction;
    static bool defaultUseFullPath;
    static bool defaultUseSelectedFilesOnly;
    static QString defaultOpenDir;
    static QString defaultSaveDir;
    static QString defaultSelectDir;
    static QMap<QString,QString> userMimes;
    static bool defaultUseSystemMimes;
    static bool defaultDoFileNamesSorting;
    static bool defaultDoShowFolderTree;
    static bool defaultHighlightFocusedView;
    static uint defaultLeftIconSize;
    static uint defaultRightIconSize;
    static DateFormat defaultDateFormat;
    //-------------------------------------

    //  initialization
    static BaseArchEngine * findEngine(const QString & fileName,QObject * parent = NULL);

    static const QStringList findOpenFilters();
    static const QStringList findCreateFilters();
    static void initDefaults();
    static void saveDefaults();
    static QSettings * settingsInstance();

    QString fileName() const;
    QString lastCommand() const;
    void sendPassword(const QString & password);
    virtual bool supportsEncripting() const;
    void terminateCommand();
    QString lastMessage() const;

    static const QMap<QString,QPair<QList<int>,int> > enginesCompressionLevels();
    static bool setEnginesCompressionLevels(const QMap<QString,int> & levels);
    static int compressionLevel(const QMetaObject & metaObject,QString & engineName,QList<int> & allLevels);
    static bool setCompressionLevel(int level,const QMetaObject & metaObject);

    bool startListContents();
    bool extractFiles(const QStringList & files,const QString & dir_path,bool with_full_path);
    bool createArchive(const QStringList & files,bool with_full_path,bool password_protected = false);
    bool updateArchive(const QStringList & files,bool with_full_path,const QString & archiveDir = QString(),bool password_protected = false);
    bool deleteFiles(const QStringList & files);

public slots:
    virtual int compressionLevel() const;
    virtual bool setCompressionLevel(int level);

    // public undefined functions
    virtual bool readOnly() const = 0;
    virtual int defaultCompressionLevel() const = 0;
    virtual BaseArchEngine * dupEngine(QObject * parent = NULL) = 0;
    virtual bool isMultiFiles() const = 0;
    virtual QList<int> compressionLevels() const = 0;
    virtual QString engineName() const = 0;

protected slots:
    // protected undefined functions
    virtual bool areExternalProgramsOK() = 0;
    virtual const QStringList suffixes() = 0;
    virtual const QString openFilter() = 0;
    virtual const QString createFilter() = 0;
    virtual QString list_contents_command() const = 0;
    // parses list_contents_* record
    virtual FileTreeItem * split_list_contents_record(const QString & rec,bool & ignore) = 0;
    // processing commands
    virtual QString uncompress_command(const QStringList & files,const QString & dir_path,bool with_full_path) const = 0;
    virtual QString create_command(const QStringList & files,bool with_full_path,bool password_protected = false) const = 0;
    virtual QString update_command(const QStringList & files,bool with_full_path,const QString & archiveDir = QString(),bool password_protected = false) const = 0;
    virtual QString delete_command(const QStringList & files) const = 0;
    // parses the stdout/stderr lines for the special phrases of password processing
    // (should return false if there is no password procesing)
    // for extract
    virtual bool parsePasswordRequiring(const QString & line,QString & fileName) = 0;
    // for create/update
    virtual bool parseUpdatePassword(const QString & line,bool & verificate) = 0;

protected:
    BaseArchEngine(const QString & fileName,QObject * parent = NULL);
    virtual ChannelFlag parsePasswordChannel() const;
    void addProcessErrorLine(const QString & error);

    // -- by default executes *_command() functions
    // -- executed from its parent public functions (without _body prefix)
    // generates signals: list_contents_record, list_contents_ok, error, error_message, password_required
    virtual bool startListContents_body();
    // generates signals: extraction_ok, error, create_password
    virtual bool extractFiles_body(const QStringList & files,const QString & dir_path,bool with_full_path);
    // generates signals: create_ok, error, create_password
    virtual bool createArchive_body(const QStringList & files,bool with_full_path,bool password_protected = false);
    // generates signals: update_ok, error, create_password
    virtual bool updateArchive_body(const QStringList & files,bool with_full_path,const QString & archiveDir = QString(),bool password_protected = false);
    // generates signals: delete_ok, error
    virtual bool deleteFiles_body(const QStringList & files);
    // --

    void post_error_signal(const QString & line);
    inline ArchiverProcess * currentProcess() { return current_process; }

    static bool isProgramAvailable(const QString & program_path);

private slots:
    void list_contents_split_record(const QString & rec);
    void extraction_readyStandardLine(const QString & line);
    void extraction_readyStandardTail(const QString & line);
    void update_readyStandardLine(const QString & line);
    void update_readyStandardTail(const QString & line);
    void any_process_finished();

signals:
    void error(const QString & line);
    void error_message(const QString & line);
    void list_contents_record(FileTreeItem * item);
    void list_contents_ok();
    void extraction_ok();
    void create_ok();
    void update_ok();
    void delete_ok();
    void password_required(const QString & fileName);
    void create_password(bool verificate);

private:
    bool another_command_active_error();

    static QList<QMetaObject> engine_objects;
    ArchiverProcess * current_process;
    QString m_fileName;
    static QSettings * p_settings;
    QString m_lastCommand;
};

#define DECLARE_ENGINE(Type) \
    template <class Type> class EngineType { \
        public: \
        EngineType() { \
            BaseArchEngine::engine_objects.append(Type::staticMetaObject); \
        } \
    }; \
    static EngineType<Type> engineType;

#endif // BASEARCHENGINE_H
