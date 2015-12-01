#ifndef FlushedProcess_H
#define FlushedProcess_H

#include <QLocalServer>
#include "writeptyprocess.h"

class QLocalSocket;

class FlushedProcess : public QObject {
    Q_OBJECT
public:
    explicit FlushedProcess(QObject *parent = 0);
    ~FlushedProcess();

    inline bool isRunning() const {
        return (process.pid() > 0);
    }

    UnixProcess::ProcessError error() const;
    int	exitCode() const;
    UnixProcess::ExitStatus exitStatus() const;
    UnixProcess::ProcessChannelMode processChannelMode() const;
    QString program() const;
    UnixProcess::ProcessChannel readChannel() const;
    void setProcessChannelMode(UnixProcess::ProcessChannelMode mode);
    void setProgram(const QString & program);
    void setReadChannel(UnixProcess::ProcessChannel channel);
    void setWorkingDirectory(const QString & dir);
    virtual void start(const QString & program, const QStringList & arguments, QIODevice::OpenMode mode = QIODevice::ReadWrite);
    virtual void start(const QString & command, QIODevice::OpenMode mode = QIODevice::ReadWrite);
    UnixProcess::ProcessState	state() const;
    bool waitForFinished(int msecs = 30000);
    bool waitForStarted(int msecs = 30000);
    bool waitForBytesWritten(int msecs);
    QString	workingDirectory() const;
    qint64 write(const QByteArray & byteArray);
    virtual QString	errorString() const;

    qint64 read(char * data, qint64 maxSize);
    QByteArray read(qint64 maxSize);
    QByteArray readAll();
    qint64 readLine(char * data, qint64 maxSize);
    QByteArray readLine(qint64 maxSize = 0);
    QByteArray readAllStandardError();
    QByteArray readAllStandardOutput();

    bool atEnd() const;
    qint64 bytesAvailable() const;
    bool canReadLine() const;
    bool waitForReadyRead(int msecs = 30000);

signals:
    void readyReadStandardOutput();
    void readyReadStandardError();
    void error(UnixProcess::ProcessError error);
    void finished(int exitCode,UnixProcess::ExitStatus exitStatus);
    void started();
    void stateChanged(UnixProcess::ProcessState newState);

public slots:
    virtual void terminate();
    virtual void kill();

private slots:
    void processFinished(int code,UnixProcess::ExitStatus status);
    void newOutConnection();
    void newErrConnection();
    void readyReadStandardOutputs();

private:
    QIODevice * readingIODevice() const;

    QLocalServer out_server;
    QLocalServer err_server;
    QLocalSocket * out_socket;
    QLocalSocket * err_socket;
    WritePtyProcess process;
    QString m_program;
    bool init_error;
};

#endif
