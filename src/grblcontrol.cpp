#include <QDebug>
#include "grblcontrol.h"

using namespace std;

extern int GSharpieReportLevel;

GrblControl::GrblControl()
{
    _port = nullptr;
    _connected = false;

    _status.state = Undef;

    _config.imperial = false;

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

    _connected = _port->open(QIODevice::ReadWrite);

    if(_connected)
        emit report(0, QString("Opened serial port ") + _port->portName());
    else
        emit report(1, QString("Cannot open serial port ") + _port->portName() +
                       QString(": ") + _port->errorString());

    return _connected;
}


void GrblControl::closeSerialPort()
{
    if(_port == nullptr)
        return;

    _port->close();
    _connected=false;
    _status.state = Undef;
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


////////  i s s u e  C o m m a n d  /////////
quint32 GrblControl::issueCommand(const char* cmd, const QString& readableName)
{
    if(!isActive())
        return 0;

    emit report(-2, QString("Sending: ") + QString(cmd).remove(QRegExp("[\r\n]")));

    Command command;
    command.id = ++_lastCmdId;
    command.name = readableName;
    command.code.assign(cmd);
    command.sent = false;
    _commands.enqueue(command);

    _sendNextCommand();

    return command.id;
}


/////  i s s u e  R e a l t i m e  C o m m a n d  //////
bool GrblControl::issueRealtimeCommand(REALTIME_COMMAND cmd)
{
    if(!isActive())
        return false;

    emit report(-2, QString("Issuing realtime Grbl command: 0x") + QString::number(cmd, 16));

    char data[2] = {static_cast<char>(cmd), 0};
    if(_port->write(data, 1) == 1 && _port->waitForBytesWritten(5)){
//        if(cmd != GET_STATUS)
//            qDebug("To Grbl: 0x%02X", static_cast<uint8_t>(cmd));//data[0]);
        return true;
    }
    else
        qDebug("Error issuing 0x%02X", static_cast<uint8_t>(cmd));

    emit report(1, QString("Cannot issue realtime Grbl command: 0x") + QString::number(cmd, 16));
    return false;
}


//////  s e n d  N e x t  C o m m a n d  //////
bool GrblControl::_sendNextCommand()
{
    int byteCount = 0;
    QQueue<Command>::iterator it;
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


///////  r e t r i e v e  V e r s i o n  ///////
bool GrblControl::_retrieveVersion(const QByteArray& line)
{
    if(line.size() >= 9){
        _version = line.mid(5,3).toDouble();
        if(_version > 0.0){
            if(_version >= MIN_SUPPORTED_VERSION){
                emit report(0, QLatin1String("Welcome to ") + line.left(9));
                return true;
            }
            else
                emit report(1, QLatin1String("Unsupported version ") + line.left(9));
        }
        else
            emit report(1, QString("Unexpected Grbl version format: ") + line.left(9));
    }
    else
        emit report(1, QString("Unexpected Grbl message: ") + line);
    return false;
}


///////  r e t r i e v e  S t a t u s  ///////
void GrblControl::_retrieveStatus(const QByteArray& line)
{
    // extract internal to <.> data and split into components
    QList<QByteArray> fields = line.mid(1, line.size()-2).split('|');

    // first field is always Machine State info
    const char st = fields[0][0];
         if(st == 'J') _status.state = Jog;
    else if(st == 'R') _status.state = Run;
    else if(st == 'H') _status.state = (fields[0][2] == 'm')? Home: Hold;
    else if(st == 'C') _status.state = Check;
    else if(st == 'I') _status.state = Idle;
    else if(st == 'A') _status.state = Alarm;
    else if(st == 'D') _status.state = Door;
    else if(st == 'S') _status.state = Sleep;
    else               _status.state = Undef;

    // process remaining fields
    enum {UNDEF, MPOS, WPOS} defaultPos = UNDEF;
    for(auto field = fields.begin()+1; field < fields.end(); ++field){
        if(field->left(4) == "MPos"){ // Machine position
            if(_readCoordinates(field->mid(5), _status.pos.mpos))
                defaultPos = MPOS;
        }
        else if(field->left(4) == "WPos"){ // Work position
            if(_readCoordinates(field->mid(5), _status.pos.wpos))
                defaultPos = WPOS;
            defaultPos = WPOS;
        }
        else if(field->left(3) == "WCO"){ // Work position
            _readCoordinates(field->mid(4), _toolOffset);
        }
        else if(field->left(2) == "FS"){ // feed rate and spindle speed
            QList<QByteArray> values = field->mid(3).split(',');
            if(values.size() >= 2){
                _status.feedrate = values[0].toInt();
                _status.spindle = values[1].toInt();
            }
        }
        else if(field->at(0) == 'F'){ // feed rate only, keep it after "FS"
            _status.feedrate = field->mid(2).toInt();
        }
        else if(field->left(2) == "Ln"){ // currently executed g-code line number (N)
            _status.line = field->mid(3).toInt();
        }
        else if(field->left(2) == "Bf"){ // Buffer size (not implemented yet)
        }
        else if(field->left(2) == "Pn"){ // Pin state (not implemented yet)
        }
        else if(field->left(2) == "Ov"){ // Override (not implemented yet)
        }
        else if(field->at(0) == 'A'){ // Accessory state (not implemented yet)
        }
        else
            qDebug() << "Unexpected status field" << *field;

        // calculate relative position
        if(defaultPos == MPOS)
            _status.pos.wpos = _status.pos.mpos - _toolOffset;
        else if(defaultPos == WPOS)
            _status.pos.mpos = _status.pos.wpos + _toolOffset;
    }

    emit statusUpdated();
}


//////  r e a d  C o o r d i n a t e s  /////
bool GrblControl::_readCoordinates(const QByteArray& line, QVector4D& pos)
{
    if(line[0] != '-' && !::isdigit(line[0]))
        return false;

    QList<QByteArray> coordinates = line.split(',');
    if(coordinates.size() < 3)
        return false;

    pos.setX(coordinates[0].toDouble());
    pos.setY(coordinates[1].toDouble());
    pos.setZ(coordinates[2].toDouble());

    return true;
}

///////  h a n d l e  P o r t  R e a d  ////////
void GrblControl::_handlePortRead()
{
    _response.append(_port->readAll());

    // process Grbl output line by line
    int lineEnd;
    while((lineEnd = _response.indexOf('\n')) >= 0){
        QByteArray line(_response.left(lineEnd-1)); // there is "\r\n" pair
        _response = _response.mid(lineEnd+1); // fast forward to the next line
        if(line.size()>=4 && line.left(4) == QStringLiteral("Grbl")){ // after reset
            _retrieveVersion(line);
        }
        else if(line[0] == '<'){ // CNC status response
            _retrieveStatus(line);
        }
        else{ // other commands
//qDebug("From Grbl: %s", line.toStdString().c_str());
            if(!_commands.isEmpty()){
                emit report(-2, QString("Grbl message: ") + line);

                Command& cmd = _commands.head();
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
            else
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

