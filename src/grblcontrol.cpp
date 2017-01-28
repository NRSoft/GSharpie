#include <QDebug>
#include "grblcontrol.h"

using namespace std;

extern int GSharpieReportLevel;

GrblControl::GrblControl()
{
    _port = nullptr;
    _opened = false;

    _status = Undef;
    _position.mx = _position.my = _position.mz = 0.0;
    _position.wx = _position.wy = _position.wz = 0.0;

    _lastCmdId = 0;
}


bool GrblControl::openSerialPort(const QString& portName, qint32 baudrate)
{
    _port = new QSerialPort(portName);
    _port->setBaudRate(baudrate);
    _port->setDataBits(QSerialPort::Data8);
    _port->setParity(QSerialPort::NoParity);
    _port->setStopBits(QSerialPort::OneStop);
    _port->setFlowControl(QSerialPort::NoFlowControl);
    _port->setReadBufferSize(128);

    connect(_port, SIGNAL(readyRead()), this, SLOT(_handlePortRead()));
    connect(_port, SIGNAL(error(QSerialPort::SerialPortError)),
             this, SLOT(_handlePortError(QSerialPort::SerialPortError)));

    _opened = _port->open(QIODevice::ReadWrite);

    if(_opened)
        emit report(0, QString("Opened serial port ") + _port->portName());
    else
        emit report(1, QString("Cannot open serial port ") + _port->portName());

    return _opened;
}


void GrblControl::closeSerialPort()
{
    if(_port == nullptr)
        return;

    _port->close();
    _opened=false;
    _status = Undef;
    emit report(0, QString("Closed serial port ") + _port->portName());
}


bool GrblControl::getSerialPortInfo(QString& portName, quint32& baudrate)
{
    if(_port == nullptr)
        return false;

    portName = _port->portName();
    baudrate = _port->baudRate();
    return true;
}


quint32 GrblControl::issueCommand(const char* cmd, const QString& readableName)
{
    if(!_opened)
        return 0;

    if(GSharpieReportLevel < -1)
        emit report(-2, QString("Sending: ") +
                        QString(cmd).remove(QRegExp("[\r\n]")));

    GrblCommand command;
    command.id = ++_lastCmdId;
    command.name = readableName;
    command.code.assign(cmd);
    command.sent = false;
    _commands.enqueue(command);

    _sendNextCommand();

    return command.id;
}


bool GrblControl::issueReset()
{
    if(!_opened)
        return false;

    if(GSharpieReportLevel < 0)
        emit report(-1, QString("Resetting Grbl..."));

    _version.clear(); // to check later if connection is established
    static char data[2] = {0x18, 0}; // "Ctrl-X" character
    _port->write(data, 1);
    _port->waitForBytesWritten(5); // make sure the bytes are written before continuing

    return true;
}


bool GrblControl::issueStatusRequest()
{
    if(!_opened)
        return false;

    static char data[2] = {'?', 0}; // "Ctrl-X" character
    _port->write(data, 1);
    _port->waitForBytesWritten(5); // make sure the bytes are written before continuing

    return true;
}


//////  s e n d  N e x t  C o m m a n d  //////
bool GrblControl::_sendNextCommand()
{
    int byteCount = 0;
    QQueue<GrblCommand>::iterator it;
    for(it = _commands.begin(); it != _commands.end(); ++it){
        if(it->sent)
            byteCount += it->code.size();
        else{ // we came to the first not sent command
            if(byteCount + it->code.size() > 127) // grbl buffer size
                return false;
//qDebug() << "To Grbl:" << QString(it->code.c_str());
            _port->write(it->code.c_str());
            it->sent = true;
            return true;
        }
    }
    return false; // all the queue have been sent
}


///////  h a n d l e  P o r t  R e a d  ////////
void GrblControl::_handlePortRead()
{
    _response.append(_port->readAll());

    // process Grbl output line by line
    int lineEnd;
    while((lineEnd = _response.indexOf('\n')) >= 0){
        QString line(_response.left(lineEnd-1)); // there is "\r\n" pair
        _response = _response.mid(lineEnd+1);
//qDebug() << "From Grbl:" << line;
        if(line[0] == '<'){ // status response to '?' enquiry
            if(line[1] == 'R')
                _status = Run;
            else if(line[1] == 'H')
                _status = (line[3] == 'm')? Home: Hold;
            else if(line[1] == 'C')
                _status = Check;
            else if(line[1] == 'I')
                _status = Idle;
            else if(line[1] == 'A')
                _status = Alarm;
            else if(line[1] == 'D')
                _status = Door;
            else
                _status = Undef;

            // TODO: more generic parsing, the current format may change
            size_t mposX = line.indexOf(',') + 6;
            size_t mposY = line.indexOf(',', mposX) + 1;
            size_t mposZ = line.indexOf(',', mposY) + 1;
            size_t lastM = line.indexOf(',', mposZ);
            size_t wposX = lastM + 6;
            size_t wposY = line.indexOf(',', wposX) + 1;
            size_t wposZ = line.indexOf(',', wposY) + 1;
            size_t lastW = line.indexOf('>', wposZ);

            _position.mx = line.mid(mposX, mposY-mposX-1).toDouble();
            _position.my = line.mid(mposY, mposZ-mposY-1).toDouble();
            _position.mz = line.mid(mposZ, lastM-mposZ).toDouble();

            _position.wx = line.mid(wposX, wposY-wposX-1).toDouble();
            _position.wy = line.mid(wposY, wposZ-wposY-1).toDouble();
            _position.wz = line.mid(wposZ, lastW-wposZ).toDouble();
        }
        else if(line.left(4) == QStringLiteral("Grbl")){ // after reset
            int last  = line.indexOf('[');
            _version = line.left(last);
            emit report(0, QString("Welcome to ") + _version);
        }
        else{ // other commands
            if(!_commands.isEmpty()){
                if(GSharpieReportLevel < -1)
                    emit report(-2, QString("Grbl message: ") + line);

                GrblCommand& cmd = _commands.head();
                if(line[0] == 'o' || line[0] == 'e' || line[0] == 'A'){ // 'ok', 'error' or 'ALARM'
                    if(line[0] == 'e' || line[0] == 'A')
                        cmd.error = line.mid(7); // everything after ':'
                        //TODO: stop command queueing if error happened
                    emit commandComplete(_commands.dequeue());
                    _sendNextCommand();
                }
                else
                    cmd.response.append(line);
            }
            else if(GSharpieReportLevel < -1)
                emit report(-2, QString("Grbl message without command: ") + line);
        }
    }
}


///////  h a n d l e  P o r t  E r r o r  ////////
void GrblControl::_handlePortError(QSerialPort::SerialPortError error)
{
    if(error == QSerialPort::ReadError)
        emit report(1, QString("Reading from serial port: ") + _port->errorString());
}

