#include "createarchivedialog.h"
#include "iconprovider.h"
#include "basearchengine.h"
#include <QBoxLayout>
#include <QDir>
#include <QIcon>
#include <QCheckBox>
#include <QHBoxLayout>

CreateArchiveDialog::CreateArchiveDialog(bool encriptCheckBoxVisible,bool fullPathCheckBoxVisible,bool useCurrentDir,QWidget *parent) : QFileDialog(parent) {
    setWindowIcon(QIcon(":/pics/utilities-file-archiver.png"));
    setAcceptMode(QFileDialog::AcceptSave);
    setOption(QFileDialog::DontUseNativeDialog);
    setLabelText(QFileDialog::Accept,tr("Create"));
    setIconProvider(IconProvider::instance());
    setWindowTitle(tr("Select the file name of new archive"));
    setDirectory(useCurrentDir?QDir::currentPath():BaseArchEngine::defaultSaveDir);
    setFileMode(QFileDialog::AnyFile);

    QStringList name_filters = BaseArchEngine::findCreateFilters();
    if (!BaseArchEngine::defaultSaveFilter.isEmpty()) {
        int index = name_filters.indexOf(BaseArchEngine::defaultSaveFilter);
        if (index >= 0) name_filters.swap(0,index);
    }

    setNameFilters(name_filters);

    QGridLayout *mainLayout = findChild<QGridLayout*>();
    Q_ASSERT(mainLayout);

    QHBoxLayout *hblayout = new QHBoxLayout(this);
    fullPathCheck = new QCheckBox(tr("Insert the files into archive with the full path"),this);
    fullPathCheck->setChecked(false);
    hblayout->addWidget(fullPathCheck);
    fullPathCheck->setVisible(fullPathCheckBoxVisible);
    encriptCheck = new QCheckBox(tr("Encrypt the files (if file format allows this)"),this);
    hblayout->addWidget(encriptCheck);
    encriptCheck->setChecked(false);
    encriptCheck->setVisible(encriptCheckBoxVisible);
    int numRows = mainLayout->rowCount();
    mainLayout->addLayout(hblayout,numRows,0,1,-1);

    connect(this,SIGNAL(accepted()),this,SLOT(accepted()));
}

void CreateArchiveDialog::accepted() {
    BaseArchEngine::defaultSaveFilter = selectedNameFilter();
    BaseArchEngine::defaultSaveDir = QFileInfo(selectedFile()).path();
}

QString CreateArchiveDialog::selectedFile() const {
    QStringList files = selectedFiles();
    if (files.count() <= 0) return QString();

    if (endsWithOneOfSuffix(files[0])) return files[0];

    QString first_suffix = firstSuffix();
    if (first_suffix.isEmpty()) return files[0];
    return files[0]+"."+first_suffix;
}

QString CreateArchiveDialog::firstSuffix() const {
    QString filter = selectedNameFilter().trimmed();
    if (filter.endsWith(')')) filter.chop(1);
    int index = filter.indexOf('(');
    if (index != -1) filter = filter.mid(index+1);

    QStringList parts = filter.split(' ',QString::SkipEmptyParts);
    if (parts.count() > 1) filter = parts[0];

    if (filter.startsWith("*.")) filter = filter.mid(2);

    return filter;
}

bool CreateArchiveDialog::endsWithOneOfSuffix(const QString & fileName) const {
    QString filter = selectedNameFilter().trimmed();
    if (filter.endsWith(')')) filter.chop(1);
    int index = filter.indexOf('(');
    if (index != -1) filter = filter.mid(index+1);

    QStringList parts = filter.split(' ',QString::SkipEmptyParts);
    for (int i=0;i<parts.count();i++) {
        filter = parts[i];
        if (filter.startsWith("*.")) filter = filter.mid(1);
        if (fileName.endsWith(filter)) return true;
    }

    return false;
}

bool CreateArchiveDialog::useEncription() const {
    return encriptCheck->isChecked();
}

bool CreateArchiveDialog::useFullPath() const {
    return fullPathCheck->isChecked();
}
