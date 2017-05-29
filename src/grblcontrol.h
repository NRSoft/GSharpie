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

    struct Config
    {
        double junctionDeviation; // $11
        double arcTolerance; // $12
        double homingFeed; // slower, $24
        double homingSeek; // faster, $25
        double homingPullOff; // mm, $27
        double stepsPerMm[4]; // $100-102 (103 for A?)
        double maxFeedRate[4]; // $110-112 (113 for A?)
        double acceleration[4]; // $120-122 (123 for A?)
        double maxTravel[4]; // $130-132 (133 for A?)
        uint32_t minSpindleSpeed; // rpm, $31
        uint32_t maxSpindleSpeed; // rpm, $30
        uint8_t stepPulse; // uSec, $0
        uint8_t stepIdleDelay; // uSec, $1
        uint8_t directionInvertMask; // $3
        uint8_t homingDirInvertMask; // $23
        uint8_t homingDebounce; // mSec, $26
        bool homingEnable; // mSec, $22
        bool stepEnableInvert; // polarity of the "Step Enuble" pin, $4
        bool limitSwitchInvert; // polarity of the limit switch pin, $5
        bool probePinInvert; // polarity of the probe pin, $6
        bool imperial; // units: [false] metric (mm) or [true] imperial (inches), $13
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
    quint32 issueCommand(const char* cmd, const QString& readableName); // cmd without '/n' at the end!
    bool issueRealtimeCommand(REALTIME_COMMAND cmd);

    bool issueJogging(const QVector4D& steps, double feedrateAdjustment);

//    inline const QString& getVersion() const {return _version;}
//    inline MACHINE_STATE getCurrentStatus(Status& status) const {status = _status; return _status.state;}

    inline const Status& getCurrentStatus() const {return _status;}

    inline const Config& getConfiguration() const {return _config;}
    void updateConfiguration(const Config& conf);

    inline void getStartupBlock(QString& block, uint32_t n) const {if(n<2) block = _startup[n];}
    void updateStartupBlock(const char* block, uint32_t n);

    void setSeekRate(int rate);
    void setFeedRate(int rate);
    inline int getSeekRate() const {return _seekRate;}
    inline int getFeedRate() const {return _feedRate;}

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
    bool _retrieveParameter(const QByteArray& line);

private:
    QSerialPort* _port;
    bool _connected;

    QQueue<GrblControl::Command> _commands;
    QByteArray  _response;
    quint32 _lastCmdId;

    const double MIN_SUPPORTED_VERSION = 1.1;
    double _version;

    Config _config;
    int _seekRate; // default seekrate (G0)
    int _feedRate; // default feedrate (G1,G2,G3)
    QString _startup[2]; // two startup blocks ("$N");

    Status _status;
    QVector4D _toolOffset;
};

#endif // GSHARPIE_GRBLCONTROL_H
