#include "categorylistview.h"
#include <QIcon>
#include <QPixmap>
#include <QStyledItemDelegate>
#include <QScrollBar>
#include <QEvent>

const int categoryIconSize = 48;

class Category {
public:
    Category() { }

    QString displayName;
    QIcon icon;
    QWidget *widget;
};

class CategoryModel : public QAbstractListModel {
public:
    CategoryModel(QObject *parent = 0);
    ~CategoryModel();

    void addCategory(const QString & displayName,QWidget *widget,const QIcon & icon = QIcon());
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    const QList<Category*> &categories() const { return m_categories; }

private:
    Category *findCategoryById(int id);

    QList<Category*> m_categories;
    QIcon m_emptyIcon;
};

CategoryModel::CategoryModel(QObject *parent) : QAbstractListModel(parent) {
    QPixmap empty(categoryIconSize, categoryIconSize);
    empty.fill(Qt::transparent);
    m_emptyIcon = QIcon(empty);
}

CategoryModel::~CategoryModel() {
    qDeleteAll(m_categories);
}

int CategoryModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : m_categories.count();
}

void CategoryModel::addCategory(const QString & displayName,QWidget *widget,const QIcon & icon) {
    int row = m_categories.count();
    beginInsertRows(QModelIndex(),row,row);
    Category * category = new Category;
    category->icon = icon;
    category->displayName = displayName;
    category->widget = widget;
    m_categories.append(category);
    endInsertRows();
}

QVariant CategoryModel::data(const QModelIndex &index, int role) const {
    switch (role) {
    case Qt::DisplayRole:
        return m_categories.at(index.row())->displayName;
    case Qt::DecorationRole: {
            QIcon icon = m_categories.at(index.row())->icon;
            if (icon.isNull())
                icon = m_emptyIcon;
            return icon;
        }
    }

    return QVariant();
}

Category *CategoryModel::findCategoryById(int id) {
    if (id >= m_categories.size()) return NULL;
    return m_categories.at(id);
}

class CategoryListViewDelegate : public QStyledItemDelegate {
public:
    CategoryListViewDelegate(QObject *parent) : QStyledItemDelegate(parent) {}

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const {
        QSize size;
        QStyleOptionViewItemV4 opt(option);
        size.setHeight(option.decorationSize.height() + 8 + option.fontMetrics.height());
        size.setWidth(qMax(option.fontMetrics.width(opt.text) + 5,option.decorationSize.width()));
        return size;
    }

    void paint(QPainter * painter,const QStyleOptionViewItem & option,const QModelIndex & index) const {
        QStyleOptionViewItemV4 opt(option);
        opt.decorationPosition = QStyleOptionViewItem::Top;
        opt.textElideMode = Qt::ElideNone;
        opt.decorationAlignment = Qt::AlignHCenter;
        opt.displayAlignment = Qt::AlignHCenter;
        opt.state = opt.state & (~QStyle::State_HasFocus);
        QStyledItemDelegate::paint(painter,opt,index);
    }
};

CategoryListView::CategoryListView(QWidget *parent) : QListView(parent) {
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setItemDelegate(new CategoryListViewDelegate(this));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setViewMode(QListView::ListMode);
    setIconSize(QSize(categoryIconSize,categoryIconSize));
    setModel(new CategoryModel(this));
}

QSize CategoryListView::sizeHint() const {
    CategoryModel * model = (CategoryModel *)this->model();
    int width = 0;
    for (int i=0;i<model->rowCount();i++) {
        QModelIndex index = model->index(i,0,QModelIndex());
        QStyleOptionViewItemV4 option;
        option.fontMetrics = fontMetrics();
        option.text = model->data(index).toString();
        option.decorationSize = QSize(categoryIconSize,categoryIconSize);
        width = qMax(itemDelegate()->sizeHint(option,index).width(),width);
    }
    return QSize(width + 5 + verticalScrollBar()->height(),QListView::sizeHint().height());
}

bool CategoryListView::event(QEvent * event) {
    bool ret = QListView::event(event);
    if (event->type() == QEvent::FontChange) {
        setMinimumWidth(sizeHint().width());
        setMaximumWidth(sizeHint().width());
        updateGeometry();
    }
    return ret;
}

bool CategoryListView::eventFilter(QObject *obj, QEvent *event) {
    if ((obj == verticalScrollBar()) &&
        ((event->type() == QEvent::Show ||
         (event->type() == QEvent::Hide)))) {
        updateGeometry();
    }
    return QListView::eventFilter(obj, event);
}

void CategoryListView::addCategory(const QString & displayName,QWidget *widget,const QIcon & icon) {
    CategoryModel * model = (CategoryModel *)this->model();
    model->addCategory(displayName,widget,icon);
    QModelIndex index = model->index(0,0,QModelIndex());
    QItemSelection selection(index,index);
    if (model->rowCount() == 1) selectionModel()->select(selection,QItemSelectionModel::ClearAndSelect);
    else selectionChanged(selection,QItemSelection());
    setMinimumWidth(sizeHint().width());
    setMaximumWidth(sizeHint().width());
    updateGeometry();
}

void CategoryListView::selectionChanged(const QItemSelection & selected,const QItemSelection & deselected) {
    QListView::selectionChanged(selected,deselected);
    QModelIndexList indexes = selected.indexes();
    if (indexes.isEmpty()) return;

    CategoryModel * model = (CategoryModel *)this->model();
    QList<Category*> categories = model->categories();
    for (int i=0;i<categories.count();i++) {
        if (categories.at(i)->widget != NULL) categories.at(i)->widget->setVisible(i == indexes.at(0).row());
    }
}
