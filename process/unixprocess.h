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

#ifndef UNIXPROCESS_H
#define UNIXPROCESS_H

#include <QIODevice>
#include <QStringList>

// QT_BEGIN_HEADER

// QT_BEGIN_NAMESPACE

// QT_MODULE(Core)

#ifndef QT_NO_PROCESS

typedef qint64 Q_PID;

class UnixProcessPrivate;

class UnixProcess : public QIODevice
{
    Q_OBJECT
public:
    enum ProcessError {
        FailedToStart, //### file not found, resource error
        Crashed,
        Timedout,
        ReadError,
        WriteError,
        UnknownError
    };
    enum ProcessState {
        NotRunning,
        Starting,
        Running
    };
    enum ProcessChannel {
        StandardOutput,
        StandardError
    };
    enum ProcessChannelMode {
        SeparateChannels,
        MergedChannels,
        ForwardedChannels
    };
    enum ExitStatus {
        NormalExit,
        CrashExit
    };
    enum ProcessFlag {
        NoFlags = 0x0,
        RawStdin = 0x1,
        RawStdout = 0x2
    };
    Q_DECLARE_FLAGS( ProcessFlags, ProcessFlag )

    explicit UnixProcess(QObject *parent = 0);
    virtual ~UnixProcess();

    void start(const QString &program, const QStringList &arguments, OpenMode mode = ReadWrite);
    void start(const QString &program, OpenMode mode = ReadWrite);

    UnixProcess::ProcessChannelMode readChannelMode() const;
    void setReadChannelMode(UnixProcess::ProcessChannelMode mode);
    UnixProcess::ProcessChannelMode processChannelMode() const;
    void setProcessChannelMode(UnixProcess::ProcessChannelMode mode);

    ProcessFlags flags() const;
    void setFlags( ProcessFlags flags );

    UnixProcess::ProcessChannel readChannel() const;
    void setReadChannel(UnixProcess::ProcessChannel channel);

    void closeReadChannel(UnixProcess::ProcessChannel channel);
    void closeWriteChannel();

    void setStandardInputFile(const QString &fileName);
    void setStandardOutputFile(const QString &fileName, OpenMode mode = Truncate);
    void setStandardErrorFile(const QString &fileName, OpenMode mode = Truncate);
    void setStandardInputSocket(const QString &fileName);
    void setStandardOutputSocket(const QString &fileName);
    void setStandardErrorSocket(const QString &fileName);
    void setStandardOutputProcess(UnixProcess *destination);

    QString workingDirectory() const;
    void setWorkingDirectory(const QString &dir);

    void setEnvironment(const QStringList &environment);
    QStringList environment() const;

    UnixProcess::ProcessError error() const;
    UnixProcess::ProcessState state() const;

    // #### Qt 5: Q_PID is a pointer on Windows and a value on Unix
    Q_PID pid() const;

    bool waitForStarted(int msecs = 30000);
    bool waitForReadyRead(int msecs = 30000);
    bool waitForBytesWritten(int msecs = 30000);
    bool waitForFinished(int msecs = 30000);

    QByteArray readAllStandardOutput();
    QByteArray readAllStandardError();

    int exitCode() const;
    UnixProcess::ExitStatus exitStatus() const;

    // QIODevice
    qint64 bytesAvailable() const;
    qint64 bytesToWrite() const;
    bool isSequential() const;
    bool canReadLine() const;
    void close();
    bool atEnd() const;

    bool isReadyWrite() const;

    static int execute(const QString &program, const QStringList &arguments);
    static int execute(const QString &program);

    static bool startDetached(const QString &program, const QStringList &arguments, const QString &workingDirectory,
                              qint64 *pid = 0);
    static bool startDetached(const QString &program, const QStringList &arguments);
    static bool startDetached(const QString &program);

    static QStringList systemEnvironment();

public Q_SLOTS:
    void terminate();
    void kill();

Q_SIGNALS:
    void started();
    void finished(int exitCode);
    void finished(int exitCode, UnixProcess::ExitStatus exitStatus);
    void error(UnixProcess::ProcessError error);
    void stateChanged(UnixProcess::ProcessState state);

    void readyReadStandardOutput();
    void readyReadStandardError();
    void readyWrite();

protected:
    void setProcessState(UnixProcess::ProcessState state);

    virtual void setupChildProcess();

    // QIODevice
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);

private:
    Q_DECLARE_PRIVATE(UnixProcess)
    Q_DISABLE_COPY(UnixProcess)

    UnixProcessPrivate* d_ptr;

    Q_PRIVATE_SLOT(d_func(), bool _q_canReadStandardOutput())
    Q_PRIVATE_SLOT(d_func(), bool _q_canReadStandardError())
    Q_PRIVATE_SLOT(d_func(), bool _q_canWrite())
    Q_PRIVATE_SLOT(d_func(), bool _q_startupNotification())
    Q_PRIVATE_SLOT(d_func(), bool _q_processDied())
    Q_PRIVATE_SLOT(d_func(), bool _q_notifyProcessDied())
    Q_PRIVATE_SLOT(d_func(), void _q_notified())
    friend class UnixProcessManager;
};

#endif // QT_NO_PROCESS

// QT_END_NAMESPACE

// QT_END_HEADER

Q_DECLARE_OPERATORS_FOR_FLAGS( UnixProcess::ProcessFlags )

#endif // UNIXPROCESS_H
#include "process/unixprocess_p.h"
