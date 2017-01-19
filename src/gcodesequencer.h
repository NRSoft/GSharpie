#ifndef GSHARPIE_GCODESEQUENCER_H
#define GSHARPIE_GCODESEQUENCER_H
#include <QObject>
#include <QMutex>
#include <QThread>
#include <QString>
#include "gsharp.h"
#include "grblcontrol.h"


class GCodeSequencer: public QThread
{
    Q_OBJECT

public:
    GCodeSequencer() {}

    void setGrblControl(GrblControl* grbl);

    void loadProgram(const QString& program);

    void run() Q_DECL_OVERRIDE;

signals:
    void currentStep(int lineNumber);

private:
    QMutex  _mutex;
    QString _program; // container for active g# program

    GrblControl* _grbl;
    gsharp::Interpreter _interp;
};

#endif // GSHARPIE_GCODESEQUENCER_H
