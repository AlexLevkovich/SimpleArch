#ifndef EXTRACTFOLDERCHOOSER_H
#define EXTRACTFOLDERCHOOSER_H

#include "folderchooser.h"

class QCheckBox;

class ExtractFolderChooser : public FolderChooser {
    Q_OBJECT
public:
    explicit ExtractFolderChooser(QWidget *parent = 0);
    bool useFullPath() const;
    bool extractSelectedItemsOnly() const;

private slots:
    void accepted();

private:
    QCheckBox * fullCheck;
    QCheckBox * extractCheck;
};

#endif // EXTRACTFOLDERCHOOSER_H
