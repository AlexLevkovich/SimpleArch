#include "archivesettingspage.h"
#include "ui_archivesettingspage.h"
#include "basearchengine.h"

ArchiveSettingsPage::ArchiveSettingsPage(QWidget *parent) : QWidget(parent), ui(new Ui::ArchiveSettingsPage) {
    ui->setupUi(this);

    ui->sortCheck->setChecked(BaseArchEngine::defaultDoFileNamesSorting);
    ui->treeCheck->setChecked(BaseArchEngine::defaultDoShowFolderTree);
    ui->highlightCheck->setChecked(BaseArchEngine::defaultHighlightFocusedView);

    setComboValue(ui->leftIconSizeCombo,BaseArchEngine::defaultLeftIconSize);
    setComboValue(ui->rightIconSizeCombo,BaseArchEngine::defaultRightIconSize);
    ui->dateCombo->setCurrentIndex((int)BaseArchEngine::defaultDateFormat);

    connect(parent,SIGNAL(accepted()),this,SLOT(parent_accepted()));
}

ArchiveSettingsPage::~ArchiveSettingsPage() {
    delete ui;
}

void ArchiveSettingsPage::parent_accepted() {
    BaseArchEngine::defaultDoFileNamesSorting = ui->sortCheck->isChecked();
    BaseArchEngine::defaultDoShowFolderTree = ui->treeCheck->isChecked();
    BaseArchEngine::defaultHighlightFocusedView = ui->highlightCheck->isChecked();
    BaseArchEngine::defaultLeftIconSize = comboValue(ui->leftIconSizeCombo);
    BaseArchEngine::defaultRightIconSize = comboValue(ui->rightIconSizeCombo);
    BaseArchEngine::defaultDateFormat = (BaseArchEngine::DateFormat)ui->dateCombo->currentIndex();
}

bool ArchiveSettingsPage::setComboValue(QComboBox * combo,uint value) {
    for (int i=0;i<combo->count();i++) {
        if (combo->itemText(i) == QString("%1 x %1").arg(value)) {
            combo->setCurrentIndex(i);
            return true;
        }
    }
    return false;
}

uint ArchiveSettingsPage::comboValue(QComboBox * combo) const {
    return combo->currentText().split(" x ").at(0).toUInt();
}
