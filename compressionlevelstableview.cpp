#include "compressionlevelstableview.h"
#include "basearchengine.h"
#include <QStandardItemModel>
#include <QItemDelegate>
#include <QComboBox>
#include <QHeaderView>

#if QT_VERSION < 0x050000
Q_DECLARE_METATYPE(QList<int>)
#endif

class ComboBoxDelegate: public QItemDelegate {
public:
    ComboBoxDelegate(QObject *parent) : QItemDelegate(parent) {}

    QWidget * createEditor(QWidget *parent,
                           const QStyleOptionViewItem &,
                           const QModelIndex & index) const {
        if (index.column() == 0) return NULL;
        QComboBox *editor = new QComboBox(parent);
        QList<int> values = index.model()->itemData(index)[Qt::UserRole].value<QList<int> >();
        for (int i=0;i<values.count();i++) {
            editor->addItem(QString::number(values.at(i)));
        }
        return editor;
    }

    void setEditorData(QWidget *editor,const QModelIndex &index) const {
        QComboBox *comboBox = static_cast<QComboBox*>(editor);
        comboBox->setCurrentIndex(comboBox->findText(index.data(Qt::DisplayRole).toString()));
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model,const QModelIndex &index) const {
        QComboBox *comboBox = static_cast<QComboBox*>(editor);
        model->setData(index, comboBox->currentText(), Qt::DisplayRole);
    }

    void updateEditorGeometry(QWidget *editor,const QStyleOptionViewItem &option, const QModelIndex &) const {
        editor->setGeometry(option.rect);
    }
};

class CompressionLevelsModel: public QStandardItemModel {
public:
    CompressionLevelsModel(int rows,int columns,QObject * parent = 0) : QStandardItemModel(rows,columns,parent) {}

    Qt::ItemFlags flags(const QModelIndex & index) const {
        if (index.column() == 0) {
            return QStandardItemModel::flags(index) & Qt::ItemIsSelectable;
        }
        return QStandardItemModel::flags(index);
    }
};

CompressionLevelsTableView::CompressionLevelsTableView(QWidget *parent) : QTableView(parent) {
    QMap<QString,QPair<QList<int>,int> > levels_map = BaseArchEngine::enginesCompressionLevels();
    model = new CompressionLevelsModel(levels_map.count(),2,this);
    model->setHorizontalHeaderItem(0,new QStandardItem(tr("Engine")));
    model->setHorizontalHeaderItem(1,new QStandardItem(tr("Compression level")));
    horizontalHeader()->setStretchLastSection(true);
#if QT_VERSION >= 0x050000
    horizontalHeader()->setSectionsClickable(false);
    horizontalHeader()->setSectionsMovable(false);
#else
    horizontalHeader()->setClickable(false);
    horizontalHeader()->setMovable(false);
#endif

    QMapIterator<QString,QPair<QList<int>,int> > map_i(levels_map);
    for (int i=0;map_i.hasNext();i++) {
        map_i.next();
        model->setItem(i,0,new QStandardItem(map_i.key()));
        model->setItem(i,1,new QStandardItem(QString::number(map_i.value().second)));
        QMap<int,QVariant> roles = model->itemData(model->index(i,1));
        roles[Qt::UserRole] = QVariant::fromValue(map_i.value().first);
        model->setItemData(model->index(i,1),roles);
    }

    setModel(model);
    setItemDelegate(new ComboBoxDelegate(this));
}

QMap<QString,int> CompressionLevelsTableView::levelsMap() const {
    QMap<QString,int> ret;
    for (int i=0;i<model->rowCount();i++) {
        ret[model->item(i,0)->text()] = model->item(i,1)->text().toInt();
    }

    return ret;
}
