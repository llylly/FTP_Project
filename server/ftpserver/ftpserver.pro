TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

LIBS += -lpthread

QMAKE_CXXFLAGS += -std=c++0x

SOURCES += main.cpp \
    msocket.cpp \
    globalsettings.cpp \
    mainsocket.cpp \
    handler.cpp

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    msocket.h \
    globalsettings.h \
    mainsocket.h \
    handler.h

