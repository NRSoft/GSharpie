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

    on_errorReport(0, QString("Program started on ") + QDate::currentDate().toString());
//    GSharpieReportLevel = -2;

    _grbl = new GrblControl();
    connect(_grbl, SIGNAL(report(int, QString)), this, SLOT(on_errorReport(int, QString)));
    connect(_grbl, SIGNAL(commandComplete(GrblCommand)), this, SLOT(on_GrblResponse(GrblCommand)));

    if(_grbl->connectSerialPort(QStringLiteral("ttyUSB0")))
        _grbl->issueReset();

    _sequencer = new GCodeSequencer();
    connect(_sequencer, SIGNAL(report(int, QString)), this, SLOT(on_errorReport(int, QString)));
    _sequencer->setGrblControl(_grbl);

    _timer = new QTimer(this);
    connect(_timer, SIGNAL(timeout()), this, SLOT(updateStatus()));
    _timer->start(200);
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

    _sequencer->loadProgram(file.readAll());
}


void MainWindow::on_btnStart_clicked()
{
    if(!_grbl->isRunning())
        return;
}


void MainWindow::on_btnKillAlarm_clicked()
{
    if(!_grbl->isRunning())
        return;
    _issueCommand("$X\n", "Alarm_Unlock");
}


void MainWindow::on_btnCheckMode_clicked()
{
    if(!_grbl->isRunning())
        return;
    _issueCommand("$C\n", "Check_Mode");
}


void MainWindow::on_btnSetOrigin_clicked()
{
    if(!_grbl->isRunning())
        return;
    _issueCommand("G10P1L20\n", "Set_Origin");
}

void MainWindow::on_btnHoming_clicked()
{
    if(!_grbl->isRunning())
        return;
    _timer->stop(); // status is not reported during homing
    _issueCommand("$H\n", "Homing");
    ui->label_Status->setText("homing");
    _restartTimer = true;
}


void MainWindow::on_GrblResponse(GrblCommand cmd)
{
    for(int i=0; i < cmd.response.size(); ++i)
        on_errorReport(0, cmd.name + QStringLiteral(": ") + cmd.response.at(i));
    if(!cmd.error.isEmpty())
        on_errorReport(1, cmd.name + QStringLiteral(": ") + cmd.error);

    if(_restartTimer){
        _restartTimer = false;
        _timer->start(200);
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


void MainWindow::updateStatus()
{
    CncPosition pos;
    GrblControl::STATUS status = _grbl->getCurrentStatus(pos);

    ui->label_Status->setText(status==GrblControl::Run?   QStringLiteral("run"):
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

    ui->btnKillAlarm->setEnabled(status==GrblControl::Alarm);
    ui->btnSetOrigin->setEnabled(status==GrblControl::Idle);
    ui->btnHoming->setEnabled(status==GrblControl::Idle);
    ui->btnCheckMode->setEnabled(status==GrblControl::Idle || status==GrblControl::Check);
    ui->btnCheckMode->setText(status==GrblControl::Check? QStringLiteral("Operation"):
                                                          QStringLiteral("Simulation"));

    _grbl->issueStatusRequest();
}


quint32 MainWindow::_issueCommand(const char* code, const QString& name)
{
    quint32 id = _grbl->issueCommand(code, name);
    if(id == 0)
        on_errorReport(1, QString("Cannot issue ") + name);
    else if(GSharpieReportLevel < 0)
        on_errorReport(-1, QString("Issued ") + name + QStringLiteral(" (") +
                           QString::number(id) + QStringLiteral(")"));
    return id;
}
