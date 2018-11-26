#include "serialport.h"

#include <QtDebug>
#include <QCoreApplication>

SerialPort::SerialPort(QObject *parent)
    : QObject(parent)
    , m_serialPort(new QSerialPort(this))
{
    m_serialPort->setPortName("COM3");
    m_serialPort->setBaudRate(QSerialPort::Baud9600);
    m_serialPort->setDataBits(QSerialPort::Data8);
    m_serialPort->setParity(QSerialPort::NoParity);
    m_serialPort->setStopBits(QSerialPort::OneStop);

    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialPort::handleReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &SerialPort::handleError);

    if (m_serialPort->open(QIODevice::ReadWrite)) {
        qDebug() << "Porta aberta";
    } else {
        qDebug() << "Porta nao aberta";
    }
}

void SerialPort::handleReadyRead()
{
    QByteArray arr = m_serialPort->readAll();

    if (arr.size()) {
        m_content.clear();
        m_content.append(arr);
//        qDebug() << "Recebido " << arr << " copiado " << m_content;
        emit received(arr);
    }
}

void SerialPort::handleError(QSerialPort::SerialPortError serialPortError)
{
    if (serialPortError == QSerialPort::ReadError) {
        qDebug() << QObject::tr("An I/O error occurred while reading "
                                "the data from port %1, error: %2")
                    .arg(m_serialPort->portName())
                    .arg(m_serialPort->errorString());
        //        QCoreApplication::exit(1);
    }
}

int SerialPort::available()
{
    return m_content.size();
}

QByteArray SerialPort::content()
{
    return m_content;
}
