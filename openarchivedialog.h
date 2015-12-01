#ifndef OPENARCHIVEDIALOG_H
#define OPENARCHIVEDIALOG_H

#include <QFileDialog>

class QCheckBox;
class QListView;
class QTreeView;

class OpenArchiveDialog : public QFileDialog {
    Q_OBJECT
public:
    explicit OpenArchiveDialog(QWidget *parent = 0);
    bool doExtractTar() const;

private slots:
    void onSelectionChanged(const QItemSelection & sel,const QItemSelection & desel);
    void accepted();

private:
    QCheckBox * tarCheck;
    QListView * listview;
    QTreeView * treeview;
};

#endif // OPENARCHIVEDIALOG_H
