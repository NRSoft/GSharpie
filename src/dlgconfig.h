#ifndef DLGCONFIG_H
#define DLGCONFIG_H

#include <QHash>
#include <QString>
#include <QDialog>
#include <QSettings>
#include <QAbstractButton>

#include "grblcontrol.h"

namespace Ui {
class DlgConfig;
}

class DlgConfig : public QDialog
{
    Q_OBJECT
public:
    static const QHash<int, QString> verbosity;

public:
    explicit DlgConfig(GrblControl* grbl, QSettings* ini, QWidget *parent = 0);
    ~DlgConfig();

    inline int verbosityLevel() const {return _verbosityLevel;}
    inline QString verbosityName() const {return verbosity[_verbosityLevel];}

    inline int refreshRate() const {return _refreshRate;}
    inline int seekRate() const {return _seekRate;}
    inline int feedRate() const {return _feedRate;}

private:
    void _disableControls();

private slots:
    void on_slider_verbosity_valueChanged(int value);

    void on_buttonBox_accepted();

    void on_btn_open_clicked();

    void on_btn_save_clicked();

    void on_check_homeEn_stateChanged(int arg1);

private:
    Ui::DlgConfig *ui;
    GrblControl* _grbl;
    QSettings* _ini;

    int _verbosityLevel;

    int _refreshRate;
    int _seekRate;
    int _feedRate;
};

#endif // DLGCONFIG_H
