#ifndef GE2RPCSERVER_H
#define GE2RPCSERVER_H

// Qt
#include <QTcpServer>

class Ge2RpcServer : public QTcpServer
{
    Q_OBJECT

public:
    Ge2RpcServer(QObject *parent = 0);

protected:
    void incomingConnection(int socketDescriptor);
};

#endif // GE2RPCSERVER_H
