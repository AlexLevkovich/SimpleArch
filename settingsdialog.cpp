#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent), ui(new Ui::SettingsDialog) {
    ui->setupUi(this);
    ui->categoriesList->addCategory(tr("Archive"),ui->ownsPage,QIcon(":/pics/utilities-file-archiver.png"));
    ui->categoriesList->addCategory(tr("Filetype associations"),ui->mimesPage,QIcon(":/pics/document-open.png"));
    ui->categoriesList->addCategory(tr("Default compression"),ui->levelsPage,QIcon(":/pics/chronometer.png"));
}

SettingsDialog::~SettingsDialog() {
    delete ui;
}
