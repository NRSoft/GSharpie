#include "dlgconfig.h"
#include "ui_dlgconfig.h"

DlgConfig::DlgConfig(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgConfig)
{
    ui->setupUi(this);
}

DlgConfig::~DlgConfig()
{
    delete ui;
}
