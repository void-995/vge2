#define QT_COORD_TYPE float

// Qt
#include <QtGui/QApplication>
#include <QtOpenGL/QGLFormat>
#include <QTranslator>
#include <QSettings>

// Project
#include "wndmain.h"
#include "ge2rpcserver.h"

QTranslator g_translator;
QSettings g_settings("KPNU/Alexandr Palamar", "Visual Graph Editor 2");

Ge2RpcServer *g_pXmlRpcServer;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    g_translator.load(g_settings.value("language", "grapheditor2_en").toString());
    app.installTranslator(&g_translator);

    QGLFormat myGLFormat;
    myGLFormat.setVersion(2, 1);
    myGLFormat.setDoubleBuffer(true);
    myGLFormat.setSampleBuffers(true);
    myGLFormat.setSamples(g_settings.value("aa_level", 4).toInt());
    QGLFormat::setDefaultFormat(myGLFormat);

    WndMain frmMain;
    frmMain.show();

    g_pXmlRpcServer = new Ge2RpcServer();
    if(!g_pXmlRpcServer->isListening())
        return -1;

    return app.exec();
}
