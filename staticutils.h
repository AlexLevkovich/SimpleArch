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
    static bool addToArchive(const QString & fileName,const QStringList & items);
    static const QString tempFileName(bool autoRemove = false);
    static void mimeOpen(const QString & filePath);
    static const QString urlOrLocalPath(const QString & url_or_path);
    static const QString urlOrLocalPath(const char* url_or_path);
};

#endif // STATICUTILS_H
