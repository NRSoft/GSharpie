#include <QDebug>
#include "grblcontrol.h"

using namespace std;

extern int GSharpieReportLevel;

GrblControl::GrblControl()
{
    _port = nullptr;
    _connected = false;

    _status.state = Undef;

    memset(&_config, 0, sizeof(Config));

    _seekRate = 500; // default, will be overwritten from ini-file
    _feedRate = 100;

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
    if(!isActive()){
        emit report(1, "Cannot issue " + readableName);
        return 0;
    }

    static char data[128];
    ::strncpy(data, cmd, 120);
    ::strcat(data, "\n");

    Command command;
    command.id = ++_lastCmdId;
    command.name = readableName;
    command.code.assign(data);
    command.sent = false;
    _commands.enqueue(command);

    emit report(-2, QString("Sending: ") + QString(cmd));
    _sendNextCommand();

    emit report(-1, QString("Issued [") + QString::number(command.id) + QStringLiteral("] ") +
                command.name + QStringLiteral(" (") + QString(cmd) + QStringLiteral(")"));
    return command.id;
}


/////  i s s u e  R e a l t i m e  C o m m a n d  //////
bool GrblControl::issueRealtimeCommand(REALTIME_COMMAND cmd)
{
    if(!isActive())
        return false;

    if(cmd != GET_STATUS)
        emit report(-2, QString("Issuing realtime Grbl command: 0x") + QString::number(cmd, 16));
    else
        emit report(-3, QString("Issuing Grbl status command: 0x") + QString::number(cmd, 16));

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
                issueCommand("$$", "Retrieve Parameters");
                issueCommand("$N", "Retrieve Startup Block");
                setSeekRate(_seekRate);
                setFeedRate(_feedRate);
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


//////  r e t r i e v e  P a r a m e t e r  //////
bool GrblControl::_retrieveParameter(const QByteArray& line)
{
    QList<QByteArray> fields = line.mid(1).split('='); // remove '$'
    if(fields.size() == 2 && fields[0].size() >= 1 && fields[0].size() <= 3){
        if(fields[0].at(0) == 'N'){ // startup block
            emit report(-1, QString("Processing startup block line: ") + line);
            if(fields[0].at(1) == '0')
                _startup[0] = fields[1];
            else if(fields[0].at(1) == '1')
                _startup[1] = fields[1];
            else
                emit report(1, "Unexpected startup block #%c" + fields[0].at(1));
            return true;
        }
        else if(::isdigit(fields[0].at(0))){
            emit report(-1, QString("Processing parameter #") + fields[0] + QString(", value = ") + fields[1]);
            bool ok;
            int id = fields[0].toInt(&ok); // parameter id
            if(ok){
                int value;
                double valueDbl;
                switch (id){
                    case 0:
                        value = fields[1].toInt(&ok);
                        if(ok) _config.stepPulse = value;
                        break;

                    case 1:
                        value = fields[1].toInt(&ok);
                        if(ok) _config.stepIdleDelay = value;
                        break;

                    case 3:
                        value = fields[1].toInt(&ok);
                        if(ok) _config.directionInvertMask = value;
                        break;

                    case 4:
                        value = fields[1].toInt(&ok);
                        if(ok) _config.stepEnableInvert = (value!=0);
                        break;

                    case 5:
                        value = fields[1].toInt(&ok);
                        if(ok) _config.limitSwitchInvert = (value!=0);
                        break;

                    case 6:
                        value = fields[1].toInt(&ok);
                        if(ok) _config.probePinInvert = (value!=0);
                        break;

                    case 11:
                        valueDbl = fields[1].toDouble(&ok);
                        if(ok) _config.junctionDeviation = valueDbl;
                        break;

                    case 12:
                        valueDbl = fields[1].toDouble(&ok);
                        if(ok) _config.arcTolerance = valueDbl;
                        break;

                    case 13:
                        value = fields[1].toInt(&ok);
                        if(ok) _config.imperial = (value!=0);
                        break;

                    case 22:
                        value = fields[1].toInt(&ok);
                        if(ok) _config.homingEnable = (value!=0);
                        break;

                    case 23:
                        value = fields[1].toInt(&ok);
                        if(ok) _config.homingDirInvertMask = value;
                        break;

                    case 24:
                        valueDbl = fields[1].toDouble(&ok);
                        if(ok) _config.homingFeed = valueDbl;
                        break;

                    case 25:
                        valueDbl = fields[1].toDouble(&ok);
                        if(ok) _config.homingSeek = valueDbl;
                        break;

                    case 26:
                        value = fields[1].toInt(&ok);
                        if(ok) _config.homingDebounce = value;
                        break;

                    case 27:
                        valueDbl = fields[1].toDouble(&ok);
                        if(ok) _config.homingPullOff = valueDbl;
                        break;

                    case 30:
                        value = fields[1].toInt(&ok);
                        if(ok) _config.maxSpindleSpeed = value;
                        break;

                    case 31:
                        value = fields[1].toInt(&ok);
                        if(ok) _config.minSpindleSpeed = value;
                        break;

                    case 100:
                    case 101:
                    case 102:
                        valueDbl = fields[1].toDouble(&ok);
                        if(ok) _config.stepsPerMm[id-100] = valueDbl;
                        break;

                    case 110:
                    case 111:
                    case 112:
                        valueDbl = fields[1].toDouble(&ok);
                        if(ok) _config.maxFeedRate[id-110] = valueDbl;
                        break;

                    case 120:
                    case 121:
                    case 122:
                        valueDbl = fields[1].toDouble(&ok);
                        if(ok) _config.acceleration[id-120] = valueDbl;
                        break;

                    case 130:
                    case 131:
                    case 132:
                        valueDbl = fields[1].toDouble(&ok);
                        if(ok) _config.maxTravel[id-130] = valueDbl;
                        break;
                }
                if(ok)
                    return true;
            }
        }
    }

    emit report(1, QString("Unrecognised Grbl parameter line: ") + line);
    return false;
}


//////  u p d a t e  C o n f i g u r a t i o n  //////
void GrblControl::updateConfiguration(const Config& conf)
{
    if(!isActive())
        return;

    char cmd[32];
    const char xyz[4] = "XYZ";

    if(conf.stepPulse != _config.stepPulse){
        ::sprintf(cmd, "$0=%d", conf.stepPulse);
        issueCommand(cmd, "Step pulse");
    }
    if(conf.stepIdleDelay != _config.stepIdleDelay){
        ::sprintf(cmd, "$1=%d", conf.stepIdleDelay);
        issueCommand(cmd, "Step idle delay");
    }
    if(conf.directionInvertMask != _config.directionInvertMask){
        ::sprintf(cmd, "$3=%d", conf.directionInvertMask);
        issueCommand(cmd, "Invert direction");
    }

    if(conf.stepEnableInvert != _config.stepEnableInvert){
        ::sprintf(cmd, "$4=%d", conf.stepEnableInvert);
        issueCommand(cmd, "Invert stepEn");
    }
    if(conf.limitSwitchInvert != _config.limitSwitchInvert){
        ::sprintf(cmd, "$5=%d", conf.limitSwitchInvert);
        issueCommand(cmd, "Invert limit switch");
    }
    if(conf.probePinInvert != _config.probePinInvert){
        ::sprintf(cmd, "$6=%d", conf.probePinInvert);
        issueCommand(cmd, "Invert probe pin");
    }

    if(conf.junctionDeviation != _config.junctionDeviation){
        ::sprintf(cmd, "$11=%.1f", conf.junctionDeviation);
        issueCommand(cmd, QString("Junction dev"));
    }
    if(conf.arcTolerance != _config.arcTolerance){
        ::sprintf(cmd, "$12=%.1f", conf.arcTolerance);
        issueCommand(cmd, QString("Arc tolerance"));
    }
    if(conf.imperial != _config.imperial){
        ::sprintf(cmd, "$13=%d", conf.imperial);
        issueCommand(cmd, "Set units");
    }

    if(conf.homingEnable != _config.homingEnable){
        ::sprintf(cmd, "$22=%d", conf.homingEnable);
        issueCommand(cmd, "Enable homing");
    }
    if(conf.homingDirInvertMask != _config.homingDirInvertMask){
        ::sprintf(cmd, "$23=%d", conf.homingDirInvertMask);
        issueCommand(cmd, "Invert homing dir");
    }
    if(conf.homingFeed != _config.homingFeed){
        ::sprintf(cmd, "$24=%.1f", conf.homingFeed);
        issueCommand(cmd, QString("Homing feed"));
    }
    if(conf.homingSeek != _config.homingSeek){
        ::sprintf(cmd, "$25=%.1f", conf.homingSeek);
        issueCommand(cmd, QString("Homing seek"));
    }
    if(conf.homingDebounce != _config.homingDebounce){
        ::sprintf(cmd, "$26=%d", conf.homingDebounce);
        issueCommand(cmd, "Homing debounce");
    }
    if(conf.homingPullOff != _config.homingPullOff){
        ::sprintf(cmd, "$27=%.1f", conf.homingPullOff);
        issueCommand(cmd, QString("Homing pull-off"));
    }

    if(conf.maxSpindleSpeed != _config.maxSpindleSpeed){
        ::sprintf(cmd, "$30=%d", conf.maxSpindleSpeed);
        issueCommand(cmd, "Max spindle speed");
    }
    if(conf.minSpindleSpeed != _config.minSpindleSpeed){
        ::sprintf(cmd, "$31=%d", conf.minSpindleSpeed);
        issueCommand(cmd, "Min spindle speed");
    }

    for(int i=0; i<3; ++i){
        if(conf.stepsPerMm[i] != _config.stepsPerMm[i]){
            ::sprintf(cmd, "$10%d=%.1f", i, conf.stepsPerMm[i]);
            issueCommand(cmd, QString("Steps per mm ") + xyz[i]);
        }
        if(conf.maxFeedRate[i] != _config.maxFeedRate[i]){
            ::sprintf(cmd, "$11%d=%.1f", i, conf.maxFeedRate[i]);
            issueCommand(cmd, QString("Max feedrate ") + xyz[i]);
        }
        if(conf.acceleration[i] != _config.acceleration[i]){
            ::sprintf(cmd, "$12%d=%.1f", i, conf.acceleration[i]);
            issueCommand(cmd, QString("Acceleration ") + xyz[i]);
        }
        if(conf.maxTravel[i] != _config.maxTravel[i]){
            ::sprintf(cmd, "$13%d=%.1f", i, conf.maxTravel[i]);
            issueCommand(cmd, QString("Max travel ") + xyz[i]);
        }
    }

    _config = conf;
}


//////  u p d a t e  S t a r t u p  B l o c k  //////
void GrblControl::updateStartupBlock(const char* block, uint32_t n)
{
    if(!isActive() || n > 1)
        return;

    if(_startup[n] == QLatin1String(block))
        return;

    char cmd[64];
    ::sprintf(cmd, "SN%d=%s", n, block);
    issueCommand(cmd, QString("Startup block ") + QString::number(n));
    _startup[n] = QLatin1String(block);
}


void GrblControl::setSeekRate(int rate)
{
    _seekRate = rate;

    if(!isActive())
        return;

    char cmd[32];
    ::sprintf(cmd, "G0F%d", _seekRate);
    issueCommand(cmd, QString("Seek rate"));
}


void GrblControl::setFeedRate(int rate)
{
    _feedRate = rate;

    if(!isActive())
        return;

    char cmd[32];
    ::sprintf(cmd, "G1F%d", _feedRate);
    issueCommand(cmd, QString("Feed rate"));
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
            emit report(-1, QString("Retrieveing grbl version from line: ") + line);
            _retrieveVersion(line);
        }
        else if(line[0] == '<'){ // CNC status response
            emit report(-3, QString("Retrieveing grbl status from line: ") + line);
            _retrieveStatus(line);
        }
        else if(line[0] == '$'){ // parameters
//            emit report(-1, QString("Retrieveing grbl parameter from line: ") + line);
            _retrieveParameter(line);
        }
        else{ // other commands
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
                else if(line[0] != '[')
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

