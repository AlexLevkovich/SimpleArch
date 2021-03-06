/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

//#define UNIX_PROCESS_DEBUG
#include <QDebug>

#ifndef QT_NO_PROCESS

#if defined UNIX_PROCESS_DEBUG
#include <QString>
#include <ctype.h>

/*
    Returns a human readable representation of the first \a len
    characters in \a data.
*/
//QT_BEGIN_NAMESPACE
static QByteArray qt_prettyDebug(const char *data, int len, int maxSize)
{
    if (!data) return "(null)";
    QByteArray out;
    for (int i = 0; i < len; ++i) {
        char c = data[i];
        if (isprint(c)) {
            out += c;
        } else switch (c) {
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            QString tmp;
            tmp.sprintf("\\%o", c);
            out += tmp.toLatin1();
        }
    }

    if (len < maxSize)
        out += "...";

    return out;
}
//QT_END_NAMESPACE
#endif

#include "qplatformdefs.h"

#include "process/unixprocess.h"
#include "process/unixprocess_p.h"

#ifdef Q_OS_MAC
#include <private/qcore_mac_p.h>
#endif

#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QList>
#include <QMap>
#include <QMutex>
#include <QSemaphore>
#include <QSocketNotifier>
#include <QThread>
#include <QTimer>

#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

//QT_BEGIN_NAMESPACE
#ifdef Q_OS_INTEGRITY
static inline char *strdup(const char *data)
{
    return qstrdup(data);
}
#endif

static qint64 qt_native_read(int fd, char *data, qint64 maxlen)
{
    qint64 ret = 0;
    do {
        ret = ::read(fd, data, maxlen);
    } while (ret == -1 && errno == EINTR);
    return ret;
}

static qint64 qt_native_write(int fd, const char *data, qint64 len)
{
    qint64 ret = 0;
    do {
        ret = ::write(fd, data, len);
    } while (ret == -1 && errno == EINTR);
    return ret;
}

static void qt_native_close(int fd)
{
    int ret;
    do {
        ret = ::close(fd);
    } while (ret == -1 && errno == EINTR);
}

static void qt_native_sigaction(int signum, const struct sigaction *act,
                                struct sigaction *oldact)
{
    int ret;
    do {
        ret = ::sigaction(signum, act, oldact);
    } while (ret == -1 && errno == EINTR);
}

static void qt_native_dup2(int oldfd, int newfd)
{
    int ret;
    do {
        ret = ::dup2(oldfd, newfd);
    } while (ret == -1 && errno == EINTR);
}

static void qt_native_chdir(const char *path)
{
    int ret;
    do {
        ret = ::chdir(path);
    } while (ret == -1 && errno == EINTR);
}

static void qt_native_execve(const char *filename, char *const argv[],
                              char *const envp[])
{
    int ret;
    do {
        ret = ::execve(filename, argv, envp);
    } while (ret == -1 && errno == EINTR);
}

static void qt_native_execv(const char *path, char *const argv[])
{
    int ret;
    do {
        ret = ::execv(path, argv);
    } while (ret == -1 && errno == EINTR);
}

static void qt_native_execvp(const char *file, char *const argv[])
{
    int ret;
    do {
        ret = ::execvp(file, argv);
    } while (ret == -1 && errno == EINTR);
}

static int qt_native_select(fd_set *fdread, fd_set *fdwrite, int timeout)
{
    struct timeval tv;
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;

    int ret;
    do {
        ret = select(FD_SETSIZE, fdread, fdwrite, 0, timeout < 0 ? 0 : &tv);
    } while (ret < 0 && (errno == EINTR));
    return ret;
}

/*
   Returns the difference between msecs and elapsed. If msecs is -1,
   however, -1 is returned.
*/
static int qt_timeout_value(int msecs, int elapsed)
{
    if (msecs == -1)
        return -1;

    int timeout = msecs - elapsed;
    return timeout < 0 ? 0 : timeout;
}

static int qt_UnixProcess_deadChild_pipe[2];
static void (*qt_sa_old_sigchld_handler)(int) = 0;
static void qt_sa_sigchld_handler(int signum)
{
    qt_native_write(qt_UnixProcess_deadChild_pipe[1], "", 1);
#if defined (UNIX_PROCESS_DEBUG)
    fprintf(stderr, "*** SIGCHLD\n");
#endif

    if (qt_sa_old_sigchld_handler && qt_sa_old_sigchld_handler != SIG_IGN)
        qt_sa_old_sigchld_handler(signum);
}


struct UnixProcessInfo {
    UnixProcess *process;
    int deathPipe;
    int exitResult;
    pid_t pid;
    int serialNumber;
};

class UnixProcessManager : public QThread
{
public:
    UnixProcessManager();
    ~UnixProcessManager();

    void run();
    void catchDeadChildren();
    void add(pid_t pid, UnixProcess *process);
    void remove(UnixProcess *process);
    void lock();
    void unlock();

private:
    QMutex mutex;
    QMap<int, UnixProcessInfo *> children;
};

Q_GLOBAL_STATIC(UnixProcessManager, processManager)

UnixProcessManager::UnixProcessManager()
{
#if defined (UNIX_PROCESS_DEBUG)
    qDebug() << "UnixProcessManager::UnixProcessManager()";
#endif
    // initialize the dead child pipe and make it non-blocking. in the
    // extremely unlikely event that the pipe fills up, we do not under any
    // circumstances want to block.
    ::pipe(qt_UnixProcess_deadChild_pipe);
    ::fcntl(qt_UnixProcess_deadChild_pipe[0], F_SETFD, FD_CLOEXEC);
    ::fcntl(qt_UnixProcess_deadChild_pipe[1], F_SETFD, FD_CLOEXEC);
    ::fcntl(qt_UnixProcess_deadChild_pipe[0], F_SETFL,
        ::fcntl(qt_UnixProcess_deadChild_pipe[0], F_GETFL) | O_NONBLOCK);
    ::fcntl(qt_UnixProcess_deadChild_pipe[1], F_SETFL,
        ::fcntl(qt_UnixProcess_deadChild_pipe[1], F_GETFL) | O_NONBLOCK);

    // set up the SIGCHLD handler, which writes a single byte to the dead
    // child pipe every time a child dies.
    struct sigaction oldAction;
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = qt_sa_sigchld_handler;
    action.sa_flags = SA_NOCLDSTOP;
    qt_native_sigaction(SIGCHLD, &action, &oldAction);
    if (oldAction.sa_handler != qt_sa_sigchld_handler)
	qt_sa_old_sigchld_handler = oldAction.sa_handler;
}

UnixProcessManager::~UnixProcessManager()
{
    // notify the thread that we're shutting down.
    qt_native_write(qt_UnixProcess_deadChild_pipe[1], "@", 1);
    qt_native_close(qt_UnixProcess_deadChild_pipe[1]);
    wait();

    // on certain unixes, closing the reading end of the pipe will cause
    // select in run() to block forever, rather than return with EBADF.
    qt_native_close(qt_UnixProcess_deadChild_pipe[0]);

    qt_UnixProcess_deadChild_pipe[0] = -1;
    qt_UnixProcess_deadChild_pipe[1] = -1;

    qDeleteAll(children.values());
    children.clear();

    struct sigaction oldAction;
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = qt_sa_old_sigchld_handler;
    action.sa_flags = SA_NOCLDSTOP;
    qt_native_sigaction(SIGCHLD, &action, &oldAction);
    if (oldAction.sa_handler != qt_sa_sigchld_handler) {
        qt_native_sigaction(SIGCHLD, &oldAction, 0);
    }
}

void UnixProcessManager::run()
{
    forever {
        fd_set readset;
        FD_ZERO(&readset);
        FD_SET(qt_UnixProcess_deadChild_pipe[0], &readset);

#if defined (UNIX_PROCESS_DEBUG)
        qDebug() << "UnixProcessManager::run() waiting for children to die";
#endif

        // block forever, or until activity is detected on the dead child
        // pipe. the only other peers are the SIGCHLD signal handler, and the
        // UnixProcessManager destructor.
        int nselect = select(qt_UnixProcess_deadChild_pipe[0] + 1, &readset, 0, 0, 0);
        if (nselect < 0) {
            if (errno == EINTR)
                continue;
            break;
        }

        // empty only one byte from the pipe, even though several SIGCHLD
        // signals may have been delivered in the meantime, to avoid race
        // conditions.
        char c;
        if (qt_native_read(qt_UnixProcess_deadChild_pipe[0], &c, 1) < 0 || c == '@')
            break;

        // catch any and all children that we can.
        catchDeadChildren();
    }
}

void UnixProcessManager::catchDeadChildren()
{
    QMutexLocker locker(&mutex);

    // try to catch all children whose pid we have registered, and whose
    // deathPipe is still valid (i.e, we have not already notified it).
    QMap<int, UnixProcessInfo *>::Iterator it = children.begin();
    while (it != children.end()) {
        // notify all children that they may have died. they need to run
        // waitpid() in their own thread.
        UnixProcessInfo *info = it.value();
        qt_native_write(info->deathPipe, "", 1);

#if defined (UNIX_PROCESS_DEBUG)
        qDebug() << "UnixProcessManager::run() sending death notice to" << info->process;
#endif
        ++it;
    }
}

static QBasicAtomicInt idCounter = Q_BASIC_ATOMIC_INITIALIZER(1);

void UnixProcessManager::add(pid_t pid, UnixProcess *process)
{
#if defined (UNIX_PROCESS_DEBUG)
    qDebug() << "UnixProcessManager::add() adding pid" << pid << "process" << process;
#endif

    // insert a new info structure for this process
    UnixProcessInfo *info = new UnixProcessInfo;
    info->process = process;
    info->deathPipe = process->d_func()->deathPipe[1];
    info->exitResult = 0;
    info->pid = pid;

    int serial = idCounter.fetchAndAddRelaxed(1);
    process->d_func()->serial = serial;
    children.insert(serial, info);
}

void UnixProcessManager::remove(UnixProcess *process)
{
    QMutexLocker locker(&mutex);

    int serial = process->d_func()->serial;
    UnixProcessInfo *info = children.value(serial);
    if (!info)
        return;

#if defined (UNIX_PROCESS_DEBUG)
    qDebug() << "UnixProcessManager::remove() removing pid" << info->pid << "process" << info->process;
#endif

    children.remove(serial);
    delete info;
}

void UnixProcessManager::lock()
{
    mutex.lock();
}

void UnixProcessManager::unlock()
{
    mutex.unlock();
}

static void qt_create_pipe(int *pipe)
{
    if (pipe[0] != -1)
        qt_native_close(pipe[0]);
    if (pipe[1] != -1)
        qt_native_close(pipe[1]);
#ifdef Q_OS_IRIX
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, pipe) == -1) {
        qWarning("UnixProcessPrivate::createPipe: Cannot create pipe %p: %s",
                 pipe, qPrintable(qt_error_string(errno)));
    }
#else
    if (::pipe(pipe) != 0) {
        qWarning("UnixProcessPrivate::createPipe: Cannot create pipe %p: %s",
                 pipe, qPrintable(qt_error_string(errno)));
    }
#endif
    ::fcntl(pipe[0], F_SETFD, FD_CLOEXEC);
    ::fcntl(pipe[1], F_SETFD, FD_CLOEXEC);
}

void UnixProcessPrivate::destroyPipe(int *pipe)
{
    if (pipe[1] != -1) {
        qt_native_close(pipe[1]);
        pipe[1] = -1;
    }
    if (pipe[0] != -1) {
        qt_native_close(pipe[0]);
        pipe[0] = -1;
    }
}

static int open_socket_or_file(const char *pathname, int flags, mode_t mode = 0, bool is_socket = false) {
	if (is_socket) {
		struct sockaddr_un name;
  		int sock;
  		size_t size;
        int buff_size = 0;

  		/* Create the socket. */
        sock = socket (AF_UNIX, SOCK_STREAM, 0);
  		if (sock < 0) return -1;

        setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &buff_size, sizeof(int));
  
        name.sun_family = AF_UNIX;
  		strncpy (name.sun_path, pathname, sizeof (name.sun_path));
  		name.sun_path[sizeof (name.sun_path) - 1] = '\0';
        size = strlen(name.sun_path) + sizeof(name.sun_family);
		if (connect(sock,(struct sockaddr *) &name,size) < 0) {
			close(sock);
			return -1;
		}
		
		return sock;
	}
	
	return QT_OPEN(pathname,flags,mode);
}

/*
    Create the pipes to a UnixProcessPrivate::Channel.

    This function must be called in order: stdin, stdout, stderr
*/
bool UnixProcessPrivate::createChannel(Channel &channel)
{
    Q_Q(UnixProcess);

    if (&channel == &stderrChannel && processChannelMode == UnixProcess::MergedChannels) {
        channel.pipe[0] = -1;
        channel.pipe[1] = -1;
        return true;
    }

    if (channel.type == Channel::Normal) {
        // we're piping this channel to our own process
        qt_create_pipe(channel.pipe);

        // create the socket notifiers
//        if (threadData->eventDispatcher) {
            if (&channel == &stdinChannel) {
                channel.notifier = new QSocketNotifier(channel.pipe[1],
                                                       QSocketNotifier::Write, q);
                QObject::connect(channel.notifier, SIGNAL(activated(int)),
                                 q, SLOT(_q_canWrite()));
                if (!(processFlags & UnixProcess::RawStdin)) {
                    // in raw mode we want to know when the process is ready to read from stdin
                    channel.notifier->setEnabled(false);
                }
            } else if ( &channel == &stderrChannel || !(processFlags&UnixProcess::RawStdout) ){
                channel.notifier = new QSocketNotifier(channel.pipe[0],
                                                       QSocketNotifier::Read, q);
                const char *receiver;
                if (&channel == &stdoutChannel)
                    receiver = SLOT(_q_canReadStandardOutput());
                else
                    receiver = SLOT(_q_canReadStandardError());
                QObject::connect(channel.notifier, SIGNAL(activated(int)),
                                 q, receiver);
            }
//        }

        return true;
    } else if (channel.type == Channel::Redirect) {
        // we're redirecting the channel to/from a file
        QByteArray fname = QFile::encodeName(channel.file);

        if (&channel == &stdinChannel) {
            // try to open in read-only mode
            channel.pipe[1] = -1;
            if ( (channel.pipe[0] = open_socket_or_file(fname, O_RDONLY, 0, channel.file_is_socket)) != -1)
                return true;    // success

            q->setErrorString(UnixProcess::tr("Could not open input redirection for reading"));
        } else {
            int mode = O_WRONLY | O_CREAT;
            if (channel.append)
                mode |= O_APPEND;
            else
                mode |= O_TRUNC;

            channel.pipe[0] = -1;
            if ( (channel.pipe[1] = open_socket_or_file(fname, mode, 0666, channel.file_is_socket)) != -1)
                return true; // success

            q->setErrorString(UnixProcess::tr("Could not open output redirection for writing"));
        }

        // could not open file
        processError = UnixProcess::FailedToStart;
        emit q->error(processError);
        cleanup();
        return false;
    } else {
        Q_ASSERT_X(channel.process, "UnixProcess::start", "Internal error");

        Channel *source;
        Channel *sink;

        if (channel.type == Channel::PipeSource) {
            // we are the source
            source = &channel;
            sink = &channel.process->stdinChannel;

            Q_ASSERT(source == &stdoutChannel);
            Q_ASSERT(sink->process == this && sink->type == Channel::PipeSink);
        } else {
            // we are the sink;
            source = &channel.process->stdoutChannel;
            sink = &channel;

            Q_ASSERT(sink == &stdinChannel);
            Q_ASSERT(source->process == this && source->type == Channel::PipeSource);
        }

        if (source->pipe[1] != INVALID_Q_PIPE || sink->pipe[0] != INVALID_Q_PIPE) {
            // already created, do nothing
            return true;
        } else {
            Q_ASSERT(source->pipe[0] == INVALID_Q_PIPE && source->pipe[1] == INVALID_Q_PIPE);
            Q_ASSERT(sink->pipe[0] == INVALID_Q_PIPE && sink->pipe[1] == INVALID_Q_PIPE);

            Q_PIPE pipe[2] = { -1, -1 };
            qt_create_pipe(pipe);
            sink->pipe[0] = pipe[0];
            source->pipe[1] = pipe[1];

            return true;
        }
    }
}

static char **_q_dupEnvironment(const QStringList &environment, int *envc)
{
    // if LD_LIBRARY_PATH exists in the current environment, but
    // not in the environment list passed by the programmer, then
    // copy it over.
#if defined(Q_OS_MAC)
    static const char libraryPath[] = "DYLD_LIBRARY_PATH";
#else
    static const char libraryPath[] = "LD_LIBRARY_PATH";
#endif
    const QString libraryPathString = QLatin1String(libraryPath);
    QStringList env = environment;
    QStringList matches = env.filter(
        QRegExp(QLatin1Char('^') + libraryPathString + QLatin1Char('=')));
    const QString envLibraryPath = QString::fromLocal8Bit(::getenv(libraryPath));
    if (matches.isEmpty() && !envLibraryPath.isEmpty()) {
        QString entry = libraryPathString;
        entry += QLatin1Char('=');
        entry += envLibraryPath;
        env << libraryPathString + QLatin1Char('=') + envLibraryPath;
    }

    char **envp = new char *[env.count() + 1];
    envp[env.count()] = 0;

    for (int j = 0; j < env.count(); ++j) {
        QString item = env.at(j);
        envp[j] = ::strdup(item.toLocal8Bit().constData());
    }

    *envc = env.count();
    return envp;
}

// under QNX RTOS we have to use vfork() when multithreading
inline pid_t qt_fork()
{
#if defined(Q_OS_QNX)
    return vfork();
#else
    return fork();
#endif
}

#ifdef Q_OS_MAC
Q_GLOBAL_STATIC(QMutex, cfbundleMutex);
#endif

void UnixProcessPrivate::Channel::clear()
{
    switch (type) {
    case PipeSource:
        Q_ASSERT(process);
        process->stdinChannel.type = Normal;
        process->stdinChannel.process = 0;
        break;
    case PipeSink:
        Q_ASSERT(process);
        process->stdoutChannel.type = Normal;
        process->stdoutChannel.process = 0;
        break;
    }

    type = Normal;
    file.clear();
    process = 0;
}

/*! \internal
*/
UnixProcessPrivate::UnixProcessPrivate()
{
    processChannel = UnixProcess::StandardOutput;
    processChannelMode = UnixProcess::SeparateChannels;
    processError = UnixProcess::UnknownError;
    processState = UnixProcess::NotRunning;
    pid = 0;
    sequenceNumber = 0;
    exitCode = 0;
    exitStatus = UnixProcess::NormalExit;
    startupSocketNotifier = 0;
    deathNotifier = 0;
    notifier = 0;
    pipeWriter = 0;
    childStartedPipe[0] = INVALID_Q_PIPE;
    childStartedPipe[1] = INVALID_Q_PIPE;
    deathPipe[0] = INVALID_Q_PIPE;
    deathPipe[1] = INVALID_Q_PIPE;
    exitCode = 0;
    crashed = false;
    dying = false;
    emittedReadyRead = false;
    emittedBytesWritten = false;
#ifdef Q_OS_UNIX
    serial = 0;
#endif
}

/*! \internal
*/
UnixProcessPrivate::~UnixProcessPrivate()
{
    if (stdinChannel.process)
        stdinChannel.process->stdoutChannel.clear();
    if (stdoutChannel.process)
        stdoutChannel.process->stdinChannel.clear();
}

/*! \internal
*/

void qDeleteInEventHandler(QObject *o);

void UnixProcessPrivate::cleanup()
{
    q_func()->setProcessState(UnixProcess::NotRunning);
    pid = 0;
    sequenceNumber = 0;
    dying = false;

    if (stdoutChannel.notifier) {
        stdoutChannel.notifier->setEnabled(false);
        delete stdoutChannel.notifier;
        stdoutChannel.notifier = 0;
    }
    if (stderrChannel.notifier) {
        stderrChannel.notifier->setEnabled(false);
        delete stderrChannel.notifier;
        stderrChannel.notifier = 0;
    }
    if (stdinChannel.notifier) {
        stdinChannel.notifier->setEnabled(false);
        delete stdinChannel.notifier;
        stdinChannel.notifier = 0;
    }
    if (startupSocketNotifier) {
        startupSocketNotifier->setEnabled(false);
        delete startupSocketNotifier;
        startupSocketNotifier = 0;
    }
    if (deathNotifier) {
        deathNotifier->setEnabled(false);
        delete deathNotifier;
        deathNotifier = 0;
    }
    if (notifier) {
        delete notifier;
        notifier = 0;
    }
    destroyPipe(stdoutChannel.pipe);
    destroyPipe(stderrChannel.pipe);
    destroyPipe(stdinChannel.pipe);
    destroyPipe(childStartedPipe);
    destroyPipe(deathPipe);
#ifdef Q_OS_UNIX
    serial = 0;
#endif
}

/*! \internal
*/
bool UnixProcessPrivate::_q_canReadStandardOutput()
{
    Q_Q(UnixProcess);
    qint64 available = bytesAvailableFromStdout();
    if (available == 0) {
        if (stdoutChannel.notifier)
            stdoutChannel.notifier->setEnabled(false);
        destroyPipe(stdoutChannel.pipe);
#if defined UNIX_PROCESS_DEBUG
        qDebug("UnixProcessPrivate::canReadStandardOutput(), 0 bytes available");
#endif
        return false;
    }

    if (!(processFlags & UnixProcess::RawStdout)) {
        char *ptr = outputReadBuffer.reserve(available);
        qint64 readBytes = readFromStdout(ptr, available);
        if (readBytes == -1) {
            processError = UnixProcess::ReadError;
            q->setErrorString(UnixProcess::tr("Error reading from process"));
            emit q->error(processError);
#if defined UNIX_PROCESS_DEBUG
            qDebug("UnixProcessPrivate::canReadStandardOutput(), failed to read from the process");
#endif
            return false;
        }
#if defined UNIX_PROCESS_DEBUG
        qDebug("UnixProcessPrivate::canReadStandardOutput(), read %d bytes from the process' output",
               int(readBytes));
#endif

        if (stdoutChannel.closed) {
            outputReadBuffer.chop(readBytes);
            return false;
        }

        outputReadBuffer.chop(available - readBytes);

        bool didRead = false;
        if (readBytes == 0) {
            if (stdoutChannel.notifier)
                stdoutChannel.notifier->setEnabled(false);
        } else if (processChannel == UnixProcess::StandardOutput) {
            didRead = true;
            if (!emittedReadyRead) {
                emittedReadyRead = true;
                emit q->readyRead();
                emittedReadyRead = false;
            }
        }
        emit q->readyReadStandardOutput();
        return didRead;
    }
    else {
        if (!emittedReadyRead) {
            emittedReadyRead = true;
            emit q->readyRead();
            emittedReadyRead = false;
        }
        emit q->readyReadStandardOutput();
        return true;
    }
}

/*! \internal
*/
bool UnixProcessPrivate::_q_canReadStandardError()
{
    Q_Q(UnixProcess);
    qint64 available = bytesAvailableFromStderr();
    if (available == 0) {
        if (stderrChannel.notifier)
            stderrChannel.notifier->setEnabled(false);
        destroyPipe(stderrChannel.pipe);
        return false;
    }

    char *ptr = errorReadBuffer.reserve(available);
    qint64 readBytes = readFromStderr(ptr, available);
    if (readBytes == -1) {
        processError = UnixProcess::ReadError;
        q->setErrorString(UnixProcess::tr("Error reading from process"));
        emit q->error(processError);
        return false;
    }
    if (stderrChannel.closed) {
        errorReadBuffer.chop(readBytes);
        return false;
    }

    errorReadBuffer.chop(available - readBytes);

    bool didRead = false;
    if (readBytes == 0) {
        if (stderrChannel.notifier)
            stderrChannel.notifier->setEnabled(false);
    } else if (processChannel == UnixProcess::StandardError) {
        didRead = true;
        if (!emittedReadyRead) {
            emittedReadyRead = true;
            emit q->readyRead();
            emittedReadyRead = false;
        }
    }
    emit q->readyReadStandardError();
    return didRead;
}

/*! \internal
*/
bool UnixProcessPrivate::_q_canWrite()
{
    Q_Q(UnixProcess);
    if (processFlags & UnixProcess::RawStdin) {
        if (stdinChannel.notifier)
            stdinChannel.notifier->setEnabled(false);
        isReadyWrite = true;
        emit q->readyWrite();
    }
    else {
        if (stdinChannel.notifier)
            stdinChannel.notifier->setEnabled(false);

        if (writeBuffer.isEmpty()) {
#if defined UNIX_PROCESS_DEBUG
            qDebug("UnixProcessPrivate::canWrite(), not writing anything (empty write buffer).");
#endif
            return false;
        }

        qint64 written = writeToStdin(writeBuffer.rawData(),
                                      writeBuffer.size());
        if (written < 0) {
            destroyPipe(stdinChannel.pipe);
            processError = UnixProcess::WriteError;
            q->setErrorString(UnixProcess::tr("Error writing to process"));
#if defined(UNIX_PROCESS_DEBUG)
            qDebug("UnixProcessPrivate::canWrite(), failed to write (%s)", strerror(errno));
#endif
            emit q->error(processError);
            return false;
        }

#if defined UNIX_PROCESS_DEBUG
        qDebug("UnixProcessPrivate::canWrite(), wrote %d bytes to the process input", int(written));
#endif

        writeBuffer.free(written);
        if (!emittedBytesWritten) {
            emittedBytesWritten = true;
            emit q->bytesWritten(written);
            emittedBytesWritten = false;
        }
        if (stdinChannel.notifier && !writeBuffer.isEmpty())
            stdinChannel.notifier->setEnabled(true);
        if (writeBuffer.isEmpty() && stdinChannel.closed)
            closeWriteChannel();
    }
    return true;
}

/*! \internal
*/
bool UnixProcessPrivate::_q_processDied()
{
#if defined UNIX_PROCESS_DEBUG
    qDebug("UnixProcessPrivate::_q_processDied()");
#endif
#ifdef Q_OS_UNIX
    if (!waitForDeadChild())
        return false;
#endif

    // the process may have died before it got a chance to report that it was
    // either running or stopped, so we will call _q_startupNotification() and
    // give it a chance to emit started() or error(FailedToStart).
    if (processState == UnixProcess::Starting) {
        if (!_q_startupNotification())
            return true;
    }

    return _q_notifyProcessDied();
}

bool UnixProcessPrivate::_q_notifyProcessDied()
{
    Q_Q(UnixProcess);
#if defined UNIX_PROCESS_DEBUG
    qDebug("UnixProcessPrivate::_q_notifyProcessDied()");
#endif

    if ( processFlags&UnixProcess::RawStdout ) {
        qint64 bytes = bytesAvailableFromStdout();
#if defined UNIX_PROCESS_DEBUG
        qDebug() << "bytesAvailableFromStdout:" << bytes;
#endif
        // wait for all data to be read
        if ( bytes > 0 ) {
            QMetaObject::invokeMethod( q, "_q_notifyProcessDied", Qt::QueuedConnection );
            return false;
        }
    }

    if (dying) {
        // at this point we know the process is dead. prevent
        // reentering this slot recursively by calling waitForFinished()
        // or opening a dialog inside slots connected to the readyRead
        // signals emitted below.
        return true;
    }
    dying = true;

    // in case there is data in the pipe line and this slot by chance
    // got called before the read notifications, call these two slots
    // so the data is made available before the process dies.
    if ( !processFlags.testFlag( UnixProcess::RawStdout ) ) {
        _q_canReadStandardOutput();
    }
    _q_canReadStandardError();
    findExitCode();

    if (crashed) {
        exitStatus = UnixProcess::CrashExit;
        processError = UnixProcess::Crashed;
        q->setErrorString(UnixProcess::tr("Process crashed"));
        emit q->error(processError);
    }

    bool wasRunning = (processState == UnixProcess::Running);

    cleanup();

    if (wasRunning) {
        // we received EOF now:
        emit q->readChannelFinished();
        // in the future:
        //emit q->standardOutputClosed();
        //emit q->standardErrorClosed();

        emit q->finished(exitCode);
        emit q->finished(exitCode, exitStatus);
    }
#if defined UNIX_PROCESS_DEBUG
    qDebug("UnixProcessPrivate::_q_notifyProcessDied() process is dead");
#endif
    return true;
}

/*! \internal
*/
bool UnixProcessPrivate::_q_startupNotification()
{
    Q_Q(UnixProcess);
#if defined UNIX_PROCESS_DEBUG
    qDebug("UnixProcessPrivate::startupNotification()");
#endif

    if (startupSocketNotifier)
        startupSocketNotifier->setEnabled(false);
    if (processStarted()) {
        q->setProcessState(UnixProcess::Running);
        emit q->started();
        return true;
    }

    q->setProcessState(UnixProcess::NotRunning);
    processError = UnixProcess::FailedToStart;
    emit q->error(processError);
#ifdef Q_OS_UNIX
    // make sure the process manager removes this entry
    waitForDeadChild();
    findExitCode();
#endif
    cleanup();
    return false;
}

/*! \internal
*/
void UnixProcessPrivate::closeWriteChannel()
{
#if defined UNIX_PROCESS_DEBUG
    qDebug("UnixProcessPrivate::closeWriteChannel()");
#endif
    if (stdinChannel.notifier) {
        stdinChannel.notifier->setEnabled(false);
        if (stdinChannel.notifier) {
            delete stdinChannel.notifier;
            stdinChannel.notifier = 0;
        }
    }
    destroyPipe(stdinChannel.pipe);
}

qint64 UnixProcessPrivate::readData( char *data, qint64 maxlen, UnixProcess::ProcessChannel channel )
{
    if (processFlags&UnixProcess::RawStdout &&
        channel == UnixProcess::StandardOutput) {
        return readFromStdout(data, maxlen);
    }
    else {
        SequentialBuffer *readBuffer = (channel == UnixProcess::StandardError)
                                       ? &errorReadBuffer
                                       : &outputReadBuffer;

        qint64 readSoFar = readBuffer->read(data,maxlen);

#if defined UNIX_PROCESS_DEBUG
        qDebug("UnixProcess::readData(%p \"%s\", %lld) == %lld",
               data, qt_prettyDebug(data, readSoFar, 16).constData(), maxlen, readSoFar);
#endif
        if (!readSoFar && processState == UnixProcess::NotRunning)
            return -1;              // EOF
        return readSoFar;
    }
}

void UnixProcessPrivate::startProcess()
{
    Q_Q(UnixProcess);

#if defined (UNIX_PROCESS_DEBUG)
    qDebug("UnixProcessPrivate::startProcess()");
#endif

    processManager()->start();

    // Initialize pipes
    qt_create_pipe(childStartedPipe);
//    if (threadData->eventDispatcher) {
        startupSocketNotifier = new QSocketNotifier(childStartedPipe[0],
                                                    QSocketNotifier::Read, q);
        QObject::connect(startupSocketNotifier, SIGNAL(activated(int)),
                         q, SLOT(_q_startupNotification()));
//    }

    qt_create_pipe(deathPipe);
    ::fcntl(deathPipe[0], F_SETFD, FD_CLOEXEC);
    ::fcntl(deathPipe[1], F_SETFD, FD_CLOEXEC);
//    if (threadData->eventDispatcher) {
        deathNotifier = new QSocketNotifier(deathPipe[0],
                                            QSocketNotifier::Read, q);
        QObject::connect(deathNotifier, SIGNAL(activated(int)),
                         q, SLOT(_q_processDied()));
//    }

    if (!createChannel(stdinChannel) ||
        !createChannel(stdoutChannel) ||
        !createChannel(stderrChannel))
        return;

    // Start the process (platform dependent)
    q->setProcessState(UnixProcess::Starting);

    // Create argument list with right number of elements, and set the final
    // one to 0.
    char **argv = new char *[arguments.count() + 2];
    argv[arguments.count() + 1] = 0;

    // Encode the program name.
    QByteArray encodedProgramName = QFile::encodeName(program);
#ifdef Q_OS_MAC
    // allow invoking of .app bundles on the Mac.
    QFileInfo fileInfo(QString::fromUtf8(encodedProgramName.constData()));
    if (encodedProgramName.endsWith(".app") && fileInfo.isDir()) {
        QCFType<CFURLRef> url = CFURLCreateWithFileSystemPath(0,
                                                          QCFString(fileInfo.absoluteFilePath()),
                                                          kCFURLPOSIXPathStyle, true);
        {
            // CFBundle is not reentrant, since CFBundleCreate might return a reference
            // to a cached bundle object. Protect the bundle calls with a mutex lock.
            QMutexLocker lock(cfbundleMutex());
            QCFType<CFBundleRef> bundle = CFBundleCreate(0, url);
            url = CFBundleCopyExecutableURL(bundle);
        }
        if (url) {
            QCFString str = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
            encodedProgramName += "/Contents/MacOS/" + static_cast<QString>(str).toUtf8();
        }
    }
#endif

    // Add the program name to the argument list.
    char *dupProgramName = ::strdup(encodedProgramName.constData());
    argv[0] = dupProgramName;

    // Add every argument to the list
    for (int i = 0; i < arguments.count(); ++i) {
        QString arg = arguments.at(i);
#ifdef Q_OS_MAC
        // Mac OS X uses UTF8 for exec, regardless of the system locale.
        argv[i + 1] = ::strdup(arg.toUtf8().constData());
#else
        argv[i + 1] = ::strdup(arg.toLocal8Bit().constData());
#endif
    }

    // Duplicate the environment.
    int envc = 0;
    char **envp = _q_dupEnvironment(environment, &envc);

    // Encode the working directory if it's non-empty, otherwise just pass 0.
    const char *workingDirPtr = 0;
    QByteArray encodedWorkingDirectory;
    if (!workingDirectory.isEmpty()) {
        encodedWorkingDirectory = QFile::encodeName(workingDirectory);
        workingDirPtr = encodedWorkingDirectory.constData();
    }

    // If the program does not specify a path, generate a list of possible
    // locations for the binary using the PATH environment variable.
    char **path = 0;
    int pathc = 0;
    if (!program.contains(QLatin1Char('/'))) {
        const QString pathEnv = QString::fromLocal8Bit(::getenv("PATH"));
        if (!pathEnv.isEmpty()) {
            QStringList pathEntries = pathEnv.split(QLatin1Char(':'), QString::SkipEmptyParts);
            if (!pathEntries.isEmpty()) {
                pathc = pathEntries.size();
                path = new char *[pathc + 1];
                path[pathc] = 0;

                for (int k = 0; k < pathEntries.size(); ++k) {
                    QByteArray tmp = QFile::encodeName(pathEntries.at(k));
                    if (!tmp.endsWith('/')) tmp += '/';
                    tmp += encodedProgramName;
                    path[k] = ::strdup(tmp.constData());
                }
            }
        }
    }

    // Start the process manager, and fork off the child process.
    processManager()->lock();
    pid_t childPid = qt_fork();
    int lastForkErrno = errno;
    if (childPid != 0) {
        // Clean up duplicated memory.
        free(dupProgramName);
        for (int i = 1; i <= arguments.count(); ++i)
            free(argv[i]);
        for (int i = 0; i < envc; ++i)
            free(envp[i]);
        for (int i = 0; i < pathc; ++i)
            free(path[i]);
        delete [] argv;
        delete [] envp;
        delete [] path;
    }
    if (childPid < 0) {
        // Cleanup, report error and return
#if defined (UNIX_PROCESS_DEBUG)
        qDebug("qt_fork failed: %s", qt_error_string(lastForkErrno));
#endif
        processManager()->unlock();
        q->setProcessState(UnixProcess::NotRunning);
        processError = UnixProcess::FailedToStart;
        q->setErrorString(UnixProcess::tr("Resource error (fork failure): %1").arg(qt_error_string(lastForkErrno)));
        emit q->error(processError);
        cleanup();
        return;
    }

    // Start the child.
    if (childPid == 0) {
        execChild(workingDirPtr, path, argv, envp);
        ::_exit(-1);
    }

    // Register the child. In the mean time, we can get a SIGCHLD, so we need
    // to keep the lock held to avoid a race to catch the child.
    processManager()->add(childPid, q);
    pid = Q_PID(childPid);
    processManager()->unlock();

    // parent
    // close the ends we don't use and make all pipes non-blocking
    ::fcntl(deathPipe[0], F_SETFL, ::fcntl(deathPipe[0], F_GETFL) | O_NONBLOCK);
    qt_native_close(childStartedPipe[1]);
    childStartedPipe[1] = -1;

    if (stdinChannel.pipe[0] != -1) {
        qt_native_close(stdinChannel.pipe[0]);
        stdinChannel.pipe[0] = -1;
    }

    if (stdinChannel.pipe[1] != -1 && !(processFlags&UnixProcess::RawStdin))
        ::fcntl(stdinChannel.pipe[1], F_SETFL, ::fcntl(stdinChannel.pipe[1], F_GETFL) | O_NONBLOCK);

    if (stdoutChannel.pipe[1] != -1) {
        qt_native_close(stdoutChannel.pipe[1]);
        stdoutChannel.pipe[1] = -1;
    }

    if (stdoutChannel.pipe[0] != -1 && !(processFlags&UnixProcess::RawStdout))
        ::fcntl(stdoutChannel.pipe[0], F_SETFL, ::fcntl(stdoutChannel.pipe[0], F_GETFL) | O_NONBLOCK);

    if (stderrChannel.pipe[1] != -1) {
        qt_native_close(stderrChannel.pipe[1]);
        stderrChannel.pipe[1] = -1;
    }
    if (stderrChannel.pipe[0] != -1)
        ::fcntl(stderrChannel.pipe[0], F_SETFL, ::fcntl(stderrChannel.pipe[0], F_GETFL) | O_NONBLOCK);
}

void UnixProcessPrivate::execChild(const char *workingDir, char **path, char **argv, char **envp)
{
    ::signal(SIGPIPE, SIG_DFL);         // reset the signal that we ignored

    Q_Q(UnixProcess);

    // copy the stdin socket
    qt_native_dup2(stdinChannel.pipe[0], fileno(stdin));

    // copy the stdout and stderr if asked to
    if (processChannelMode != UnixProcess::ForwardedChannels) {
        qt_native_dup2(stdoutChannel.pipe[1], fileno(stdout));

        // merge stdout and stderr if asked to
        if (processChannelMode == UnixProcess::MergedChannels) {
            qt_native_dup2(fileno(stdout), fileno(stderr));
        } else {
            qt_native_dup2(stderrChannel.pipe[1], fileno(stderr));
        }
    }

    // make sure this fd is closed if execvp() succeeds
    qt_native_close(childStartedPipe[0]);
    ::fcntl(childStartedPipe[1], F_SETFD, FD_CLOEXEC);

    // enter the working directory
    if (workingDir)
        qt_native_chdir(workingDir);

    // this is a virtual call, and it base behavior is to do nothing.
    q->setupChildProcess();

    // execute the process
    if (environment.isEmpty()) {
        qt_native_execvp(argv[0], argv);
    } else {
        if (path) {
            char **arg = path;
            while (*arg) {
                argv[0] = *arg;
#if defined (UNIX_PROCESS_DEBUG)
                fprintf(stderr, "UnixProcessPrivate::execChild() searching / starting %s\n", argv[0]);
#endif
                qt_native_execve(argv[0], argv, envp);
                ++arg;
            }
        } else {
#if defined (UNIX_PROCESS_DEBUG)
            fprintf(stderr, "UnixProcessPrivate::execChild() starting %s\n", argv[0]);
#endif
            qt_native_execve(argv[0], argv, envp);
        }
    }

    // notify failure
#if defined (UNIX_PROCESS_DEBUG)
    fprintf(stderr, "UnixProcessPrivate::execChild() failed, notifying parent process\n");
#endif
    qt_native_write(childStartedPipe[1], "", 1);
    qt_native_close(childStartedPipe[1]);
    childStartedPipe[1] = -1;
}

bool UnixProcessPrivate::processStarted()
{
    char c;
    int i = qt_native_read(childStartedPipe[0], &c, 1);
    if (startupSocketNotifier) {
        startupSocketNotifier->setEnabled(false);
        startupSocketNotifier->deleteLater();
        startupSocketNotifier = 0;
    }
    qt_native_close(childStartedPipe[0]);
    childStartedPipe[0] = -1;

#if defined (UNIX_PROCESS_DEBUG)
    qDebug("UnixProcessPrivate::processStarted() == %s", i <= 0 ? "true" : "false");
#endif
    return i <= 0;
}

qint64 UnixProcessPrivate::bytesAvailableFromStdout() const
{
    size_t nbytes = 0;
    qint64 available = 0;
    if (::ioctl(stdoutChannel.pipe[0], FIONREAD, (char *) &nbytes) >= 0)
        available = (qint64) nbytes;
#if defined (UNIX_PROCESS_DEBUG)
    qDebug("UnixProcessPrivate::bytesAvailableFromStdout() == %lld", available);
#endif
    return available;
}

qint64 UnixProcessPrivate::bytesAvailableFromStderr() const
{
    size_t nbytes = 0;
    qint64 available = 0;
    if (::ioctl(stderrChannel.pipe[0], FIONREAD, (char *) &nbytes) >= 0)
        available = (qint64) nbytes;
#if defined (UNIX_PROCESS_DEBUG)
    qDebug("UnixProcessPrivate::bytesAvailableFromStderr() == %lld", available);
#endif
    return available;
}

qint64 UnixProcessPrivate::readFromStdout(char *data, qint64 maxlen)
{
    qint64 bytesRead = qt_native_read(stdoutChannel.pipe[0], data, maxlen);
#if defined UNIX_PROCESS_DEBUG
    qDebug("UnixProcessPrivate::readFromStdout(%p \"%s\", %lld) == %lld",
           data, qt_prettyDebug(data, bytesRead, 16).constData(), maxlen, bytesRead);
#endif
    return bytesRead;
}

qint64 UnixProcessPrivate::readFromStderr(char *data, qint64 maxlen)
{
    qint64 bytesRead = qt_native_read(stderrChannel.pipe[0], data, maxlen);
#if defined UNIX_PROCESS_DEBUG
    qDebug("UnixProcessPrivate::readFromStderr(%p \"%s\", %lld) == %lld",
           data, qt_prettyDebug(data, bytesRead, 16).constData(), maxlen, bytesRead);
#endif
    return bytesRead;
}

static void qt_ignore_sigpipe()
{
    // Set to ignore SIGPIPE once only.
    static QBasicAtomicInt atom = Q_BASIC_ATOMIC_INITIALIZER(0);
    if (atom.testAndSetRelaxed(0, 1)) {
        struct sigaction noaction;
        memset(&noaction, 0, sizeof(noaction));
        noaction.sa_handler = SIG_IGN;
        qt_native_sigaction(SIGPIPE, &noaction, 0);
    }
}

qint64 UnixProcessPrivate::writeToStdin(const char *data, qint64 maxlen)
{
    qt_ignore_sigpipe();

    qint64 written = qt_native_write(stdinChannel.pipe[1], data, maxlen);
#if defined UNIX_PROCESS_DEBUG
    qDebug("UnixProcessPrivate::writeToStdin(%p \"%s\", %lld) == %lld",
           data, qt_prettyDebug(data, maxlen, 16).constData(), maxlen, written);
#endif
    return written;
}

void UnixProcessPrivate::terminateProcess()
{
#if defined (UNIX_PROCESS_DEBUG)
    qDebug("UnixProcessPrivate::killProcess()");
#endif
    if (pid)
        ::kill(pid_t(pid), SIGTERM);
}

void UnixProcessPrivate::killProcess()
{
#if defined (UNIX_PROCESS_DEBUG)
    qDebug("UnixProcessPrivate::killProcess()");
#endif
    if (pid)
        ::kill(pid_t(pid), SIGKILL);
}

bool UnixProcessPrivate::waitForStarted(int msecs)
{
    Q_Q(UnixProcess);

#if defined (UNIX_PROCESS_DEBUG)
    qDebug("UnixProcessPrivate::waitForStarted(%d) waiting for child to start (fd = %d)", msecs,
	   childStartedPipe[0]);
#endif

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(childStartedPipe[0], &fds);
    int ret;
    do {
        ret = qt_native_select(&fds, 0, msecs);
    } while (ret < 0 && errno == EINTR);
    if (ret == 0) {
        processError = UnixProcess::Timedout;
        q->setErrorString(UnixProcess::tr("Process operation timed out"));
#if defined (UNIX_PROCESS_DEBUG)
        qDebug("UnixProcessPrivate::waitForStarted(%d) == false (timed out)", msecs);
#endif
        return false;
    }

    bool startedEmitted = _q_startupNotification();
#if defined (UNIX_PROCESS_DEBUG)
    qDebug("UnixProcessPrivate::waitForStarted() == %s", startedEmitted ? "true" : "false");
#endif
    return startedEmitted;
}

bool UnixProcessPrivate::waitForReadyRead(int msecs)
{
    Q_Q(UnixProcess);
#if defined (UNIX_PROCESS_DEBUG)
    qDebug("UnixProcessPrivate::waitForReadyRead(%d)", msecs);
#endif

    QTime stopWatch;
    stopWatch.start();

    forever {
        fd_set fdread;
        fd_set fdwrite;

        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);

        if (processState == UnixProcess::Starting)
            FD_SET(childStartedPipe[0], &fdread);

        if (stdoutChannel.pipe[0] != -1)
            FD_SET(stdoutChannel.pipe[0], &fdread);
        if (stderrChannel.pipe[0] != -1)
            FD_SET(stderrChannel.pipe[0], &fdread);

        FD_SET(deathPipe[0], &fdread);

        if (!writeBuffer.isEmpty() && stdinChannel.pipe[1] != -1)
            FD_SET(stdinChannel.pipe[1], &fdwrite);

        int timeout = qt_timeout_value(msecs, stopWatch.elapsed());
        int ret = qt_native_select(&fdread, &fdwrite, timeout);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            break;
        }
        if (ret == 0) {
            processError = UnixProcess::Timedout;
            q->setErrorString(UnixProcess::tr("Process operation timed out"));
            return false;
        }

        if (childStartedPipe[0] != -1 && FD_ISSET(childStartedPipe[0], &fdread)) {
            if (!_q_startupNotification())
                return false;
        }

        bool readyReadEmitted = false;
        if (stdoutChannel.pipe[0] != -1 && FD_ISSET(stdoutChannel.pipe[0], &fdread)) {
            bool canRead = _q_canReadStandardOutput();
            if (processChannel == UnixProcess::StandardOutput && canRead)
                readyReadEmitted = true;
        }
        if (stderrChannel.pipe[0] != -1 && FD_ISSET(stderrChannel.pipe[0], &fdread)) {
            bool canRead = _q_canReadStandardError();
            if (processChannel == UnixProcess::StandardError && canRead)
                readyReadEmitted = true;
        }
        if (readyReadEmitted)
            return true;

        if (stdinChannel.pipe[1] != -1 && FD_ISSET(stdinChannel.pipe[1], &fdwrite))
            _q_canWrite();

        if (deathPipe[0] == -1 || FD_ISSET(deathPipe[0], &fdread)) {
            if (_q_processDied())
                return false;
        }
    }
    return false;
}

bool UnixProcessPrivate::waitForBytesWritten(int msecs)
{
    Q_Q(UnixProcess);
#if defined (UNIX_PROCESS_DEBUG)
    qDebug("UnixProcessPrivate::waitForBytesWritten(%d)", msecs);
#endif

    if (processFlags&UnixProcess::RawStdin)
        return true;

    QTime stopWatch;
    stopWatch.start();

    while (!writeBuffer.isEmpty()) {
        fd_set fdread;
        fd_set fdwrite;

        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);

        if (processState == UnixProcess::Starting)
            FD_SET(childStartedPipe[0], &fdread);

        if (stdoutChannel.pipe[0] != -1)
            FD_SET(stdoutChannel.pipe[0], &fdread);
        if (stderrChannel.pipe[0] != -1)
            FD_SET(stderrChannel.pipe[0], &fdread);

        FD_SET(deathPipe[0], &fdread);

        if (!writeBuffer.isEmpty() && stdinChannel.pipe[1] != -1)
            FD_SET(stdinChannel.pipe[1], &fdwrite);

	int timeout = qt_timeout_value(msecs, stopWatch.elapsed());
	int ret = qt_native_select(&fdread, &fdwrite, timeout);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            break;
        }

        if (ret == 0) {
	    processError = UnixProcess::Timedout;
        q->setErrorString(UnixProcess::tr("Process operation timed out"));
	    return false;
	}

	if (childStartedPipe[0] != -1 && FD_ISSET(childStartedPipe[0], &fdread)) {
	    if (!_q_startupNotification())
		return false;
	}

	if (stdinChannel.pipe[1] != -1 && FD_ISSET(stdinChannel.pipe[1], &fdwrite))
	    return _q_canWrite();

	if (stdoutChannel.pipe[0] != -1 && FD_ISSET(stdoutChannel.pipe[0], &fdread))
	    _q_canReadStandardOutput();

	if (stderrChannel.pipe[0] != -1 && FD_ISSET(stderrChannel.pipe[0], &fdread))
	    _q_canReadStandardError();

	if (deathPipe[0] == -1 || FD_ISSET(deathPipe[0], &fdread)) {
            if (_q_processDied())
                return false;
        }
    }

    return false;
}

bool UnixProcessPrivate::waitForFinished(int msecs)
{
    Q_Q(UnixProcess);
#if defined (UNIX_PROCESS_DEBUG)
    qDebug("UnixProcessPrivate::waitForFinished(%d)", msecs);
#endif

    QTime stopWatch;
    stopWatch.start();

    forever {
        fd_set fdread;
        fd_set fdwrite;

        FD_ZERO(&fdread);
        FD_ZERO(&fdwrite);

        if (processState == UnixProcess::Starting)
            FD_SET(childStartedPipe[0], &fdread);

        if (stdoutChannel.pipe[0] != -1)
            FD_SET(stdoutChannel.pipe[0], &fdread);
        if (stderrChannel.pipe[0] != -1)
            FD_SET(stderrChannel.pipe[0], &fdread);

        if (processState == UnixProcess::Running)
            FD_SET(deathPipe[0], &fdread);

        if (!writeBuffer.isEmpty() && stdinChannel.pipe[1] != -1)
            FD_SET(stdinChannel.pipe[1], &fdwrite);

	int timeout = qt_timeout_value(msecs, stopWatch.elapsed());
	int ret = qt_native_select(&fdread, &fdwrite, timeout);
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            break;
        }
	if (ret == 0) {
	    processError = UnixProcess::Timedout;
        q->setErrorString(UnixProcess::tr("Process operation timed out"));
	    return false;
	}

	if (childStartedPipe[0] != -1 && FD_ISSET(childStartedPipe[0], &fdread)) {
	    if (!_q_startupNotification())
		return false;
	}
	if (stdinChannel.pipe[1] != -1 && FD_ISSET(stdinChannel.pipe[1], &fdwrite))
	    _q_canWrite();

	if (stdoutChannel.pipe[0] != -1 && FD_ISSET(stdoutChannel.pipe[0], &fdread))
	    _q_canReadStandardOutput();

	if (stderrChannel.pipe[0] != -1 && FD_ISSET(stderrChannel.pipe[0], &fdread))
	    _q_canReadStandardError();

	if (deathPipe[0] == -1 || FD_ISSET(deathPipe[0], &fdread)) {
            if (_q_processDied())
                return true;
	}
    }
    return false;
}

bool UnixProcessPrivate::waitForWrite(int msecs)
{
    fd_set fdwrite;
    FD_ZERO(&fdwrite);
    FD_SET(stdinChannel.pipe[1], &fdwrite);

    int ret;
    do {
        ret = qt_native_select(0, &fdwrite, msecs < 0 ? 0 : msecs) == 1;
    } while (ret < 0 && errno == EINTR);
    return ret == 1;
}

void UnixProcessPrivate::findExitCode()
{
    Q_Q(UnixProcess);
    processManager()->remove(q);
}

bool UnixProcessPrivate::waitForDeadChild()
{
    Q_Q(UnixProcess);

    // read a byte from the death pipe
    char c;
    qt_native_read(deathPipe[0], &c, 1);

    // check if our process is dead
    int exitStatus;
    pid_t waitResult = 0;
    do {
        waitResult = waitpid(pid_t(pid), &exitStatus, WNOHANG);
    } while ((waitResult == -1 && errno == EINTR));
    if (waitResult > 0) {
        processManager()->remove(q);
        crashed = !WIFEXITED(exitStatus);
        exitCode = WEXITSTATUS(exitStatus);
#if defined UNIX_PROCESS_DEBUG
        qDebug() << "UnixProcessPrivate::waitForDeadChild() dead with exitCode"
                 << exitCode << ", crashed?" << crashed;
#endif
        return true;
    }
#if defined UNIX_PROCESS_DEBUG
    qDebug() << "UnixProcessPrivate::waitForDeadChild() not dead!";
#endif
    return false;
}

void UnixProcessPrivate::_q_notified()
{
}

/*! \internal
 */
bool UnixProcessPrivate::startDetached(const QString &program, const QStringList &arguments, const QString &workingDirectory, qint64 *pid)
{
    processManager()->start();

    QByteArray encodedWorkingDirectory = QFile::encodeName(workingDirectory);

    // To catch the startup of the child
    int startedPipe[2];
    ::pipe(startedPipe);
    // To communicate the pid of the child
    int pidPipe[2];
    ::pipe(pidPipe);

    pid_t childPid = qt_fork();
    if (childPid == 0) {
        struct sigaction noaction;
        memset(&noaction, 0, sizeof(noaction));
        noaction.sa_handler = SIG_IGN;
        qt_native_sigaction(SIGPIPE, &noaction, 0);

        ::setsid();

        qt_native_close(startedPipe[0]);
        qt_native_close(pidPipe[0]);

        pid_t doubleForkPid = qt_fork();
        if (doubleForkPid == 0) {
            ::fcntl(startedPipe[1], F_SETFD, FD_CLOEXEC);
            qt_native_close(pidPipe[1]);

            if (!encodedWorkingDirectory.isEmpty())
                qt_native_chdir(encodedWorkingDirectory.constData());

            char **argv = new char *[arguments.size() + 2];
            for (int i = 0; i < arguments.size(); ++i) {
#ifdef Q_OS_MAC
                argv[i + 1] = ::strdup(arguments.at(i).toUtf8().constData());
#else
                argv[i + 1] = ::strdup(arguments.at(i).toLocal8Bit().constData());
#endif
            }
            argv[arguments.size() + 1] = 0;

            if (!program.contains(QLatin1Char('/'))) {
                const QString path = QString::fromLocal8Bit(::getenv("PATH"));
                if (!path.isEmpty()) {
                    QStringList pathEntries = path.split(QLatin1Char(':'));
                    for (int k = 0; k < pathEntries.size(); ++k) {
                        QByteArray tmp = QFile::encodeName(pathEntries.at(k));
                        if (!tmp.endsWith('/')) tmp += '/';
                        tmp += QFile::encodeName(program);
                        argv[0] = tmp.data();
                        qt_native_execv(argv[0], argv);
                    }
                }
            } else {
                QByteArray tmp = QFile::encodeName(program);
                argv[0] = tmp.data();
                qt_native_execv(argv[0], argv);
            }

            struct sigaction noaction;
            memset(&noaction, 0, sizeof(noaction));
            noaction.sa_handler = SIG_IGN;
            qt_native_sigaction(SIGPIPE, &noaction, 0);

            // '\1' means execv failed
            char c = '\1';
            qt_native_write(startedPipe[1], &c, 1);
            qt_native_close(startedPipe[1]);
            ::_exit(1);
        } else if (doubleForkPid == -1) {
            struct sigaction noaction;
            memset(&noaction, 0, sizeof(noaction));
            noaction.sa_handler = SIG_IGN;
            qt_native_sigaction(SIGPIPE, &noaction, 0);

            // '\2' means internal error
            char c = '\2';
            qt_native_write(startedPipe[1], &c, 1);
        }

        qt_native_close(startedPipe[1]);
        qt_native_write(pidPipe[1], (const char *)&doubleForkPid, sizeof(pid_t));
        qt_native_chdir("/");
        ::_exit(1);
    }

    qt_native_close(startedPipe[1]);
    qt_native_close(pidPipe[1]);

    if (childPid == -1) {
        qt_native_close(startedPipe[0]);
        qt_native_close(pidPipe[0]);
        return false;
    }

    char reply = '\0';
    int startResult = qt_native_read(startedPipe[0], &reply, 1);
    int result;
    qt_native_close(startedPipe[0]);
    while (::waitpid(childPid, &result, 0) == -1 && errno == EINTR)
    { }
    bool success = (startResult != -1 && reply == '\0');
    if (success && pid) {
        pid_t actualPid = 0;
        if (qt_native_read(pidPipe[0], (char *)&actualPid, sizeof(pid_t)) == sizeof(pid_t)) {
            *pid = actualPid;
        } else {
            *pid = 0;
        }
    }
    qt_native_close(pidPipe[0]);
    return success;
}

void UnixProcessPrivate::initializeProcessManager()
{
    (void) processManager();
}

//QT_END_NAMESPACE

#endif // QT_NO_PROCESS
