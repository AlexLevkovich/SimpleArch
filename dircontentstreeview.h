#ifndef DirContentsTreeView_H
#define DirContentsTreeView_H

#include "basetreeview.h"

class DirContentsTreeView : public BaseTreeView  {
    Q_OBJECT
public:
    DirContentsTreeView(QWidget * parent = NULL);
    void setModel(QAbstractItemModel * model);
    bool selectPrev(bool testing_only = false);
    bool selectNext(bool testing_only = false);
    bool selectUp(bool testing_only = false);
    void selectRoot();

public slots:
    void setRootIndex(const QModelIndex & index);

protected slots:
    void index_activated(const QModelIndex & index);

protected:
    bool isLeftView() const { return false; }
    bool isColumnVisible(int column) const;
    uint iconSize() const;
    void rowsInserted(const QModelIndex & parent,int start,int end);

private:
    QModelIndexList history;
    int current_index;
    QModelIndex root_index;
};

#endif // DirContentsTreeView_H
