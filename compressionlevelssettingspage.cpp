#include "compressionlevelssettingspage.h"
#include "ui_compressionlevelssettingspage.h"
#include "basearchengine.h"

CompressionLevelsSettingsPage::CompressionLevelsSettingsPage(QWidget *parent) : QWidget(parent), ui(new Ui::CompressionLevelsSettingsPage) {
    ui->setupUi(this);

    connect(parent,SIGNAL(accepted()),this,SLOT(parent_accepted()));
}

CompressionLevelsSettingsPage::~CompressionLevelsSettingsPage() {
    delete ui;
}

void CompressionLevelsSettingsPage::parent_accepted() {
    BaseArchEngine::setEnginesCompressionLevels(ui->levelsView->levelsMap());
}
