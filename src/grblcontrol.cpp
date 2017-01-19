#include <QDebug>
#include "grblcontrol.h"

using namespace std;

GrblControl::GrblControl(const QString &portName, qint32 baudrate)
{
    _port = new QSerialPort(portName);
    _port->setBaudRate(baudrate);
    _port->setDataBits(QSerialPort::Data8);
    _port->setParity(QSerialPort::NoParity);
    _port->setStopBits(QSerialPort::OneStop);
    _port->setFlowControl(QSerialPort::NoFlowControl);
    _port->setReadBufferSize(128);
    _connected = false;

    _readTimeout  = 200;
}


bool GrblControl::connect(QString& grblVersion)
{
    _connected = _port->open(QIODevice::ReadWrite);
    qDebug() << "opening serial port :" << _connected;
    if(_connected){
        _connected = issueReset(grblVersion);
        qDebug() << "connecting to GRBL :" << _connected;
    }
    return _connected;
}


bool GrblControl::issueReset(QString& grblVersion)
{
    string response;
    static char data[2] = {0x18, 0}; // "Ctrl-X" character
    bool success = _send(data, response);

    if(success){
        size_t first = response.find("Grbl");
        size_t last  = response.find('[');
        if(first!=string::npos && last!=string::npos)
            grblVersion = response.substr(first, last-first).c_str();
        else
            grblVersion.clear();
    }

//    qDebug() << "issuing reset :" << QString(response.c_str());

    return success;
}


///////  s e n d  ///////
bool GrblControl::_send(const string& data, string& response)
{
    if(_connected){
        _port->write(data.c_str());

        if(_port->waitForBytesWritten(1)){
            if(_port->waitForReadyRead(_readTimeout)){
                response.assign(_port->readAll().constData());
                while(_port->waitForReadyRead(1))
                    response += _port->readAll().constData();
                return true;
            }
            else
                response.assign("read timeout");
        }
        else
            response.assign("write timeout");
    }
    else
        response.assign("no connection");

    return false;

}
