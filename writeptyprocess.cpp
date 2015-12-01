#include "writeptyprocess.h"
#include "eventloop.h"
#include <QTimer>

#define PROCESS_NOT_STARTED "Process is not started!"

WritePtyProcess::WritePtyProcess(QObject * parent) : UnixProcess(parent) {
    emittedBytesWritten = false;
    m_pty = new KPty();
    m_pty->open();
    writeNotifier = new QSocketNotifier(m_pty->masterFd(),QSocketNotifier::Write,this);
    writeNotifier->setEnabled(false);
    QObject::connect(writeNotifier,SIGNAL(activated(int)),this,SLOT(canWrite()));
}

WritePtyProcess::~WritePtyProcess() {
    delete m_pty;
}

void WritePtyProcess::setErrorString(const QString & str) {
    UnixProcess::setErrorString(str);
}

bool WritePtyProcess::waitForBytesWritten(int msecs) {
    if (state() != UnixProcess::Running) {
        setErrorString(tr(PROCESS_NOT_STARTED));
        return false;
    }
    EventLoop pause;
    if (msecs >= 0) QTimer::singleShot(msecs,&pause,SLOT(cancel()));
    connect(this,SIGNAL(bytesWritten(qint64)),&pause,SLOT(quit()));
    return (pause.exec() == 0);
}

void WritePtyProcess::canWrite() {
    writeNotifier->setEnabled(false);
    if (write_data.isEmpty()) return;

    qt_ignore_sigpipe();
    int wroteBytes = ::write(m_pty->masterFd(),write_data.data(),write_data.size());
    if (wroteBytes < 0) {
        setErrorString(tr("Error writing to PTY!"));
        emit error(UnixProcess::WriteError);
        return;
    }
    write_data.free(wroteBytes);
    if (!emittedBytesWritten) {
        emittedBytesWritten = true;
        emit bytesWritten(wroteBytes);
        emittedBytesWritten = false;
    }

    if (!write_data.isEmpty()) writeNotifier->setEnabled(true);
}

void WritePtyProcess::setupChildProcess() {
    m_pty->setCTty();
    dup2(m_pty->slaveFd(),0);

    UnixProcess::setupChildProcess();
}

qint64 WritePtyProcess::writeData(const char *data, qint64 len) {
    if (len < 0) return -1;
    if (len == 0) return 0;

    write_data += QByteArray(data,len);
    writeNotifier->setEnabled(true);
    return len;
}

void WritePtyProcess::qt_ignore_sigpipe() {
    static QBasicAtomicInt atom = Q_BASIC_ATOMIC_INITIALIZER(0);
    if (atom.testAndSetRelaxed(0, 1)) {
        struct sigaction noaction;
        memset(&noaction, 0, sizeof(noaction));
        noaction.sa_handler = SIG_IGN;
        sigaction(SIGPIPE, &noaction, 0);
    }
}
