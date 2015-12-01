#ifndef FILESSELECTORDIALOG_H
#define FILESSELECTORDIALOG_H

#include <QFileDialog>

class QCheckBox;
class QListView;
class QTreeView;
class QPushButton;
class QLineEdit;

class FilesSelectorDialog : public QFileDialog {
    Q_OBJECT
public:
    explicit FilesSelectorDialog(bool encriptCheckBoxVisible,QWidget *parent = 0);
    bool useFullPath() const;
    bool useEncription() const;
    QStringList selectedFiles() const;

private slots:
    void onSelectionChanged();
    void accepted();
    void accept();

private:
    QCheckBox * fullPathCheck;
    QCheckBox * encriptCheck;
    QListView * listview;
    QTreeView * treeview;
    QPushButton * okButton;
    QLineEdit *lineEdit;
    QStringList m_selected_files;
};

#endif // FILESSELECTORDIALOG_H
