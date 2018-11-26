#include <QCoreApplication>
#include <QtDebug>

#include "serialport.h"
#include "tinygps.h"


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    SerialPort port(&a);
//    TinyGPS gps(&a);

//    if (port.available()) {
//        qDebug() << "available " << port.available();
//        for (auto i : port.content())
//            gps << i;

//        long latitude, longitude;

//        gps.get_position(&latitude, &longitude);
//        qDebug() << "Lat: " << latitude << " Long: " << longitude;
//    }

    return a.exec();
}
