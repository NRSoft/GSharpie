#ifndef GSHARPIE_GRBLCONTROL_H
#define GSHARPIE_GRBLCONTROL_H
#include <stdint.h>
#include <string>
#include <QtGlobal>
#include <QtSerialPort/QSerialPort>
#include <QQueue>

using namespace std;


struct CncPosition
{
    double mx, my, mz; // in machine coordinates
    double wx, wy, wz; // in work coordinates
};


struct GrblCommand
// only for commands with 'ok' or 'error' response
// (in fact everything exept '?' and 'Ctrl-X')
{
    quint64 id; // sequence number
    QString name; // readble name
    std::string code; // grbl code
    bool sent;  // can be delayed
    QString error; // error message, or empty if 'ok'
    QStringList response; // information lines
};


class GrblControl: public QObject
{
    Q_OBJECT

public:
    enum STATUS{Undef, Idle, Run, Hold, Door, Home, Alarm, Check};

public:
    GrblControl();
    ~GrblControl() {closeSerialPort();}

    bool openSerialPort(const QString& portName, qint32 baudrate=115200);
    void closeSerialPort();
    bool getSerialPortInfo(QString& portName, quint32& baudrate);
    inline bool isOpened() const {return _opened;}
    inline bool isActive() const {return _opened && !_version.isEmpty();}

    // any command with 'ok' or 'error' return (everything except '?' and 'ctrl-X')
    // returns command id for references when completed, or 0 if error
    quint32 issueCommand(const char* cmd, const QString& readableName);

    bool issueReset(); // 'ctrl-X'
    bool issueStatusRequest(); // '?'


    inline const QString& getVersion() const {return _version;}
    inline STATUS getCurrentStatus(CncPosition& pos) const {pos = _position; return _status;}
    inline int getQueueSize() const {return _commands.size();}

signals:
    void report(int level, const QString& msg); // levels: debug(-), normal(0), errors(+)
    void commandComplete(GrblCommand cmd); // keep cmd as copy, as it will be deleted from the queue!

private slots:
    void _handlePortRead();
    void _handlePortError(QSerialPort::SerialPortError error);

private:
    bool _sendNextCommand();

private:
    QString _version;

    QSerialPort* _port;
    bool _opened;

    QQueue<GrblCommand> _commands;
    QByteArray  _response;
    quint32 _lastCmdId;


    STATUS _status;
    CncPosition _position;
};

#endif // GSHARPIE_GRBLCONTROL_H
