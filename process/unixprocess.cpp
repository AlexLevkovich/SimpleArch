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

#if defined UNIX_PROCESS_DEBUG
#include <QDebug>
#include <QString>
#include <ctype.h>

//QT_BEGIN_NAMESPACE
/*
    Returns a human readable representation of the first \a len
    characters in \a data.
*/
static QByteArray qt_prettyDebug(const char *data, int len, int maxSize)
{
    if (!data) return "(null)";
    QByteArray out;
    for (int i = 0; i < len && i < maxSize; ++i) {
        char c = data[i];
        if (isprint(c)) {
            out += c;
        } else switch (c) {
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            char buf[5];
            qsnprintf(buf, sizeof(buf), "\\%3o", c);
            buf[4] = '\0';
            out += QByteArray(buf);
        }
    }

    if (len < maxSize)
        out += "...";

    return out;
}

//QT_END_NAMESPACE

#endif

#include "process/unixprocess.h"
#include "process/unixprocess_p.h"
#include <QByteArray>
#include <QDateTime>
#include <QCoreApplication>
#include <QSocketNotifier>
#include <QTimer>

#ifndef QT_NO_PROCESS

//QT_BEGIN_NAMESPACE

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

/*!
    \class UnixProcess

    \brief The UnixProcess class is used to start external programs and
    to communicate with them.

    \ingroup io
    \ingroup misc
    \mainclass
    \reentrant

    To start a process, pass the name and command line arguments of
    the program you want to run as arguments to start(). For example:

    \snippet doc/src/snippets/UnixProcess/UnixProcess-simpleexecution.cpp 0
    \dots
    \snippet doc/src/snippets/UnixProcess/UnixProcess-simpleexecution.cpp 1
    \snippet doc/src/snippets/UnixProcess/UnixProcess-simpleexecution.cpp 2

    UnixProcess then enters the \l Starting state, and when the program
    has started, UnixProcess enters the \l Running state and emits
    started().

    UnixProcess allows you to treat a process as a sequential I/O
    device. You can write to and read from the process just as you
    would access a network connection using QTcpSocket. You can then
    write to the process's standard input by calling write(), and
    read the standard output by calling read(), readLine(), and
    getChar(). Because it inherits QIODevice, UnixProcess can also be
    used as an input source for QXmlReader, or for generating data to
    be uploaded using QFtp.

    \note On Windows CE, reading and writing to a process is not supported.

    When the process exits, UnixProcess reenters the \l NotRunning state
    (the initial state), and emits finished().

    The finished() signal provides the exit code and exit status of
    the process as arguments, and you can also call exitCode() to
    obtain the exit code of the last process that finished, and
    exitStatus() to obtain its exit status. If an error occurs at
    any point in time, UnixProcess will emit the error() signal. You
    can also call error() to find the type of error that occurred
    last, and state() to find the current process state.

    \section1 Communicating via Channels

    Processes have two predefined output channels: The standard
    output channel (\c stdout) supplies regular console output, and
    the standard error channel (\c stderr) usually supplies the
    errors that are printed by the process. These channels represent
    two separate streams of data. You can toggle between them by
    calling setReadChannel(). UnixProcess emits readyRead() when data is
    available on the current read channel. It also emits
    readyReadStandardOutput() when new standard output data is
    available, and when new standard error data is available,
    readyReadStandardError() is emitted. Instead of calling read(),
    readLine(), or getChar(), you can explicitly read all data from
    either of the two channels by calling readAllStandardOutput() or
    readAllStandardError().

    The terminology for the channels can be misleading. Be aware that
    the process's output channels correspond to UnixProcess's
    \e read channels, whereas the process's input channels correspond
    to UnixProcess's \e write channels. This is because what we read
    using UnixProcess is the process's output, and what we write becomes
    the process's input.

    UnixProcess can merge the two output channels, so that standard
    output and standard error data from the running process both use
    the standard output channel. Call setProcessChannelMode() with
    MergedChannels before starting the process to activative
    this feature. You also have the option of forwarding the output of
    the running process to the calling, main process, by passing
    ForwardedChannels as the argument.

    Certain processes need special environment settings in order to
    operate. You can set environment variables for your process by
    calling setEnvironment(). To set a working directory, call
    setWorkingDirectory(). By default, processes are run in the
    current working directory of the calling process.

    \section1 Synchronous Process API

    UnixProcess provides a set of functions which allow it to be used
    without an event loop, by suspending the calling thread until
    certain signals are emitted:

    \list
    \o waitForStarted() blocks until the process has started.

    \o waitForReadyRead() blocks until new data is
    available for reading on the current read channel.

    \o waitForBytesWritten() blocks until one payload of
    data has been written to the process.

    \o waitForFinished() blocks until the process has finished.
    \endlist

    Calling these functions from the main thread (the thread that
    calls QApplication::exec()) may cause your user interface to
    freeze.

    The following example runs \c gzip to compress the string "Qt
    rocks!", without an event loop:

    \snippet doc/src/snippets/process/process.cpp 0

    \section1 Notes for Windows Users

    Some Windows commands (for example, \c dir) are not provided by
    separate applications, but by the command interpreter itself.
    If you attempt to use UnixProcess to execute these commands directly,
    it won't work. One possible solution is to execute the command
    interpreter itself (\c{cmd.exe} on some Windows systems), and ask
    the interpreter to execute the desired command.

    \sa QBuffer, QFile, QTcpSocket
*/

/*!
    \enum UnixProcess::ProcessChannel

    This enum describes the process channels used by the running process.
    Pass one of these values to setReadChannel() to set the
    current read channel of UnixProcess.

    \value StandardOutput The standard output (stdout) of the running
           process.

    \value StandardError The standard error (stderr) of the running
           process.

    \sa setReadChannel()
*/

/*!
    \enum UnixProcess::ProcessChannelMode

    This enum describes the process channel modes of UnixProcess. Pass
    one of these values to setProcessChannelMode() to set the
    current read channel mode.

    \value SeparateChannels UnixProcess manages the output of the
    running process, keeping standard output and standard error data
    in separate internal buffers. You can select the UnixProcess's
    current read channel by calling setReadChannel(). This is the
    default channel mode of UnixProcess.

    \value MergedChannels UnixProcess merges the output of the running
    process into the standard output channel (\c stdout). The
    standard error channel (\c stderr) will not receive any data. The
    standard output and standard error data of the running process
    are interleaved.

    \value ForwardedChannels UnixProcess forwards the output of the
    running process onto the main process. Anything the child process
    writes to its standard output and standard error will be written
    to the standard output and standard error of the main process.

    \sa setReadChannelMode()
*/

/*!
    \enum UnixProcess::ProcessError

    This enum describes the different types of errors that are
    reported by UnixProcess.

    \value FailedToStart The process failed to start. Either the
    invoked program is missing, or you may have insufficient
    permissions to invoke the program.

    \value Crashed The process crashed some time after starting
    successfully.

    \value Timedout The last waitFor...() function timed out. The
    state of UnixProcess is unchanged, and you can try calling
    waitFor...() again.

    \value WriteError An error occurred when attempting to write to the
    process. For example, the process may not be running, or it may
    have closed its input channel.

    \value ReadError An error occurred when attempting to read from
    the process. For example, the process may not be running.

    \value UnknownError An unknown error occurred. This is the default
    return value of error().

    \sa error()
*/

/*!
    \enum UnixProcess::ProcessState

    This enum describes the different states of UnixProcess.

    \value NotRunning The process is not running.

    \value Starting The process is starting, but the program has not
    yet been invoked.

    \value Running The process is running and is ready for reading and
    writing.

    \sa state()
*/

/*!
    \enum UnixProcess::ExitStatus

    This enum describes the different exit statuses of UnixProcess.

    \value NormalExit The process exited normally.

    \value CrashExit The process crashed.

    \sa exitStatus()
*/

/*!
    \fn void UnixProcess::error(UnixProcess::ProcessError error)

    This signal is emitted when an error occurs with the process. The
    specified \a error describes the type of error that occurred.
*/

/*!
    \fn void UnixProcess::started()

    This signal is emitted by UnixProcess when the process has started,
    and state() returns \l Running.
*/

/*!
    \fn void UnixProcess::stateChanged(UnixProcess::ProcessState newState)

    This signal is emitted whenever the state of UnixProcess changes. The
    \a newState argument is the state UnixProcess changed to.
*/

/*!
    \fn void UnixProcess::finished(int exitCode)
    \obsolete
    \overload

    Use finished(int exitCode, UnixProcess::ExitStatus status) instead.
*/

/*!
    \fn void UnixProcess::finished(int exitCode, UnixProcess::ExitStatus exitStatus)

    This signal is emitted when the process finishes. \a exitCode is the exit
    code of the process, and \a exitStatus is the exit status.  After the
    process has finished, the buffers in UnixProcess are still intact. You can
    still read any data that the process may have written before it finished.

    \sa exitStatus()
*/

/*!
    \fn void UnixProcess::readyReadStandardOutput()

    This signal is emitted when the process has made new data
    available through its standard output channel (\c stdout). It is
    emitted regardless of the current \l{readChannel()}{read channel}.

    \sa readAllStandardOutput(), readChannel()
*/

/*!
    \fn void UnixProcess::readyReadStandardError()

    This signal is emitted when the process has made new data
    available through its standard error channel (\c stderr). It is
    emitted regardless of the current \l{readChannel()}{read
    channel}.

    \sa readAllStandardError(), readChannel()
*/

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

/*!
    Constructs a UnixProcess object with the given \a parent.
*/
UnixProcess::UnixProcess(QObject *parent)
    : QIODevice(parent),
      d_ptr( new UnixProcessPrivate )
{
    d_ptr->q_ptr = this;
#if defined UNIX_PROCESS_DEBUG
    qDebug("UnixProcess::UnixProcess(%p)", parent);
#endif
}

/*!
    Destructs the UnixProcess object, i.e., killing the process.

    Note that this function will not return until the process is
    terminated.
*/
UnixProcess::~UnixProcess()
{
    Q_D(UnixProcess);
    if (d->processState != UnixProcess::NotRunning) {
        qWarning("UnixProcess: Destroyed while process is still running.");
        kill();
        waitForFinished();
    }
#ifdef Q_OS_UNIX
    // make sure the process manager removes this entry
    d->findExitCode();
#endif
    d->cleanup();
    delete d;
}

UnixProcess::ProcessFlags UnixProcess::flags() const
{
    Q_D(const UnixProcess);
    return d->processFlags;
}

void UnixProcess::setFlags( UnixProcess::ProcessFlags flags )
{
    Q_D(UnixProcess);
    d->processFlags = flags;
}

/*!
    \obsolete
    Returns the read channel mode of the UnixProcess. This function is
    equivalent to processChannelMode()

    \sa processChannelMode()
*/
UnixProcess::ProcessChannelMode UnixProcess::readChannelMode() const
{
    return processChannelMode();
}

/*!
    \obsolete

    Use setProcessChannelMode(\a mode) instead.

    \sa setProcessChannelMode()
*/
void UnixProcess::setReadChannelMode(UnixProcess::ProcessChannelMode mode)
{
    setProcessChannelMode(mode);
}

/*!
    \since 4.2

    Returns the channel mode of the UnixProcess standard output and
    standard error channels.

    \sa setReadChannelMode(), ProcessChannelMode, setReadChannel()
*/
UnixProcess::ProcessChannelMode UnixProcess::processChannelMode() const
{
    Q_D(const UnixProcess);
    return d->processChannelMode;
}

/*!
    \since 4.2

    Sets the channel mode of the UnixProcess standard output and standard
    error channels to the \a mode specified.
    This mode will be used the next time start() is called. For example:

    \snippet doc/src/snippets/code/src_corelib_io_UnixProcess.cpp 0

    \sa readChannelMode(), ProcessChannelMode, setReadChannel()
*/
void UnixProcess::setProcessChannelMode(UnixProcess::ProcessChannelMode mode)
{
    Q_D(UnixProcess);
    d->processChannelMode = mode;
}

/*!
    Returns the current read channel of the UnixProcess.

    \sa setReadChannel()
*/
UnixProcess::ProcessChannel UnixProcess::readChannel() const
{
    Q_D(const UnixProcess);
    return d->processChannel;
}

/*!
    Sets the current read channel of the UnixProcess to the given \a
    channel. The current input channel is used by the functions
    read(), readAll(), readLine(), and getChar(). It also determines
    which channel triggers UnixProcess to emit readyRead().

    \sa readChannel()
*/
void UnixProcess::setReadChannel(UnixProcess::ProcessChannel channel)
{
    Q_D(UnixProcess);
//     if (d->processChannel != channel) {
//         QByteArray buf = d->buffer.readAll();
//         if (d->processChannel == UnixProcess::StandardOutput) {
//             for (int i = buf.size() - 1; i >= 0; --i)
//                 d->outputReadBuffer.ungetChar(buf.at(i));
//         } else {
//             for (int i = buf.size() - 1; i >= 0; --i)
//                 d->errorReadBuffer.ungetChar(buf.at(i));
//         }
//     }
    d->processChannel = channel;
}

/*!
    Closes the read channel \a channel. After calling this function,
    UnixProcess will no longer receive data on the channel. Any data that
    has already been received is still available for reading.

    Call this function to save memory, if you are not interested in
    the output of the process.

    \sa closeWriteChannel(), setReadChannel()
*/
void UnixProcess::closeReadChannel(UnixProcess::ProcessChannel channel)
{
    Q_D(UnixProcess);

    if (channel == UnixProcess::StandardOutput) {
        d->stdoutChannel.closed = true;
        if ( d->processFlags&RawStdout )
            d->destroyPipe(d->stdoutChannel.pipe);
    }
    else
        d->stderrChannel.closed = true;
}

/*!
    Schedules the write channel of UnixProcess to be closed. The channel
    will close once all data has been written to the process. After
    calling this function, any attempts to write to the process will
    fail.

    Closing the write channel is necessary for programs that read
    input data until the channel has been closed. For example, the
    program "more" is used to display text data in a console on both
    Unix and Windows. But it will not display the text data until
    UnixProcess's write channel has been closed. Example:

    \snippet doc/src/snippets/code/src_corelib_io_UnixProcess.cpp 1

    The write channel is implicitly opened when start() is called.

    \sa closeReadChannel()
*/
void UnixProcess::closeWriteChannel()
{
    Q_D(UnixProcess);
    d->stdinChannel.closed = true; // closing
    if (d->writeBuffer.isEmpty())
        d->closeWriteChannel();
}

/*!
    \since 4.2

    Redirects the process' standard input to the file indicated by \a
    fileName. When an input redirection is in place, the UnixProcess
    object will be in read-only mode (calling write() will result in
    error).

    If the file \a fileName does not exist at the moment start() is
    called or is not readable, starting the process will fail.

    Calling setStandardInputFile() after the process has started has no
    effect.

    \sa setStandardOutputFile(), setStandardErrorFile(),
        setStandardOutputProcess()
*/
void UnixProcess::setStandardInputFile(const QString &fileName)
{
    Q_D(UnixProcess);
    d->stdinChannel = fileName;
}

void UnixProcess::setStandardInputSocket(const QString &fileName)
{
    Q_D(UnixProcess);
    d->stdinChannel = fileName;
	d->stdoutChannel.file_is_socket = true;
}

/*!
    \since 4.2

    Redirects the process' standard output to the file \a
    fileName. When the redirection is in place, the standard output
    read channel is closed: reading from it using read() will always
    fail, as will readAllStandardOutput().

    If the file \a fileName doesn't exist at the moment start() is
    called, it will be created. If it cannot be created, the starting
    will fail.

    If the file exists and \a mode is QIODevice::Truncate, the file
    will be truncated. Otherwise (if \a mode is QIODevice::Append),
    the file will be appended to.

    Calling setStandardOutputFile() after the process has started has
    no effect.

    \sa setStandardInputFile(), setStandardErrorFile(),
        setStandardOutputProcess()
*/
void UnixProcess::setStandardOutputFile(const QString &fileName, OpenMode mode)
{
    Q_ASSERT(mode == Append || mode == Truncate);
    Q_D(UnixProcess);

    d->stdoutChannel = fileName;
    d->stdoutChannel.append = mode == Append;
}

void UnixProcess::setStandardOutputSocket(const QString &fileName)
{
    Q_D(UnixProcess);

    d->stdoutChannel = fileName;
    d->stdoutChannel.file_is_socket = true;
}

/*!
    \since 4.2

    Redirects the process' standard error to the file \a
    fileName. When the redirection is in place, the standard error
    read channel is closed: reading from it using read() will always
    fail, as will readAllStandardError(). The file will be appended to
    if \a mode is Append, otherwise, it will be truncated.

    See setStandardOutputFile() for more information on how the file
    is opened.

    Note: if setProcessChannelMode() was called with an argument of
    UnixProcess::MergedChannels, this function has no effect.

    \sa setStandardInputFile(), setStandardOutputFile(),
        setStandardOutputProcess()
*/
void UnixProcess::setStandardErrorFile(const QString &fileName, OpenMode mode)
{
    Q_ASSERT(mode == Append || mode == Truncate);
    Q_D(UnixProcess);

    d->stderrChannel = fileName;
    d->stderrChannel.append = mode == Append;
}

void UnixProcess::setStandardErrorSocket(const QString &fileName)
{
    Q_D(UnixProcess);

    d->stderrChannel = fileName;
    d->stderrChannel.file_is_socket = true;
}

/*!
    \since 4.2

    Pipes the standard output stream of this process to the \a
    destination process' standard input.

    The following shell command:
    \snippet doc/src/snippets/code/src_corelib_io_UnixProcess.cpp 2

    Can be accomplished with UnixProcesses with the following code:
    \snippet doc/src/snippets/code/src_corelib_io_UnixProcess.cpp 3
*/
void UnixProcess::setStandardOutputProcess(UnixProcess *destination)
{
    UnixProcessPrivate *dfrom = d_func();
    UnixProcessPrivate *dto = destination->d_func();
    dfrom->stdoutChannel.pipeTo(dto);
    dto->stdinChannel.pipeFrom(dfrom);
}

/*!
    If UnixProcess has been assigned a working directory, this function returns
    the working directory that the UnixProcess will enter before the program has
    started. Otherwise, (i.e., no directory has been assigned,) an empty
    string is returned, and UnixProcess will use the application's current
    working directory instead.

    \sa setWorkingDirectory()
*/
QString UnixProcess::workingDirectory() const
{
    Q_D(const UnixProcess);
    return d->workingDirectory;
}

/*!
    Sets the working directory to \a dir. UnixProcess will start the
    process in this directory. The default behavior is to start the
    process in the working directory of the calling process.

    \sa workingDirectory(), start()
*/
void UnixProcess::setWorkingDirectory(const QString &dir)
{
    Q_D(UnixProcess);
    d->workingDirectory = dir;
}

/*!
    Returns the native process identifier for the running process, if
    available.  If no process is currently running, 0 is returned.
*/
Q_PID UnixProcess::pid() const
{
    Q_D(const UnixProcess);
    return d->pid;
}

/*! \reimp

    This function operates on the current read channel.

    \sa readChannel(), setReadChannel()
*/
bool UnixProcess::canReadLine() const
{
    Q_D(const UnixProcess);
    const SequentialBuffer *readBuffer = (d->processChannel == UnixProcess::StandardError)
                                         ? &d->errorReadBuffer
                                         : &d->outputReadBuffer;
    return readBuffer->canReadLine() || QIODevice::canReadLine();
}

/*!
    Closes all communication with the process and kills it. After calling this
    function, UnixProcess will no longer emit readyRead(), and data can no
    longer be read or written.
*/
void UnixProcess::close()
{
    emit aboutToClose();
    while (waitForBytesWritten(-1))
        ;
    kill();
    waitForFinished(-1);
    QIODevice::close();
}

/*! \reimp

   Returns true if the process is not running, and no more data is available
   for reading; otherwise returns false.
*/
bool UnixProcess::atEnd() const
{
    Q_D(const UnixProcess);
    const SequentialBuffer *readBuffer = (d->processChannel == UnixProcess::StandardError)
                                         ? &d->errorReadBuffer
                                         : &d->outputReadBuffer;
    return QIODevice::atEnd() && (!isOpen() || readBuffer->isEmpty());
}

/*! \reimp
*/
bool UnixProcess::isSequential() const
{
    return true;
}

/*! \reimp
*/
qint64 UnixProcess::bytesAvailable() const
{
    Q_D(const UnixProcess);
    const SequentialBuffer *readBuffer = (d->processChannel == UnixProcess::StandardError)
                                         ? &d->errorReadBuffer
                                         : &d->outputReadBuffer;
#if defined UNIX_PROCESS_DEBUG
    qDebug("UnixProcess::bytesAvailable() == %i (%s)", readBuffer->size(),
           (d->processChannel == UnixProcess::StandardError) ? "stderr" : "stdout");
#endif
    return readBuffer->size() + QIODevice::bytesAvailable();
}

/*! \reimp
*/
qint64 UnixProcess::bytesToWrite() const
{
    Q_D(const UnixProcess);
    qint64 size = d->writeBuffer.size();
    return size;
}

/*!
    Returns the type of error that occurred last.

    \sa state()
*/
UnixProcess::ProcessError UnixProcess::error() const
{
    Q_D(const UnixProcess);
    return d->processError;
}

/*!
    Returns the current state of the process.

    \sa stateChanged(), error()
*/
UnixProcess::ProcessState UnixProcess::state() const
{
    Q_D(const UnixProcess);
    return d->processState;
}

/*!
    Sets the environment that UnixProcess will use when starting a process to the
    \a environment specified which consists of a list of key=value pairs.

    For example, the following code adds the \c{C:\\BIN} directory to the list of
    executable paths (\c{PATHS}) on Windows:

    \snippet doc/src/snippets/UnixProcess-environment/main.cpp 0

    \sa environment(), systemEnvironment()
*/
void UnixProcess::setEnvironment(const QStringList &environment)
{
    Q_D(UnixProcess);
    d->environment = environment;
}

/*!
    Returns the environment that UnixProcess will use when starting a
    process, or an empty QStringList if no environment has been set
    using setEnvironment(). If no environment has been set, the
    environment of the calling process will be used.

    \note The environment settings are ignored on Windows CE,
    as there is no concept of an environment.

    \sa setEnvironment(), systemEnvironment()
*/
QStringList UnixProcess::environment() const
{
    Q_D(const UnixProcess);
    return d->environment;
}

/*!
    Blocks until the process has started and the started() signal has
    been emitted, or until \a msecs milliseconds have passed.

    Returns true if the process was started successfully; otherwise
    returns false (if the operation timed out or if an error
    occurred).

    This function can operate without an event loop. It is
    useful when writing non-GUI applications and when performing
    I/O operations in a non-GUI thread.

    \warning Calling this function from the main (GUI) thread
    might cause your user interface to freeze.

    If msecs is -1, this function will not time out.

    \sa started(), waitForReadyRead(), waitForBytesWritten(), waitForFinished()
*/
bool UnixProcess::waitForStarted(int msecs)
{
    Q_D(UnixProcess);
    if (d->processState == UnixProcess::Starting) {
        if (!d->waitForStarted(msecs))
            return false;
        setProcessState(UnixProcess::Running);
        emit started();
    }
    return d->processState == UnixProcess::Running;
}

/*! \reimp
*/
bool UnixProcess::waitForReadyRead(int msecs)
{
    Q_D(UnixProcess);

    if (d->processState == UnixProcess::NotRunning)
        return false;
    if (d->processChannel == UnixProcess::StandardOutput && d->stdoutChannel.closed)
        return false;
    if (d->processChannel == UnixProcess::StandardError && d->stderrChannel.closed)
        return false;
    return d->waitForReadyRead(msecs);
}

/*! \reimp
*/
bool UnixProcess::waitForBytesWritten(int msecs)
{
    Q_D(UnixProcess);
    if (d->processState == UnixProcess::NotRunning)
        return false;
    if (d->processState == UnixProcess::Starting) {
        QTime stopWatch;
        stopWatch.start();
        bool started = waitForStarted(msecs);
        if (!started)
            return false;
        if (msecs != -1)
            msecs -= stopWatch.elapsed();
    }

    return d->waitForBytesWritten(msecs);
}

/*!
    Blocks until the process has finished and the finished() signal
    has been emitted, or until \a msecs milliseconds have passed.

    Returns true if the process finished; otherwise returns false (if
    the operation timed out or if an error occurred).

    This function can operate without an event loop. It is
    useful when writing non-GUI applications and when performing
    I/O operations in a non-GUI thread.

    \warning Calling this function from the main (GUI) thread
    might cause your user interface to freeze.

    If msecs is -1, this function will not time out.

    \sa finished(), waitForStarted(), waitForReadyRead(), waitForBytesWritten()
*/
bool UnixProcess::waitForFinished(int msecs)
{
    Q_D(UnixProcess);
    if (d->processState == UnixProcess::NotRunning)
        return false;
    if (d->processState == UnixProcess::Starting) {
        QTime stopWatch;
        stopWatch.start();
        bool started = waitForStarted(msecs);
        if (!started)
            return false;
        if (msecs != -1)
            msecs -= stopWatch.elapsed();
    }

    return d->waitForFinished(msecs);
}

/*!
    Sets the current state of the UnixProcess to the \a state specified.

    \sa state()
*/
void UnixProcess::setProcessState(UnixProcess::ProcessState state)
{
    Q_D(UnixProcess);
    if (d->processState == state)
        return;
    d->processState = state;
    emit stateChanged(state);
}

/*!
  This function is called in the child process context just before the
    program is executed on Unix or Mac OS X (i.e., after \e fork(), but before
    \e execve()). Reimplement this function to do last minute initialization
    of the child process. Example:

    \snippet doc/src/snippets/code/src_corelib_io_UnixProcess.cpp 4

    You cannot exit the process (by calling exit(), for instance) from
    this function. If you need to stop the program before it starts
    execution, your workaround is to emit finished() and then call
    exit().

    \warning This function is called by UnixProcess on Unix and Mac OS X
    only. On Windows, it is not called.
*/
void UnixProcess::setupChildProcess()
{
}

/*! \reimp
*/
qint64 UnixProcess::readData(char *data, qint64 maxlen)
{
    Q_D(UnixProcess);
    return d->readData( data, maxlen, d->processChannel );
}

/*! \reimp
*/
qint64 UnixProcess::writeData(const char *data, qint64 len)
{
    Q_D(UnixProcess);

    if (d->stdinChannel.closed) {
#if defined UNIX_PROCESS_DEBUG
    qDebug("UnixProcess::writeData(%p \"%s\", %lld) == 0 (write channel closing)",
           data, qt_prettyDebug(data, len, 16).constData(), len);
#endif
        return 0;
    }

    if (d->processFlags & UnixProcess::RawStdin) {
        d->waitForBytesWritten();
        qint64 r = d->writeToStdin(data, len);
        if ( r > 0 )
            emit bytesWritten(r);
        return r;
    }
    else {
        char *dest = d->writeBuffer.reserve(len);
        memcpy(dest, data, len);
        if (d->stdinChannel.notifier)
            d->stdinChannel.notifier->setEnabled(true);
#if defined UNIX_PROCESS_DEBUG
        qDebug("UnixProcess::writeData(%p \"%s\", %lld) == %lld (written to buffer)",
               data, qt_prettyDebug(data, len, 16).constData(), len, len);
#endif
        return len;
    }
}

/*!
    Regardless of the current read channel, this function returns all
    data available from the standard output of the process as a
    QByteArray.

    \sa readyReadStandardOutput(), readAllStandardError(), readChannel(), setReadChannel()
*/
QByteArray UnixProcess::readAllStandardOutput()
{
    Q_D(UnixProcess);
    if (!(d->processFlags&RawStdout)) {
        UnixProcess::ProcessChannel tmp = readChannel();
        setReadChannel(UnixProcess::StandardOutput);
        QByteArray data = readAll();
        setReadChannel(tmp);
        return data;
    }
    else {
        return QByteArray();
    }
}

/*!
    Regardless of the current read channel, this function returns all
    data available from the standard error of the process as a
    QByteArray.

    \sa readyReadStandardError(), readAllStandardOutput(), readChannel(), setReadChannel()
*/
QByteArray UnixProcess::readAllStandardError()
{
    Q_D(UnixProcess);
    if (d->processFlags&RawStdout) {
        //
        // HACK: this is an ugly hack to get around the following problem:
        // K3b uses UnixProcess from different threads. This is no problem unless
        // the read channel is changed here while the other thread tries to read
        // from stdout. It will then result in two reads from stderr instead
        // (this one and the other thread which originally wanted to read from
        // stdout).
        // The "solution" atm is to reimplement QIODevice::readAll here, ignoring its
        // buffer (no real problem since K3b::Process is always opened Unbuffered)
        //
        QByteArray tmp;
        tmp.resize(int(d->errorReadBuffer.size()));
        qint64 readBytes = d->readData(tmp.data(), tmp.size(), UnixProcess::StandardError);
        tmp.resize(readBytes < 0 ? 0 : int(readBytes));
        return tmp;
    }
    else {
        UnixProcess::ProcessChannel tmp = readChannel();
        setReadChannel(UnixProcess::StandardError);
        QByteArray data = readAll();
        setReadChannel(tmp);
        return data;
    }
}

/*!
    Starts the program \a program in a new process, passing the
    command line arguments in \a arguments. The OpenMode is set to \a
    mode. UnixProcess will immediately enter the Starting state. If the
    process starts successfully, UnixProcess will emit started();
    otherwise, error() will be emitted.

    Note that arguments that contain spaces are not passed to the
    process as separate arguments.

    \bold{Windows:} Arguments that contain spaces are wrapped in quotes.

    \note Processes are started asynchronously, which means the started()
    and error() signals may be delayed. Call waitForStarted() to make
    sure the process has started (or has failed to start) and those signals
    have been emitted.

    \sa pid(), started(), waitForStarted()
*/
void UnixProcess::start(const QString &program, const QStringList &arguments, OpenMode mode)
{
    Q_D(UnixProcess);
    if (d->processState != UnixProcess::NotRunning) {
        qWarning("UnixProcess::start: Process is already running");
        return;
    }

#if defined UNIX_PROCESS_DEBUG
    qDebug() << "UnixProcess::start(" << program << "," << arguments << "," << mode << ")";
#endif

    d->outputReadBuffer.clear();
    d->errorReadBuffer.clear();

    d->isReadyWrite = false;

    if (d->stdinChannel.type != UnixProcessPrivate::Channel::Normal)
        mode &= ~WriteOnly;     // not open for writing
    if (d->stdoutChannel.type != UnixProcessPrivate::Channel::Normal &&
        (d->stderrChannel.type != UnixProcessPrivate::Channel::Normal ||
         d->processChannelMode == UnixProcess::MergedChannels))
        mode &= ~ReadOnly;      // not open for reading
    if (mode == 0)
        mode = Unbuffered;
    QIODevice::open(mode);

    d->stdinChannel.closed = false;
    d->stdoutChannel.closed = false;
    d->stderrChannel.closed = false;

    d->program = program;
    d->arguments = arguments;

    d->exitCode = 0;
    d->exitStatus = UnixProcess::NormalExit;
    d->processError = UnixProcess::UnknownError;
    setErrorString( QString() );
    d->startProcess();
}


static QStringList parseCombinedArgString(const QString &program)
{
    QStringList args;
    QString tmp;
    int quoteCount = 0;
    bool inQuote = false;

    // handle quoting. tokens can be surrounded by double quotes
    // "hello world". three consecutive double quotes represent
    // the quote character itself.
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

/*!
    \overload

    Starts the program \a program in a new process. \a program is a
    single string of text containing both the program name and its
    arguments. The arguments are separated by one or more
    spaces. For example:

    \snippet doc/src/snippets/code/src_corelib_io_UnixProcess.cpp 5

    The \a program string can also contain quotes, to ensure that arguments
    containing spaces are correctly supplied to the new process. For example:

    \snippet doc/src/snippets/code/src_corelib_io_UnixProcess.cpp 6

    Note that, on Windows, quotes need to be both escaped and quoted.
    For example, the above code would be specified in the following
    way to ensure that \c{"My Documents"} is used as the argument to
    the \c dir executable:

    \snippet doc/src/snippets/code/src_corelib_io_UnixProcess.cpp 7

    The OpenMode is set to \a mode.
*/
void UnixProcess::start(const QString &program, OpenMode mode)
{
    QStringList args = parseCombinedArgString(program);

    QString prog = args.first();
    args.removeFirst();

    start(prog, args, mode);
}

/*!
    Attempts to terminate the process.

    The process may not exit as a result of calling this function (it is given
    the chance to prompt the user for any unsaved files, etc).

    On Windows, terminate() posts a WM_CLOSE message to all toplevel windows
    of the process and then to the main thread of the process itself. On Unix
    and Mac OS X the SIGTERM signal is sent.

    Console applications on Windows that do not run an event loop, or whose
    event loop does not handle the WM_CLOSE message, can only be terminated by
    calling kill().

    \sa kill()
*/
void UnixProcess::terminate()
{
    Q_D(UnixProcess);
    d->terminateProcess();
}

/*!
    Kills the current process, causing it to exit immediately.

    On Windows, kill() uses TerminateProcess, and on Unix and Mac OS X, the
    SIGKILL signal is sent to the process.

    \sa terminate()
*/
void UnixProcess::kill()
{
    Q_D(UnixProcess);
    d->killProcess();
}

/*!
    Returns the exit code of the last process that finished.
*/
int UnixProcess::exitCode() const
{
    Q_D(const UnixProcess);
    return d->exitCode;
}

/*!
    \since 4.1

    Returns the exit status of the last process that finished.

    On Windows, if the process was terminated with TerminateProcess()
    from another application this function will still return NormalExit
    unless the exit code is less than 0.
*/
UnixProcess::ExitStatus UnixProcess::exitStatus() const
{
    Q_D(const UnixProcess);
    return d->exitStatus;
}

/*!
    Starts the program \a program with the arguments \a arguments in a
    new process, waits for it to finish, and then returns the exit
    code of the process. Any data the new process writes to the
    console is forwarded to the calling process.

    The environment and working directory are inherited by the calling
    process.

    On Windows, arguments that contain spaces are wrapped in quotes.
*/
int UnixProcess::execute(const QString &program, const QStringList &arguments)
{
    UnixProcess process;
    process.setReadChannelMode(UnixProcess::ForwardedChannels);
    process.start(program, arguments);
    process.waitForFinished(-1);
    return process.exitCode();
}

/*!
    \overload

    Starts the program \a program in a new process. \a program is a
    single string of text containing both the program name and its
    arguments. The arguments are separated by one or more spaces.
*/
int UnixProcess::execute(const QString &program)
{
    UnixProcess process;
    process.setReadChannelMode(UnixProcess::ForwardedChannels);
    process.start(program);
    process.waitForFinished(-1);
    return process.exitCode();
}

/*!
    Starts the program \a program with the arguments \a arguments in a
    new process, and detaches from it. Returns true on success;
    otherwise returns false. If the calling process exits, the
    detached process will continue to live.

    Note that arguments that contain spaces are not passed to the
    process as separate arguments.

    \bold{Unix:} The started process will run in its own session and act
    like a daemon.

    \bold{Windows:} Arguments that contain spaces are wrapped in quotes.
    The started process will run as a regular standalone process.

    The process will be started in the directory \a workingDirectory.

    If the function is successful then *\a pid is set to the process
    identifier of the started process.
*/
bool UnixProcess::startDetached(const QString &program,
			     const QStringList &arguments,
			     const QString &workingDirectory,
                             qint64 *pid)
{
    return UnixProcessPrivate::startDetached(program,
					  arguments,
					  workingDirectory,
					  pid);
}

/*!
    Starts the program \a program with the given \a arguments in a
    new process, and detaches from it. Returns true on success;
    otherwise returns false. If the calling process exits, the
    detached process will continue to live.

    Note that arguments that contain spaces are not passed to the
    process as separate arguments.

    \bold{Unix:} The started process will run in its own session and act
    like a daemon.

    \bold{Windows:} Arguments that contain spaces are wrapped in quotes.
    The started process will run as a regular standalone process.
*/
bool UnixProcess::startDetached(const QString &program,
			     const QStringList &arguments)
{
    return UnixProcessPrivate::startDetached(program, arguments);
}

/*!
    \overload

    Starts the program \a program in a new process. \a program is a
    single string of text containing both the program name and its
    arguments. The arguments are separated by one or more spaces.

    The \a program string can also contain quotes, to ensure that arguments
    containing spaces are correctly supplied to the new process.
*/
bool UnixProcess::startDetached(const QString &program)
{
    QStringList args = parseCombinedArgString(program);

    QString prog = args.first();
    args.removeFirst();

    return UnixProcessPrivate::startDetached(prog, args);
}

QT_BEGIN_INCLUDE_NAMESPACE
#ifdef Q_OS_MAC
# include <crt_externs.h>
# define environ (*_NSGetEnviron())
#else
  extern char **environ;
#endif
QT_END_INCLUDE_NAMESPACE

/*!
    \since 4.1

    Returns the environment of the calling process as a list of
    key=value pairs. Example:

    \snippet doc/src/snippets/code/src_corelib_io_UnixProcess.cpp 8

    \sa environment(), setEnvironment()
*/
QStringList UnixProcess::systemEnvironment()
{
    QStringList tmp;
    char *entry = 0;
    int count = 0;
    while ((entry = environ[count++]))
         tmp << QString::fromLocal8Bit(entry);
    return tmp;
}

bool UnixProcess::isReadyWrite() const
{
    Q_D(const UnixProcess);
    return d->isReadyWrite;
}


/*!
    \typedef Q_PID
    \relates UnixProcess

    Typedef for the identifiers used to represent processes on the underlying
    platform. On Unix, this corresponds to \l qint64; on Windows, it
    corresponds to \c{_PROCESS_INFORMATION*}.

    \sa UnixProcess::pid()
*/


//QT_END_NAMESPACE

#endif // QT_NO_PROCESS

