#ifndef WRITEPTYPROCESS_H
#define WRITEPTYPROCESS_H

#include "process/unixprocess.h"
#include <QSocketNotifier>
#include "kpty.h"
#include "sequentialbuffer.h"

#include <unistd.h>
#include <errno.h>
#include <signal.h>


class WritePtyProcess : public UnixProcess {
    Q_OBJECT
public:
    WritePtyProcess(QObject * parent = NULL);
    ~WritePtyProcess();
    void setErrorString(const QString & str);
    bool waitForBytesWritten(int msecs = 30000);

private slots:
    void canWrite();

protected:
    void setupChildProcess();
    qint64 writeData(const char *data, qint64 len);

private:
    static void qt_ignore_sigpipe();

    KPty * m_pty;
    SequentialBuffer write_data;
    QSocketNotifier * writeNotifier;
    bool emittedBytesWritten;
};

#endif // WRITEPTYPROCESS_H
