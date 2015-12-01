#include <QFileIconProvider>
#include <QMap>
#include <QIcon>
#include <QFileInfo>
#ifndef Q_WS_WIN
#include <QMimeDatabase>
#endif
#include <QFileIconProvider>

class IconProvider : public QFileIconProvider {
public:
    static IconProvider * instance();
    QIcon fileIcon(const QString &filename,bool encripted = false,bool is_link = false) const;
    QIcon dirIcon(bool encripted = false,bool is_link = false) const;
    QIcon icon(const QFileInfo & info) const;
    QIcon icon(IconType type) const;

private:
    IconProvider() : QFileIconProvider() {}
    QIcon addPicture(const QIcon & icon,const QString & picture_path) const;
    QIcon _fileIcon(const QString &filename) const;

    static IconProvider *self;
    QMap<QString,QIcon> iconCache;
#ifndef Q_WS_WIN
    QMimeDatabase  mime_database;
#endif
};
