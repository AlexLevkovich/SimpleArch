#ifndef BASETREEVIEW_H
#define BASETREEVIEW_H

#include <QTreeView>
#include <QAction>
#include <QMap>
#include <QMenu>
#include <QProxyStyle>

class QContextMenuEvent;

class MyProxyStyle : public QProxyStyle {
public:
    virtual void drawPrimitive(PrimitiveElement element, const QStyleOption * option,
                               QPainter * painter, const QWidget * widget = 0) const;
};

class BaseTreeView : public QTreeView {
    Q_OBJECT
public:
    explicit BaseTreeView(QWidget *parent = 0);
    bool isRowsDeletionPossible() const;
    bool isFilesInsertionPossible() const;
    bool isFilesExtractionPossible() const;
    bool calcAdditionalInfo(qreal & all_files_size,qreal & files_count);
    void addContextMenuAction(const QAction * action);
    void addContextMenuAction(const QAction * action,bool (*enabler)(BaseTreeView *));
    QString selectedPath() const;

    static bool openFile_enabler(BaseTreeView * view);

public slots:
    void deleteSelectedRows();
    void insertFiles(const QStringList & files,bool use_full_path,bool password_protected = false);
    void extractFiles(const QString & folderPath,bool use_full_path,bool extract_selected_only);
    void selectAllItems();
    void selectItemsByWildcard(const QString & exp);

private slots:
    void post_init();
    void sourceModelChanged();

protected:
    void select_root_dir();
    void select_all_items();
    void select_items(const QItemSelection & selection);
    void select_index(const QModelIndex & index);
    bool is_dir(const QModelIndex & index);
    bool is_link(const QModelIndex & index);
    char type(const QModelIndex & index);
    void contextMenuEvent(QContextMenuEvent * event);
    virtual bool isLeftView() const = 0;
    virtual bool isColumnVisible(int column) const = 0;
    virtual uint iconSize() const = 0;

signals:
    void dir_activated(const QModelIndex & index);
    void operationStatesUpdated();

protected slots:
    virtual void index_activated(const QModelIndex & index);
    void selectionChanged(const QItemSelection & selected,const QItemSelection & deselected);

private:
    QMenu menu;
    QMap<QAction *,bool (*)(BaseTreeView *)> enablers_map;

    static MyProxyStyle * no_item_current_selection_style;
    static bool files_only_compare(const QModelIndex & index);
};

#endif // BASETREEVIEW_H
