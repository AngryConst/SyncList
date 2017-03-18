#-------------------------------------------------
#
# Project created by QtCreator 2016-06-24T12:41:05
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++14

TARGET = SyncList
TEMPLATE = app


SOURCES += src/main.cpp\
        src/mainwindow.cpp \
    src/tcoresync.cpp \
    src/tfileinfo.cpp \
    src/tlog.cpp \
    src/logcategories.cpp \
    src/tsettingswindow.cpp

HEADERS  += src/mainwindow.h \
    src/tcoresync.h \
    src/tfileinfo.h \
    src/tlog.h \
    src/logcategories.h \
    src/tsingleton.h \
    src/tsettingswindow.h

FORMS    += src/mainwindow.ui \
    src/tsettingswindow.ui
