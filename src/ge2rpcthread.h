#ifndef GE2RPCTHREAD_H
#define GE2RPCTHREAD_H

// Qt
#include <QThread>
#include <QTcpSocket>
#include <QDomNode>

class Ge2RpcThread : public QThread
{
Q_OBJECT

public:
    Ge2RpcThread(int socketDescriptor, QObject *parent);
    void run();

signals:
    void error(QTcpSocket::SocketError socketError);

private:
    QDomNode ClearGraph(QDomNodeList nodeListParams);
    QDomNode AddVertices(QDomNodeList nodeListParams);
    QDomNode AddEdges(QDomNodeList nodeListParams);
    QDomNode GetGraph(QDomNodeList nodeListParams);
    QDomNode SetGraph(QDomNodeList nodeListParams);
    QDomNode GetEdgesOfVertex(QDomNodeList nodeListParams);
    QDomNode DeleteVertex(QDomNodeList nodeListParams);
    QDomNode RemoveEdge(QDomNodeList nodeListParams);
    QDomNode GetSelectedIndexes(QDomNodeList nodeListParams);
    QDomNode GetVertexByIndex(QDomNodeList nodeListParams);
    QDomNode MarkGraphsPart(QDomNodeList nodeListParams);

    int m_socketDescriptor;
};

#endif // GE2RPCTHREAD_H
