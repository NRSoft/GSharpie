#include "grblcontrol.h"

GrblControl::GrblControl(const QString &portName, qint32 baudrate)
{
    _port->setPortName(portName);
    _port->setBaudRate(baudrate);
    _port->setDataBits(QSerialPort::Data8);
    _port->setParity(QSerialPort::NoParity);
    _port->setStopBits(QSerialPort::OneStop);
    _port->setFlowControl(QSerialPort::NoFlowControl);
    _connected = false;
}


bool GrblControl::connect()
{
    _connected = _port->open(QIODevice::ReadWrite);
    qDebug() << "Is serial port opened?" << _connected;
    if(!_connected)
        return false;

    qDebug() << "Is GRBL connected?" << _connected;
    return true;
}


void GrblControl::issueReset()
{
    static char data[2] = {0x18, 0}; // "Ctrl-X" character
    _port->write(data);
}

const string GrblControl::send(const string& line)
{

}
