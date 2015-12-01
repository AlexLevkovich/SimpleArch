#include "iconprovider.h"
#include <QPixmap>
#include <QPaintEngine>

#ifdef Q_WS_WIN
#include <windows.h>
#if QT_VERSION >= 0x050000
Q_GUI_EXPORT QPixmap qt_pixmapFromWinHICON(HICON icon);
#else
QPixmap qt_pixmapFromWinHICON(HICON icon) {
    return QPixmap::fromWinHICON(icon);
}
#endif
#endif

#define MAX_ICON_SIZE 48

IconProvider *IconProvider::self = 0;

IconProvider *IconProvider::instance() {
    if(self == NULL) self = new IconProvider();
    return self;
}

QIcon IconProvider::_fileIcon(const QString &filename) const {
    QFileInfo fileInfo(filename);

#ifdef Q_WS_WIN
    if (fileInfo.suffix().isEmpty()) return instance()->iconProvider.icon(QFileIconProvider::File);
    if (fileInfo.exists()) return instance()->iconProvider.icon(fileInfo);

    if (!instance()->iconCache.contains(fileInfo.suffix())) {
        SHFILEINFO shFileInfo;
        unsigned long val = 0;

        val = SHGetFileInfo((const wchar_t *)("foo." + fileInfo.suffix()).utf16(), 0, &shFileInfo,
                            sizeof(SHFILEINFO), SHGFI_ICON | SHGFI_USEFILEATTRIBUTES);

        // Even if GetFileInfo returns a valid result, hIcon can be empty in some cases
        if (val && shFileInfo.hIcon) {
            QIcon icon = QIcon(qt_pixmapFromWinHICON(shFileInfo.hIcon));
            instance()->iconCache[fileInfo.suffix()] = icon;
            DestroyIcon(shFileInfo.hIcon);
            return icon;
        }
        else return instance()->iconProvider.icon(QFileIconProvider::File);
    }
    else return instance()->iconCache[fileInfo.suffix()];
#else
    if (!instance()->iconCache.contains(fileInfo.suffix())) {
        QMimeType type = mime_database.mimeTypeForFile(filename);
        QIcon icon = QIcon::fromTheme(type.iconName());
        if (icon.isNull()) icon = QIcon::fromTheme(type.genericIconName());
        if (icon.isNull()) icon = QIcon::fromTheme("unknown");
        instance()->iconCache[fileInfo.suffix()] = icon;
        return icon;
    }
    else return instance()->iconCache[fileInfo.suffix()];
#endif

    return instance()->icon(QFileIconProvider::File);
}

QIcon IconProvider::fileIcon(const QString &filename,bool encripted,bool is_link) const {
    QIcon icon = _fileIcon(filename);
    icon = encripted?addPicture(icon,":/pics/encripted.png"):icon;
    icon = is_link?addPicture(icon,":/pics/symlink.png"):icon;
    return icon;
}

QIcon IconProvider::icon(IconType type) const {
    return QFileIconProvider::icon(type);
}

QIcon IconProvider::dirIcon(bool encripted,bool is_link) const {
    QIcon icon = instance()->icon(QFileIconProvider::Folder);
    icon = encripted?addPicture(icon,":/pics/encripted.png"):icon;
    icon = is_link?addPicture(icon,":/pics/symlink.png"):icon;
    return icon;
}

QIcon IconProvider::icon(const QFileInfo & info) const {
    if (info.isDir()) return QFileIconProvider::icon(info);
    else return fileIcon(info.fileName());
}

QIcon IconProvider::addPicture(const QIcon & icon,const QString & picture_path) const {
    QPixmap pixmap = icon.pixmap(MAX_ICON_SIZE,MAX_ICON_SIZE);
    if (pixmap.size().width() < MAX_ICON_SIZE) pixmap = pixmap.scaled(QSize(MAX_ICON_SIZE,MAX_ICON_SIZE),Qt::KeepAspectRatio,Qt::SmoothTransformation);
    QPainter(&pixmap).drawPixmap(QRect(0,0,48,48),QPixmap(picture_path),QRect(0,0,48,48));
    return QIcon(pixmap);
}
