#include <cstdio>
#include <QDebug>
#include <QTime>
#include <QFile>
#include <QFileDialog>
#include <QTextBlock>
#include <QTextCursor>
#include <QStyleOptionSlider>
#include "dlgserialport.h"
#include "dlgconfig.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

int GSharpieReportLevel = 0; // global


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->text_errorLog->clear();

    on_errorReport(0, QString("Program started on ") + QDate::currentDate().toString());
//GSharpieReportLevel = -1;

    _grbl = new GrblControl();
    connect(_grbl, SIGNAL(report(int, QString)), this, SLOT(on_errorReport(int, QString)));
    connect(_grbl, SIGNAL(commandComplete(GrblControl::Command)),
             this, SLOT(on_GrblResponse(GrblControl::Command)));

    _sequencer = new GCodeSequencer();
    _sequencer->setGrblControl(_grbl);

    _timerGCode = new QTimer(this); // sequencer timer
    connect(_timerGCode, SIGNAL(timeout()), this, SLOT(_sendGCode()));

    _settings = new QSettings("GSharpie.ini", QSettings::IniFormat);

    _settings->beginGroup("Serial_Port");
    if(_settings->value("open_on_startup", false).toBool()){
        if(_grbl->openSerialPort(_settings->value("port_name").toString(),
                                 _settings->value("baud_rate").toInt()))
            _grbl->issueRealtimeCommand(GrblControl::GET_STATUS);
    }
    _settings->endGroup();

    _settings->beginGroup("CNC_Control");
    _grbl->setSeekRate(_settings->value("seek_rate", 500).toInt());
    _grbl->setFeedRate(_settings->value("feed_rate", 100).toInt());
    _statusTimerPeriod = 1000 / _settings->value("refresh_rate", 5).toInt(); // careful with high refresh rates!
    _settings->endGroup();

    _timerStatus = new QTimer(this); // status timer
    connect(_timerStatus, SIGNAL(timeout()), this, SLOT(_statusRequest()));
    connect(_grbl, SIGNAL(statusUpdated()), this, SLOT(_updateStatus()));
    _timerStatus->start(_statusTimerPeriod);
    _restartTimer = false;

    _initMainControls();

    _initJoggingControls();

    _keyShiftPressed = false;
    _keyCtrlPressed  = false;
    _keyAltPressed   = false;
    _keyMetaPressed  = false;

    qApp->installEventFilter(this);
}


MainWindow::~MainWindow()
{    
    delete _sequencer;
    delete _grbl;
    delete _settings;
    delete ui;
}

bool MainWindow::_programEditingMode() const
{
    return !ui->edit_textGCode->isReadOnly();
}

bool MainWindow::_commandEditingMode() const
{
    return qApp->focusWidget() == ui->edit_singleCommand;
}


//////  e v e n t  F i l t e r  //////
bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    if(_programEditingMode() || _commandEditingMode() ||
        qApp->focusWidget() == ui->spin_minX ||
        qApp->focusWidget() == ui->spin_maxX ||
        qApp->focusWidget() == ui->spin_minY ||
        qApp->focusWidget() == ui->spin_maxY ||
        qApp->focusWidget() == ui->spin_minZ ||
        qApp->focusWidget() == ui->spin_maxZ)
            return false; // keyboard control is prohibited
//return false;

    if(event->type()==QEvent::KeyPress){
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        int k = keyEvent->key();
//        qDebug("Key pressed: 0x%08X", k);

        // adjust jog feedrate
        if(k == Qt::Key_Comma || k == Qt::Key_Less){
            int pos = ui->dial_jogFeed->sliderPosition() - 1;
            if(pos >= ui->dial_jogFeed->minimum())
                ui->dial_jogFeed->setSliderPosition(pos);
        }
        else if(k == Qt::Key_Period || k == Qt::Key_Greater){
            int pos = ui->dial_jogFeed->sliderPosition() + 1;
            if(pos <= ui->dial_jogFeed->maximum())
                ui->dial_jogFeed->setSliderPosition(pos);
        }

        if(!keyEvent->isAutoRepeat()){
            // key modifiers
            if(k == Qt::Key_Shift)
                _keyShiftPressed = true;
            else if(k == Qt::Key_Control)
                _keyCtrlPressed = true;
            else if(k == Qt::Key_Alt)
                _keyAltPressed = true;
            else if(k == Qt::Key_Meta)
                _keyMetaPressed = true;

            // jogging control
            else if(k == Qt::Key_Right)
                on_btn_jogRight_pressed();
            else if(k == Qt::Key_Left)
                on_btn_jogLeft_pressed();
            else if(k == Qt::Key_Up)
                on_btn_jogForward_pressed();
            else if(k == Qt::Key_Down)
                on_btn_jogBackward_pressed();
            else if(k == Qt::Key_PageUp)
                on_btn_jogUp_pressed();
            else if(k == Qt::Key_PageDown)
                on_btn_jogDown_pressed();

            // execution control
            else if(k == Qt::Key_Escape) // pause execution or reset
                _grbl->issueRealtimeCommand(_keyShiftPressed? GrblControl::SOFT_RESET:
                                                              GrblControl::FEED_HOLD);
            else if(k == Qt::Key_Space) // resume execution
                _grbl->issueRealtimeCommand(GrblControl::RESUME);
        }
        return true;
    }
    else if(event->type()==QEvent::KeyRelease){
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if(!keyEvent->isAutoRepeat()){
            int k = keyEvent->key();
//            qDebug("Key released: 0x%08X", k);
            // key modifiers
            if(k == Qt::Key_Shift)
                _keyShiftPressed = false;
            else if(k == Qt::Key_Control)
                _keyCtrlPressed = false;
            else if(k == Qt::Key_Alt)
                _keyAltPressed = false;
            else if(k == Qt::Key_Meta)
                _keyMetaPressed = false;

            // jogging control
            else if(k == Qt::Key_Right)
                on_btn_jogRight_released();
            else if(k == Qt::Key_Left)
                on_btn_jogLeft_released();
            else if(k == Qt::Key_Up)
                on_btn_jogForward_released();
            else if(k == Qt::Key_Down)
                on_btn_jogBackward_released();
            else if(k == Qt::Key_PageUp)
                on_btn_jogUp_released();
            else if(k == Qt::Key_PageDown)
                on_btn_jogDown_released();
        }
        return true;
    }
    else {
        return QObject::eventFilter(obj, event);
    }
    return false;
}


void MainWindow::on_btn_loadGCode_clicked()
{
    QString name = QFileDialog::getOpenFileName(this, QStringLiteral("Open file"), ".",
                        QStringLiteral("G-sharp (*.ngs);;G-code (*.nc);;All files (*.*)"));
    if(name.isEmpty()) return;

    QFile file(name);
    if(!file.open(QFile::ReadOnly | QFile::Text)){
        on_errorReport(1, QString("Cannot open file ") + name);
        return;
    }
    on_errorReport(0, QString("Opened file ") + name);

    QString program(file.readAll());
    ui->edit_textGCode->setPlainText(program);

    _loadSequencer(program);
}


void MainWindow::on_btn_saveGCode_clicked()
{
    if(ui->edit_textGCode->document()->isEmpty()) return;

    QString name = QFileDialog::getSaveFileName(this, QStringLiteral("Save file"), ".",
                        QStringLiteral("G-sharp (*.ngs);;G-code (*.nc);;All files (*.*)"));
    if(name.isEmpty()) return;

    QFile file(name);
    if(!file.open(QIODevice::WriteOnly| QFile::Text)){
        on_errorReport(1, QString("Cannot open file ") + name + " for writing");
        return;
    }

    file.write(ui->edit_textGCode->document()->toPlainText().toLatin1());
}


void MainWindow::on_btn_editGCode_clicked()
{
    if(ui->edit_textGCode->isReadOnly()){
        ui->edit_textGCode->setReadOnly(false);
        ui->btn_runGCode->setEnabled(false);
        ui->btn_saveGCode->setEnabled(false);
        ui->btn_editGCode->setIcon(QIcon(":/icons/icons/no_edit.png"));
        ui->label_stateGCode->setText("editing");

        _paletteNoEdit = ui->edit_textGCode->palette();
        QPalette p = _paletteNoEdit;
        p.setColor(QPalette::Base, QColor(255, 255, 128, 64));
        p.setColor(QPalette::Text, Qt::black);
        ui->edit_textGCode->setPalette(p);
        ui->edit_textGCode->setFocus();
    }
    else{
        ui->edit_textGCode->setReadOnly(true);
        ui->btn_editGCode->setIcon(QIcon(":/icons/icons/edit.png"));

        ui->edit_textGCode->setPalette(_paletteNoEdit);

        _loadSequencer(ui->edit_textGCode->document()->toPlainText());
    }
}


void MainWindow::on_btn_runGCode_clicked()
{
    on_errorReport(0, QString("Program started"));
    _timerGCode->start(5);
}


void MainWindow::on_btn_serial_clicked()
{
    DlgSerialPort dlgPort(_grbl, _settings, this);
    dlgPort.exec();
}


void MainWindow::on_btn_settings_clicked()
{
    qApp->removeEventFilter(this);
    DlgConfig dlgConfig(_grbl, _settings);
    if(dlgConfig.exec() == QDialog::Accepted){
        _statusTimerPeriod = 1000 / dlgConfig.refreshRate();
        _restartTimer = true;
        if(dlgConfig.verbosityLevel() != GSharpieReportLevel){
            GSharpieReportLevel = 0;
            on_errorReport(0, QString("Verbosity is set to ") + dlgConfig.verbosityName());
            GSharpieReportLevel = dlgConfig.verbosityLevel();
        }
        ui->label_units->setText(_grbl->getConfiguration().imperial? "inches": "mm");
    }
    qApp->installEventFilter(this);
}


//void MainWindow::on_btn_simulation_clicked()
//{
//   _grbl->issueCommand("$C", "Check mode");
//}


void MainWindow::on_btn_unlock_clicked()
{
    if(_grbl->issueCommand("$X", "Unlock alarm") > 0)
        _grbl->setFeedRate(_grbl->getFeedRate()); // reset feedrate
}


void MainWindow::on_btn_reset_clicked()
{
    _initMainControls(); // disable unti reset
    _grbl->issueRealtimeCommand(GrblControl::SOFT_RESET);
}


void MainWindow::on_btn_setXOrigin_clicked()
{
    char cmd[32];
    ::sprintf(cmd, "G10L20P%dX0", ui->combo_toolOffset->currentIndex()+1);
    _grbl->issueCommand(cmd, "Set X origin");
}


void MainWindow::on_btn_setYOrigin_clicked()
{
    char cmd[32];
    ::sprintf(cmd, "G10L20P%dY0", ui->combo_toolOffset->currentIndex()+1);
    _grbl->issueCommand(cmd, "Set Y origin");
}


void MainWindow::on_btn_setZOrigin_clicked()
{
    char cmd[32];
    ::sprintf(cmd, "G10L20P%dZ0", ui->combo_toolOffset->currentIndex()+1);
    _grbl->issueCommand(cmd, "Set Z origin");
}


void MainWindow::on_btn_setXYZOrigin_pressed()
{
    char cmd[32];
    ::sprintf(cmd, "G10L20P%dX0Y0Z0", ui->combo_toolOffset->currentIndex()+1);
    _grbl->issueCommand(cmd, "Set XYZ origin");
}


void MainWindow::on_btn_setMinLimit_clicked()
{
    ui->spin_minX->setValue(_grbl->getCurrentStatus().pos.wpos.x());
    ui->spin_minY->setValue(_grbl->getCurrentStatus().pos.wpos.y());
    ui->spin_minZ->setValue(_grbl->getCurrentStatus().pos.wpos.z());
}


void MainWindow::on_btn_setMaxLimit_clicked()
{
    ui->spin_maxX->setValue(_grbl->getCurrentStatus().pos.wpos.x());
    ui->spin_maxY->setValue(_grbl->getCurrentStatus().pos.wpos.y());
    ui->spin_maxZ->setValue(_grbl->getCurrentStatus().pos.wpos.z());
}


void MainWindow::on_btn_homing_clicked()
{
    if(!_grbl->isActive()) return;
    _timerStatus->stop(); // status is not reported during homing
    _grbl->issueCommand("$H", "Homing");
    ui->label_status->setText("homing");
    _restartTimer = true;
}


void MainWindow::on_btn_singleCommand_clicked()
{
    if(ui->edit_singleCommand->text().isEmpty())
        return;
    _grbl->issueCommand(ui->edit_singleCommand->text().toLatin1().constData(), "User command");
    on_errorReport(0, QString("User command: ") + ui->edit_singleCommand->text());
    ui->edit_singleCommand->clear();
    this->setFocus();
}


void MainWindow::on_edit_singleCommand_returnPressed()
{
    on_btn_singleCommand_clicked();
}


void MainWindow::on_combo_toolOffset_currentIndexChanged(const QString &arg1)
{
    _grbl->issueCommand(arg1.toLatin1().constData(), "Tool offset");
}


void MainWindow::on_GrblResponse(GrblControl::Command cmd)
{
    for(int i=0; i < cmd.response.size(); ++i)
        on_errorReport(0, cmd.name + QStringLiteral(": ") + cmd.response.at(i));

    if(cmd.error.isEmpty())
        on_errorReport(-1, QString("Confirmed [") + QString::number(cmd.id) + QStringLiteral("]: ok"));
    else
        on_errorReport(1, cmd.name + QStringLiteral(": ") + cmd.error);

    if(_restartTimer){
        on_errorReport(-1, QString("Restarting status timer with period = ") + QString::number(_statusTimerPeriod));
        _restartTimer = false;
        _timerStatus->start(_statusTimerPeriod);
    }
}


void MainWindow::on_errorReport(int level, const QString& msg)
{
    if(level < GSharpieReportLevel)
        return;

    QString prefix = QTime::currentTime().toString("(H:mm:ss)");
    if(level > 0)
        prefix += QStringLiteral("Err: ");
    else if(level == 0)
        prefix += QStringLiteral("Msg: ");
    else
        prefix += QStringLiteral("Dbg: ");

    ui->text_errorLog->appendPlainText(prefix + msg);
}


void MainWindow::_loadSequencer(const QString& program)
{
    QString errorMsg;
    int errorLine = _sequencer->loadProgram(program, &errorMsg);
    ui->edit_textGCode->enableHighlight(errorLine > 0);
    if(errorLine > 0){
        QTextCursor cursor(ui->edit_textGCode->document()->findBlockByLineNumber(errorLine-1));
        ui->edit_textGCode->setTextCursor(cursor);
        on_errorReport(1, QString("Parsing g-code ") + errorMsg);
    }
    else
        on_errorReport(0, QString("Program is ready to run"));

    ui->btn_saveGCode->setEnabled(!ui->edit_textGCode->document()->isEmpty());
    ui->btn_runGCode->setEnabled(_grbl->isActive() && _sequencer->isReady());
    ui->label_stateGCode->setText(ui->btn_runGCode->isEnabled()? "ready": "");
}


//////  s t a t u s  R e q u e s t  //////
void MainWindow::_statusRequest()
{
    if(_grbl->isActive())
        _grbl->issueRealtimeCommand(GrblControl::GET_STATUS);
}


/////  u p d a t e  S t a t u s  //////
void MainWindow::_updateStatus()
{
    GrblControl::Status status = _grbl->getCurrentStatus();
    const GrblControl::MACHINE_STATE& state = status.state;

    ui->label_status->setText(state==GrblControl::Jog?   QStringLiteral("jogging"):
                              state==GrblControl::Run?   QStringLiteral("running"):
                              state==GrblControl::Home?  QStringLiteral("homing"):
                              state==GrblControl::Check? QStringLiteral("simulation"):
                              state==GrblControl::Idle?  QStringLiteral("idle"):
                              state==GrblControl::Alarm? QStringLiteral("alarm lock"):
                              state==GrblControl::Hold?  QStringLiteral("holding"):
                              state==GrblControl::Door?  QStringLiteral("door open"):
                              state==GrblControl::Sleep? QStringLiteral("dormant"):
                                                                         QStringLiteral("disconnected"));

    ui->label_WPosX->setText(QString::number(status.pos.wpos.x(), 'f', 3));
    ui->label_WPosY->setText(QString::number(status.pos.wpos.y(), 'f', 3));
    ui->label_WPosZ->setText(QString::number(status.pos.wpos.z(), 'f', 3));

    ui->label_MPosX->setText(QString::number(status.pos.mpos.x(), 'f', 3));
    ui->label_MPosY->setText(QString::number(status.pos.mpos.y(), 'f', 3));
    ui->label_MPosZ->setText(QString::number(status.pos.mpos.z(), 'f', 3));

    ui->btn_unlock->setEnabled(state==GrblControl::Alarm);
    ui->btn_reset->setEnabled(_grbl->isOpened());
    ui->btn_settings->setEnabled(_grbl->isActive());

    // --- simulation mode has to be thoroughly tested ---
    //    ui->btn_simulation->setEnabled(state==GrblControl::Idle || state==GrblControl::Check);
    //    ui->btn_simulation->setText(state==GrblControl::Check? QStringLiteral("Operation"):
    //                                                           QStringLiteral("Simulation"));

    ui->btn_jogLeft->setEnabled(state==GrblControl::Idle || state==GrblControl::Jog); // X
    ui->btn_jogRight->setEnabled(state==GrblControl::Idle || state==GrblControl::Jog);
    ui->btn_jogForward->setEnabled(state==GrblControl::Idle || state==GrblControl::Jog); // Y
    ui->btn_jogBackward->setEnabled(state==GrblControl::Idle || state==GrblControl::Jog);
    ui->btn_jogUp->setEnabled(state==GrblControl::Idle || state==GrblControl::Jog); // Z
    ui->btn_jogDown->setEnabled(state==GrblControl::Idle || state==GrblControl::Jog);

    ui->btn_homing->setEnabled(state==GrblControl::Idle || state==GrblControl::Alarm);
    ui->btn_setXOrigin->setEnabled(state==GrblControl::Idle);
    ui->btn_setYOrigin->setEnabled(state==GrblControl::Idle);
    ui->btn_setZOrigin->setEnabled(state==GrblControl::Idle);
    ui->btn_setXYZOrigin->setEnabled(state==GrblControl::Idle);

    ui->label_units->setText(_grbl->getConfiguration().imperial? "inches": "mm");
}


void MainWindow::_sendGCode()
{
    if(!_grbl->isActive())
        return;

    int queue = _grbl->getQueueSize();
    if(queue >= 5)
        return; // already too many commands

    if(!_sequencer->isReady())
        return;

    int lineNum;
    std::string gcode;
    QString errorMsg;
    if(!_sequencer->nextLine(lineNum, gcode, &errorMsg)){
        if(errorMsg.isEmpty())
            on_errorReport(0, QString("Program finished"));
        else{
            ui->edit_textGCode->enableHighlight(true);
            QTextCursor cursor(ui->edit_textGCode->document()->findBlockByLineNumber(lineNum-1));
            ui->edit_textGCode->setTextCursor(cursor);
            on_errorReport(1, QString("Running g-code ") + errorMsg);
        }
        _timerGCode->stop();
        _sequencer->rewindProgram();
        _grbl->clearQueue();
        return;
    }

    _grbl->issueCommand(gcode.c_str(), "G-Code");

    if(queue < 3)
        _timerGCode->start(5);
    else
        _timerGCode->start(50);
}


void MainWindow::_initMainControls()
{
    ui->btn_runGCode->setEnabled(false);
    ui->btn_saveGCode->setEnabled(false);
    ui->edit_textGCode->setReadOnly(true);
    ui->label_stateGCode->clear();

    ui->btn_unlock->setEnabled(false);
    ui->btn_reset->setEnabled(false);
    ui->btn_settings->setEnabled(false);
    ui->btn_simulation->setEnabled(false);

    ui->btn_jogLeft->setEnabled(false); // X
    ui->btn_jogRight->setEnabled(false);
    ui->btn_jogForward->setEnabled(false); // Y
    ui->btn_jogBackward->setEnabled(false);
    ui->btn_jogUp->setEnabled(false); // Z
    ui->btn_jogDown->setEnabled(false);

    ui->btn_homing->setEnabled(false);
    ui->btn_setXOrigin->setEnabled(false);
    ui->btn_setYOrigin->setEnabled(false);
    ui->btn_setZOrigin->setEnabled(false);
    ui->btn_setXYZOrigin->setEnabled(false);
}
