#ifndef GSHARPIE_GRBLCONTROL_H
#define GSHARPIE_GRBLCONTROL_H
#include <stdint.h>
#include <string>
#include <QVector4D>
#include <QtSerialPort/QSerialPort>
#include <QQueue>


struct CncToolPosition // relative to the workpiece
{
    QVector4D mpos; // fourth dimension may be added in later versions
    QVector4D wpos;
};


//struct GrblCommand
//// only for commands with 'ok' or 'error' response
//{
//    quint64 id; // sequence number
//    QString name; // readble name
//    std::string code; // grbl code
//    bool sent;  // can be delayed
//    QString error; // error message, or empty if 'ok'
//    QStringList response; // information lines
//};


class GrblControl: public QObject
{
    Q_OBJECT

public: // type definitions
    enum MACHINE_STATE{Undef, Idle, Run, Hold, Jog, Alarm, Door, Check, Home, Sleep};

    enum REALTIME_COMMAND{SOFT_RESET       = 0x18,
                          GET_STATUS       = 0x3F,
                          RESUME           = 0x7E,
                          FEED_HOLD        = 0x21,
                          SAFETY_DOOR      = 0x84,
                          CANCEL_JOGGING   = 0x85,
                          FEED_DEFAULT     = 0x90,
                          FEED_INCREASE_10 = 0x91,
                          FEED_DECREASE_10 = 0x92,
                          FEED_INCREASE_1  = 0x93,
                          FEED_DECREASE_1  = 0x94};

    struct Status
    {
        CncToolPosition pos;
        MACHINE_STATE state;
        int32_t feedrate; // rate
        int32_t spindle; // speed
        int32_t line; // currently executed line
    };

    struct Command
    { // only for commands with 'ok' or 'error' response
        quint64 id; // sequence number
        QString name; // readble name
        std::string code; // grbl code
        bool sent;  // can be delayed
        QString error; // error message, or empty if 'ok'
        QStringList response; // information lines
    };

public:
    GrblControl();
    ~GrblControl() {closeSerialPort();}

    bool openSerialPort(const QString& portName, qint32 baudrate=115200);
    void closeSerialPort();
    bool getSerialPortInfo(QString& portName, quint32& baudrate);
    inline bool isOpened() const {return _connected;}
    inline bool isActive() const {return _connected && _version >= MIN_SUPPORTED_VERSION;}

    // any command with 'ok' or 'error' return (everything except '?' and 'ctrl-X')
    // returns command id for references when completed, or 0 if error
    quint32 issueCommand(const char* cmd, const QString& readableName);
    bool issueRealtimeCommand(REALTIME_COMMAND cmd);

    bool issueJogging(const QVector4D& steps, double feedrateAdjustment);

//    inline const QString& getVersion() const {return _version;}
//    inline MACHINE_STATE getCurrentStatus(Status& status) const {status = _status; return _status.state;}
    inline const Status& getCurrentStatus() const {return _status;}
    inline int getQueueSize() const {return _commands.size();}
    inline void clearQueue() {_port->clear(QSerialPort::Output); _commands.clear();}

signals:
    void report(int level, const QString& msg); // progressive levels: debug(-), information(0), errors(+)
    void commandComplete(GrblControl::Command cmd); // keep cmd as copy, as it will be deleted from the queue!
    void statusUpdated();

private slots:
    void _handlePortRead();
    void _handlePortError(QSerialPort::SerialPortError error);

private:
    bool _sendNextCommand();
    bool _retrieveVersion(const QByteArray& line);
    void _retrieveStatus(const QByteArray& line);
    bool _readCoordinates(const QByteArray& line, QVector4D& pos);

private:
    QSerialPort* _port;
    bool _connected;

    QQueue<GrblControl::Command> _commands;
    QByteArray  _response;
    quint32 _lastCmdId;

    const double MIN_SUPPORTED_VERSION = 1.1;
    double _version;

    bool _metric; // units: [true] metric (mm) or [false] imperial (inches)
    double _joggingFeedrate; // before adjustment coefficient is applied

    Status _status;
    QVector4D _toolOffset;
};

#endif // GSHARPIE_GRBLCONTROL_H
