#include "mimesettingspage.h"
#include "ui_mimesettingspage.h"
#include "basearchengine.h"

MimeSettingsPage::MimeSettingsPage(QWidget *parent) : QWidget(parent), ui(new Ui::MimeSettingsPage) {
    ui->setupUi(this);

    ui->systemMimeCheck->setChecked(BaseArchEngine::defaultUseSystemMimes);
    ui->ownMimesTable->addUserMimes(BaseArchEngine::userMimes);

    connect(parent,SIGNAL(accepted()),this,SLOT(parent_accepted()));
}

MimeSettingsPage::~MimeSettingsPage() {
    delete ui;
}

void MimeSettingsPage::parent_accepted() {
    BaseArchEngine::defaultUseSystemMimes = ui->systemMimeCheck->isChecked();
    BaseArchEngine::userMimes = ui->ownMimesTable->userMimes();
}
