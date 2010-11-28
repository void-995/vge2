// Qt
#include <QtNetwork>
#include <QtXml>

// Project
#include "wptdefs.h"
#include "ge2rpcthread.h"

#define XML_ATTRIBUTE_FOR_ITEM(nodeList, index, attr) nodeList.at(index).attributes().namedItem(attr).nodeValue()

extern QVector<grVertex> qvGraphVertices;
extern QVector<grEdge> qvGraphEdges;
extern QVector<int> qvMarkedVerticesIndexes;
extern QVector<int> qvMarkedEdgesIndexes;

extern int selectedPointNumFst;
extern int selectedPointNumSnd;

extern bool needToRebuildRD;

extern int globalStackIsModified;

Ge2RpcThread::Ge2RpcThread(int socketDescriptor, QObject *parent)
: QThread(parent), m_socketDescriptor(socketDescriptor)
{

}

void Ge2RpcThread::run()
{
    QTcpSocket *pTcpSocket = new QTcpSocket();
    if (!pTcpSocket->setSocketDescriptor(m_socketDescriptor))
    {
        emit error(pTcpSocket->error());
        delete pTcpSocket;
        return;
    }

    pTcpSocket->waitForReadyRead();

    char cBuffer[1024];
    QString strLine;
    qint64 lineLength;

    lineLength = pTcpSocket->readLine(cBuffer, sizeof(cBuffer));
    if(lineLength != -1)
    {
        strLine = cBuffer;
        strLine = strLine.trimmed();

        QRegExp postRequest("POST[\\s]{1,}/RPC[\\s]{1,}HTTP/1.[01]{1,1}", Qt::CaseInsensitive);

        if(strLine.contains(postRequest))
        {
            QString userAgent;
            QString host;
            QString contentType;
            qint64 contentLength = 0;

            // Read headers till first blank line
            while(!strLine.isEmpty())
            {
                lineLength = pTcpSocket->readLine(cBuffer, sizeof(cBuffer));
                if(lineLength != -1)
                {
                    strLine = cBuffer;
                    strLine = strLine.trimmed();

                    if(!strLine.isEmpty())
                    {
                        QString strHeader = strLine.section(":", 0, 0);
                        strHeader = strHeader.trimmed().toLower();

                        QString strHeaderValue = strLine.section(":", 1, -1);
                        strHeaderValue = strHeaderValue.trimmed();

                        if(strHeader == "user-agent")
                            userAgent = strHeaderValue;
                        else if(strHeader == "host")
                            host = strHeaderValue;
                        else if(strHeader == "content-type")
                            contentType = strHeaderValue;
                        else if(strHeader == "content-length")
                            contentLength = strHeaderValue.toLongLong();
                    }
                }
                else
                {
                    // TODO: Add error reading event handling
                }
            }

            // Read Xml from Request
            QString strXmlRequest;
            for(int bytesRead = 0; bytesRead < contentLength; bytesRead += 32)
            {
                memset(cBuffer, 0, 1024);
                pTcpSocket->read(cBuffer, (contentLength - bytesRead) >= 32 ? 32 : (contentLength - bytesRead));
                strXmlRequest += cBuffer;
            }

            if(contentType == "application/x-www-form-urlencoded")
            {
                QByteArray byteArray;
                strXmlRequest = strXmlRequest.section("=", 1, -1).replace("+", " ");
                byteArray = strXmlRequest.toUtf8();
                strXmlRequest = QUrl::fromPercentEncoding(byteArray);
            }

            QDomDocument xmlRequest;
            if(xmlRequest.setContent(strXmlRequest, false))
            {
                QDomElement rootElement = xmlRequest.documentElement();
                if(rootElement.nodeName().toLower() == QString("methodsCall").toLower())
                {
                    QDomDocument xmlDoc;
                    QDomElement xmlRoot = xmlDoc.createElement("methodsResponse");

                    QDomNodeList nodeListMethods = rootElement.childNodes();

                    for(int i = 0; i < nodeListMethods.size(); i++)
                    {
                        QString strMethodName = nodeListMethods.at(i).attributes().namedItem("name").nodeValue();
                        QDomNodeList nodeListParams = nodeListMethods.at(i).childNodes();

                        if(strMethodName == "ClearGraph")
                            xmlRoot.appendChild(ClearGraph(nodeListParams));
                        else if(strMethodName == "AddVertices")
                            xmlRoot.appendChild(AddVertices(nodeListParams));
                        else if(strMethodName == "AddEdges")
                            xmlRoot.appendChild(AddEdges(nodeListParams));
                        else if(strMethodName == "GetGraph")
                            xmlRoot.appendChild(GetGraph(nodeListParams));
                        else if(strMethodName == "SetGraph")
                            xmlRoot.appendChild(SetGraph(nodeListParams));
                        else if(strMethodName == "GetEdgesOfVertex")
                            xmlRoot.appendChild(GetEdgesOfVertex(nodeListParams));
                        else if(strMethodName == "DeleteVertex")
                            xmlRoot.appendChild(DeleteVertex(nodeListParams));
                        else if(strMethodName == "RemoveEdge")
                            xmlRoot.appendChild(RemoveEdge(nodeListParams));
                        else if(strMethodName == "GetSelectedIndexes")
                            xmlRoot.appendChild(GetSelectedIndexes(nodeListParams));
                        else if(strMethodName == "GetVertexByIndex")
                            xmlRoot.appendChild(GetVertexByIndex(nodeListParams));
                        else if(strMethodName == "MarkGraphsPart")
                            xmlRoot.appendChild(MarkGraphsPart(nodeListParams));
                    }

                    xmlDoc.appendChild(xmlRoot);

                    QString strResponse200Body;
                    QString strResponse200Type;

                    if(xmlRoot.hasChildNodes())
                    {
                        strResponse200Body = "<?xml version=\"1.0\"?>\n" + xmlDoc.toString(4);
                        strResponse200Type = "application/xml";
                    }
                    else
                    {
                        strResponse200Body = "Internal error, sorry.";
                        strResponse200Type = "text/plain";
                    }

                    QDateTime dateUtc = QDateTime::currentDateTimeUtc();
                    QLocale cLocale = QLocale::c();

                    QString strResponse200Head = QString("HTTP/1.1 200 OK\n"
                                                     "Connection: close\n"
                                                     "Content-Length: %1\n"
                                                     "Content-Type: %3\n"
                                                     "Date: %2 GMT\n"
                                                     "Server: VGE2 RPC Server/1.0.4 Alpha").arg(
                                                             strResponse200Body.size()).arg(
                                                             dateUtc.toString("%1, dd %2 yyyy hh:mm:ss").arg(
                                                                     cLocale.dayName(dateUtc.date().dayOfWeek(), QLocale::ShortFormat)).arg(
                                                                             cLocale.monthName(dateUtc.date().month(), QLocale::ShortFormat)
                                                                             )
                                                                     ).arg(strResponse200Type);

                    QString strResponse200 = strResponse200Head + "\n\n" + strResponse200Body;

                    pTcpSocket->write(strResponse200.toLatin1(), strResponse200.size());
                    if(pTcpSocket->state() != QTcpSocket::UnconnectedState)
                        pTcpSocket->waitForBytesWritten();
                }
            }
            else
            {
                // TODO: Add Xml parsing error handling
            }
        }
        else
        {
            QString strResponse501Body = "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\n"
            "        \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n"
            "<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
            "<head>\n"
            "<title>501 Not Implemented</title>\n"
            "</head>\n"
            "<body>\n"
            "<h1>501 Not Implemented</h1>\n"
            "<hr/>\n"
            "The server either does not recognise the request method, or it lacks the ability to fulfill the request.\n"
            "</body>\n"
            "</html>";

            QDateTime dateUtc = QDateTime::currentDateTimeUtc();
            QLocale cLocale = QLocale::c();

            QString strResponse501Head = QString("HTTP/1.1 501 OK\n"
                                             "Connection: close\n"
                                             "Content-Length: %1\n"
                                             "Content-Type: text/html\n"
                                             "Date: %2 GMT\n"
                                             "Server: VGE2 RPC Server/1.0.4 Alpha").arg(
                                                     strResponse501Body.size()).arg(
                                                     dateUtc.toString("%1, dd %2 yyyy hh:mm:ss").arg(
                                                             cLocale.dayName(dateUtc.date().dayOfWeek(), QLocale::ShortFormat)).arg(
                                                                     cLocale.monthName(dateUtc.date().month(), QLocale::ShortFormat)
                                                                     )
                                                             );

            QString strResponse501 = strResponse501Head + "\n\n" + strResponse501Body;

            pTcpSocket->write(strResponse501.toLatin1(), strResponse501.size());
            if(pTcpSocket->state() != QTcpSocket::UnconnectedState)
                pTcpSocket->waitForBytesWritten();
        }
    }
    else
    {
        // TODO: Add error reading event handling
    }

    pTcpSocket->disconnectFromHost();
    if(pTcpSocket->state() != QTcpSocket::UnconnectedState)
        pTcpSocket->waitForDisconnected();

    delete pTcpSocket;
}

QDomNode Ge2RpcThread::ClearGraph(QDomNodeList nodeListParams)
{
    int retCode;
    QString retDescription;

    globalStackIsModified += qvGraphVertices.size() > 0;

    qvGraphVertices.clear();
    qvGraphEdges.clear();
    qvMarkedVerticesIndexes.clear();
    qvMarkedEdgesIndexes.clear();

    needToRebuildRD = true;

    retCode = 0;
    retDescription = "OK";

    // RET_ClearGraph:
    QDomDocument xmlDoc;

    QDomElement responseRoot = xmlDoc.createElement("method");
    responseRoot.setAttribute("name", "ClearGraph");

    QDomElement outParam = xmlDoc.createElement("param");
    outParam.setAttribute("type", "int");
    outParam.setAttribute("value", retCode);
    outParam.setAttribute("description", retDescription);

    responseRoot.appendChild(outParam);

    return responseRoot;
}

QDomNode Ge2RpcThread::AddVertices(QDomNodeList nodeListParams)
{
    int retCode;
    QString retDescription;

    int verticesCount;
    QVector<grVertex> vertices;

    if(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 0, "type") != "int")
    {
        retCode = 1;
        retDescription = "Incorrect parameter: vertices count must be type of int";
        goto RET_AddVertices;
    }
    else
    {
        bool bOK;
        verticesCount = XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 0, "value").toInt(&bOK);
        if(!bOK)
        {
            retCode = 1;
            retDescription = "Incorrect parameter: error while parsing vertices count value";
            goto RET_AddVertices;
        }
    }

    for(int j = 1; j < verticesCount + 1; j++)
    {
        if(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, j, "type") != "vertex")
        {
            retCode = 1;
            retDescription = "Incorrect parameter: error while parsing vertex value";
            goto RET_AddVertices;
        }
        else
        {
            grVertex vertex;
            vertex.vecOrigin.setX(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, j, "x").toFloat());
            vertex.vecOrigin.setY(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, j, "y").toFloat());
            vertex.vecOrigin.setZ(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, j, "z").toFloat());

            vertices.push_back(vertex);
        }
    }

    for(int i = 0; i < verticesCount; i++)
    {
        for(int j = 0; j < qvGraphVertices.size(); j++)
        {
            QVector3D vecDifference = vertices[i].vecOrigin - qvGraphVertices[j].vecOrigin;
            if(vecDifference.length() < 15.0f)
            {
                retCode = 2;
                retDescription = "Illegal vertices positions";
                goto RET_AddVertices;
            }
        }
    }

    for(int i = 0; i < verticesCount; i++)
        for(int j = 0; j < verticesCount; j++)
            if(i != j)
            {
                QVector3D vecDifference = vertices[i].vecOrigin - vertices[j].vecOrigin;
                if(vecDifference.length() < 15.0f)
                {
                    retCode = 2;
                    retDescription = "Illegal vertices positions";
                    goto RET_AddVertices;
                }
            }

    for(int i = 0; i < verticesCount; i++)
        qvGraphVertices.push_back(vertices[i]);

    retCode = 0;
    retDescription = "OK";

    needToRebuildRD = true;
    globalStackIsModified++;

    RET_AddVertices:
    QDomDocument xmlDoc;

    QDomElement responseRoot = xmlDoc.createElement("method");
    responseRoot.setAttribute("name", "AddVertices");

    QDomElement outParam = xmlDoc.createElement("param");
    outParam.setAttribute("type", "int");
    outParam.setAttribute("value", retCode);
    outParam.setAttribute("description", retDescription);

    responseRoot.appendChild(outParam);

    vertices.clear();

    return responseRoot;
}

QDomNode Ge2RpcThread::AddEdges(QDomNodeList nodeListParams)
{
    int retCode;
    QString retDescription;

    int edgesCount;
    QVector<grEdge> edges;

    if(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 0, "type") != "int")
    {
        retCode = 1;
        retDescription = "Incorrect parameter: edges count must be type of int";
        goto RET_AddEdges;
    }
    else
    {
        bool bOK;
        edgesCount = XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 0, "value").toInt(&bOK);
        if(!bOK)
        {
            retCode = 1;
            retDescription = "Incorrect parameter: error while parsing edges count value";
            goto RET_AddEdges;
        }
    }

    for(int j = 1; j < edgesCount + 1; j++)
    {
        if(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, j, "type") != "edge")
        {
            retCode = 1;
            retDescription = "Incorrect parameter: error while parsing edge value";
            goto RET_AddEdges;
        }
        else
        {
            bool bOK;
            grEdge edge;

            edge.iFrom = XML_ATTRIBUTE_FOR_ITEM(nodeListParams, j, "from").toInt(&bOK);
            if(!bOK)
            {
                retCode = 1;
                retDescription = "Incorrect parameter: error while parsing edge's \"from\" value";
                goto RET_AddEdges;
            }

            edge.iTo = XML_ATTRIBUTE_FOR_ITEM(nodeListParams, j, "to").toInt(&bOK);
            if(!bOK)
            {
                retCode = 1;
                retDescription = "Incorrect parameter: error while parsing edge's \"to\" value";
                goto RET_AddEdges;
            }

            edges.push_back(edge);
        }
    }

    for(int i = 0; i < edgesCount; i++)
    {
        if((edges[i].iFrom < 0 || edges[i].iFrom >= qvGraphVertices.size()) ||
           (edges[i].iTo < 0 || edges[i].iTo >= qvGraphVertices.size()) ||
           (edges[i].iFrom == edges[i].iTo))
        {
            retCode = 2;
            retDescription = "Incorrect parameter: one of edges has wrong index in values \"from\" or \"to\"";
            goto RET_AddEdges;
        }

        for(int j = 0; j < qvGraphEdges.size(); j++)
        {
            if((edges[i].iFrom == qvGraphEdges[j].iFrom) &&
               (edges[i].iTo == qvGraphEdges[j].iTo))
            {
                retCode = 2;
                retDescription = "Incorrect parameter: one of edges already exists";
                goto RET_AddEdges;
            }
        }
    }

    for(int i = 0; i < edgesCount; i++)
        for(int j = 0; j < edgesCount; j++)
            if(i != j)
            {
                if((edges[i].iFrom == edges[j].iFrom) &&
                   (edges[i].iTo == edges[j].iTo))
                {
                    retCode = 2;
                    retDescription = "Incorrect parameter: duplicate in parameters";
                    goto RET_AddEdges;
                }
            }

    for(int i = 0; i < edgesCount; i++)
        qvGraphEdges.push_back(edges[i]);

    retCode = 0;
    retDescription = "OK";

    needToRebuildRD = true;
    globalStackIsModified++;

    RET_AddEdges:
    QDomDocument xmlDoc;

    QDomElement responseRoot = xmlDoc.createElement("method");
    responseRoot.setAttribute("name", "AddEdges");

    QDomElement outParam = xmlDoc.createElement("param");
    outParam.setAttribute("type", "int");
    outParam.setAttribute("value", retCode);
    outParam.setAttribute("description", retDescription);

    responseRoot.appendChild(outParam);

    edges.clear();

    return responseRoot;
}

QDomNode Ge2RpcThread::GetGraph(QDomNodeList nodeListParams)
{
    int retCode;
    QString retDescription;

    needToRebuildRD = true;

    retCode = 0;
    retDescription = "OK";

    // RET_GetGraph:
    QDomDocument xmlDoc;

    QDomElement responseRoot = xmlDoc.createElement("method");
    responseRoot.setAttribute("name", "GetGraph");

    QDomElement outParam = xmlDoc.createElement("param");
    outParam.setAttribute("type", "int");
    outParam.setAttribute("value", retCode);
    outParam.setAttribute("description", retDescription);

    QDomElement outVerticesCount = xmlDoc.createElement("param");
    outVerticesCount.setAttribute("type", "int");
    outVerticesCount.setAttribute("value", qvGraphVertices.size());
    outVerticesCount.setAttribute("description", "verticesCount");

    QDomElement outEdgesCount = xmlDoc.createElement("param");
    outEdgesCount.setAttribute("type", "int");
    outEdgesCount.setAttribute("value", qvGraphEdges.size());
    outEdgesCount.setAttribute("description", "edgesCount");

    responseRoot.appendChild(outParam);
    responseRoot.appendChild(outVerticesCount);
    responseRoot.appendChild(outEdgesCount);

    for(int i = 0; i < qvGraphVertices.size(); i++)
    {
        QDomElement outVertexNode = xmlDoc.createElement("param");
        outVertexNode.setAttribute("type", "vertex");
        outVertexNode.setAttribute("x", qvGraphVertices[i].vecOrigin.x());
        outVertexNode.setAttribute("y", qvGraphVertices[i].vecOrigin.y());
        outVertexNode.setAttribute("z", qvGraphVertices[i].vecOrigin.z());

        responseRoot.appendChild(outVertexNode);
    }

    for(int i = 0; i < qvGraphEdges.size(); i++)
    {
        QDomElement outEdgeNode = xmlDoc.createElement("param");
        outEdgeNode.setAttribute("type", "edge");
        outEdgeNode.setAttribute("from", qvGraphEdges[i].iFrom);
        outEdgeNode.setAttribute("to", qvGraphEdges[i].iTo);

        responseRoot.appendChild(outEdgeNode);
    }

    return responseRoot;
}

QDomNode Ge2RpcThread::SetGraph(QDomNodeList nodeListParams)
{
    int retCode;
    QString retDescription;

    int verticesCount;
    QVector<grVertex> vertices;

    int edgesCount;
    QVector<grEdge> edges;

    if(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 0, "type") != "int")
    {
        retCode = 1;
        retDescription = "Incorrect parameter: vertices count must be type of int";
        goto RET_SetGraph;
    }
    else
    {
        bool bOK;
        verticesCount = XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 0, "value").toInt(&bOK);
        if(!bOK)
        {
            retCode = 1;
            retDescription = "Incorrect parameter: error while parsing vertices count value";
            goto RET_SetGraph;
        }
    }

    if(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 1, "type") != "int")
    {
        retCode = 1;
        retDescription = "Incorrect parameter: edges count must be type of int";
        goto RET_SetGraph;
    }
    else
    {
        bool bOK;
        edgesCount = XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 1, "value").toInt(&bOK);
        if(!bOK)
        {
            retCode = 1;
            retDescription = "Incorrect parameter: error while parsing edges count value";
            goto RET_SetGraph;
        }
    }

    for(int j = 2; j < verticesCount + 2; j++)
    {
        if(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, j, "type") != "vertex")
        {
            retCode = 1;
            retDescription = "Incorrect parameter: error while parsing vertex value";
            goto RET_SetGraph;
        }
        else
        {
            grVertex vertex;
            vertex.vecOrigin.setX(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, j, "x").toFloat());
            vertex.vecOrigin.setY(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, j, "y").toFloat());
            vertex.vecOrigin.setZ(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, j, "z").toFloat());

            vertices.push_back(vertex);
        }
    }

    for(int i = 0; i < verticesCount; i++)
        for(int j = 0; j < verticesCount; j++)
            if(i != j)
            {
                QVector3D vecDifference = vertices[i].vecOrigin - vertices[j].vecOrigin;
                if(vecDifference.length() < 15.0f)
                {
                    retCode = 2;
                    retDescription = "Illegal vertices positions";
                    goto RET_SetGraph;
                }
            }

    for(int j = verticesCount + 2; j < edgesCount + verticesCount + 2; j++)
    {
        if(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, j, "type") != "edge")
        {
            retCode = 1;
            retDescription = "Incorrect parameter: error while parsing edge value";
            goto RET_SetGraph;
        }
        else
        {
            bool bOK;
            grEdge edge;

            edge.iFrom = XML_ATTRIBUTE_FOR_ITEM(nodeListParams, j, "from").toInt(&bOK);
            if(!bOK)
            {
                retCode = 1;
                retDescription = "Incorrect parameter: error while parsing edge's \"from\" value";
                goto RET_SetGraph;
            }

            edge.iTo = XML_ATTRIBUTE_FOR_ITEM(nodeListParams, j, "to").toInt(&bOK);
            if(!bOK)
            {
                retCode = 1;
                retDescription = "Incorrect parameter: error while parsing edge's \"to\" value";
                goto RET_SetGraph;
            }

            edges.push_back(edge);
        }
    }

    for(int i = 0; i < edgesCount; i++)
    {
        if((edges[i].iFrom < 0 || edges[i].iFrom >= verticesCount) ||
           (edges[i].iTo < 0 || edges[i].iTo >= verticesCount) ||
           (edges[i].iFrom == edges[i].iTo))
        {
            retCode = 2;
            retDescription = "Incorrect parameter: one of edges has wrong index in values \"from\" or \"to\"";
            goto RET_SetGraph;
        }
    }

    for(int i = 0; i < edgesCount; i++)
        for(int j = 0; j < edgesCount; j++)
            if(i != j)
            {
                if((edges[i].iFrom == edges[j].iFrom) &&
                   (edges[i].iTo == edges[j].iTo))
                {
                    retCode = 2;
                    retDescription = "Incorrect parameter: duplicate in parameters";
                    goto RET_SetGraph;
                }
            }

    globalStackIsModified += qvGraphVertices.size() > 0;
    globalStackIsModified += verticesCount > 0;
    globalStackIsModified += edgesCount > 0;

    qvGraphVertices.clear();
    qvGraphEdges.clear();
    qvMarkedVerticesIndexes.clear();
    qvMarkedEdgesIndexes.clear();

    for(int i = 0; i < verticesCount; i++)
        qvGraphVertices.push_back(vertices[i]);

    for(int i = 0; i < edgesCount; i++)
        qvGraphEdges.push_back(edges[i]);

    needToRebuildRD = true;

    retCode = 0;
    retDescription = "OK";

    RET_SetGraph:
    QDomDocument xmlDoc;

    QDomElement responseRoot = xmlDoc.createElement("method");
    responseRoot.setAttribute("name", "SetGraph");

    QDomElement outParam = xmlDoc.createElement("param");
    outParam.setAttribute("type", "int");
    outParam.setAttribute("value", retCode);
    outParam.setAttribute("description", retDescription);

    responseRoot.appendChild(outParam);

    vertices.clear();
    edges.clear();

    return responseRoot;
}

QDomNode Ge2RpcThread::GetEdgesOfVertex(QDomNodeList nodeListParams)
{
    int retCode;
    QString retDescription;

    int vertexIndex;
    int connectionWay;

    QVector<grEdge> qvGraphEdgesCopy;

    if(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 0, "type") != "int")
    {
        retCode = 1;
        retDescription = "Incorrect parameter: index of vertex must be type of int";
        goto RET_GetEdgesOfVertex;
    }
    else
    {
        bool bOK;
        vertexIndex = XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 0, "value").toInt(&bOK);
        if(!bOK)
        {
            retCode = 1;
            retDescription = "Incorrect parameter: error while parsing index of vertex value";
            goto RET_GetEdgesOfVertex;
        }
    }

    if(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 1, "type") != "int")
    {
        retCode = 1;
        retDescription = "Incorrect parameter: connection way code must be type of int";
        goto RET_GetEdgesOfVertex;
    }
    else
    {
        bool bOK;
        connectionWay = XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 1, "value").toInt(&bOK);
        if(!bOK)
        {
            retCode = 1;
            retDescription = "Incorrect parameter: error while parsing connection way code value";
            goto RET_GetEdgesOfVertex;
        }
    }

    if(vertexIndex >= qvGraphVertices.size() || vertexIndex < 0)
    {
        retCode = 1;
        retDescription = "Incorrect parameter: index of vertex is out of bounds";
        goto RET_GetEdgesOfVertex;
    }

    if(connectionWay > 2 || connectionWay < 0)
    {
        retCode = 1;
        retDescription = "Incorrect parameter: connection way code is out of bounds";
        goto RET_GetEdgesOfVertex;
    }

    for(int i = 0; i < qvGraphEdges.size(); i++)
        qvGraphEdgesCopy.push_back(qvGraphEdges[i]);

    for(int i = 0; i < qvGraphEdgesCopy.size(); i++)
    {
        switch(connectionWay)
        {
        case 0: {
                if(qvGraphEdgesCopy[i].iFrom != (int)vertexIndex && qvGraphEdgesCopy[i].iTo != (int)vertexIndex)
                {
                    qvGraphEdgesCopy.remove(i, 1);
                    i--;
                }
            }
            break;
        case 1: {
                if(qvGraphEdgesCopy[i].iFrom != (int)vertexIndex)
                {
                    qvGraphEdgesCopy.remove(i, 1);
                    i--;
                }
            }
            break;
        case 2: {
                if(qvGraphEdgesCopy[i].iTo != (int)vertexIndex)
                {
                    qvGraphEdgesCopy.remove(i, 1);
                    i--;
                }
            }
            break;
        }
    }

    retCode = 0;
    retDescription = "OK";

    RET_GetEdgesOfVertex:
    QDomDocument xmlDoc;

    QDomElement responseRoot = xmlDoc.createElement("method");
    responseRoot.setAttribute("name", "GetEdgesOfVertex");

    QDomElement outParam = xmlDoc.createElement("param");
    outParam.setAttribute("type", "int");
    outParam.setAttribute("value", retCode);
    outParam.setAttribute("description", retDescription);

    responseRoot.appendChild(outParam);

    if(retCode == 0)
    {
        QDomElement outEdgesCount = xmlDoc.createElement("param");
        outEdgesCount.setAttribute("type", "int");
        outEdgesCount.setAttribute("value", qvGraphEdgesCopy.size());
        outEdgesCount.setAttribute("description", "edgesCount");

        responseRoot.appendChild(outEdgesCount);

        for(int i = 0; i < qvGraphEdgesCopy.size(); i++)
        {
            QDomElement outEdgeNode = xmlDoc.createElement("param");
            outEdgeNode.setAttribute("type", "edge");
            outEdgeNode.setAttribute("from", qvGraphEdgesCopy[i].iFrom);
            outEdgeNode.setAttribute("to", qvGraphEdgesCopy[i].iTo);

            responseRoot.appendChild(outEdgeNode);
        }

        qvGraphEdgesCopy.clear();
    }

    return responseRoot;
}

QDomNode Ge2RpcThread::DeleteVertex(QDomNodeList nodeListParams)
{
    int retCode;
    QString retDescription;

    int vertexIndex;

    if(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 0, "type") != "int")
    {
        retCode = 1;
        retDescription = "Incorrect parameter: index of vertex must be type of int";
        goto RET_DeleteVertex;
    }
    else
    {
        bool bOK;
        vertexIndex = XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 0, "value").toInt(&bOK);
        if(!bOK)
        {
            retCode = 1;
            retDescription = "Incorrect parameter: error while parsing index of vertex value";
            goto RET_DeleteVertex;
        }
    }

    if(vertexIndex >= qvGraphVertices.size() || vertexIndex < 0)
    {
        retCode = 1;
        retDescription = "Incorrect parameter: index of vertex is out of bounds";
        goto RET_DeleteVertex;
    }

    qvGraphVertices.remove(vertexIndex, 1);

    for(int i = 0; i < qvGraphEdges.size(); i++)
    {
            if(qvGraphEdges[i].iFrom == vertexIndex || qvGraphEdges[i].iTo == vertexIndex)
            {
                    qvGraphEdges.remove(i, 1);
                    i--;
            }
    }

    for(int i = 0; i < qvGraphEdges.size(); i++)
    {
        if(qvGraphEdges[i].iFrom >= vertexIndex)
                qvGraphEdges[i].iFrom--;
        if(qvGraphEdges[i].iTo >= vertexIndex)
                qvGraphEdges[i].iTo--;
    }

    retCode = 0;
    retDescription = "OK";

    needToRebuildRD = true;
    globalStackIsModified++;

    RET_DeleteVertex:
    QDomDocument xmlDoc;

    QDomElement responseRoot = xmlDoc.createElement("method");
    responseRoot.setAttribute("name", "DeleteVertex");

    QDomElement outParam = xmlDoc.createElement("param");
    outParam.setAttribute("type", "int");
    outParam.setAttribute("value", retCode);
    outParam.setAttribute("description", retDescription);

    responseRoot.appendChild(outParam);

    return responseRoot;
}

QDomNode Ge2RpcThread::RemoveEdge(QDomNodeList nodeListParams)
{
    int retCode = 2;
    QString retDescription = "Edge not found";

    grEdge edge;

    if(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 0, "type") != "edge")
    {
        retCode = 1;
        retDescription = "Incorrect parameter: error while parsing edge value";
        goto RET_RemoveEdge;
    }
    else
    {
        bool bOK;

        edge.iFrom = XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 0, "from").toInt(&bOK);
        if(!bOK)
        {
            retCode = 1;
            retDescription = "Incorrect parameter: error while parsing edge's \"from\" value";
            goto RET_RemoveEdge;
        }

        edge.iTo = XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 0, "to").toInt(&bOK);
        if(!bOK)
        {
            retCode = 1;
            retDescription = "Incorrect parameter: error while parsing edge's \"to\" value";
            goto RET_RemoveEdge;
        }
    }

    if((edge.iFrom < 0 || edge.iFrom >= qvGraphVertices.size()) ||
       (edge.iTo < 0 || edge.iTo >= qvGraphVertices.size()) ||
       (edge.iFrom == edge.iTo))
    {
        retCode = 2;
        retDescription = "Incorrect parameter: one of edges has wrong index in values \"from\" or \"to\"";
        goto RET_RemoveEdge;
    }

    for(int i = 0; i < qvGraphEdges.size(); i++)
    {
        if(qvGraphEdges[i].iFrom == edge.iFrom && qvGraphEdges[i].iTo == edge.iTo)
        {
            qvGraphEdges.remove(i, 1);
            globalStackIsModified++;
            needToRebuildRD = true;

            retCode = 0;
            retDescription = "OK";
            goto RET_RemoveEdge;
        }
    }

    RET_RemoveEdge:
    QDomDocument xmlDoc;

    QDomElement responseRoot = xmlDoc.createElement("method");
    responseRoot.setAttribute("name", "RemoveEdge");

    QDomElement outParam = xmlDoc.createElement("param");
    outParam.setAttribute("type", "int");
    outParam.setAttribute("value", retCode);
    outParam.setAttribute("description", retDescription);

    responseRoot.appendChild(outParam);

    return responseRoot;
}

QDomNode Ge2RpcThread::GetSelectedIndexes(QDomNodeList nodeListParams)
{
    int retCode;
    QString retDescription;

    retCode = 0;
    retDescription = "OK";

    // GetSelectedIndexes:
    QDomDocument xmlDoc;

    QDomElement responseRoot = xmlDoc.createElement("method");
    responseRoot.setAttribute("name", "GetSelectedIndexes");

    QDomElement outParam = xmlDoc.createElement("param");
    outParam.setAttribute("type", "int");
    outParam.setAttribute("value", retCode);
    outParam.setAttribute("description", retDescription);

    responseRoot.appendChild(outParam);

    QDomElement outFirstSelectedIndex = xmlDoc.createElement("param");
    outFirstSelectedIndex.setAttribute("type", "int");
    outFirstSelectedIndex.setAttribute("value", selectedPointNumFst);
    outFirstSelectedIndex.setAttribute("description", "firstIndex");

    responseRoot.appendChild(outFirstSelectedIndex);

    QDomElement outSecondSelectedIndex = xmlDoc.createElement("param");
    outSecondSelectedIndex.setAttribute("type", "int");
    outSecondSelectedIndex.setAttribute("value", selectedPointNumSnd);
    outSecondSelectedIndex.setAttribute("description", "secondIndex");

    responseRoot.appendChild(outSecondSelectedIndex);

    return responseRoot;
}

QDomNode Ge2RpcThread::GetVertexByIndex(QDomNodeList nodeListParams)
{
    int retCode;
    QString retDescription;

    int vertexIndex;

    if(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 0, "type") != "int")
    {
        retCode = 1;
        retDescription = "Incorrect parameter: index of vertex must be type of int";
        goto RET_GetVertexByIndex;
    }
    else
    {
        bool bOK;
        vertexIndex = XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 0, "value").toInt(&bOK);
        if(!bOK)
        {
            retCode = 1;
            retDescription = "Incorrect parameter: error while parsing index of vertex value";
            goto RET_GetVertexByIndex;
        }
    }

    if(vertexIndex >= qvGraphVertices.size() || vertexIndex < 0)
    {
        retCode = 1;
        retDescription = "Incorrect parameter: index of vertex is out of bounds";
        goto RET_GetVertexByIndex;
    }

    retCode = 0;
    retDescription = "OK";

    RET_GetVertexByIndex:
    QDomDocument xmlDoc;

    QDomElement responseRoot = xmlDoc.createElement("method");
    responseRoot.setAttribute("name", "GetVertexByIndex");

    QDomElement outParam = xmlDoc.createElement("param");
    outParam.setAttribute("type", "int");
    outParam.setAttribute("value", retCode);
    outParam.setAttribute("description", retDescription);

    responseRoot.appendChild(outParam);

    if(retCode == 0)
    {
        QDomElement outVertexNode = xmlDoc.createElement("param");
        outVertexNode.setAttribute("type", "vertex");
        outVertexNode.setAttribute("x", qvGraphVertices[vertexIndex].vecOrigin.x());
        outVertexNode.setAttribute("y", qvGraphVertices[vertexIndex].vecOrigin.y());
        outVertexNode.setAttribute("z", qvGraphVertices[vertexIndex].vecOrigin.z());

        responseRoot.appendChild(outVertexNode);
    }

    return responseRoot;
}

QDomNode Ge2RpcThread::MarkGraphsPart(QDomNodeList nodeListParams)
{
    int retCode;
    QString retDescription;

    int verticesCount;
    QVector<int> verticesIndexes;

    int edgesCount;
    QVector<grEdge> edges;

    if(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 0, "type") != "int")
    {
        retCode = 1;
        retDescription = "Incorrect parameter: vertices count must be type of int";
        goto RET_MarkGraphsPart;
    }
    else
    {
        bool bOK;
        verticesCount = XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 0, "value").toInt(&bOK);
        if(!bOK)
        {
            retCode = 1;
            retDescription = "Incorrect parameter: error while parsing vertices count value";
            goto RET_MarkGraphsPart;
        }
    }

    if(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 1, "type") != "int")
    {
        retCode = 1;
        retDescription = "Incorrect parameter: edges count must be type of int";
        goto RET_MarkGraphsPart;
    }
    else
    {
        bool bOK;
        edgesCount = XML_ATTRIBUTE_FOR_ITEM(nodeListParams, 1, "value").toInt(&bOK);
        if(!bOK)
        {
            retCode = 1;
            retDescription = "Incorrect parameter: error while parsing edges count value";
            goto RET_MarkGraphsPart;
        }
    }

    for(int j = 2; j < verticesCount + 2; j++)
    {
        if(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, j, "type") != "int")
        {
            retCode = 1;
            retDescription = "Incorrect parameter: error while parsing one of indexes";
            goto RET_MarkGraphsPart;
        }
        else
        {
            bool bOK;
            int index = XML_ATTRIBUTE_FOR_ITEM(nodeListParams, j, "value").toInt(&bOK);
            if(!bOK)
            {
                retCode = 1;
                retDescription = "Incorrect parameter: error while parsing one of indexes";
                goto RET_MarkGraphsPart;
            }

            verticesIndexes.push_back(index);
        }
    }

    for(int i = 0; i < verticesCount; i++)
    {
        if(verticesIndexes[i] < 0 || verticesIndexes[i] >= qvGraphVertices.size())
        {
            retCode = 1;
            retDescription = "Incorrect parameter: one of indexes is out of bounds";
            goto RET_MarkGraphsPart;
        }
    }

    for(int j = verticesCount + 2; j < edgesCount + verticesCount + 2; j++)
    {
        if(XML_ATTRIBUTE_FOR_ITEM(nodeListParams, j, "type") != "edge")
        {
            retCode = 1;
            retDescription = "Incorrect parameter: error while parsing edge value";
            goto RET_MarkGraphsPart;
        }
        else
        {
            bool bOK;
            grEdge edge;

            edge.iFrom = XML_ATTRIBUTE_FOR_ITEM(nodeListParams, j, "from").toInt(&bOK);
            if(!bOK)
            {
                retCode = 1;
                retDescription = "Incorrect parameter: error while parsing edge's \"from\" value";
                goto RET_MarkGraphsPart;
            }

            edge.iTo = XML_ATTRIBUTE_FOR_ITEM(nodeListParams, j, "to").toInt(&bOK);
            if(!bOK)
            {
                retCode = 1;
                retDescription = "Incorrect parameter: error while parsing edge's \"to\" value";
                goto RET_MarkGraphsPart;
            }

            edges.push_back(edge);
        }
    }

    qvMarkedVerticesIndexes.clear();
    for(int i = 0; i < verticesCount; i++)
        qvMarkedVerticesIndexes.push_back(verticesIndexes[i]);

    qSort(qvMarkedVerticesIndexes);
    for(int i = 0; i < qvMarkedVerticesIndexes.size() - 1; i++)
        if(qvMarkedVerticesIndexes[i] == qvMarkedVerticesIndexes[i + 1])
        {
            qvMarkedVerticesIndexes.remove(i, 1);
            i--;
        }

    qvMarkedEdgesIndexes.clear();
    for(int i = 0; i < edgesCount; i++)
    {
        for(int j = 0; j < qvGraphEdges.size(); j++)
        {
            if(edges[i].iFrom == qvGraphEdges[j].iFrom && edges[i].iTo == qvGraphEdges[j].iTo)
            {
                qvMarkedEdgesIndexes.push_back(j);
                break;
            }
        }
    }

    qSort(qvMarkedEdgesIndexes);
    for(int i = 0; i < qvMarkedEdgesIndexes.size() - 1; i++)
        if(qvMarkedEdgesIndexes[i] == qvMarkedEdgesIndexes[i + 1])
        {
            qvMarkedEdgesIndexes.remove(i, 1);
            i--;
        }

    retCode = 0;
    retDescription = "OK";

    RET_MarkGraphsPart:
    QDomDocument xmlDoc;

    QDomElement responseRoot = xmlDoc.createElement("method");
    responseRoot.setAttribute("name", "MarkGraphsPart");

    QDomElement outParam = xmlDoc.createElement("param");
    outParam.setAttribute("type", "int");
    outParam.setAttribute("value", retCode);
    outParam.setAttribute("description", retDescription);

    responseRoot.appendChild(outParam);

    return responseRoot;
}
