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

#ifndef UNIX_PROCESS_P_H
#define UNIX_PROCESS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "process/unixprocess.h"
#include <QStringList>
#include "sequentialbuffer.h"

typedef int Q_PIPE;
#define INVALID_Q_PIPE -1

#ifndef QT_NO_PROCESS

//QT_BEGIN_NAMESPACE
class QSocketNotifier;
class QWindowsPipeWriter;
class QWinEventNotifier;
class QTimer;

class UnixProcessPrivate
{
public:
    Q_DECLARE_PUBLIC(UnixProcess)
    UnixProcess* q_ptr;

    struct Channel {
        enum ProcessChannelType {
            Normal = 0,
            PipeSource = 1,
            PipeSink = 2,
            Redirect = 3
            // if you add "= 4" here, increase the number of bits below
        };

        Channel() : process(0), notifier(0), type(Normal), closed(false), append(false), file_is_socket(false)
        {
            pipe[0] = INVALID_Q_PIPE;
            pipe[1] = INVALID_Q_PIPE;
        }

        void clear();

        Channel &operator=(const QString &fileName)
        {
            clear();
            file = fileName;
            type = fileName.isEmpty() ? Normal : Redirect;
            return *this;
        }

        void pipeTo(UnixProcessPrivate *other)
        {
            clear();
            process = other;
            type = PipeSource;
        }

        void pipeFrom(UnixProcessPrivate *other)
        {
            clear();
            process = other;
            type = PipeSink;
        }

        QString file;
        UnixProcessPrivate *process;
        QSocketNotifier *notifier;
        Q_PIPE pipe[2];

        unsigned type : 2;
        bool closed : 1;
        bool append : 1;
		bool file_is_socket : 1;
    };

    UnixProcessPrivate();
    virtual ~UnixProcessPrivate();

    // private slots
    bool _q_canReadStandardOutput();
    bool _q_canReadStandardError();
    bool _q_canWrite();
    bool _q_startupNotification();
    bool _q_processDied();
    bool _q_notifyProcessDied();
    void _q_notified();

    UnixProcess::ProcessChannel processChannel;
    UnixProcess::ProcessChannelMode processChannelMode;
    UnixProcess::ProcessFlags processFlags;
    UnixProcess::ProcessError processError;
    UnixProcess::ProcessState processState;
    QString workingDirectory;
    Q_PID pid;
    int sequenceNumber;

    bool dying;
    bool emittedReadyRead;
    bool emittedBytesWritten;

    Channel stdinChannel;
    Channel stdoutChannel;
    Channel stderrChannel;
    bool createChannel(Channel &channel);
    void closeWriteChannel();

    QString program;
    QStringList arguments;
    QStringList environment;

    SequentialBuffer outputReadBuffer;
    SequentialBuffer errorReadBuffer;
    SequentialBuffer writeBuffer;

    Q_PIPE childStartedPipe[2];
    Q_PIPE deathPipe[2];
    void destroyPipe(Q_PIPE pipe[2]);

    QSocketNotifier *startupSocketNotifier;
    QSocketNotifier *deathNotifier;

    // the wonderful windows notifier
    QTimer *notifier;
    QWindowsPipeWriter *pipeWriter;
    QWinEventNotifier *processFinishedNotifier;

    void startProcess();
#ifdef Q_OS_UNIX
    void execChild(const char *workingDirectory, char **path, char **argv, char **envp);
#endif
    bool processStarted();
    void terminateProcess();
    void killProcess();
    void findExitCode();
#ifdef Q_OS_UNIX
    bool waitForDeadChild();
#endif
    static bool startDetached(const QString &program, const QStringList &arguments, const QString &workingDirectory = QString(),
                              qint64 *pid = 0);

    int exitCode;
    UnixProcess::ExitStatus exitStatus;
    bool crashed;
#ifdef Q_OS_UNIX
    int serial;
#endif

    bool isReadyWrite;

    bool waitForStarted(int msecs = 30000);
    bool waitForReadyRead(int msecs = 30000);
    bool waitForBytesWritten(int msecs = 30000);
    bool waitForFinished(int msecs = 30000);
    bool waitForWrite(int msecs = 30000);

    qint64 bytesAvailableFromStdout() const;
    qint64 bytesAvailableFromStderr() const;
    qint64 readFromStdout(char *data, qint64 maxlen);
    qint64 readFromStderr(char *data, qint64 maxlen);
    qint64 writeToStdin(const char *data, qint64 maxlen);

    qint64 readData( char *data, qint64 len, UnixProcess::ProcessChannel channel );

    void cleanup();
#ifdef Q_OS_UNIX
    static void initializeProcessManager();
#endif
};

//QT_END_NAMESPACE

#endif // QT_NO_PROCESS

#endif // UNIX_PROCESS_P_H
