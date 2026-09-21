!equals(QT_MAJOR_VERSION, 6): error("TIATracker tests require Qt 6.")
QT = core
CONFIG += console c++17
CONFIG -= app_bundle debug_and_release
TEMPLATE = app
TARGET = applicationdata-tests
DESTDIR = .
INCLUDEPATH += ..
SOURCES += resource-setup.cpp ../applicationdata.cpp
HEADERS += ../applicationdata.h
win32: include(../windows-resources.pri)
