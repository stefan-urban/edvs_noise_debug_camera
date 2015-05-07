TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -pthread

SOURCES += main.cpp \
    pseudoterminal.cpp

HEADERS += \
    pseudoterminal.h

