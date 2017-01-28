#include <QString>
#include <QSerialPortInfo>
#include "dlgserialport.h"
#include "ui_dlgserialport.h"

DlgSerialPort::DlgSerialPort(GrblControl* grbl, QSettings* ini, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgSerialPort),
    _grbl(grbl),
    _ini(ini)
{
    ui->setupUi(this);

    _ini->beginGroup("Serial_Port");

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    foreach(const QSerialPortInfo& port, ports){
       ui->comboPort->addItem(port.portName());
       if(_ini->value("port_name").toString() == port.portName())
           ui->comboPort->setCurrentText(port.portName());
    }

    QList<int> baudrates = QSerialPortInfo::standardBaudRates();
    foreach(int baudrate, baudrates){
        ui->comboBaudRate->addItem(QString::number(baudrate));
        if(_ini->value("baud_rate", 115200).toInt() == baudrate)
            ui->comboBaudRate->setCurrentText(QString::number(baudrate));
    }

    ui->checkStartup->setChecked(_ini->value("open_on_startup", true).toBool());

    _ini->endGroup();

    _ReflectStatus();
}

DlgSerialPort::~DlgSerialPort()
{
    delete ui;
}


void DlgSerialPort::_ReflectStatus()
{
    ui->txtStatus->setText(_grbl->isOpened()? "opened": "closed");
    ui->btnOpen->setText(_grbl->isOpened()? "Close": "Open");
    ui->comboPort->setEnabled(!_grbl->isOpened());
    ui->comboBaudRate->setEnabled(!_grbl->isOpened());
}


void DlgSerialPort::on_btnOpen_clicked()
{
    if(_grbl->isOpened())
        _grbl->closeSerialPort();
    else{
        if(_grbl->openSerialPort(ui->comboPort->currentText(),
                                 ui->comboBaudRate->currentText().toInt()))
            _grbl->issueReset();
    }

    _ReflectStatus();
}


void DlgSerialPort::on_buttonBox_accepted()
{
    _ini->beginGroup("Serial_Port");

    _ini->setValue("port_name", ui->comboPort->currentText());
    _ini->setValue("baud_rate", ui->comboBaudRate->currentText());
    _ini->setValue("open_on_startup", ui->checkStartup->isChecked());

    _ini->endGroup();
}
