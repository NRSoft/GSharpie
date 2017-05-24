#ifndef GSHARPIE_GCODESEQUENCER_H
#define GSHARPIE_GCODESEQUENCER_H
#include <QObject>
#include <QString>
#include "gsharp.h"
#include "grblcontrol.h"



class GCodeSequencer: public QObject
{
    Q_OBJECT

public:
    GCodeSequencer() {_grbl = nullptr;}

    void setGrblControl(GrblControl* grbl);

    // returns error line number or 0 if no errors
    int loadProgram(const QString& program, QString* errorMsg=nullptr);

    void rewindProgram();

    inline bool isReady() const {return _ready;}

    bool nextLine(int& lineNumber, std::string& line, QString* errorMsg=nullptr);

private:
    GrblControl* _grbl;
    gsharp::Interpreter _interp;

    bool _ready;
};

#endif // GSHARPIE_GCODESEQUENCER_H
