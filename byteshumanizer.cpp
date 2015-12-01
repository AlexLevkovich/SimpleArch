#include "byteshumanizer.h"
#include <QStringList>

const QString BytesHumanizer::labels[9] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB"};

BytesHumanizer::BytesHumanizer(const QString & val) {
    m_was_error = false;

    QString value = val.simplified();
    int index = 0;
    for (int i=0;i<9;i++) {
        if (value.endsWith(" "+labels[i])) {
            index = i;
            break;
        }
    }

    bool ok = false;
    m_value = value.split(" ",QString::SkipEmptyParts).at(0).toDouble(&ok);
    if (!ok) {
        m_value = 0.0;
        m_was_error = true;
        return;
    }

    if (index == 0) return;

    for (int i=0;i<index;i++) {
        m_value *= 1024.0;
    }
}

BytesHumanizer::BytesHumanizer(qreal value) {
    m_value = value;
    m_was_error = false;
}

QString BytesHumanizer::toString(int precision) const {
    qreal value = m_value;

    int index = 0;
    for(int i=0;i<8;i++) {
        if(value <= 2048.0 && value >= -2048.0) {
            break;
        }
        value /= 1024.0;
        index++;
    }

    QString zero(precision,'0');
    QString str_value(QString("%1").arg(value,0,'f',precision));
    if (str_value.endsWith("."+zero)) {
        str_value = str_value.left(str_value.length()-1-zero.length());
    }

    return QString("%1 %2").arg(str_value).arg(labels[index]);
}
