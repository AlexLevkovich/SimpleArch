#include "archiverprocess.h"
#include <QMapIterator>
#include <QDebug>

#define ANOTHER_PROCESS_ACTIVE "This process is already active!!!"
#define TIMEOUT 1000

QList<ArchiverProcess *> ArchiverProcess::active_processes;

ArchiverProcess::ArchiverProcess(QObject *parent) : FlushedProcess(parent) {
    qRegisterMetaType<UnixProcess::ExitStatus>("UnixProcess::ExitStatus");

    was_terminated = false;
    isReading = false;
    init_connections();
}

ArchiverProcess::~ArchiverProcess() {}

void ArchiverProcess::init_connections() {
    connect(this,SIGNAL(error(UnixProcess::ProcessError)),this,SLOT(onError(UnixProcess::ProcessError)));
    connect(this,SIGNAL(finished(int,UnixProcess::ExitStatus)),this,SLOT(onFinished(int,UnixProcess::ExitStatus)),Qt::QueuedConnection);
    connect(this,SIGNAL(readyReadStandardError()),this,SLOT(onReadyReadStandardError()));
    connect(this,SIGNAL(readyReadStandardOutput()),this,SLOT(onReadyReadStandardOutput()));
}

void ArchiverProcess::onError(UnixProcess::ProcessError) {
    if (!wasTerminated()) emit error(program()+": "+errorString());
}

QString	ArchiverProcess::errorString() const {
    if (!m_errors.isEmpty()) return m_errors;

    return FlushedProcess::errorString();
}

void ArchiverProcess::onFinished(int code,UnixProcess::ExitStatus status) {
    setReadChannel(UnixProcess::StandardOutput);
    bool at_end = atEnd();
    setReadChannel(UnixProcess::StandardError);
    if (at_end && atEnd() && !isReading) {
        if (code != 0) {
            if (wasTerminated()) m_errors.clear();
            if (!m_errors.isEmpty()) emit error(m_errors);
            else emit error(((status == UnixProcess::CrashExit) && !wasTerminated())?FlushedProcess::errorString():"");
        }
        else emit finished();
        m_errors.clear();
        deleteLater();
        active_processes.removeAll(this);
    }
    else {
        if (!isReading) {
            onReadyReadStandardOutput();
            onReadyReadStandardError();
        }
        QMetaObject::invokeMethod(this,"onFinished",Qt::QueuedConnection,Q_ARG(int,code),Q_ARG(UnixProcess::ExitStatus,status));
    }
}

void ArchiverProcess::onReadyReadStandardError() {
    if (!active_processes.contains(this)) return;

    isReading = true;

    setReadChannel(UnixProcess::StandardError);

    QString line;
    err_buffer += readAll();
    while(!(line = QString::fromLocal8Bit(err_buffer.readLine())).isEmpty()) {
        if (line.endsWith('\n')) line.chop(1);
        if (line.endsWith('\r')) line.chop(1);

        addErrorLine(line);

        emit readyStandardErrorLine(line);
        if (was_terminated) break;
    }

    if (err_buffer.size() > 0) emit peekStandardErrorTail(QString::fromLocal8Bit(err_buffer.data()));

    isReading = false;
}

void ArchiverProcess::onReadyReadStandardOutput() {
    if (!active_processes.contains(this)) return;

    isReading = true;

    setReadChannel(UnixProcess::StandardOutput);

    QString line;
    out_buffer += readAll();

    while(!(line = QString::fromLocal8Bit(out_buffer.readLine())).isEmpty()) {
        if (line.endsWith('\n')) line.chop(1);
        if (line.endsWith('\r')) line.chop(1);
        emit readyStandardOutputLine(line);
        if (was_terminated) break;
    }

    if (out_buffer.size() > 0) emit peekStandardOutputTail(line);

    isReading = false;
}

bool ArchiverProcess::wasTerminated() const {
    return was_terminated;
}

void ArchiverProcess::terminate() {
    was_terminated = true;
    FlushedProcess::terminate();
}

void ArchiverProcess::kill() {
    was_terminated = true;
    FlushedProcess::kill();
}

void ArchiverProcess::start(const QString & program, const QStringList & arguments, QIODevice::OpenMode mode) {
    if (active_processes.contains(this)) {
        QMetaObject::invokeMethod(this,"error",Qt::QueuedConnection,Q_ARG(QString,tr(ANOTHER_PROCESS_ACTIVE)));
        m_errors.clear();
        deleteLater();
        return;
    }

    isReading = false;
    active_processes.append(this);

    FlushedProcess::start(program,arguments,mode);
}

void ArchiverProcess::start(const QString & command, QIODevice::OpenMode mode) {
    if (active_processes.contains(this)) {
        QMetaObject::invokeMethod(this,"error",Qt::QueuedConnection,Q_ARG(QString,tr(ANOTHER_PROCESS_ACTIVE)));
        m_errors.clear();
        deleteLater();
        return;
    }

    isReading = false;
    active_processes.append(this);

    FlushedProcess::start(command,mode);
}

void ArchiverProcess::clearStdoutTail() {
    if (out_buffer.size() > 0) out_buffer.readAll();
}

void ArchiverProcess::clearStderrTail() {
    if (err_buffer.size() > 0) err_buffer.readAll();
}

void ArchiverProcess::addErrorLine(const QString & error) {
    if (!error.isEmpty()) m_errors += error + '\n';
}
