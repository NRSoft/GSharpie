#ifndef GSHARPIE_DLGSERIALPORT_H
#define GSHARPIE_DLGSERIALPORT_H

#include <QDialog>
#include <QSettings>
#include "grblcontrol.h"

namespace Ui {
class DlgSerialPort;
}

class DlgSerialPort : public QDialog
{
    Q_OBJECT

public:
    explicit DlgSerialPort(GrblControl* grbl, QSettings* ini, QWidget *parent = 0);
    ~DlgSerialPort();

private slots:
    void on_btnOpen_clicked();
    void on_buttonBox_accepted();

private:
    void _ReflectStatus();

private:
    Ui::DlgSerialPort *ui;

    GrblControl* _grbl;
    QSettings* _ini;
};

#endif // GSHARPIE_DLGSERIALPORT_H
