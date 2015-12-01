#ifndef SequentialBuffer_H
#define SequentialBuffer_H

#include <QByteArray>

class SequentialBuffer {
public:
    SequentialBuffer();
    SequentialBuffer(const QByteArray & data);
    SequentialBuffer(const char * data,qint64 size);
    void setData(const QByteArray & data);
    void setData(const char * data,qint64 size);

    QByteArray read(qint64 maxSize);
    qint64 read(char *data, qint64 maxlen);
    void free(qint64 maxSize);
    QByteArray readAll();
    QByteArray readLine();
    qint64 readLine(char * data,qint64 length);
    void write(const QByteArray & bytes);
    void write(char * data,qint64 size);
    SequentialBuffer & operator=(const SequentialBuffer & other);
    SequentialBuffer & operator=(const QByteArray & other);
    SequentialBuffer & operator+=(const SequentialBuffer & other);
    SequentialBuffer & operator+=(const QByteArray & other);
    bool canReadLine() const;
    qint64 size() const;
    bool isEmpty() const;
    void chop(qint64 bytes);
    void clear();

    // be carefully with these functions
    char * reserve(qint64 maxlen);
    char * rawData();
    // returned array will be unusable after buffer array deleted
    const QByteArray data();

private:
    void tail(qint64 start_index);
    void resize(qint64 bytes);
    char * constRawData() const;
    qint64 indexOf(char ch) const;

    QByteArray buffer;
    qint64 start_index;
};

#endif // SequentialBuffer_H
