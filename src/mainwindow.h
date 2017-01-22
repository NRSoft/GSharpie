#ifndef GSHARPIE_MAINWINDOW_H
#define GSHARPIE_MAINWINDOW_H

#include <QTimer>
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
    void on_btnLoad_clicked();
    void on_btnStart_clicked();
    void on_btnKillAlarm_clicked();
    void on_btnCheckMode_clicked();
    void on_btnSetOrigin_clicked();
    void on_btnHoming_clicked();

    void on_GrblResponse(GrblCommand cmd);
    void on_errorReport(int level, const QString& msg);

    void updateStatus();

private:
    quint32 _issueCommand(const char* code, const QString& name);

private:
    Ui::MainWindow *ui;

    QTimer *_timer;
    GrblControl* _grbl;
    GCodeSequencer* _sequencer;

    bool _restartTimer;
};

#endif // GSHARPIE_MAINWINDOW_H
