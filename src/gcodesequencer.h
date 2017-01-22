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

    bool loadProgram(const QString& program);
    void rewindProgram();

    bool nextLine(int& lineNumber, string& line);

signals:
    void report(int level, const QString& msg); // levels: debug(-), normal(0), errors(+)

private:
    GrblControl* _grbl;
    gsharp::Interpreter _interp;
};

#endif // GSHARPIE_GCODESEQUENCER_H
