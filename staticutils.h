#ifndef STATICUTILS_H
#define STATICUTILS_H

#include <QDir>
#include <QStringList>
class QMainWindow;

class StaticUtils {
public:
    static bool copyPath(const QDir & src,const QDir & dst);
    static const QString currentUserName();
    static QMainWindow * findMainWindow();
    static const QString inputPasswordRequest(const QString & fileName);
    static const QString inputPasswordRequest(bool verify_request = false);
    static void setAppWaitStatus(const QString & message);
    static void restoreAppStatus();
    static const QString simplifyLeft(const QString & str);
    static void simplifyRight(QString & str);
    static bool createArchive(const QStringList & items);
    static bool extractArchive(const QString & fileName,const QString & toDir);
    static const QString tempFileName(bool autoRemove = false);
    static void mimeOpen(const QString & filePath);
};

#endif // STATICUTILS_H
