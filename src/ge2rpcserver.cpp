// Qt
#include <QtNetwork>
#include <QMessageBox>

// Project
#include "ge2rpcserver.h"
#include "ge2rpcthread.h"
#include "wndmain.h"

extern WndMain *g_pWndMain;

Ge2RpcServer::Ge2RpcServer(QObject *parent): QTcpServer(parent)
{
    if(!listen(QHostAddress::Any, 13669))
        QMessageBox::critical(g_pWndMain, tr("Visual graph editor"), tr("Couldn't start RPC server. Program will quit now."));
}

void Ge2RpcServer::incomingConnection(int socketDescriptor)
{
    Ge2RpcThread *thread = new Ge2RpcThread(socketDescriptor, this);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}
