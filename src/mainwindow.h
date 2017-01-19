#ifndef GSHARPIE_MAINWINDOW_H
#define GSHARPIE_MAINWINDOW_H

#include <QMainWindow>

#include "grblcontrol.h"
#include "gcodesequencer.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_btnStart_clicked();

    void on_sequencerFinished();
    void on_sequencerStep(int lineNumber);

    void on_btnLoad_clicked();

private:
    Ui::MainWindow *ui;

    GrblControl* _grbl;
    GCodeSequencer* _sequencer;
};

#endif // GSHARPIE_MAINWINDOW_H
