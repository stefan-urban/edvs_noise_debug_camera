TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

TARGET = debugcamera

LIBS += -pthread

SOURCES += main.cpp \
    pseudoterminal.cpp

HEADERS += \
    pseudoterminal.h

