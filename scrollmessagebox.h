#ifndef SCROLLMESSAGEBOX_H
#define SCROLLMESSAGEBOX_H

#include <QDialog>
#include <QMessageBox>
#include <QPixmap>

class QPushButton;
class QDialogButtonBox;

class ScrollMessageBox : public QDialog {
    Q_OBJECT
public:
    ScrollMessageBox(QMessageBox::Icon icon, QString const& title, QString const& labelTitle,
                     QString const& text,QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                     QWidget* parent = 0);

    void setDefaultButton(QMessageBox::StandardButton button);

    static QMessageBox::StandardButton critical(QWidget* parent, QString const& title, QString const& labelTitle, QString const& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static QMessageBox::StandardButton information(QWidget* parent, QString const& title, QString const& labelTitle, QString const& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static QMessageBox::StandardButton question(QWidget* parent, QString const& title, QString const& labelTitle, QString const& text, QMessageBox::StandardButtons buttons = QMessageBox::Yes | QMessageBox::No, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);
    static QMessageBox::StandardButton warning(QWidget* parent, QString const& title, QString const& labelTitle, QString const& text, QMessageBox::StandardButtons buttons = QMessageBox::Ok, QMessageBox::StandardButton defaultButton = QMessageBox::NoButton);

private:
    QPixmap standardIcon(QMessageBox::Icon icon);
    void setDefaultButton(QPushButton *button);

    QDialogButtonBox *buttonBox;

private slots:
    void handle_buttonClicked(QAbstractButton *button);
};

#endif // SCROLLMESSAGEBOX_H
