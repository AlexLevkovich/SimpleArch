#ifndef MIMESETTINGSPAGE_H
#define MIMESETTINGSPAGE_H

#include <QWidget>

namespace Ui {
class MimeSettingsPage;
}

class MimeSettingsPage : public QWidget {
    Q_OBJECT

public:
    explicit MimeSettingsPage(QWidget *parent);
    ~MimeSettingsPage();

private slots:
    void parent_accepted();

private:
    Ui::MimeSettingsPage *ui;
};

#endif // MIMESETTINGSPAGE_H
