#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <QObject>
#include <QSerialPort>
#include <QTimer>

#include "tinygps.h"

class SerialPort : public QObject
{
    Q_OBJECT
public:
    explicit SerialPort(QObject *parent = nullptr);

    int available();
    QByteArray content();

signals:
    void received(QByteArray);

private slots:
    void handleReadyRead();
    void handleTimeout();
    void handleError(QSerialPort::SerialPortError serialPortError);

private:
    QSerialPort *m_serialPort = nullptr;

    QTimer m_timer;
    QByteArray m_content;
    TinyGPS m_gps;
};

#endif // SERIALPORT_H
