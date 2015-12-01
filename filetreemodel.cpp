#include "filetreeitem.h"
#include "filetreemodel.h"
#include "byteshumanizer.h"
#include <QStringList>
#include <QDateTime>
#include <QDebug>
#include <QUrl>
#include <QMimeData>
#include "dragarchengine.h"
#include "droparchengine.h"
#include "removearchengine.h"
#include "basearchengine.h"
#include "staticutils.h"

#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>

const QString getUserName() {
  QString userName;
  register struct passwd *pw;
  pw = getpwuid(getuid());
  if (pw)
  userName = pw->pw_name;
  return userName;
}

FileTreeModel::FileTreeModel(const QString & filePath,QObject *parent) : QAbstractItemModel(parent) {
    m_filePath = filePath;
    engine = NULL;
    createRootItem();
}

FileTreeModel::~FileTreeModel() {
    delete rootItem;
}

void FileTreeModel::createRootItem() {
    rootItem = new FileTreeItem();
    rootItemDir = FileTreeItem::createRootItem(m_filePath);
    rootItemDir->setParentItem(rootItem);
    rootItem->appendChild(rootItemDir);
}

int FileTreeModel::columnCount(const QModelIndex &/*parent*/) const {
    return 9;
}

QVariant FileTreeModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return QVariant();

    FileTreeItem *item = static_cast<FileTreeItem*>(index.internalPointer());

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case 0: return item->name();
            case 1: return item->link_to();
            case 2: return item->perms();
            case 3: return item->user();
            case 4: return item->group();
            case 5: return BytesHumanizer(item->file_size()).toString();
            case 6: return (BaseArchEngine::defaultDateFormat == BaseArchEngine::ISO)?item->date().toString("yyyy-MM-dd mm:ss"):item->date().toString(Qt::SystemLocaleShortDate);
            case 7: return item->path();
            case 8: return item->archivePath();
        }
    }

    if (role == Qt::DecorationRole && index.column() == 0) {
        return item->icon();
    }

    if (role == Qt::UserRole) {
        switch (index.column()) {
            case 0: return (item == rootItemDir);
            case 5: return QVariant::fromValue<qreal>(item->file_size());
        }
    }

    return QVariant();
}

QVariant FileTreeModel::headerData(int section, Qt::Orientation /*orientation*/,int role) const {
    if (role != Qt::DisplayRole) return QVariant();

    switch (section) {
    case 0: return tr("File name");
    case 1: return tr("Links to");
    case 2: return tr("Permissions");
    case 3: return tr("Owner");
    case 4: return tr("Group");
    case 5: return tr("File length");
    case 6: return tr("Date");
    // should be hidden
    case 7: return "Path";
    case 8: return "ArchivePath";
    }
    return QVariant();
}

Qt::ItemFlags FileTreeModel::flags(const QModelIndex &index) const {
    if (!index.isValid())
        return 0;

    FileTreeItem * item = static_cast<FileTreeItem*>(index.internalPointer());

    bool isReadOnly = (BaseArchEngine::main_arch_engine != NULL) && BaseArchEngine::main_arch_engine->readOnly();

    if ((rootItemDir != NULL) && (item == rootItemDir)) return Qt::ItemIsSelectable | Qt::ItemIsEnabled | (isReadOnly?(Qt::ItemFlag)0:Qt::ItemIsDropEnabled);

    if (item->isEncripted()) return Qt::ItemIsSelectable | Qt::ItemIsEnabled | (isReadOnly?(Qt::ItemFlag)0:Qt::ItemIsDropEnabled);

    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | (isReadOnly?(Qt::ItemFlag)0:Qt::ItemIsDropEnabled);
}

QModelIndex FileTreeModel::index(int row, int column, const QModelIndex &parent) const {
    if (rootItemDir == NULL) return QModelIndex();

    FileTreeItem *parentItem;

    if (!parent.isValid()) parentItem = rootItem;
    else parentItem = static_cast<FileTreeItem*>(parent.internalPointer());

    FileTreeItem *childItem = parentItem->child(row);
    if (childItem) return createIndex(row, column, childItem);

    return QModelIndex();
}

QModelIndex FileTreeModel::index(int row, int column,FileTreeItem * parentItem) const {
    FileTreeItem *childItem = parentItem->child(row);
    if (childItem) return createIndex(row, column, childItem);

    return QModelIndex();
}

QModelIndex FileTreeModel::index(FileTreeItem * childItem,int row, int column) const {
    if (childItem) return createIndex(row, column, childItem);
    return QModelIndex();
}

QModelIndex FileTreeModel::parent(const QModelIndex &index) const {
    if (!index.isValid()) return QModelIndex();

    FileTreeItem *childItem = static_cast<FileTreeItem*>(index.internalPointer());
    FileTreeItem *parentItem = childItem->parentItem();
    if (parentItem == NULL) return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int FileTreeModel::rowCount(const QModelIndex &parent) const {
    FileTreeItem *parentItem;
    if (parent.column() > 0) return 0;

    if (!parent.isValid()) parentItem = rootItem;
    else parentItem = static_cast<FileTreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}

void FileTreeModel::findOrCreateFullPathOfItem(FileTreeItem * item) {
    QStringList parts = item->path().split("/",QString::SkipEmptyParts);
    FileTreeItem * parent_item = rootItemDir;

    if ((prevImportedItem != NULL) && !prevImportedItem->path().isEmpty() && (item->path().startsWith(prevImportedItem->path()))) {
        parts = item->path().mid(prevImportedItem->path().length()).split("/",QString::SkipEmptyParts);
        parent_item = prevImportedItem->parentItem();
    }

    int i;
    int index = -1;
    for (i=0;(i < parts.count()) && ((index = parent_item->childIndex(parts[i])) >= 0);i++) {
        parent_item = parent_item->child(index);
    }

    if (i < parts.count()) {
        for (int j=i;j < parts.count();j++) {
            FileTreeItem * sub_dir_item = new FileTreeItem(((parent_item != rootItemDir)?(parent_item->path() + parent_item->name() + "/"):"") + parts[j] + "/","","d"+item->perms().mid(1),item->user(),item->group(),0,item->date());
            parent_item->appendChild(sub_dir_item);
            sub_dir_item->setParentItem(parent_item);
            parent_item = sub_dir_item;
        }
    }

    if (parent_item->childIndex(item->name()) < 0) {
        parent_item->appendChild(item);
        item->setParentItem(parent_item);
        prevImportedItem = item;
    }
    else delete item;
}

void FileTreeModel::importModelItem(FileTreeItem * item) {
    if (rootItem == NULL) return;
    if (item == NULL) return;
    if (item->path().isEmpty() && item->name().isEmpty()) {
        delete item;
        return;
    }

    if (rootItemDir->childCount() > 0) {
        if (prevImportedItem->path() == item->path()) {
            prevImportedItem->parentItem()->appendChild(item);
            item->setParentItem(prevImportedItem->parentItem());
            prevImportedItem = item;
        }
        else if (prevImportedItem->is_dir() && (prevImportedItem->path() + prevImportedItem->name() + "/") == item->path()) {
            prevImportedItem->appendChild(item);
            item->setParentItem(prevImportedItem);
            prevImportedItem = item;
        }
        else findOrCreateFullPathOfItem(item);
    }
    else {
        prevImportedItem = NULL;
        findOrCreateFullPathOfItem(item);
    }
}

QMimeData * FileTreeModel::mimeData(const QModelIndexList & indexes) const {
    if (BaseArchEngine::main_arch_engine == NULL || indexes.count() <= 0) return NULL;

    QStringList pathes;
    for (int i=0;i<indexes.count();i++) {
        if (indexes[i].column() != 0) continue;
        FileTreeItem * item = static_cast<FileTreeItem*>(indexes[i].internalPointer());
        if (item == rootItemDir) continue;
        pathes.append(item->archivePath());
    }

    FileTreeModel * p_this = (FileTreeModel *)this;
    if (p_this->engine != NULL && pathes == p_this->engine->files()) return p_this->engine->mimeData();
    if (p_this->engine != NULL) delete p_this->engine;
    p_this->engine = new DragArchEngine(BaseArchEngine::main_arch_engine,pathes,p_this);
    StaticUtils::setAppWaitStatus(tr("Extracting files from archive..."));
    QMimeData * ret = p_this->engine->exec();
    StaticUtils::restoreAppStatus();
    return ret;
}

Qt::DropActions FileTreeModel::supportedDropActions() const {
    return Qt::CopyAction;
}

Qt::DropActions FileTreeModel::supportedDragActions() const {
    return Qt::CopyAction;
}

bool FileTreeModel::dropMimeData(const QMimeData * data,Qt::DropAction,int row,int,const QModelIndex & _parent) {
    if (BaseArchEngine::main_arch_engine == NULL) return false;

    QModelIndex parent = _parent;
    QList<QUrl> urls = data->urls();
    if ((urls.count() > 0) && (row < 0)) {
        FileTreeItem * parent_item = static_cast<FileTreeItem*>(parent.internalPointer());
        if (!parent_item->is_dir()) {
            parent_item = parent_item->parentItem();
            parent = index(parent_item->row(),0,parent_item->parentItem());
        }

        if (BaseArchEngine::main_arch_engine->isMultiFiles()) {
            for (int i=0;i<urls.count();i++) {
                QFileInfo info(urls[i].path());
                if (!urls[i].isLocalFile() || !info.exists()) continue;
                int index = parent_item->childIndex(info.fileName());
                if (index < 0) continue;
                beginRemoveRows(parent,index,index);
                parent_item->removeChild(index);
                endRemoveRows();
            }
        }

        row = parent_item->childCount();
        int last_row = row + urls.count() - 1;
        for (int i=0;i<urls.count();i++) {
            if (!urls[i].isLocalFile() || !QFileInfo(urls[i].path()).exists()) {
                last_row--;
            }
        }

        if (row > last_row) return false;

        StaticUtils::setAppWaitStatus(tr("Adding the files to archive..."));
        bool ret = DropArchEngine(BaseArchEngine::main_arch_engine,(parent_item == rootItemDir)?"":(parent_item->path()+parent_item->name()),urls).exec();
        StaticUtils::restoreAppStatus();
        if (!ret) return false;

        if (BaseArchEngine::main_arch_engine->isMultiFiles()) {
            beginInsertRows(parent,row,last_row);

            for (int i=0,k=0;i<urls.count();i++) {
                if (!urls[i].isLocalFile() || !QFileInfo(urls[i].path()).exists()) {
                    continue;
                }
                parent_item->insertChild(new FileTreeItem(QFileInfo(urls[i].path()),parent_item),row+k);
                k++;
            }
            endInsertRows();
        }
    }
    else return false;

    return true;
}

QStringList FileTreeModel::mimeTypes() const {
    return (QStringList() << "text/uri-list");
}

bool FileTreeModel::removeRows(int row,int count,const QModelIndex & parent) {
    if (BaseArchEngine::main_arch_engine == NULL) {
        beginRemoveRows(QModelIndex(),0,0);
        rootItem->removeChild(0);
        rootItemDir = NULL;
        endRemoveRows();
        return true;
    }

    if (count <= 0 || row < 0) return false;
    if (!parent.isValid()) return false;
    if (!BaseArchEngine::main_arch_engine->isMultiFiles()) return false;

    FileTreeItem * parent_item = static_cast<FileTreeItem*>(parent.internalPointer());
    if (row >= parent_item->childCount()) return false;

    QStringList fileNames;
    int last_row = row + count -1;
    for (int i=row;i<=last_row;i++) {
        FileTreeItem * child_item = parent_item->child(i);
        fileNames.append(child_item->archivePath());
    }

    StaticUtils::setAppWaitStatus(tr("Removing the files from archive..."));
    bool ret = RemoveArchEngine(BaseArchEngine::main_arch_engine,fileNames).exec();
    StaticUtils::restoreAppStatus();
    if (!ret) return false;

    beginRemoveRows(parent,row,last_row);
    for (int i=last_row;i>=row;i--) {
        parent_item->removeChild(i);
    }
    endRemoveRows();

    return true;
}

bool FileTreeModel::insertRows(const QModelIndex & parent,const QStringList & files,bool use_full_path,bool password_protected) {
    if (BaseArchEngine::main_arch_engine == NULL) return false;
    if (!parent.isValid()) return false;

    FileTreeItem * parent_item = static_cast<FileTreeItem*>(parent.internalPointer());

    if (BaseArchEngine::main_arch_engine->isMultiFiles()) {
        for (int i=0;i<files.count();i++) {
            QFileInfo info(files[i]);
            if (!info.exists()) continue;
            int index = parent_item->childIndex(info.fileName());
            if (index < 0) continue;
            beginRemoveRows(parent,index,index);
            parent_item->removeChild(index);
            endRemoveRows();
        }
    }

    int row = parent_item->childCount();
    int last_row = row + files.count() - 1;
    for (int i=0;i<files.count();i++) {
        if (!QFileInfo(files[i]).exists()) {
            last_row--;
        }
    }

    if (row > last_row) return false;

    StaticUtils::setAppWaitStatus(tr("Adding the files to archive..."));
    bool ret = DropArchEngine(BaseArchEngine::main_arch_engine,(parent_item == rootItemDir)?"":(parent_item->path()+parent_item->name()),files,use_full_path,password_protected).exec();
    StaticUtils::restoreAppStatus();
    if (!ret) return false;

    if (BaseArchEngine::main_arch_engine->isMultiFiles()) {
        for (int i=0;i<files.count();i++) {
            if (!QFileInfo(files[i]).exists()) {
                continue;
            }
            QFileInfo info(files[i]);
            FileTreeItem * _parent_item = parent_item;
            if (use_full_path) {
                _parent_item = FileTreeItem::findPathAfterItem(info.filePath(),parent_item);
                if (_parent_item == NULL) _parent_item = FileTreeItem::findOrCreatePathAfterItem(info.filePath(),parent_item);
                else last_row--;
            }
            _parent_item->appendChild(new FileTreeItem(info,_parent_item,true,password_protected));
        }

        beginInsertRows(parent,row,last_row);
        endInsertRows();
    }

    return true;
}

QModelIndexList FileTreeModel::match(const QModelIndex &start, int role,
                                     bool (*compare)(const QModelIndex &), int hits,
                                     bool recurse,bool wrap) const {
    QModelIndexList result;
    bool allHits = (hits == -1);
    QModelIndex p = parent(start);
    int from = start.row();
    int to = rowCount(p);

    // iterates twice if wrapping
    for (int i = 0; (wrap && i < 2) || (!wrap && i < 1); ++i) {
        for (int r = from; (r < to) && (allHits || result.count() < hits); ++r) {
            QModelIndex idx = index(r, start.column(), p);
            if (!idx.isValid())
                 continue;

            if (compare(idx)) result.append(idx);

            if (recurse && hasChildren(idx)) { // search the hierarchy
                result += match(index(0, idx.column(), idx), role,compare,
                                (allHits ? -1 : hits - result.count()),recurse,wrap);
            }
        }
        // prepare for the next iteration
        from = 0;
        to = start.row();
    }
    return result;
}
