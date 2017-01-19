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

    bool connect(QString& grblVersion);
    inline void disconnect() {_port->close(); _connected=false;}
    inline bool isConnected() const {return _connected;}

    bool issueReset(QString& grblVersion);
    //const string send(const string& line);
    string issueCommand(string cmd);

private:
    bool _send(const string& data, string& response); // returns response

private:
    QSerialPort* _port;
    bool _connected;

    int _readTimeout; // ms
};

#endif // GSHARPIE_GRBLCONTROL_H
