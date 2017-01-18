#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QErrorMessage>
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    _sequencer = new GCodeSequencer();
    connect(_sequencer, SIGNAL(finished()), this, SLOT(on_sequencerFinished()));
    connect(_sequencer, SIGNAL(currentStep(int)), this, SLOT(on_sequencerStep(int)));
}


MainWindow::~MainWindow()
{
    delete _sequencer;
    delete ui;
}


void MainWindow::on_btnLoad_clicked()
{
    QString name = QFileDialog::getOpenFileName(this, QStringLiteral("Open file"), ".",
                        QStringLiteral("G-sharp (*.ngs);;G-code (*.nc);;All files (*.*)"));
    if(name.isEmpty()) return;

    QFile file(name);
    if(!file.open(QFile::ReadOnly | QFile::Text)){
        QErrorMessage e; e.showMessage(QString("Cannot open file ") + name);
        return;
    }

    _sequencer->loadProgram(file.readAll());
}


void MainWindow::on_btnStart_clicked()
{
    if(_sequencer->isRunning())
        _sequencer->requestInterruption();
    else{
        qDebug()<<"starting";
        _sequencer->start();
        qDebug()<<"started";

        ui->btnStart->setText(QStringLiteral("Stop"));
    }
}


void MainWindow::on_sequencerFinished()
{
    ui->btnStart->setText(QStringLiteral("Start"));
    qDebug() << "finished";
}


void MainWindow::on_sequencerStep(int lineNumber)
{
    qDebug() << "line" << lineNumber;
}
