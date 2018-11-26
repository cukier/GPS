#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <QObject>
#include <QSerialPort>

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
//    void handleTimeout();
    void handleError(QSerialPort::SerialPortError serialPortError);

private:
    QSerialPort *m_serialPort = nullptr;

    QByteArray m_content;
};

#endif // SERIALPORT_H
