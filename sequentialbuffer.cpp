#include "sequentialbuffer.h"

SequentialBuffer::SequentialBuffer() {
    start_index = 0;
}

SequentialBuffer::SequentialBuffer(const QByteArray & data) {
    setData(data);
}

SequentialBuffer::SequentialBuffer(const char * data,qint64 size) {
    setData(data,size);
}

void SequentialBuffer::setData(const QByteArray & data) {
    buffer = data;
    start_index = 0;
}

void SequentialBuffer::setData(const char * data,qint64 size) {
    buffer.resize(size);
    memcpy(buffer.data(),data,size);
    start_index = 0;
}

QByteArray SequentialBuffer::read(qint64 maxSize) {
    QByteArray ret;
    if (maxSize <= 0) return ret;

    qint64 size = qMin(this->size(),maxSize);
    ret.resize(size);
    memcpy(ret.data(),rawData(),size);

    if (this->size() <= maxSize) clear();
    else tail(maxSize);

    return ret;
}

qint64 SequentialBuffer::read(char *data, qint64 maxSize) {
    if (this->size() <= maxSize) {
        qint64 size = this->size();
        memcpy(data,rawData(),size);
        clear();
        return size;
    }

    memcpy(data,rawData(),maxSize);
    tail(maxSize);

    return maxSize;
}

QByteArray SequentialBuffer::readAll() {
    return read(size());
}

qint64 SequentialBuffer::indexOf(char ch) const {
    qint64 index = buffer.indexOf(ch,start_index);
    if (index < 0) return -1;
    return index - start_index;
}

QByteArray SequentialBuffer::readLine() {
    int index = indexOf('\n');
    return (index < 0)?QByteArray():read(index+1);
}

qint64 SequentialBuffer::readLine(char * data,qint64 length) {
    if (length <= 0) return -1;
    int index = indexOf('\n');
    if (index < 0) return -1;
    if (index >= length) return -1;

    return read(data,index+1);
}

void SequentialBuffer::write(const QByteArray & bytes) {
    buffer += bytes;
}

void SequentialBuffer::write(char * data,qint64 size) {
    if (size <= 0) return;
    qint64 prev_size = this->size();
    resize(prev_size+size);
    memcpy(rawData()+prev_size,data,size);
}

SequentialBuffer & SequentialBuffer::operator=(const SequentialBuffer & other) {
    setData(other.constRawData(),other.size());
    return *this;
}

SequentialBuffer & SequentialBuffer::operator=(const QByteArray & other) {
    setData(other);
    return *this;
}

SequentialBuffer & SequentialBuffer::operator+=(const SequentialBuffer & other) {
    qint64 count = buffer.size();
    buffer.resize(count+other.size());
    memcpy(buffer.data()+count,other.constRawData(),other.size());
    return *this;
}

SequentialBuffer & SequentialBuffer::operator+=(const QByteArray & other) {
    qint64 count = buffer.size();
    buffer.resize(count+other.length());
    memcpy(buffer.data()+count,other.data(),other.length());
    return *this;
}

bool SequentialBuffer::canReadLine() const {
    return (indexOf('\n') >= 0);
}

qint64 SequentialBuffer::size() const {
    return buffer.count() - start_index;
}

bool SequentialBuffer::isEmpty() const {
    return (size() <= 0);
}

void SequentialBuffer::free(qint64 maxSize) {
    if (maxSize <= 0) return;

    if (size() <= maxSize) clear();
    else tail(maxSize);
}

void SequentialBuffer::tail(qint64 startindex) {
    if (startindex <= 0) return;
    start_index += startindex;
}

char * SequentialBuffer::reserve(qint64 maxlen) {
    if (maxlen <= 0) return NULL;
    qint64 count = size();
    resize(count + maxlen);
    return rawData() + count;
}

void SequentialBuffer::chop(qint64 bytes) {
    if (bytes <= 0) return;
    buffer.chop(bytes);
}

char * SequentialBuffer::rawData() {
    return buffer.data() + start_index;
}

void SequentialBuffer::clear() {
    buffer.truncate(0);
    start_index = 0;
}

void SequentialBuffer::resize(qint64 bytes) {
    if (bytes < 0) return;
    if (bytes == 0) start_index = 0;
    buffer.resize(start_index+bytes);
}

const QByteArray SequentialBuffer::data() {
    return QByteArray::fromRawData(rawData(),size());
}

char * SequentialBuffer::constRawData() const {
    return ((SequentialBuffer *)this)->rawData();
}
