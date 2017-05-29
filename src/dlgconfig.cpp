//#include <QThread>
#include <QFileDialog>
#include "dlgconfig.h"
#include "ui_dlgconfig.h"

extern int GSharpieReportLevel;

const QHash<int, QString> DlgConfig::verbosity =
    {{-2,"TRACE"}, {-1,"DEBUG"}, {0,"INFO"}, {1,"WARN"}, {2,"ERROR"}};

DlgConfig::DlgConfig(GrblControl* grbl, QSettings* ini, QWidget *parent):
  QDialog(parent), ui(new Ui::DlgConfig),
  _grbl(grbl), _ini(ini)
{
    ui->setupUi(this);

    _ini->beginGroup("CNC_Control");
    _refreshRate = _ini->value("refresh_rate", 5).toInt();
    _seekRate = _ini->value("seek_rate", 500).toInt();
    _feedRate = _ini->value("feed_rate", 100).toInt();
    _ini->endGroup();

    ui->edit_refreshRate->setText(QString::number(_refreshRate));
    ui->edit_seekRate->setText(QString::number(_seekRate));
    ui->edit_workRate->setText(QString::number(_feedRate));

    _verbosityLevel = GSharpieReportLevel;
    ui->slider_verbosity->setValue(-_verbosityLevel);
    ui->label_verbosity->setText(verbosityName());

    if(_grbl->isActive()){
        const GrblControl::Config& conf = _grbl->getConfiguration();

        ui->combo_units->setCurrentIndex(conf.imperial? 1: 0);
        ui->edit_maxTravelX->setText(QString::number(conf.maxTravel[0]));
        ui->edit_maxTravelY->setText(QString::number(conf.maxTravel[1]));
        ui->edit_maxTravelZ->setText(QString::number(conf.maxTravel[2]));
        ui->edit_resolutionX->setText(QString::number(conf.stepsPerMm[0]));
        ui->edit_resolutionY->setText(QString::number(conf.stepsPerMm[1]));
        ui->edit_resolutionZ->setText(QString::number(conf.stepsPerMm[2]));
        ui->edit_maxRateX->setText(QString::number(conf.maxFeedRate[0]));
        ui->edit_maxRateY->setText(QString::number(conf.maxFeedRate[1]));
        ui->edit_maxRateZ->setText(QString::number(conf.maxFeedRate[2]));
        ui->edit_accelerationX->setText(QString::number(conf.acceleration[0]));
        ui->edit_accelerationY->setText(QString::number(conf.acceleration[1]));
        ui->edit_accelerationZ->setText(QString::number(conf.acceleration[2]));
        ui->check_dirInvertX->setChecked((conf.directionInvertMask&0x1) != 0);
        ui->check_dirInvertY->setChecked((conf.directionInvertMask&0x2) != 0);
        ui->check_dirInvertZ->setChecked((conf.directionInvertMask&0x4) != 0);
        ui->edit_junctDeviation->setText(QString::number(conf.junctionDeviation));
        ui->edit_arcTolerance->setText(QString::number(conf.arcTolerance));
        ui->edit_stepPulse->setText(QString::number(conf.stepPulse));
        ui->edit_stepDelay->setText(QString::number(conf.stepIdleDelay));
        ui->check_homeEn->setChecked(conf.homingEnable);
        ui->check_homeInvertX->setChecked((conf.homingDirInvertMask&0x1) != 0);
        ui->check_homeInvertY->setChecked((conf.homingDirInvertMask&0x2) != 0);
        ui->check_homeInvertZ->setChecked((conf.homingDirInvertMask&0x4) != 0);
        ui->edit_homeSeek->setText(QString::number(conf.homingSeek));
        ui->edit_homeFeed->setText(QString::number(conf.homingFeed));
        ui->edit_pullOff->setText(QString::number(conf.homingPullOff));
        ui->edit_debounce->setText(QString::number(conf.homingDebounce));
        ui->check_invertStepEn->setChecked(conf.stepEnableInvert);
        ui->check_invertLimit->setChecked(conf.limitSwitchInvert);
        ui->check_invertProbe->setChecked(conf.probePinInvert);
        ui->edit_minSpindle->setText(QString::number(conf.minSpindleSpeed));
        ui->edit_maxSpindle->setText(QString::number(conf.maxSpindleSpeed));

        QString startup[2];
        _grbl->getStartupBlock(startup[0], 0);
        _grbl->getStartupBlock(startup[1], 1);
        ui->edit_startup0->setText(startup[0]);
        ui->edit_startup1->setText(startup[1]);
    }
    else
        _disableControls();
}


DlgConfig::~DlgConfig()
{
    delete ui;
}


////  s l i d e r  v e r b o s i t y  v a l u e  C h a n g e d  /////
void DlgConfig::on_slider_verbosity_valueChanged(int value)
{
    _verbosityLevel = -value;
    ui->label_verbosity->setText(verbosityName());
}


////////  b t n  s a v e  c l i c k e d  ///////
void DlgConfig::on_btn_save_clicked()
{
    QString name = QFileDialog::getSaveFileName(this, QStringLiteral("Save configuration"), ".",
                        QStringLiteral("Grbl config (*.grbl);;All files (*.*)"));
    if(name.isEmpty()) return;
    if(!name.contains('.'))
        name += QString(".grbl");

    QSettings* settings = new QSettings(name, QSettings::IniFormat);
    settings->beginGroup("Grbl_Config");

    settings->setValue("refresh_rate", ui->edit_refreshRate->text());
    settings->setValue("units", ui->combo_units->currentIndex());
    settings->setValue("max_travel_x", ui->edit_maxTravelX->text());
    settings->setValue("max_travel_y", ui->edit_maxTravelY->text());
    settings->setValue("max_travel_z", ui->edit_maxTravelZ->text());
    settings->setValue("resolution_x", ui->edit_resolutionX->text());
    settings->setValue("resolution_y", ui->edit_resolutionY->text());
    settings->setValue("resolution_z", ui->edit_resolutionZ->text());
    settings->setValue("max_rate_x", ui->edit_maxRateX->text());
    settings->setValue("max_rate_y", ui->edit_maxRateY->text());
    settings->setValue("max_rate_z", ui->edit_maxRateZ->text());
    settings->setValue("acceleration_x", ui->edit_accelerationX->text());
    settings->setValue("acceleration_y", ui->edit_accelerationY->text());
    settings->setValue("acceleration_z", ui->edit_accelerationZ->text());
    settings->setValue("dir_invert_x", ui->check_dirInvertX->isChecked()? "1": "0");
    settings->setValue("dir_invert_y", ui->check_dirInvertY->isChecked()? "1": "0");
    settings->setValue("dir_invert_z", ui->check_dirInvertZ->isChecked()? "1": "0");
    settings->setValue("junction_deviation", ui->edit_junctDeviation->text());
    settings->setValue("arc_tolerance", ui->edit_arcTolerance->text());
    settings->setValue("step_pulse", ui->edit_stepPulse->text());
    settings->setValue("step_delay", ui->edit_stepDelay->text());
    settings->setValue("homing_enabled", ui->check_homeEn->isChecked()? "1": "0");
    settings->setValue("homing_invert_x", ui->check_homeInvertX->isChecked()? "1": "0");
    settings->setValue("homing_invert_y", ui->check_homeInvertY->isChecked()? "1": "0");
    settings->setValue("homing_invert_z", ui->check_homeInvertZ->isChecked()? "1": "0");
    settings->setValue("homing_seekrate", ui->edit_homeSeek->text());
    settings->setValue("homing_feedrate", ui->edit_homeFeed->text());
    settings->setValue("homing_pull_off", ui->edit_pullOff->text());
    settings->setValue("homing_debounce", ui->edit_debounce->text());
    settings->setValue("step_enable_invert", ui->check_invertStepEn->isChecked()? "1": "0");
    settings->setValue("limit_switch_invert", ui->check_invertLimit->isChecked()? "1": "0");
    settings->setValue("probe_invert", ui->check_invertProbe->isChecked()? "1": "0");
    settings->setValue("spindle_speed_min", ui->edit_minSpindle->text());
    settings->setValue("spindle_speed_max", ui->edit_maxSpindle->text());
    settings->setValue("startup_block0", ui->edit_startup0->text());
    settings->setValue("startup_block1", ui->edit_startup1->text());
    settings->setValue("seek_rate", ui->edit_seekRate->text());
    settings->setValue("feed_rate", ui->edit_workRate->text());

    settings->endGroup();
}


////////  b t n  o p e n  c l i c k e d  ///////
void DlgConfig::on_btn_open_clicked()
{
    QString name = QFileDialog::getOpenFileName(this, QStringLiteral("Open configuration"), ".",
                        QStringLiteral("Grbl config (*.grbl);;All files (*.*)"));
    if(name.isEmpty()) return;
    if(!name.contains('.'))
        name += QString(".grbl");

    QSettings* settings = new QSettings(name, QSettings::IniFormat);

    settings->beginGroup("Grbl_Config");

    if(settings->contains("refresh_rate"))
        ui->edit_refreshRate->setText(settings->value("refresh_rate").toString());
    if(settings->contains("units"))
        ui->combo_units->setCurrentIndex(settings->value("units").toInt());
    if(settings->contains("max_travel_x"))
        ui->edit_maxTravelX->setText(settings->value("max_travel_x").toString());
    if(settings->contains("max_travel_y"))
        ui->edit_maxTravelY->setText(settings->value("max_travel_y").toString());
    if(settings->contains("max_travel_z"))
        ui->edit_maxTravelZ->setText(settings->value("max_travel_z").toString());
    if(settings->contains("resolution_x"))
        ui->edit_resolutionX->setText(settings->value("resolution_x").toString());
    if(settings->contains("resolution_y"))
        ui->edit_resolutionY->setText(settings->value("resolution_y").toString());
    if(settings->contains("resolution_z"))
        ui->edit_resolutionZ->setText(settings->value("resolution_z").toString());
    if(settings->contains("max_rate_x"))
        ui->edit_maxRateX->setText(settings->value("max_rate_x").toString());
    if(settings->contains("max_rate_y"))
        ui->edit_maxRateY->setText(settings->value("max_rate_y").toString());
    if(settings->contains("max_rate_z"))
        ui->edit_maxRateZ->setText(settings->value("max_rate_z").toString());
    if(settings->contains("acceleration_x"))
        ui->edit_accelerationX->setText(settings->value("acceleration_x").toString());
    if(settings->contains("acceleration_y"))
        ui->edit_accelerationY->setText(settings->value("acceleration_y").toString());
    if(settings->contains("acceleration_z"))
        ui->edit_accelerationZ->setText(settings->value("acceleration_z").toString());
    if(settings->contains("dir_invert_x"))
        ui->check_dirInvertX->setChecked(settings->value("dir_invert_x").toBool());
    if(settings->contains("dir_invert_y"))
        ui->check_dirInvertY->setChecked(settings->value("dir_invert_y").toBool());
    if(settings->contains("dir_invert_z"))
        ui->check_dirInvertZ->setChecked(settings->value("dir_invert_z").toBool());
    if(settings->contains("junction_deviation"))
        ui->edit_junctDeviation->setText(settings->value("junction_deviation").toString());
    if(settings->contains("arc_tolerance"))
        ui->edit_arcTolerance->setText(settings->value("arc_tolerance").toString());
    if(settings->contains("step_pulse"))
        ui->edit_stepPulse->setText(settings->value("step_pulse").toString());
    if(settings->contains("step_delay"))
        ui->edit_stepDelay->setText(settings->value("step_delay").toString());
    if(settings->contains("homing_enabled"))
        ui->check_homeEn->setChecked(settings->value("homing_enabled").toBool());
    if(settings->contains("homing_invert_x"))
        ui->check_homeInvertX->setChecked(settings->value("homing_invert_x").toBool());
    if(settings->contains("homing_invert_y"))
        ui->check_homeInvertY->setChecked(settings->value("homing_invert_y").toBool());
    if(settings->contains("homing_invert_z"))
        ui->check_homeInvertZ->setChecked(settings->value("homing_invert_z").toBool());
    if(settings->contains("homing_seekrate"))
        ui->edit_homeSeek->setText(settings->value("homing_seekrate").toString());
    if(settings->contains("homing_feedrate"))
        ui->edit_homeFeed->setText(settings->value("homing_feedrate").toString());
    if(settings->contains("homing_pull_off"))
        ui->edit_pullOff->setText(settings->value("homing_pull_off").toString());
    if(settings->contains("homing_debounce"))
        ui->edit_debounce->setText(settings->value("homing_debounce").toString());
    if(settings->contains("step_enable_invert"))
        ui->check_invertStepEn->setChecked(settings->value("step_enable_invert").toBool());
    if(settings->contains("limit_switch_invert"))
        ui->check_invertLimit->setChecked(settings->value("limit_switch_invert").toBool());
    if(settings->contains("probe_invert"))
        ui->check_invertProbe->setChecked(settings->value("probe_invert").toBool());
    if(settings->contains("spindle_speed_min"))
        ui->edit_minSpindle->setText(settings->value("spindle_speed_min").toString());
    if(settings->contains("spindle_speed_max"))
        ui->edit_maxSpindle->setText(settings->value("spindle_speed_max").toString());
    if(settings->contains("startup_block0"))
        ui->edit_startup0->setText(settings->value("startup_block0").toString());
    if(settings->contains("startup_block1"))
        ui->edit_startup1->setText(settings->value("startup_block1").toString());
    if(settings->contains("seek_rate"))
        ui->edit_seekRate->setText(settings->value("seek_rate").toString());
    if(settings->contains("feed_rate"))
        ui->edit_workRate->setText(settings->value("feed_rate").toString());

    settings->endGroup();
}


////////  b u t t o n  B o x  a c c e p t e d  ///////
void DlgConfig::on_buttonBox_accepted()
{
    _refreshRate = ui->edit_refreshRate->text().toInt();
    _seekRate = ui->edit_seekRate->text().toInt();
    _feedRate = ui->edit_workRate->text().toInt();

    _ini->beginGroup("CNC_Control");
    _ini->setValue("refresh_rate", _refreshRate);
    _ini->setValue("seek_rate", _seekRate);
    _ini->setValue("feed_rate", _feedRate);
    _ini->endGroup();

    if(_grbl->isActive()){
        GrblControl::Config conf;
        conf.imperial = ui->combo_units->currentIndex() != 0;
        conf.stepsPerMm[0] = ui->edit_resolutionX->text().toDouble();
        conf.stepsPerMm[1] = ui->edit_resolutionY->text().toDouble();
        conf.stepsPerMm[2] = ui->edit_resolutionZ->text().toDouble();
        conf.maxTravel[0] = ui->edit_maxTravelX->text().toDouble();
        conf.maxTravel[1] = ui->edit_maxTravelY->text().toDouble();
        conf.maxTravel[2] = ui->edit_maxTravelZ->text().toDouble();
        conf.maxFeedRate[0] = ui->edit_maxRateX->text().toDouble();
        conf.maxFeedRate[1] = ui->edit_maxRateY->text().toDouble();
        conf.maxFeedRate[2] = ui->edit_maxRateZ->text().toDouble();
        conf.acceleration[0] = ui->edit_accelerationX->text().toDouble();
        conf.acceleration[1] = ui->edit_accelerationY->text().toDouble();
        conf.acceleration[2] = ui->edit_accelerationZ->text().toDouble();
        conf.directionInvertMask = ui->check_dirInvertX->isChecked()? 1: 0;
        conf.directionInvertMask |= ui->check_dirInvertY->isChecked()? 2: 0;
        conf.directionInvertMask |= ui->check_dirInvertZ->isChecked()? 4: 0;
        conf.homingDirInvertMask = ui->check_homeInvertX->isChecked()? 1: 0;
        conf.homingDirInvertMask |= ui->check_homeInvertY->isChecked()? 2: 0;
        conf.homingDirInvertMask |= ui->check_homeInvertZ->isChecked()? 4: 0;
        conf.homingEnable = ui->check_homeEn->isChecked();
        conf.homingSeek = ui->edit_homeSeek->text().toDouble();
        conf.homingFeed = ui->edit_homeFeed->text().toDouble();
        conf.homingPullOff = ui->edit_pullOff->text().toDouble();
        conf.homingDebounce = ui->edit_debounce->text().toInt();
        conf.stepEnableInvert = ui->check_invertStepEn->isChecked();
        conf.limitSwitchInvert = ui->check_invertLimit->isChecked();
        conf.probePinInvert = ui->check_invertProbe->isChecked();
        conf.junctionDeviation = ui->edit_junctDeviation->text().toDouble();
        conf.arcTolerance = ui->edit_arcTolerance->text().toDouble();
        conf.stepPulse = ui->edit_stepPulse->text().toDouble();
        conf.stepIdleDelay = ui->edit_stepDelay->text().toDouble();
        conf.minSpindleSpeed = ui->edit_minSpindle->text().toDouble();
        conf.maxSpindleSpeed = ui->edit_maxSpindle->text().toDouble();

        _grbl->updateConfiguration(conf);

        _grbl->updateStartupBlock(ui->edit_startup0->text().toLatin1().constData(), 0);
        _grbl->updateStartupBlock(ui->edit_startup1->text().toLatin1().constData(), 1);

        _grbl->setSeekRate(ui->edit_seekRate->text().toInt());
        _grbl->setFeedRate(ui->edit_workRate->text().toInt());
    }
}


/////  d i s a b l e  C o n t r o l s  //////
void DlgConfig::_disableControls()
{
    ui->combo_units->setEnabled(false);

    ui->edit_maxTravelX->setEnabled(false);
    ui->edit_maxTravelY->setEnabled(false);
    ui->edit_maxTravelZ->setEnabled(false);

    ui->edit_resolutionX->setEnabled(false);
    ui->edit_resolutionY->setEnabled(false);
    ui->edit_resolutionZ->setEnabled(false);

    ui->edit_maxRateX->setEnabled(false);
    ui->edit_maxRateY->setEnabled(false);
    ui->edit_maxRateZ->setEnabled(false);

    ui->edit_accelerationX->setEnabled(false);
    ui->edit_accelerationY->setEnabled(false);
    ui->edit_accelerationZ->setEnabled(false);

    ui->check_dirInvertX->setEnabled(false);
    ui->check_dirInvertY->setEnabled(false);
    ui->check_dirInvertZ->setEnabled(false);

    ui->edit_junctDeviation->setEnabled(false);
    ui->edit_arcTolerance->setEnabled(false);

    ui->edit_stepPulse->setEnabled(false);
    ui->edit_stepDelay->setEnabled(false);

    ui->check_homeEn->setEnabled(false);
    ui->check_homeInvertX->setEnabled(false);
    ui->check_homeInvertY->setEnabled(false);
    ui->check_homeInvertZ->setEnabled(false);

    ui->edit_homeSeek->setEnabled(false);
    ui->edit_homeFeed->setEnabled(false);
    ui->edit_pullOff->setEnabled(false);
    ui->edit_debounce->setEnabled(false);

    ui->check_invertStepEn->setEnabled(false);
    ui->check_invertLimit->setEnabled(false);
    ui->check_invertProbe->setEnabled(false);

    ui->edit_minSpindle->setEnabled(false);
    ui->edit_maxSpindle->setEnabled(false);

    ui->edit_startup0->setEnabled(false);
    ui->edit_startup1->setEnabled(false);

    ui->edit_seekRate->setEnabled(false);
    ui->edit_workRate->setEnabled(false);
}

void DlgConfig::on_check_homeEn_stateChanged(int arg1)
{
    ui->check_homeInvertX->setEnabled(arg1 == Qt::Checked);
    ui->check_homeInvertY->setEnabled(arg1 == Qt::Checked);
    ui->check_homeInvertZ->setEnabled(arg1 == Qt::Checked);

    ui->edit_homeSeek->setEnabled(arg1 == Qt::Checked);
    ui->edit_homeFeed->setEnabled(arg1 == Qt::Checked);
    ui->edit_pullOff->setEnabled(arg1 == Qt::Checked);
    ui->edit_debounce->setEnabled(arg1 == Qt::Checked);
}
