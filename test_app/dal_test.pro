QT = core

CONFIG += c++17 cmdline

DESTDIR  = $$PWD/../bin
TARGET = DAL_test
TEMPLATE = app

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        asynchmgrtest.cpp \
        main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    asynchmgrtest.h \

INCLUDEPATH += $$PWD/../include
DEPENDPATH += $$PWD/../include

CONFIG(release, debug|release): LIBS += -L$$PWD/../lib/ -ldal
CONFIG(debug, debug|release): LIBS += -L$$PWD/../lib/ -ldal_debug
