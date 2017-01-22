#include <string>
#include <QDebug>
#include "gcodesequencer.h"
#include "grblcontrol.h"


void GCodeSequencer::setGrblControl(GrblControl* grbl)
{
    _grbl = grbl;
}


bool GCodeSequencer::loadProgram(const QString& program)
{
    try{
        _interp.Load(program.toStdString());
    }
    catch(std::exception& e){
        emit report(1, QString("Parsing G# file: ") + e.what());
        return false;
    }

    return true;
}


void GCodeSequencer::rewindProgram()
{
    _interp.Rewind();
}


bool GCodeSequencer::nextLine(int& lineNumber, std::string& line)
{
    gsharp::ExtraInfo extra;
    while(_interp.Step(line, extra)){
        if(!line.empty()){
qDebug() << "Next cmd in sequence:" << line.c_str();
            lineNumber = _interp.GetCurrentLineNumber();
            return true;
        }
    }
    return false;
}
