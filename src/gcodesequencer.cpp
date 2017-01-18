#include <QDebug>
#include "gcodesequencer.h"

void GCodeSequencer::run() //Q_DECL_OVERRIDE
{
    int i;
    qDebug()<<"thread started";
    for(i=0; i<10; ++i){
        emit currentStep(i);
        sleep(1);
        if(isInterruptionRequested())
            break;
    }
    qDebug()<<"thread finished, i =" << i;
}
