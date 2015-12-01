#include <QStringList>
#include <QDirIterator>
#include "filetreeitem.h"
#include "iconprovider.h"

const QByteArray FileTreeItem::permissions(const QFileInfo & info) {
    QByteArray ret(info.isDir()?"d":"-");
    ret += (info.permission(QFile::ReadOwner))?"r":"-";
    ret += (info.permission(QFile::WriteOwner))?"w":"-";
    ret += (info.permission(QFile::ExeOwner))?"x":"-";
    ret += (info.permission(QFile::ReadGroup))?"r":"-";
    ret += (info.permission(QFile::WriteGroup))?"w":"-";
    ret += (info.permission(QFile::ExeGroup))?"x":"-";
    ret += (info.permission(QFile::ReadOther))?"r":"-";
    ret += (info.permission(QFile::WriteOther))?"w":"-";
    ret += (info.permission(QFile::ExeOther))?"x":"-";

    return ret;
}

FileTreeItem::FileTreeItem() {
    m_parentItem = NULL;
    m_perms = "-r--------";
}

FileTreeItem::FileTreeItem(const QFileInfo & fileInfo,const FileTreeItem * parentItem,
                           bool scan_subdirs,bool encripted) {
    m_path = (parentItem->rootItem?"":parentItem->archivePath()) + fileInfo.fileName();
    if (fileInfo.isDir()) m_path += "/";
    m_perms = permissions(fileInfo);
    m_user = fileInfo.owner();
    m_group = fileInfo.group();
    m_link_to = fileInfo.symLinkTarget();
    m_file_size = fileInfo.size();
    m_date = fileInfo.lastModified();
    m_parentItem = (FileTreeItem *)parentItem;
    m_encripted = encripted;
    rootItem = false;
    if (is_dir()) {
        if (scan_subdirs) {
            QDirIterator di(fileInfo.filePath(),QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
            while (di.hasNext()) {
                di.next();
                appendChild(new FileTreeItem(QFileInfo(di.filePath()),this,true,encripted));
            }
        }
    }
}

FileTreeItem::FileTreeItem(const QString & file_path,const QString & link_to,
                           const QByteArray & perms,const QString & user,const QString & group,
                           qreal file_size,const QDateTime & date,bool encripted) {
    m_path = file_path;
    m_link_to = link_to;
    m_perms = perms;
    m_user = user;
    m_group = group;
    m_file_size = file_size;
    m_date = date;
    m_encripted = encripted;
    rootItem = false;
}

FileTreeItem::~FileTreeItem() {
    qDeleteAll(m_childItems);
}

FileTreeItem * FileTreeItem::createRootItem(const QString & fileName) {
    FileTreeItem * item = new FileTreeItem();
    item->m_parentItem = NULL;
    item->m_path = QFileInfo(fileName).fileName();
    item->m_perms = "-r--------";
    item->m_encripted = false;
    item->icon();
    item->m_perms[0] = 'd';
    item->m_file_size = 0;
    item->m_date = QDateTime::currentDateTime();
    item->rootItem = true;

    return item;
}

FileTreeItem * FileTreeItem::dup(FileTreeItem * item,bool doCopyParentAndChilden) {
    if (item == NULL) return NULL;

    FileTreeItem * dup_item = new FileTreeItem();
    if (doCopyParentAndChilden) {
        dup_item->m_childItems = item->m_childItems;
        dup_item->m_parentItem = item->m_parentItem;
    }
    else dup_item->m_parentItem = NULL;
    dup_item->m_path = item->m_path;
    dup_item->m_link_to = item->m_link_to;
    dup_item->m_perms = item->m_perms;
    dup_item->m_user = item->m_user;
    dup_item->m_group = item->m_group;
    dup_item->m_file_size = item->m_file_size;
    dup_item->m_date = item->m_date;
    dup_item->m_icon = item->m_icon;
    dup_item->m_encripted = item->m_encripted;

    return dup_item;
}

FileTreeItem * FileTreeItem::findPathAfterItem(const QString & _path,const FileTreeItem * _parentItem) {
    FileTreeItem * parentItem = (FileTreeItem *)_parentItem;
    QString path = _path;
    if (path.endsWith("/")) path = path.left(path.length()-1);
    path = QFileInfo(path).path();

    QStringList parts = path.split("/",QString::SkipEmptyParts);
    for (int i=0;i<parts.count();i++) {
        int index = parentItem->childIndex(parts[i]);
        if (index < 0) return NULL;
        parentItem = parentItem->child(index);
    }

    return parentItem;
}

FileTreeItem * FileTreeItem::findOrCreatePathAfterItem(const QString & _path,const FileTreeItem * _parentItem) {
    FileTreeItem * parentItem = (FileTreeItem *)_parentItem;
    QString path = _path;
    if (path.endsWith("/")) path = path.left(path.length()-1);
    path = QFileInfo(path).path();

    QStringList parts = path.split("/",QString::SkipEmptyParts);
    bool no_more_parts = false;
    path.clear();
    for (int i=0;i<parts.count();i++) {
        path += "/" + parts[i];
        int index = no_more_parts?-1:parentItem->childIndex(parts[i]);
        if (index < 0) {
            FileTreeItem * childItem = new FileTreeItem(QFileInfo(path),parentItem,false);
            parentItem->appendChild(childItem);
            parentItem = childItem;
            no_more_parts = true;
        }
        else parentItem = parentItem->child(index);
    }

    return parentItem;
}

QString FileTreeItem::link_to() const {
    return m_link_to;
}

QString FileTreeItem::path() const {
    QString path = m_path;
    if (path.startsWith("./")) path = path.mid(2);
    if (path.endsWith('/')) path.chop(1);
    int index = path.lastIndexOf('/');
    if (index < 0) return "";
    path.chop(path.length()-index-1);

    return path;
}

QString FileTreeItem::archivePath() const {
    return m_path;
}

QString FileTreeItem::name() const {
    if (m_path == "./") return "";

    QString path = m_path;
    if (path.endsWith('/')) path.chop(1);
    int index = path.lastIndexOf('/');
    if (index < 0) return path;
    return path.mid(index+1);
}

QByteArray FileTreeItem::perms() const {
    return m_perms;
}

QString FileTreeItem::user() const {
    return m_user;
}

QString FileTreeItem::group() const {
    return m_group;
}

qreal FileTreeItem::file_size() const {
    return m_file_size;
}

QDateTime FileTreeItem::date() const {
    return m_date;
}

QIcon FileTreeItem::icon() const {
    if (m_icon.isNull()) {
        FileTreeItem * p_this = (FileTreeItem *)this;
        if (is_dir()) p_this->m_icon=IconProvider::instance()->dirIcon(m_encripted,is_link());
        else p_this->m_icon=IconProvider::instance()->fileIcon(name(),m_encripted,is_link());
    }

    return m_icon;
}

bool FileTreeItem::isEncripted() const {
    return m_encripted;
}

bool FileTreeItem::is_dir() const {
    return m_perms.startsWith("d");
}

bool FileTreeItem::is_link() const {
    return m_perms.startsWith("l");
}

void FileTreeItem::appendChild(FileTreeItem *item) {
    m_childItems.append(item);
}

bool FileTreeItem::insertChild(FileTreeItem *item,uint pos) {
    if (pos > childCount()) return false;
    m_childItems.insert(pos,item);
    return true;
}

bool FileTreeItem::removeChild(uint pos) {
    if (pos >= childCount()) return false;
    FileTreeItem * childItem = child(pos);
    m_childItems.removeAt(pos);
    delete childItem;
    return true;
}

int FileTreeItem::childIndex(const QString & name) const {
    FileTreeItem * p_this = (FileTreeItem *)this;
    for (int i=0;i<childCount();i++) {
        if (p_this->child(i)->name() == name) return i;
    }
    return -1;
}

FileTreeItem *FileTreeItem::child(int row) {
    if (row >= childCount()) return NULL;

    return m_childItems.at(row);
}

int FileTreeItem::childCount() const {
    return m_childItems.count();
}

FileTreeItem *FileTreeItem::parentItem() {
    return m_parentItem;
}

void FileTreeItem::setParentItem(FileTreeItem * parent) {
    m_parentItem = parent;
}

int FileTreeItem::row() const {
    if (m_parentItem) return m_parentItem->m_childItems.indexOf(const_cast<FileTreeItem*>(this));

    return -1;
} 
