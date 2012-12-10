# -------------------------------------------------
# Project created by QtCreator 2010-03-22T15:53:57
# -------------------------------------------------
win32:RC_FILE = qgrapheditor2.rc
unix:LIBS += -lGLU
# -------------------------------------------------
TRANSLATIONS = grapheditor2_en.ts \
    grapheditor2_ua.ts \
    grapheditor2_ru.ts
QT += opengl network xml script
TARGET = grapheditor2
TEMPLATE = app
SOURCES += main.cpp \
    wndmain.cpp \
    gldrawer.cpp \
    gldrawer3d.cpp \
    ge2rpcserver.cpp \
    ge2rpcthread.cpp \
    scriptmanager.cpp
HEADERS += wndmain.h \
    gldrawer.h \
    wptdefs.h \
    gldrawer3d.h \
    ge2rpcserver.h \
    ge2rpcthread.h \
    scriptmanager.h
FORMS += wndmain.ui
RESOURCES += resource.qrc
OTHER_FILES += qgrapheditor2.rc \
    qgrapheditor2.ico \
    qgrapheditor2.png
DEFINES += _CRT_SECURE_NO_WARNINGS
