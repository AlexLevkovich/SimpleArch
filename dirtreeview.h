#ifndef DIRTREEVIEW_H
#define DIRTREEVIEW_H

#include "basetreeview.h"

class FileTreeModel;

class DirTreeView : public BaseTreeView  {
    Q_OBJECT
public:
    DirTreeView(QWidget * parent = NULL);

public slots:
    void select_dir(const QModelIndex & index);
    void updateView();
    void selectRoot();

protected slots:
    void index_activated(const QModelIndex & index);

private slots:
    void slot_selectionChanged(const QItemSelection & selected,const QItemSelection & deselected);
    void update();
    void rowsCountChanged(const QModelIndex & parent, int start, int end);
    void onError();
    void post_onError();

signals:
    void modelUpdated();
    void selectionChanged(const QModelIndex & selected);
    void long_processing_started();
    void long_processing_completed(bool was_error);

protected:
    bool isLeftView() const { return true; }
    bool isColumnVisible(int column) const;
    uint iconSize() const;

private:
    FileTreeModel * temp_model;
};

#endif // DIRTREEVIEW_H
