#ifndef GSHARPIE_MAINWINDOW_H
#define GSHARPIE_MAINWINDOW_H

#include <QTimer>
#include <QMainWindow>
#include <QSettings>

#include "grblcontrol.h"
#include "gcodesequencer.h"


struct CncConfig
{

};

namespace Ui {
class MainWindow;
}


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    bool eventFilter(QObject* obj, QEvent* event);

private slots:

    void on_GrblResponse(GrblControl::Command cmd);
    void on_errorReport(int level, const QString& msg);

    void _statusRequest();
    void _updateStatus();
    void _sendGCode();

    void on_dial_jogFeed_valueChanged(int value);

    void on_btn_jogUp_pressed();
    void on_btn_jogUp_released();
    void on_btn_jogDown_pressed();
    void on_btn_jogDown_released();
    void on_btn_jogLeft_pressed();
    void on_btn_jogLeft_released();
    void on_btn_jogRight_pressed();
    void on_btn_jogRight_released();
    void on_btn_jogForward_pressed();
    void on_btn_jogForward_released();
    void on_btn_jogBackward_pressed();
    void on_btn_jogBackward_released();

    void on_btn_setXYZOrigin_pressed();
    void on_btn_setXOrigin_clicked();
    void on_btn_setYOrigin_clicked();
    void on_btn_setZOrigin_clicked();

    void on_btn_setMinLimit_clicked();
    void on_btn_setMaxLimit_clicked();

    void on_btn_serial_clicked();

    void on_btn_homing_clicked();

    void on_btn_reset_clicked();

    void on_btn_unlock_clicked();

    void on_btn_simulation_clicked();

    //    void on_btnLoad_clicked();
    //    void on_btnSave_clicked();
    //    void on_btnEdit_clicked();
    void on_btn_runGCode_clicked();

    void on_btn_singleCommand_clicked();

    void on_btn_loadGCode_clicked();

    void on_btn_saveGCode_clicked();

    void on_btn_editGCode_clicked();

    void on_btn_settings_clicked();

private:
    quint32 _issueCommand(const char* code, const QString& name);
    void _loadSequencer(const QString& program);

    bool _programEditingMode() const;
    bool _commandEditingMode() const;

    void _initJoggingControls();
    void _prepareJogCommand(char axis, double limit);
    void _cancelJogging();

private:
    Ui::MainWindow *ui;

    QTimer *_timerStatus; // update status every ...
    QTimer *_timerGCode;  // send next gcode command every ...
    GrblControl* _grbl;
    GCodeSequencer* _sequencer;

    QSettings* _settings;
    QPalette _paletteNoEdit;

    int _statusTimerPeriod; // ms, related to frequency of status updates
    bool _restartTimer;

    // keyboard control
    bool _keyShiftPressed;
    bool _keyCtrlPressed;
    bool _keyAltPressed;
    bool _keyMetaPressed; // "windows" key

    int _jogRate;
    int _defaultJogDialPosition; // loaded from settings
    bool _isJogging; // faster than reading status from grbl
    char _jogCommand[32];
};

#endif // GSHARPIE_MAINWINDOW_H
