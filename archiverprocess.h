#ifndef ARCHIVERPROCESS_H
#define ARCHIVERPROCESS_H

#include "flushedprocess.h"
#include <QStringList>
#include <QByteArray>
#include <QMap>
#include <QTimer>
#include "sequentialbuffer.h"

class ArchiverProcess : public FlushedProcess {
    Q_OBJECT
public:
    ArchiverProcess(QObject *parent = 0);
    ~ArchiverProcess();
    void start(const QString & program, const QStringList & arguments, QIODevice::OpenMode mode = QIODevice::ReadWrite);
    void start(const QString & command, QIODevice::OpenMode mode = QIODevice::ReadWrite);

    bool wasTerminated() const;
    void addErrorLine(const QString & error);
    QString	errorString() const;

    void clearStdoutTail();
    void clearStderrTail();

    static QList<ArchiverProcess *> active_processes;

signals:
    void error(const QString & error);
    void peekStandardOutputTail(const QString & line);
    void peekStandardErrorTail(const QString & line);
    void readyStandardOutputLine(const QString & line);
    void readyStandardErrorLine(const QString & line);
    void finished();

public slots:
    void terminate();
    void kill();

private slots:
    void onError(UnixProcess::ProcessError error);
    void onFinished(int code,UnixProcess::ExitStatus status);
    void onReadyReadStandardError();
    void onReadyReadStandardOutput();

private:
    void init_connections();

    bool was_terminated;
    QString m_errors;
    SequentialBuffer out_buffer;
    SequentialBuffer err_buffer;
    bool isReading;
};

#endif // ARCHIVERPROCESS_H
