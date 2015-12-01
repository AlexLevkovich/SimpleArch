#ifndef COMPRESSIONLEVELSSETTINGSPAGE_H
#define COMPRESSIONLEVELSSETTINGSPAGE_H

#include <QWidget>

namespace Ui {
class CompressionLevelsSettingsPage;
}

class CompressionLevelsSettingsPage : public QWidget {
    Q_OBJECT

public:
    explicit CompressionLevelsSettingsPage(QWidget *parent = 0);
    ~CompressionLevelsSettingsPage();

private slots:
    void parent_accepted();

private:
    Ui::CompressionLevelsSettingsPage *ui;
};

#endif // COMPRESSIONLEVELSSETTINGSPAGE_H
