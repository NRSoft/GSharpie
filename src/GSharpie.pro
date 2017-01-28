#-------------------------------------------------
#
# Project created by QtCreator 2017-01-17T17:01:52
#
#-------------------------------------------------

QT       += core gui widgets serialport
CONFIG   += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GSharpie
TEMPLATE = app

INCLUDEPATH += ../../GSharp/include
LIBS += ../../GSharp/lib/libgsharp.a

SOURCES += main.cpp\
        mainwindow.cpp \
    grblcontrol.cpp \
    gcodesequencer.cpp \
    gcodeeditor.cpp \
    dlgserialport.cpp

HEADERS  += mainwindow.h \
    grblcontrol.h \
    gcodesequencer.h \
    gcodeeditor.h \
    dlgserialport.h

FORMS    += mainwindow.ui \
    dlgserialport.ui

RESOURCES += \
    gsharpie.qrc
