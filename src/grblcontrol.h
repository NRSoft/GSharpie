#ifndef GSHARPIE_GRBLCONTROL_H
#define GSHARPIE_GRBLCONTROL_H
#include <stdint.h>
#include <string>
#include <QtGlobal>
#include <QtSerialPort/QSerialPort>

using namespace std;


class GrblControl
{
public:
    GrblControl(const QString& portName, qint32 baudrate=115200);
    ~GrblControl() {disconnect();}

    bool connect();
    inline void disconnect() {_port->close(); _connected=false;}
    inline bool isConnected() const {return _connected;}

    void issueReset();
    const string send(const string& line);
private:
    QSerialPort* _port;
    bool _connected;
};

#endif // GSHARPIE_GRBLCONTROL_H
