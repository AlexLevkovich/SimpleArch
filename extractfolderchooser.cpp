#include "extractfolderchooser.h"
#include "basearchengine.h"
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QFileInfo>
#include "ui_folderchooser.h"

ExtractFolderChooser::ExtractFolderChooser(QWidget *parent) : FolderChooser(BaseArchEngine::defaultSaveDir,parent) {
    QBoxLayout *layout = ui->buttonBox->findChild<QBoxLayout*>();
    Q_ASSERT(layout);
    QVBoxLayout * vlayout = new QVBoxLayout(this);
    fullCheck = new QCheckBox(tr("Extract from archive with the full path"),ui->buttonBox);
    fullCheck->setChecked(BaseArchEngine::defaultUseFullPath);
    extractCheck = new QCheckBox(tr("Extract the selected files only"),ui->buttonBox);
    extractCheck->setChecked(BaseArchEngine::defaultUseSelectedFilesOnly);
    vlayout->insertWidget(0, fullCheck);
    vlayout->insertWidget(1, extractCheck);
    vlayout->setSpacing(0);
    layout->insertLayout(0,vlayout);

    connect(this,SIGNAL(accepted()),this,SLOT(accepted()));
}

bool ExtractFolderChooser::useFullPath() const {
    return fullCheck->isChecked();
}

bool ExtractFolderChooser::extractSelectedItemsOnly() const {
    return extractCheck->isChecked();
}

void ExtractFolderChooser::accepted() {
    BaseArchEngine::defaultSaveDir = QFileInfo(folderPath()).filePath();
    BaseArchEngine::defaultUseFullPath = fullCheck->isChecked();
    BaseArchEngine::defaultUseSelectedFilesOnly = extractCheck->isChecked();
}
