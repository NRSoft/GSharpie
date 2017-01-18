#include <string>
#include <QDebug>
#include "gcodesequencer.h"
#include "grblcontrol.h"


void GCodeSequencer::loadProgram(const QString& program)
{
    _mutex.lock();
    _program = program;
    _mutex.unlock();
}


void GCodeSequencer::run()
{
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
