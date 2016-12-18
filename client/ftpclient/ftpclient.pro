TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++0x

SOURCES += main.cpp \
    globalsettings.cpp \
    msocket.cpp \
    instsocket.cpp

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    globalsettings.h \
    msocket.h \
    instsocket.h

