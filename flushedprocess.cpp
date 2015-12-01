#include "flushedprocess.h"
#include "staticutils.h"
#include <QLocalSocket>
#include <QTimer>
#include <QFileInfo>
#include "eventloop.h"
#include <QDebug>

#define PROCESS_NOT_STARTED "Process is not started!"
#define CANNOT_INIT_PROCESS "FlushedProcess: Cannot create temporary socket!"
#define NOTIFIER_TIMEOUT    100

FlushedProcess::FlushedProcess(QObject *parent) : QObject(parent) {
    qRegisterMetaType<UnixProcess::ExitStatus>("UnixProcess::ExitStatus");
    qRegisterMetaType<UnixProcess::ProcessError>("UnixProcess::ProcessError");

    init_error = false;
    out_socket = NULL;
    err_socket = NULL;

    if (!out_server.listen(StaticUtils::tempFileName(true))) {
        init_error = true;
        return;
    }
    if (!err_server.listen(StaticUtils::tempFileName(true))) {
        init_error = true;
        return;
    }

    connect(&out_server,SIGNAL(newConnection()),this,SLOT(newOutConnection()));
    connect(&err_server,SIGNAL(newConnection()),this,SLOT(newErrConnection()));
    process.setStandardOutputSocket(out_server.serverName());
    process.setStandardErrorSocket(err_server.serverName());

    connect(&process,SIGNAL(finished(int,UnixProcess::ExitStatus)),this,SLOT(processFinished(int,UnixProcess::ExitStatus)),Qt::QueuedConnection);
    connect(&process,SIGNAL(error(UnixProcess::ProcessError)),this,SIGNAL(error(UnixProcess::ProcessError)));
    connect(&process,SIGNAL(started()),this,SIGNAL(started()));
    connect(&process,SIGNAL(stateChanged(UnixProcess::ProcessState)),this,SIGNAL(stateChanged(UnixProcess::ProcessState)));
}

void FlushedProcess::newOutConnection() {
    if (out_socket != NULL) delete out_socket;
    out_socket = out_server.nextPendingConnection();
    out_socket->setParent(this);
    connect(out_socket,SIGNAL(readyRead()),this,SIGNAL(readyReadStandardOutput()));
}

void FlushedProcess::newErrConnection() {
    if (err_socket != NULL) delete err_socket;
    err_socket = err_server.nextPendingConnection();
    err_socket->setParent(this);
    connect(err_socket,SIGNAL(readyRead()),this,SIGNAL(readyReadStandardError()));
}

void FlushedProcess::processFinished(int code,UnixProcess::ExitStatus status) {
    readyReadStandardOutputs();
    emit finished(code,status);
}

void FlushedProcess::readyReadStandardOutputs() {
    if ((err_socket != NULL) && err_socket->bytesAvailable()) emit readyReadStandardError();
    if ((out_socket != NULL) && out_socket->bytesAvailable()) emit readyReadStandardOutput();
}

FlushedProcess::~FlushedProcess() {
    if (isRunning()) {
        process.kill();
        process.waitForFinished();
    }

    QString name = out_server.serverName();
    out_server.close();
    QLocalServer::removeServer(name);
    name = err_server.serverName();
    err_server.close();
    QLocalServer::removeServer(name);
}

QIODevice * FlushedProcess::readingIODevice() const {
    FlushedProcess * p_this = (FlushedProcess *)this;
    return (readChannel() == UnixProcess::StandardError)?p_this->err_socket:p_this->out_socket;
}

bool FlushedProcess::atEnd() const {
    QIODevice * dev = readingIODevice();
    return (dev == NULL)?true:dev->atEnd();
}

qint64 FlushedProcess::bytesAvailable() const {
    QIODevice * dev = readingIODevice();
    return (dev == NULL)?0:dev->bytesAvailable();
}

bool FlushedProcess::canReadLine() const {
    QIODevice * dev = readingIODevice();
    return (dev == NULL)?false:dev->canReadLine();
}

bool FlushedProcess::waitForReadyRead(int msecs) {
    if (!isRunning()) {
        process.setErrorString(tr(PROCESS_NOT_STARTED));
        return false;
    }
    EventLoop pause;
    if (msecs >= 0) QTimer::singleShot(msecs,&pause,SLOT(cancel()));
    connect(this,(readChannel() == UnixProcess::StandardError)?
                 SIGNAL(readyReadStandardError()):
                 SIGNAL(readyReadStandardOutput()),&pause,SLOT(quit()));
    return (pause.exec() == 0);
}

qint64 FlushedProcess::read(char * data, qint64 maxlen) {
    QIODevice * dev = readingIODevice();
    if (dev == NULL) return -1;
    qint64 ret = dev->read(data,maxlen);
    if (ret < 0) {
        process.setErrorString(tr("Error reading from device!"));
        QMetaObject::invokeMethod(this,"error",Qt::QueuedConnection,Q_ARG(UnixProcess::ProcessError,UnixProcess::ReadError));
    }
    return ret;
}

QByteArray FlushedProcess::read(qint64 maxSize) {
    QIODevice * dev = readingIODevice();
    return (dev == NULL)?QByteArray():dev->read(maxSize);
}

QByteArray FlushedProcess::readAll() {
    QIODevice * dev = readingIODevice();
    return (dev == NULL)?QByteArray():dev->readAll();
}

qint64 FlushedProcess::readLine(char * data, qint64 maxSize) {
    QIODevice * dev = readingIODevice();
    return (dev == NULL)?-1:dev->readLine(data,maxSize);
}

QByteArray FlushedProcess::readLine(qint64 maxSize) {
    QIODevice * dev = readingIODevice();
    return (dev == NULL)?QByteArray():dev->readLine(maxSize);
}

QByteArray FlushedProcess::readAllStandardError() {
    return (err_socket == NULL)?QByteArray():err_socket->readAll();
}

QByteArray FlushedProcess::readAllStandardOutput() {
    return (out_socket == NULL)?QByteArray():out_socket->readAll();
}

UnixProcess::ProcessError FlushedProcess::error() const {
    return init_error?UnixProcess::FailedToStart:process.error();
}

int	FlushedProcess::exitCode() const {
    return process.exitCode();
}

UnixProcess::ExitStatus FlushedProcess::exitStatus() const {
    return process.exitStatus();
}

UnixProcess::ProcessChannelMode FlushedProcess::processChannelMode() const {
    return process.processChannelMode();
}

QString	FlushedProcess::program() const {
    return m_program;
}

UnixProcess::ProcessChannel FlushedProcess::readChannel() const {
    return process.readChannel();
}

void FlushedProcess::setProcessChannelMode(UnixProcess::ProcessChannelMode mode) {
    process.setProcessChannelMode(mode);
}

void FlushedProcess::setProgram(const QString & program) {
    m_program = program;
}

void FlushedProcess::setReadChannel(UnixProcess::ProcessChannel channel) {
    process.setReadChannel(channel);
}

void FlushedProcess::setWorkingDirectory(const QString & dir) {
    process.setWorkingDirectory(dir);
}

void FlushedProcess::start(const QString & program, const QStringList & arguments, QIODevice::OpenMode mode) {
    if (init_error) {
        process.setErrorString(tr(CANNOT_INIT_PROCESS));
        QMetaObject::invokeMethod(this,"error",Qt::QueuedConnection,Q_ARG(UnixProcess::ProcessError,UnixProcess::FailedToStart));
        return;
    }

    m_program = program;
    process.start(program,arguments,mode);
}

static QStringList parseCombinedArgString(const QString &program) {
    QStringList args;
    QString tmp;
    int quoteCount = 0;
    bool inQuote = false;

    for (int i = 0; i < program.size(); ++i) {
        if (program.at(i) == QLatin1Char('"')) {
            ++quoteCount;
            if (quoteCount == 3) {
                // third consecutive quote
                quoteCount = 0;
                tmp += program.at(i);
            }
            continue;
        }
        if (quoteCount) {
            if (quoteCount == 1)
                inQuote = !inQuote;
            quoteCount = 0;
        }
        if (!inQuote && program.at(i).isSpace()) {
            if (!tmp.isEmpty()) {
                args += tmp;
                tmp.clear();
            }
        } else {
            tmp += program.at(i);
        }
    }
    if (!tmp.isEmpty())
        args += tmp;

    return args;
}

void FlushedProcess::start(const QString & command, QIODevice::OpenMode mode) {
    if (init_error) {
        process.setErrorString(tr(CANNOT_INIT_PROCESS));
        QMetaObject::invokeMethod(this,"error",Qt::QueuedConnection,Q_ARG(UnixProcess::ProcessError,UnixProcess::FailedToStart));
        return;
    }

    m_program = parseCombinedArgString(command).first();
    process.start(command,mode);
}

UnixProcess::ProcessState FlushedProcess::state() const {
    return process.state();
}

bool FlushedProcess::waitForFinished(int msecs) {
    return process.waitForFinished(msecs);
}

bool FlushedProcess::waitForBytesWritten(int msecs) {
    return process.waitForBytesWritten(msecs);
}

bool FlushedProcess::waitForStarted(int msecs) {
    return process.waitForStarted(msecs);
}

QString	FlushedProcess::workingDirectory() const {
    return process.workingDirectory();
}

qint64 FlushedProcess::write(const QByteArray & byteArray) {
    return process.write(byteArray);
}

QString	FlushedProcess::errorString() const {
    return process.errorString();
}

void FlushedProcess::terminate() {
    process.terminate();
}

void FlushedProcess::kill() {
    process.kill();
}
