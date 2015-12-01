#ifndef ARCHIVESETTINGSPAGE_H
#define ARCHIVESETTINGSPAGE_H

#include <QWidget>

namespace Ui {
class ArchiveSettingsPage;
}

class QComboBox;

class ArchiveSettingsPage : public QWidget {
    Q_OBJECT

public:
    explicit ArchiveSettingsPage(QWidget *parent = 0);
    ~ArchiveSettingsPage();

private slots:
    void parent_accepted();

private:
    bool setComboValue(QComboBox * combo,uint value);
    uint comboValue(QComboBox * combo) const;

    Ui::ArchiveSettingsPage *ui;
};

#endif // ARCHIVESETTINGSPAGE_H
