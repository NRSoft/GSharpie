#include <QDebug>
#include <QTime>
#include <QFile>
#include <QFileDialog>
#include "mainwindow.h"
#include "ui_mainwindow.h"

int GSharpieReportLevel = 0; // global


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->txtErrorLog->clear();
    ui->btnStart->setEnabled(false);
    ui->comboJogDist->setCurrentIndex(3);
    ui->editGCode->setReadOnly(true);

    on_errorReport(0, QString("Program started on ") + QDate::currentDate().toString());
//    GSharpieReportLevel = -1;

    _grbl = new GrblControl();
    connect(_grbl, SIGNAL(report(int, QString)), this, SLOT(on_errorReport(int, QString)));
    connect(_grbl, SIGNAL(commandComplete(GrblCommand)), this, SLOT(on_GrblResponse(GrblCommand)));

    if(_grbl->connectSerialPort(QStringLiteral("ttyUSB0")))
        _grbl->issueReset();

    _sequencer = new GCodeSequencer();
    _sequencer->setGrblControl(_grbl);

    _timerGCode = new QTimer(this);
    connect(_timerGCode, SIGNAL(timeout()), this, SLOT(_sendGCode()));

    _timerStatus = new QTimer(this);
    connect(_timerStatus, SIGNAL(timeout()), this, SLOT(_updateStatus()));
    _timerStatus->start(200);
    _restartTimer = false;
}


MainWindow::~MainWindow()
{
    delete _sequencer;
    delete _grbl;
    delete ui;
}


void MainWindow::on_btnLoad_clicked()
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

    QString errorMsg;
    QString program(file.readAll());
    if(!_sequencer->loadProgram(program, &errorMsg))
        on_errorReport(1, QString("Parsing g-code line ") + errorMsg);
    ui->btnStart->setEnabled(_grbl->isActive() && _sequencer->isReady());

    ui->editGCode->setPlainText(program);
}


void MainWindow::on_btnStart_clicked()
{
    on_errorReport(0, QString("Program started"));
    _timerGCode->start(5);
}


void MainWindow::on_btnReset_clicked()
{
    if(_grbl->isActive())
        _grbl->issueReset();
}


void MainWindow::on_btnUnlock_clicked()
{
    if(!_grbl->isActive())
        return;
    _issueCommand("$X", "Unlock_Alarm");
}


void MainWindow::on_btnSimulation_clicked()
{
    if(!_grbl->isActive())
        return;
    _issueCommand("$C", "Check_Mode");
}


void MainWindow::on_btnSetOrigin_clicked()
{
    if(!_grbl->isActive())
        return;
    _issueCommand("G10P1L20X0Y0Z0", "Set_Origin");
}

void MainWindow::on_btnHoming_clicked()
{
    if(!_grbl->isActive())
        return;
    _timerStatus->stop(); // status is not reported during homing
    _issueCommand("$H", "Homing");
    ui->label_Status->setText("homing");
    _restartTimer = true;
}


void MainWindow::on_btnJogR_clicked()
{
    if(!_grbl->isActive())
        return;

    QString cmd("G91X");
    cmd += ui->comboJogDist->currentText();
    _issueCommand(cmd.toLatin1().constData(), "Jog_R");
    _issueCommand("G90", "Abs_Pos");
}

void MainWindow::on_btnJogL_clicked()
{
    if(!_grbl->isActive())
        return;

    QString cmd("G91X-");
    cmd += ui->comboJogDist->currentText();
    _issueCommand(cmd.toLatin1().constData(), "Jog_L");
    _issueCommand("G90", "Abs_Pos");
}


void MainWindow::on_btnJogF_clicked()
{
    if(!_grbl->isActive())
        return;

    QString cmd("G91Y");
    cmd += ui->comboJogDist->currentText();
    _issueCommand(cmd.toLatin1().constData(), "Jog_F");
    _issueCommand("G90", "Abs_Pos");
}


void MainWindow::on_btnJogB_clicked()
{
    if(!_grbl->isActive())
        return;

    QString cmd("G91Y-");
    cmd += ui->comboJogDist->currentText();
    _issueCommand(cmd.toLatin1().constData(), "Jog_B");
    _issueCommand("G90", "Abs_Pos");
}


void MainWindow::on_btnJogU_clicked()
{
    if(!_grbl->isActive())
        return;

    QString cmd("G91Z");
    cmd += ui->comboJogDist->currentText();
    _issueCommand(cmd.toLatin1().constData(), "Jog_U");
    _issueCommand("G90", "Abs_Pos");
}


void MainWindow::on_btnJogD_clicked()
{
    if(!_grbl->isActive())
        return;

    QString cmd("G91Z-");
    cmd += ui->comboJogDist->currentText();
    _issueCommand(cmd.toLatin1().constData(), "Jog_D");
    _issueCommand("G90", "Abs_Pos");
}


void MainWindow::on_GrblResponse(GrblCommand cmd)
{
    for(int i=0; i < cmd.response.size(); ++i)
        on_errorReport(0, cmd.name + QStringLiteral(": ") + cmd.response.at(i));

    if(cmd.error.isEmpty())
        on_errorReport(-1, QString("Confirmed [") + QString::number(cmd.id) + QStringLiteral("]: ok"));
    else
        on_errorReport(1, cmd.name + QStringLiteral(": ") + cmd.error);

    if(_restartTimer){
        _restartTimer = false;
        _timerStatus->start(200);
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

    ui->txtErrorLog->appendPlainText(prefix + msg);
}


void MainWindow::_updateStatus()
{
    CncPosition pos;
    GrblControl::STATUS status = _grbl->getCurrentStatus(pos);

    ui->label_Status->setText(status==GrblControl::Run?   QStringLiteral("running"):
                              status==GrblControl::Home?  QStringLiteral("homing"):
                              status==GrblControl::Check? QStringLiteral("simulation"):
                              status==GrblControl::Idle?  QStringLiteral("idle"):
                              status==GrblControl::Alarm? QStringLiteral("alarm lock"):
                              status==GrblControl::Hold?  QStringLiteral("holding"):
                              status==GrblControl::Door?  QStringLiteral("door open"):
                                                          QStringLiteral("disconnected"));

    ui->label_WPosX->setText(QString::number(pos.wx, 'f', 3));
    ui->label_WPosY->setText(QString::number(pos.wy, 'f', 3));
    ui->label_WPosZ->setText(QString::number(pos.wz, 'f', 3));

    ui->label_MPosX->setText(QString::number(pos.mx, 'f', 3));
    ui->label_MPosY->setText(QString::number(pos.my, 'f', 3));
    ui->label_MPosZ->setText(QString::number(pos.mz, 'f', 3));

    ui->btnJogL->setEnabled(status==GrblControl::Idle); // X
    ui->btnJogR->setEnabled(status==GrblControl::Idle);
    ui->btnJogF->setEnabled(status==GrblControl::Idle); // Y
    ui->btnJogB->setEnabled(status==GrblControl::Idle);
    ui->btnJogU->setEnabled(status==GrblControl::Idle); // Z
    ui->btnJogD->setEnabled(status==GrblControl::Idle);

    ui->comboJogDist->setEnabled(status==GrblControl::Idle);

    ui->btnSetOrigin->setEnabled(status==GrblControl::Idle);
    ui->btnHoming->setEnabled(status==GrblControl::Idle);

    ui->btnSimulation->setEnabled(status==GrblControl::Idle || status==GrblControl::Check);
    ui->btnSimulation->setText(status==GrblControl::Check? QStringLiteral("Operation"):
                                                          QStringLiteral("Simulation"));

    ui->btnUnlock->setEnabled(status==GrblControl::Alarm);

    if(_grbl->isActive())
        _grbl->issueStatusRequest();
}


void MainWindow::_sendGCode()
{
    if(!_grbl->isActive() || !_sequencer->isReady())
        return;

    int queue = _grbl->getQueueSize();
    if(queue >= 5)
        return; // already too many commands

    int lineNum;
    std::string gcode;
    if(!_sequencer->nextLine(lineNum, gcode)){
        on_errorReport(0, QString("Program finished"));
        _timerGCode->stop();
        _sequencer->rewindProgram();
        return;
    }

    _issueCommand(gcode.c_str(), "G-Code");

    if(queue < 3)
        _timerGCode->start(5);
    else
        _timerGCode->start(50);
}


quint32 MainWindow::_issueCommand(const char* code, const QString& name)
{
    static char data[128];
    ::strncpy(data, code, 120);
    ::strcat(data, "\n");

    quint32 id = _grbl->issueCommand(data, name);
    if(id == 0)
        on_errorReport(1, QString("Cannot issue ") + name);
    else if(GSharpieReportLevel < 0)
        on_errorReport(-1, QString("Issued [") + QString::number(id) + QStringLiteral("] ") +
                           name + QStringLiteral(" (") + QString(code) + QStringLiteral(")"));

    return id;
}
