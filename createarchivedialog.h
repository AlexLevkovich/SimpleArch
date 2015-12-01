#ifndef CreateArchiveDialog_H
#define CreateArchiveDialog_H

#include <QFileDialog>

class QCheckBox;

class CreateArchiveDialog : public QFileDialog {
    Q_OBJECT
public:
    CreateArchiveDialog(bool encriptCheckBoxVisible,bool fullPathCheckBoxVisible,bool useCurrentDir,QWidget *parent = 0);
    QString selectedFile() const;
    bool useEncription() const;
    bool useFullPath() const;

private slots:
    void accepted();

private:
    QString firstSuffix() const;
    bool endsWithOneOfSuffix(const QString & fileName) const;

    QCheckBox * encriptCheck;
    QCheckBox * fullPathCheck;
};

#endif // CreateArchiveDialog_H
