#include <string>
#include <QDebug>
#include "gcodesequencer.h"
#include "grblcontrol.h"


void GCodeSequencer::setGrblControl(GrblControl* grbl)
{
    if(!isRunning()){
        _grbl = grbl;
    }
}


void GCodeSequencer::loadProgram(const QString& program)
{
    if(!isRunning()){
        _mutex.lock();
        _program = program;
        _mutex.unlock();
    }
}


void GCodeSequencer::run()
{
    if(!_grbl->isConnected()){
        qDebug()<<"GRBL controller is not connected";
        return;
    }

    _mutex.lock();
    std::string program = _program.toStdString();
    _mutex.unlock();

    try{
        _interp.Load(program);
    }
    catch(std::exception& e){
       qDebug() << "File parsing error: " << e.what();
       return;
    }

    std::string str;
    gsharp::ExtraInfo extra;
    try{
        while(_interp.Step(str, extra)){
             // get next G-Code line
             if(!str.empty()){
                 emit currentStep(_interp.GetCurrentLineNumber());
                 qDebug() << "cmd" << QString(str.c_str()) << "(" << _interp.GetCurrentLineNumber() << ")";
                 sleep(1);
                 if(isInterruptionRequested()){
                    qDebug()<<"thread terminated";
                    return;
                 }
             }
        }
    }
    catch(std::exception& e){
       qDebug() << "Interpreter error: " << e.what();
       return;
    }

    qDebug()<<"thread finished successfully";
}
