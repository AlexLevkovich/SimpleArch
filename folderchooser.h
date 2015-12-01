#ifndef FOLDERCHOOSER_H
#define FOLDERCHOOSER_H

#include <QDialog>

namespace Ui {
class FolderChooser;
}

class FolderChooser : public QDialog {
    Q_OBJECT

public:
    FolderChooser(const QString & start_folder,QWidget *parent = 0);
    ~FolderChooser();
    QString folderPath() const;

private slots:
    void selectionChanged();

protected:
    Ui::FolderChooser *ui;
};

#endif // FOLDERCHOOSER_H
