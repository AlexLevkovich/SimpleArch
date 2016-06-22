#ifndef FileTreeItem_H
#define FileTreeItem_H

#include <QList>
#include <QDateTime>
#include <QIcon>
#include <QFileInfo>

class FileTreeItem {
public:
    FileTreeItem();
    FileTreeItem(const QFileInfo & fileInfo,const FileTreeItem * parentItem = NULL,
                 bool scan_subdirs = true,bool encripted = false);
    FileTreeItem(const QString & file_path,const QString & link_to,
                 const QByteArray & perms,const QString & user,const QString & group,
                 qreal file_size,const QDateTime & date,bool encripted = false);
    ~FileTreeItem();

    void appendChild(FileTreeItem *child);
    bool insertChild(FileTreeItem *child,uint pos);
    bool removeChild(uint pos);

    FileTreeItem *child(int row);
    int childCount() const;
    int childIndex(const QString & name) const;
    int childIndex(const FileTreeItem * item) const;
    int row() const;
    FileTreeItem *parentItem();
    void setParentItem(FileTreeItem * parent);
    QString link_to() const;
    QString path() const;
    QString archivePath() const;
    QString name() const;
    QByteArray perms() const;
    QString user() const;
    QString group() const;
    qreal file_size() const;
    QDateTime date() const;
    bool is_dir() const;
    bool is_link() const;
    char type() const;
    QIcon icon() const;
    bool isEncripted() const;

    static FileTreeItem * dup(FileTreeItem * item,bool doCopyParentAndChilden = true);
    static FileTreeItem * createRootItem(const QString & fileName);
    static const QByteArray permissions(const QFileInfo & info);
    static FileTreeItem * findPathAfterItem(const QString & path,const FileTreeItem * parentItem);
    static FileTreeItem * findOrCreatePathAfterItem(const QString & path,const FileTreeItem * parentItem);

private:
    QList<FileTreeItem*> m_childItems;
    FileTreeItem *m_parentItem;
    QString m_link_to;
    QByteArray m_perms;
    QString m_user;
    QString m_group;
    qreal m_file_size;
    QDateTime m_date;
    QIcon m_icon;
    QString m_path;
    bool m_encripted;
    bool rootItem;
};

#endif // FileTreeItem_H
