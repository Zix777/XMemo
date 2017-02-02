#-------------------------------------------------
#
# Project created by QtCreator 2017-01-26T20:05:22
#
#-------------------------------------------------

QT       += core gui
QT       += sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = XMemo
TEMPLATE = app


SOURCES += main.cpp\
        XMemo.cpp \
    Settings.cpp \
    DbOperator.cpp \
    MemoInfo.cpp \
    MemoWidget.cpp

HEADERS  += XMemo.h \
    Settings.h \
    DbOperator.h \
    MemoInfo.h \
    MemoWidget.h

RESOURCES += \
    res.qrc

DISTFILES +=
