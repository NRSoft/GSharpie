#include "mainwindow.h"
#include "ui_mainwindow.h"


//////  i n i t  J o g g i n g  C o n t r o l s  /////
void MainWindow::_initJoggingControls()
{
    _isJogging = false;

    ui->dial_jogFeed->setWrapping(false);
    ui->dial_jogFeed->setRange(0, 26);
    _setJogRate(_grbl->getSeekRate());
}


///////  p r e p a r e  J o g  C o m m a n d  //////
void MainWindow::_prepareJogCommand(char axis, double limit)
{
    ::sprintf(_jogCommand, "$J=G90G21F%d%c%.3f", _jogRate, axis, limit);
}


//////  c a n c e l  J o g g i n g  //////
void MainWindow::_cancelJogging()
{
    if(!_isJogging){
//        on_errorReport(1, QString("Attempt to cancel jogging on idle"));
        return;
    }
    _isJogging = false;
    _grbl->issueRealtimeCommand(GrblControl::CANCEL_JOGGING);
}


/////  s e t  J o g  R a t e  ////
void MainWindow::_setJogRate(int value)
{
    int pos, rate;
    for(pos = 0; pos <= 26; ++pos){
        rate = _pos2Rate(pos);
        if(rate >= value)
            break;
    }
    ui->dial_jogFeed->setSliderPosition(pos);
}


//////  p o s  2  R a t e  //////
int MainWindow::_pos2Rate(int pos)
{
    if(pos <= 0) // limit min to 30ms max step period? (grbl)
        return 20; // min rate
    if(pos < 9)
        return 10*(pos+2); // 20 to 100
    if(pos < 18)
        return 100*(pos-7); // 200 to 1000
    if(pos < 26)
        return 1000 + 100*(pos-16); // 1200 to 2000
    return 2000; // max rate
}


/////  j o g  F e e d  v a l u e  C h a n g e d  ////
void MainWindow::on_dial_jogFeed_valueChanged(int value)
{
    _jogRate = _pos2Rate(value);
    ui->label_jogFeed->setText(QString::number(_jogRate));
}


/////  j o g  R i g h t  p r e s s e d  /////
void MainWindow::on_btn_jogRight_pressed()
{
    if(_grbl->getCurrentStatus().state == GrblControl::Idle){
        _prepareJogCommand('X', ui->check_obeyLimits->isChecked()? ui->spin_maxX->value(): 9999.);
        _grbl->issueCommand(_jogCommand, "Jog right");
        _isJogging = true;
    }
}


/////  j o g  R i g h t  r e l e a s e d  /////
void MainWindow::on_btn_jogRight_released()
{
    _cancelJogging();
}


/////  j o g  L e f t  p r e s s e d  /////
void MainWindow::on_btn_jogLeft_pressed()
{
    if(_grbl->getCurrentStatus().state == GrblControl::Idle){
        _prepareJogCommand('X', ui->check_obeyLimits->isChecked()? ui->spin_minX->value(): -9999.);
        _grbl->issueCommand(_jogCommand, "Jog left");
        _isJogging = true;
    }
}


/////  j o g  L e f t  r e l e a s e d  /////
void MainWindow::on_btn_jogLeft_released()
{
    _cancelJogging();
}


/////  j o g  F o r w a r d  p r e s s e d  /////
void MainWindow::on_btn_jogForward_pressed()
{
    if(_grbl->getCurrentStatus().state == GrblControl::Idle){
        _prepareJogCommand('Y', ui->check_obeyLimits->isChecked()? ui->spin_maxY->value(): 9999.);
        _grbl->issueCommand(_jogCommand, "Jog forward");
        _isJogging = true;
    }
}


/////  j o g  F o r w a r d  r e l e a s e d  /////
void MainWindow::on_btn_jogForward_released()
{
    _cancelJogging();
}


/////  j o g  B a c k w a r d  p r e s s e d  /////
void MainWindow::on_btn_jogBackward_pressed()
{
    if(_grbl->getCurrentStatus().state == GrblControl::Idle){
        _prepareJogCommand('Y', ui->check_obeyLimits->isChecked()? ui->spin_minY->value(): -9999.);
        _grbl->issueCommand(_jogCommand, "Jog backward");
        _isJogging = true;
    }
}


/////  j o g  B a c k w a r d  r e l e a s e d  /////
void MainWindow::on_btn_jogBackward_released()
{
    _cancelJogging();
}


/////  j o g  U p  p r e s s e d  /////
void MainWindow::on_btn_jogUp_pressed()
{
    if(_grbl->getCurrentStatus().state == GrblControl::Idle){
        _prepareJogCommand('Z', ui->check_obeyLimits->isChecked()? ui->spin_maxZ->value(): 9999.);
        _grbl->issueCommand(_jogCommand, "Jog up");
        _isJogging = true;
    }
}


/////  j o g  U p  r e l e a s e d  /////
void MainWindow::on_btn_jogUp_released()
{
    _cancelJogging();
}


/////  j o g  D o w n  p r e s s e d  /////
void MainWindow::on_btn_jogDown_pressed()
{
    if(_grbl->getCurrentStatus().state == GrblControl::Idle){
        _prepareJogCommand('Z', ui->check_obeyLimits->isChecked()? ui->spin_minZ->value(): -9999.);
        _grbl->issueCommand(_jogCommand, "Jog down");
        _isJogging = true;
    }
}


/////  j o g  D o w n  r e l e a s e d  /////
void MainWindow::on_btn_jogDown_released()
{
    _cancelJogging();
}
