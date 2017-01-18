#-------------------------------------------------
#
# Project created by QtCreator 2017-01-17T17:01:52
#
#-------------------------------------------------

QT       += core gui
CONFIG   += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = GSharpie
TEMPLATE = app

INCLUDEPATH += ../../GSharp/include
LIBS += ../../GSharp/lib/libgsharp.a

SOURCES += main.cpp\
        mainwindow.cpp \
    grblcontrol.cpp \
    gcodesequencer.cpp

HEADERS  += mainwindow.h \
    grblcontrol.h \
    gcodesequencer.h

FORMS    += mainwindow.ui
