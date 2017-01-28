#ifndef GSHARPIE_MAINWINDOW_H
#define GSHARPIE_MAINWINDOW_H

#include <QTimer>
#include <QMainWindow>
#include <QSettings>

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
    void on_btnEdit_clicked();
    void on_btnStart_clicked();

    void on_btnSerial_clicked();

    void on_btnSimulation_clicked();
    void on_btnSetOrigin_clicked();
    void on_btnUnlock_clicked();
    void on_btnHoming_clicked();
    void on_btnReset_clicked();

    void on_btnJogD_clicked();
    void on_btnJogU_clicked();
    void on_btnJogF_clicked();
    void on_btnJogB_clicked();
    void on_btnJogL_clicked();
    void on_btnJogR_clicked();

    void on_GrblResponse(GrblCommand cmd);
    void on_errorReport(int level, const QString& msg);

    void _updateStatus();
    void _sendGCode();

    void on_btnSave_clicked();

private:
    quint32 _issueCommand(const char* code, const QString& name);
    void _loadSequencer(const QString& program);

private:
    Ui::MainWindow *ui;

    QTimer *_timerStatus; // update status every ...
    QTimer *_timerGCode;  // send next gcode command every ...
    GrblControl* _grbl;
    GCodeSequencer* _sequencer;

    QSettings* _settings;
    QPalette _paletteNoEdit;

    bool _restartTimer;
};

#endif // GSHARPIE_MAINWINDOW_H
