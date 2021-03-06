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


int GCodeSequencer::loadProgram(const QString& program, QString* errorMsg)
{
    _ready = false;
    try{
        _interp.Load(program.toStdString());
    }
    catch(std::exception& e){
        if(errorMsg)
            *errorMsg = QString(e.what());
        return _interp.GetCurrentLineNumber(); // error line
    }

    _ready = true;
    return 0; // loaded successfully
}


void GCodeSequencer::rewindProgram()
{
    _interp.Rewind();
}


bool GCodeSequencer::nextLine(int& lineNumber, std::string& line, QString* errorMsg)
{
    gsharp::ExtraInfo extra;
    try{
        while(_interp.Step(line, extra)){
            if(!line.empty()){
//qDebug() << "Next cmd in sequence:" << line.c_str();
                lineNumber = _interp.GetCurrentLineNumber();
                return true;
            }
        }
    }
    catch(std::exception& e){
        if(errorMsg)
            *errorMsg = QString(e.what());
        lineNumber = _interp.GetCurrentLineNumber();
        return false; // error line
    }
    return false; // finished
}
