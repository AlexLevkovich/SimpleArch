#ifndef CATEGORYLISTVIEW_H
#define CATEGORYLISTVIEW_H

#include <QListView>

class CategoryListView : public QListView {
    Q_OBJECT
public:
    CategoryListView(QWidget *parent = 0);
    void addCategory(const QString & displayName,QWidget *widget,const QIcon & icon = QIcon());

protected:
    virtual QSize sizeHint() const;
    virtual bool eventFilter(QObject *obj, QEvent *event);
    virtual bool event(QEvent *event);
    virtual void selectionChanged(const QItemSelection & selected,const QItemSelection & deselected);
};

#endif // CATEGORYLISTVIEW_H
