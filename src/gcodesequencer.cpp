#include <string>
#include <QDebug>
#include "gcodesequencer.h"
#include "grblcontrol.h"


void GCodeSequencer::setGrblControl(GrblControl* grbl)
{
    _ready = false;
    _grbl = grbl;
    _interp.EnablePrettyFormat(false); // tight lines, no spaces
}


bool GCodeSequencer::loadProgram(const QString& program, QString* errorMsg)
{
    _ready = false;
    try{
        _interp.Load(program.toStdString());
    }
    catch(std::exception& e){
        if(errorMsg)
            *errorMsg = QString(e.what());
        return false;
    }

    _ready = true;
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
