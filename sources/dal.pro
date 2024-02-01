QT = core

CONFIG += c++17 cmdline
CONFIG += sharedlib

DESTDIR = $$PWD/../lib
TEMPLATE = lib

CONFIG(release, debug|release): TARGET = dal
CONFIG(debug, debug|release): TARGET = dal_debug

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += ../include
INCLUDEPATH += ../include/device_abstraction
INCLUDEPATH += ../include/device_abstraction/asynch
INCLUDEPATH += ../include/device_abstraction/drivers
INCLUDEPATH += ../include/device_abstraction/material
INCLUDEPATH += ../include/device_abstraction/qbased

SOURCES += \
        device_abstraction/asynch/actionqueue.cpp \
        device_abstraction/asynch/queuemanager.cpp \
        device_abstraction/devicexternal.cpp \
        device_abstraction/drivers/dbadapter.cpp \
        device_abstraction/drivers/ptzadapter.cpp \
        device_abstraction/qbased/qasynchinterface.cpp \
        device_abstraction/qbased/qsimplemanager.cpp \

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    ../include/device_abstraction/asynch/actionqueue.h \
    ../include/device_abstraction/asynch/queuemanager.h \
    ../include/device_abstraction/devicexternal.h \
    ../include/device_abstraction/drivers/adapterinterface.h \
    ../include/device_abstraction/drivers/dbadapter.h \
    ../include/device_abstraction/drivers/ptzadapter.h \
    ../include/device_abstraction/material/actionabstract.h \
    ../include/device_abstraction/material/concreteaction.h \
    ../include/device_abstraction/material/concreteresponse.h \
    ../include/device_abstraction/material/responseabstract.h \
    ../include/device_abstraction/qbased/qasynchinterface.h \
    ../include/device_abstraction/qbased/qsimplemanager.h \
    ../include/device_abstraction/dalcore.h \
    ../include/device_abstraction/material/channel.h \
    ../include/device_abstraction/material/promise.h
