#include "scrollmessagebox.h"
#include <QScrollArea>
#include <QIcon>
#include <QStyle>
#include <QGridLayout>
#include <QAbstractButton>
#include <QPushButton>
#include <QApplication>
#include <QDesktopWidget>
#include <QFontMetrics>
#include <QTextStream>
#include <QDialogButtonBox>
#include <QScrollBar>
#include <QLabel>

ScrollMessageBox::ScrollMessageBox(QMessageBox::Icon icon, QString const& title,QString const& labelTitle,
                                   QString const& text, QMessageBox::StandardButtons buttons,
                                   QWidget* parent) :
                                   QDialog(parent,
                                   Qt::Dialog | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint) {
  QLabel * tlabel = new QLabel(this);
  tlabel->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
  tlabel->setText(labelTitle);

  QLabel * label = new QLabel(this);
  label->setTextInteractionFlags(Qt::TextInteractionFlags(style()->styleHint(QStyle::SH_MessageBox_TextInteractionFlags, 0, this)));
  label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
  label->setOpenExternalLinks(true);
  label->setContentsMargins(2, 0, 0, 0);
  label->setIndent(9);
  label->setMaximumSize(INT_MAX,INT_MAX);

  QScrollArea * scroll = new QScrollArea(this);
  scroll->setWidget(label);
  scroll->setWidgetResizable(true);

  QLabel * iconLabel = new QLabel(this);
  iconLabel->setPixmap(standardIcon((QMessageBox::Icon)icon));
  iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  buttonBox = new QDialogButtonBox((QDialogButtonBox::StandardButtons)((uint)buttons),Qt::Horizontal,this);
  buttonBox->setCenterButtons(style()->styleHint(QStyle::SH_MessageBox_CenterButtons, 0, this));
  QObject::connect(buttonBox, SIGNAL(clicked(QAbstractButton*)),
    this, SLOT(handle_buttonClicked(QAbstractButton*)));

  QGridLayout *grid = new QGridLayout(this);

  grid->addWidget(iconLabel, 0, 0, 2, 1, Qt::AlignTop);
  grid->addWidget(tlabel, 0, 1, 1, 1);
  grid->addWidget(scroll, 1, 1, 1, 1);
  grid->addWidget(buttonBox, 2, 0, 1, 2);
  grid->setSizeConstraint(QLayout::SetDefaultConstraint);
  setLayout(grid);

  if (!title.isEmpty() || !text.isEmpty()) {
    setWindowTitle(title);
    label->setText(text);
  }

  setModal(true);

  QSize screenSize = QApplication::desktop()->availableGeometry(QCursor::pos()).size();
  int width = label->sizeHint().width() + qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent);

  width = qMin(screenSize.width() - 200,width);

  int height = label->heightForWidth(width);
  height = qMin(height + qApp->style()->pixelMetric(QStyle::PM_ScrollBarExtent), screenSize.height() - 200);
  height -= buttonBox->sizeHint().height() - (3*layout()->spacing());

  scroll->setMinimumSize(width, height);
}

QPixmap ScrollMessageBox::standardIcon(QMessageBox::Icon icon) {
  QStyle *style = this->style();
  int iconSize = style->pixelMetric(QStyle::PM_MessageBoxIconSize, 0, this);
  QIcon tmpIcon;
  switch (icon) {
    case QMessageBox::Information:
      tmpIcon = style->standardIcon(QStyle::SP_MessageBoxInformation, 0, this);
      break;
    case QMessageBox::Warning:
      tmpIcon = style->standardIcon(QStyle::SP_MessageBoxWarning, 0, this);
      break;
    case QMessageBox::Critical:
      tmpIcon = style->standardIcon(QStyle::SP_MessageBoxCritical, 0, this);
      break;
    case QMessageBox::Question:
      tmpIcon = style->standardIcon(QStyle::SP_MessageBoxQuestion, 0, this);
    default:
      break;
  }
  if (!tmpIcon.isNull()) return tmpIcon.pixmap(iconSize, iconSize);

  return QPixmap();
}

void ScrollMessageBox::handle_buttonClicked(QAbstractButton *button) {
  int ret = buttonBox->standardButton(button);
  done(ret);
}

void ScrollMessageBox::setDefaultButton(QPushButton *button) {
  if (!buttonBox->buttons().contains(button)) return;

  button->setDefault(true);
  button->setFocus();
}

void ScrollMessageBox::setDefaultButton(QMessageBox::StandardButton button) {
  setDefaultButton(buttonBox->button((QDialogButtonBox::StandardButton)button));
}

QMessageBox::StandardButton ScrollMessageBox::critical(QWidget* parent, QString const& title, QString const& labelTitle, QString const& text,
                                                       QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
  ScrollMessageBox box(QMessageBox::Critical, title, labelTitle, text, buttons, parent);
  box.setDefaultButton(defaultButton);
  return static_cast<QMessageBox::StandardButton>(box.exec());
}

QMessageBox::StandardButton ScrollMessageBox::information(QWidget* parent, QString const& title, QString const& labelTitle, QString const& text,
                                                          QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
  ScrollMessageBox box(QMessageBox::Information, title, labelTitle, text, buttons, parent);
  box.setDefaultButton(defaultButton);
  return static_cast<QMessageBox::StandardButton>(box.exec());
}

QMessageBox::StandardButton ScrollMessageBox::question(QWidget* parent, QString const& title, QString const& labelTitle, QString const& text,
                                                       QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
  ScrollMessageBox box(QMessageBox::Question, title, labelTitle, text, buttons, parent);
  box.setDefaultButton(defaultButton);
  return static_cast<QMessageBox::StandardButton>(box.exec());
}

QMessageBox::StandardButton ScrollMessageBox::warning(QWidget* parent, QString const& title, QString const& labelTitle, QString const& text,
                                                      QMessageBox::StandardButtons buttons, QMessageBox::StandardButton defaultButton) {
  ScrollMessageBox box(QMessageBox::Warning, title, labelTitle, text, buttons, parent);
  box.setDefaultButton(defaultButton);
  return static_cast<QMessageBox::StandardButton>(box.exec());
}
