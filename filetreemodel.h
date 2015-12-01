#ifndef FileTreeModel_H
#define FileTreeModel_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

class FileTreeItem;
class DragArchEngine;
class QMimeData;

class FileTreeModel : public QAbstractItemModel {
    Q_OBJECT

public:
    FileTreeModel(const QString & filePath,QObject *parent = 0);
    ~FileTreeModel();

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const ;
    Qt::ItemFlags flags(const QModelIndex &index) const ;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const ;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const ;
    QModelIndex index(int row, int column,
                      FileTreeItem * parentItem) const ;
    QModelIndex index(FileTreeItem * childItem,
                      int row, int column) const ;
    QModelIndex parent(const QModelIndex &index) const ;
    int rowCount(const QModelIndex &parent = QModelIndex()) const ;
    int columnCount(const QModelIndex &parent = QModelIndex()) const ;
    QMimeData * mimeData(const QModelIndexList & indexes) const;
    bool insertRows(const QModelIndex & parent,const QStringList & files,bool use_full_path,bool password_protected = false);
    QModelIndexList match(const QModelIndex & start,int role,bool (*compare)(const QModelIndex &),int hits = 1,bool recurse = true,bool wrap = false) const;

protected:
    bool removeRows(int row,int count,const QModelIndex & parent = QModelIndex());
    Qt::DropActions supportedDropActions() const;
    Qt::DropActions supportedDragActions() const;
    bool dropMimeData(const QMimeData * data,Qt::DropAction action,int row,int column,const QModelIndex & parent);
    QStringList mimeTypes() const;

public slots:
    void importModelItem(FileTreeItem * item);

signals:
    void item_found(const QModelIndex & parentIndex,const QModelIndex & index);

private:
    void findOrCreateFullPathOfItem(FileTreeItem * item);
    void createRootItem();

    FileTreeItem *rootItem;
    FileTreeItem *rootItemDir;
    FileTreeItem *prevImportedItem;
    QString m_filePath;
    DragArchEngine * engine;
};

#endif // FileTreeModel_H
