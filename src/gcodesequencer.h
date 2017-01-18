#ifndef GSHARPIE_GCODESEQUENCER_H
#define GSHARPIE_GCODESEQUENCER_H
#include <QObject>
#include <QThread>


class GCodeSequencer: public QThread
{
    Q_OBJECT

public:
    GCodeSequencer() {}

    void run() Q_DECL_OVERRIDE;

signals:
    void currentStep(int lineNumber);

};

#endif // GSHARPIE_GCODESEQUENCER_H
