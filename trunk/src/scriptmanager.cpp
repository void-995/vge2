// Qt
#include <QObject>
#include <QScriptEngine>
#include <QScriptContext>
#include <QScriptValueIterator>
#include <QFile>
#include <QDir>
#include <QStringList>
#include <QMessageBox>
#include <QInputDialog>

// Project
#include "wptdefs.h"
#include "wndmain.h"
#include "scriptmanager.h"

extern QVector<grVertex> qvGraphVertices;
extern QVector<grEdge> qvGraphEdges;
extern QVector<int> qvMarkedVerticesIndexes;
extern QVector<int> qvMarkedEdgesIndexes;
extern int selectedPointNumFst;
extern int selectedPointNumSnd;
extern bool needToRebuildRD;
extern WndMain *g_pWndMain;
extern unsigned int lastVertexIndex;
extern int globalStackIsModified;

Q_DECLARE_METATYPE(grVertex)
Q_DECLARE_METATYPE(grEdge)

static QScriptValue scriptVGE2ClearGraph(QScriptContext *, QScriptEngine *);
static QScriptValue scriptVGE2AddVertices(QScriptContext *, QScriptEngine *);
static QScriptValue scriptVGE2AddEdges(QScriptContext *, QScriptEngine *);
static QScriptValue scriptVGE2GetGraph(QScriptContext *, QScriptEngine *);
static QScriptValue scriptVGE2SetGraph(QScriptContext *, QScriptEngine *);
static QScriptValue scriptVGE2GetEdgesOfVertex(QScriptContext *, QScriptEngine *);
static QScriptValue scriptVGE2DeleteVertex(QScriptContext *, QScriptEngine *);
static QScriptValue scriptVGE2RemoveEdge(QScriptContext *, QScriptEngine *);
static QScriptValue scriptVGE2GetSelectedIndexes(QScriptContext *, QScriptEngine *);
static QScriptValue scriptVGE2GetVertexByIndex(QScriptContext *, QScriptEngine *);
static QScriptValue scriptVGE2MarkGraphsPart(QScriptContext *, QScriptEngine *);

static QScriptValue scriptAlert(QScriptContext *, QScriptEngine *);
static QScriptValue scriptConfirm(QScriptContext *, QScriptEngine *);
static QScriptValue scriptPrompt(QScriptContext *, QScriptEngine *);

QScriptValue new_grVertex(QScriptContext *, QScriptEngine *);
QScriptValue toScriptValue_grVertex(QScriptEngine *, const grVertex &);
void fromScriptValue_grVertex(const QScriptValue &, grVertex &);

QScriptValue new_grEdge(QScriptContext *, QScriptEngine *);
QScriptValue toScriptValue_grEdge(QScriptEngine *, const grEdge &);
void fromScriptValue_grEdge(const QScriptValue &, grEdge &);

static QScriptValue scriptVGE2ClearGraph(QScriptContext *context, QScriptEngine *)
{
    if(context->argumentCount() != 0)
    {
        context->throwError("clearGraph - invalid parameters");
        return QScriptValue();
    }

    globalStackIsModified += qvGraphVertices.size() > 0;

    qvGraphVertices.clear();
    qvGraphEdges.clear();
    qvMarkedVerticesIndexes.clear();
    qvMarkedEdgesIndexes.clear();

    needToRebuildRD = true;

    return QScriptValue();
}

static QScriptValue scriptVGE2AddVertices(QScriptContext *context, QScriptEngine *)
{
    if(context->argumentCount() != 1 || !context->argument(0).isArray())
    {
        context->throwError("addVertices - invalid parameters");
        return QScriptValue();
    }

    int verticesCount = context->argument(0).property("length").toInt32();

    QVector<grVertex> vertices;
    vertices.resize(verticesCount);

    for(int i = 0; i < verticesCount; i++)
    {
        if(!context->argument(0).property(i).isObject())
        {
            context->throwError("addVertices - invalid parameters");
            return QScriptValue();
        }
        else
        {
            fromScriptValue_grVertex(context->argument(0).property(i), vertices[i]);
        }
    }

    for(int i = 0; i < verticesCount; i++)
    {
        for(int j = 0; j < qvGraphVertices.size(); j++)
        {
            QVector3D vecDifference = vertices[i].vecOrigin - qvGraphVertices[j].vecOrigin;
            if(vecDifference.length() < 15.0f)
            {
                context->throwError("addVertices - invalid parameters");
                return QScriptValue();
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
                    context->throwError("addVertices - invalid parameters");
                    return QScriptValue();
                }
            }

    for(int i = 0; i < verticesCount; i++)
        qvGraphVertices.push_back(vertices[i]);

    needToRebuildRD = true;
    globalStackIsModified++;

    return QScriptValue();
}

static QScriptValue scriptVGE2AddEdges(QScriptContext *context, QScriptEngine *)
{
    if(context->argumentCount() != 1 || !context->argument(0).isArray())
    {
        context->throwError("addEdges - invalid parameters");
        return QScriptValue();
    }

    int edgesCount = context->argument(0).property("length").toInt32();

    QVector<grEdge> edges;
    edges.resize(edgesCount);

    for(int i = 0; i < edgesCount; i++)
    {
        if(!context->argument(0).property(i).isObject())
        {
            context->throwError("addEdges - invalid parameters");
            return QScriptValue();
        }
        else
        {
            fromScriptValue_grEdge(context->argument(0).property(i), edges[i]);
        }
    }

    for(int i = 0; i < edgesCount; i++)
    {
        if((edges[i].iFrom < 0 || edges[i].iFrom >= qvGraphVertices.size()) ||
           (edges[i].iTo < 0 || edges[i].iTo >= qvGraphVertices.size()) ||
           (edges[i].iFrom == edges[i].iTo))
        {
            context->throwError("addEdges - invalid parameters");
            return QScriptValue();
        }

        for(int j = 0; j < qvGraphEdges.size(); j++)
        {
            if((edges[i].iFrom == qvGraphEdges[j].iFrom) &&
               (edges[i].iTo == qvGraphEdges[j].iTo))
            {
                context->throwError("addEdges - invalid parameters");
                return QScriptValue();
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
                    context->throwError("addEdges - invalid parameters");
                    return QScriptValue();
                }
            }

    for(int i = 0; i < edgesCount; i++)
        qvGraphEdges.push_back(edges[i]);

    needToRebuildRD = true;
    globalStackIsModified++;

    return QScriptValue();
}

static QScriptValue scriptVGE2GetGraph(QScriptContext *context, QScriptEngine *engine)
{
    if(context->argumentCount() != 2 || !context->argument(0).isArray() || !context->argument(0).isArray())
    {
        context->throwError("getGraph - invalid parameters");
        return QScriptValue();
    }

    context->argument(0).setProperty("length", engine->toScriptValue(qvGraphVertices.size()));
    for(int i = 0; i < qvGraphVertices.size(); i++)
        context->argument(0).setProperty(i, engine->toScriptValue(qvGraphVertices[i]));

    context->argument(1).setProperty("length", engine->toScriptValue(qvGraphEdges.size()));
    for(int i = 0; i < qvGraphEdges.size(); i++)
        context->argument(1).setProperty(i, engine->toScriptValue(qvGraphEdges[i]));

    return QScriptValue();
}

static QScriptValue scriptVGE2SetGraph(QScriptContext *context, QScriptEngine *)
{
    if(context->argumentCount() != 2 || !context->argument(0).isArray() || !context->argument(1).isArray())
    {
        context->throwError("setGraph - invalid parameters");
        return QScriptValue();
    }

    int verticesCount = context->argument(0).property("length").toInt32();

    QVector<grVertex> vertices;
    vertices.resize(verticesCount);

    for(int i = 0; i < verticesCount; i++)
    {
        if(!context->argument(0).property(i).isObject())
        {
            context->throwError("setGraph - invalid parameters");
            return QScriptValue();
        }
        else
        {
            fromScriptValue_grVertex(context->argument(0).property(i), vertices[i]);
        }
    }

    for(int i = 0; i < verticesCount; i++)
        for(int j = 0; j < verticesCount; j++)
            if(i != j)
            {
                QVector3D vecDifference = vertices[i].vecOrigin - vertices[j].vecOrigin;
                if(vecDifference.length() < 15.0f)
                {
                    context->throwError("setGraph - invalid parameters");
                    return QScriptValue();
                }
            }

    int edgesCount = context->argument(1).property("length").toInt32();

    QVector<grEdge> edges;
    edges.resize(edgesCount);

    for(int i = 0; i < edgesCount; i++)
    {
        if(!context->argument(1).property(i).isObject())
        {
            context->throwError("setGraph - invalid parameters");
            return QScriptValue();
        }
        else
        {
            fromScriptValue_grEdge(context->argument(1).property(i), edges[i]);
        }
    }

    for(int i = 0; i < edgesCount; i++)
    {
        if((edges[i].iFrom < 0 || edges[i].iFrom >= verticesCount) ||
           (edges[i].iTo < 0 || edges[i].iTo >= verticesCount) ||
           (edges[i].iFrom == edges[i].iTo))
        {
            context->throwError("setGraph - invalid parameters");
            return QScriptValue();
        }
    }

    for(int i = 0; i < edgesCount; i++)
        for(int j = 0; j < edgesCount; j++)
            if(i != j)
            {
                if((edges[i].iFrom == edges[j].iFrom) &&
                   (edges[i].iTo == edges[j].iTo))
                {
                    context->throwError("setGraph - invalid parameters");
                    return QScriptValue();
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

    return QScriptValue();
}

static QScriptValue scriptVGE2GetEdgesOfVertex(QScriptContext *context, QScriptEngine *engine)
{
    if(context->argumentCount() != 3 || !context->argument(0).isNumber() || !context->argument(1).isNumber() || !context->argument(2).isArray())
    {
        context->throwError("getEdgesOfVertex - invalid parameters");
        return QScriptValue();
    }

    int vertexIndex = context->argument(0).toInt32();
    int connectionWay = context->argument(1).toInt32();
    QVector<grEdge> qvGraphEdgesCopy;

    if(vertexIndex >= qvGraphVertices.size() || vertexIndex < 0)
    {
        context->throwError("getEdgesOfVertex - invalid parameters");
        return QScriptValue();
    }

    if(connectionWay > 2 || connectionWay < 0)
    {
        context->throwError("getEdgesOfVertex - invalid parameters");
        return QScriptValue();
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

    context->argument(2).setProperty("length", engine->toScriptValue(qvGraphEdgesCopy.size()));
    for(int i = 0; i < qvGraphEdgesCopy.size(); i++)
        context->argument(2).setProperty(i, engine->toScriptValue(qvGraphEdgesCopy[i]));

    return QScriptValue();
}

static QScriptValue scriptVGE2DeleteVertex(QScriptContext *context, QScriptEngine *)
{
    if(context->argumentCount() != 1 || !context->argument(0).isNumber())
    {
        context->throwError("deleteVertex - invalid parameters");
        return QScriptValue();
    }

    int vertexIndex = context->argument(0).toInt32();

    if(vertexIndex >= qvGraphVertices.size() || vertexIndex < 0)
    {
        context->throwError("deleteVertex - invalid parameters");
        return QScriptValue();
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

    needToRebuildRD = true;
    globalStackIsModified++;

    return QScriptValue();
}

static QScriptValue scriptVGE2RemoveEdge(QScriptContext *context, QScriptEngine *)
{
    if(context->argumentCount() != 1 || !context->argument(0).isObject())
    {
        context->throwError("removeEdge - invalid parameters");
        return QScriptValue();
    }

    grEdge edge;
    fromScriptValue_grEdge(context->argument(0), edge);

    if((edge.iFrom < 0 || edge.iFrom >= qvGraphVertices.size()) ||
       (edge.iTo < 0 || edge.iTo >= qvGraphVertices.size()) ||
       (edge.iFrom == edge.iTo))
    {
        context->throwError("removeEdge - invalid parameters");
        return QScriptValue();
    }

    for(int i = 0; i < qvGraphEdges.size(); i++)
    {
        if(qvGraphEdges[i].iFrom == edge.iFrom && qvGraphEdges[i].iTo == edge.iTo)
        {
            qvGraphEdges.remove(i, 1);
            globalStackIsModified++;
            needToRebuildRD = true;

            return QScriptValue();
        }
    }

    context->throwError("removeEdge - invalid parameters");
    return QScriptValue();
}

static QScriptValue scriptVGE2GetSelectedIndexes(QScriptContext *context, QScriptEngine *)
{
    if(context->argumentCount() != 1 || !context->argument(0).isArray())
    {
        context->throwError("getSelectedIndexes - invalid parameters");
        return QScriptValue();
    }

    context->argument(0).setProperty("length", 2);
    context->argument(0).setProperty(0, selectedPointNumFst);
    context->argument(0).setProperty(1, selectedPointNumSnd);

    return QScriptValue();
}

static QScriptValue scriptVGE2GetVertexByIndex(QScriptContext *context, QScriptEngine *)
{
    if(context->argumentCount() != 2 || !context->argument(0).isNumber() || !context->argument(1).isObject())
    {
        context->throwError("getVertexByIndex - invalid parameters");
        return QScriptValue();
    }

    int vertexIndex = context->argument(0).toInt32();

    if(vertexIndex >= qvGraphVertices.size() || vertexIndex < 0)
    {
        context->throwError("getVertexByIndex - invalid parameters");
        return QScriptValue();
    }

    context->argument(1).setProperty("x", qvGraphVertices[vertexIndex].vecOrigin.x());
    context->argument(1).setProperty("y", qvGraphVertices[vertexIndex].vecOrigin.y());
    context->argument(1).setProperty("z", qvGraphVertices[vertexIndex].vecOrigin.z());

    return QScriptValue();
}

static QScriptValue scriptVGE2MarkGraphsPart(QScriptContext *context, QScriptEngine *)
{
    if(context->argumentCount() != 2 || !context->argument(0).isArray() || !context->argument(1).isArray())
    {
        context->throwError("markGraphsPart - invalid parameters");
        return QScriptValue();
    }

    int verticesCount = context->argument(0).property("length").toInt32();
    QVector<int> verticesIndexes;
    verticesIndexes.resize(verticesCount);

    for(int i = 0; i < verticesCount; i++)
    {
        if(!context->argument(0).property(i).isNumber())
        {
            context->throwError("markGraphsPart - invalid parameters");
            return QScriptValue();
        }
        else
        {
            verticesIndexes[i] = context->argument(0).property(i).toInt32();
        }
    }

    for(int i = 0; i < verticesCount; i++)
    {
        if(verticesIndexes[i] < 0 || verticesIndexes[i] >= qvGraphVertices.size())
        {
            context->throwError("markGraphsPart - invalid parameters");
            return QScriptValue();
        }
    }

    int edgesCount = context->argument(1).property("length").toInt32();
    QVector<grEdge> edges;
    edges.resize(edgesCount);

    for(int i = 0; i < edgesCount; i++)
    {
        if(!context->argument(1).property(i).isObject())
        {
            context->throwError("markGraphsPart - invalid parameters");
            return QScriptValue();
        }
        else
        {
            fromScriptValue_grEdge(context->argument(1).property(i), edges[i]);
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

    return QScriptValue();
}

static QScriptValue scriptAlert(QScriptContext *context, QScriptEngine *)
{
    QMessageBox::warning(g_pWndMain, "QtScript Engine", context->argument(0).toString() + QString("\t"));
    return QScriptValue();
}

static QScriptValue scriptConfirm(QScriptContext *context, QScriptEngine *)
{
    QScriptValue retValue = QMessageBox::question(g_pWndMain, "QtScript Engine", context->argument(0).toString() + QString("\t"), QMessageBox::Ok, QMessageBox::Cancel) == QMessageBox::Ok;
    return retValue;
}

static QScriptValue scriptPrompt(QScriptContext *context, QScriptEngine *)
{
    QScriptValue retValue;

    bool bOK = false;
    QString retString = QInputDialog::getText(g_pWndMain, "QtScript Engine", context->argument(0).toString() + QString("\t"), QLineEdit::Normal, context->argument(1).toString(), &bOK);

    if(bOK)
        retValue = retString;

    return retValue;
}

QScriptValue new_grVertex(QScriptContext *context, QScriptEngine *engine)
{
    grVertex vertex;

    if(context->argumentCount() == 1)
    {
        float arg = context->argument(0).toVariant().toFloat();
        vertex.vecOrigin = QVector3D(arg, arg, arg);
    }
    else if(context->argumentCount() == 3)
    {
        float x = context->argument(0).toVariant().toFloat();
        float y = context->argument(1).toVariant().toFloat();
        float z = context->argument(2).toVariant().toFloat();
        vertex.vecOrigin = QVector3D(x, y, z);
    }

    return engine->toScriptValue(vertex);
}

QScriptValue toScriptValue_grVertex(QScriptEngine *engine, const grVertex &vertex)
{
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", vertex.vecOrigin.x());
    obj.setProperty("y", vertex.vecOrigin.y());
    obj.setProperty("z", vertex.vecOrigin.z());
    return obj;
}

void fromScriptValue_grVertex(const QScriptValue &obj, grVertex &vertex)
{
    vertex.vecOrigin.setX(obj.property("x").toVariant().toFloat());
    vertex.vecOrigin.setY(obj.property("y").toVariant().toFloat());
    vertex.vecOrigin.setZ(obj.property("z").toVariant().toFloat());
}

QScriptValue new_grEdge(QScriptContext *context, QScriptEngine *engine)
{
    grEdge edge;

    if(context->argumentCount() == 2)
    {
        edge.iFrom = context->argument(0).toInt32();
        edge.iTo = context->argument(1).toInt32();
    }

    return engine->toScriptValue(edge);
}

QScriptValue toScriptValue_grEdge(QScriptEngine *engine, const grEdge &edge)
{
    QScriptValue obj = engine->newObject();
    obj.setProperty("from", edge.iFrom);
    obj.setProperty("to", edge.iTo);
    return obj;
}

void fromScriptValue_grEdge(const QScriptValue &obj, grEdge &edge)
{
    edge.iFrom = obj.property("from").toInt32();
    edge.iTo = obj.property("to").toInt32();
}

ScriptManager::ScriptManager(QObject *parent) :
    QObject(parent)
{
    m_pScriptEngine = new QScriptEngine();

    QDir scriptsDirectory = QDir("scripts");
    QStringList filesList;

    filesList = scriptsDirectory.entryList(QStringList(QString("*.js")), QDir::Files);
    for(int i = 0; i < filesList.size(); i++)
    {
        QFile file;
        file.setFileName("scripts/" + filesList[i]);

        if(file.open(QIODevice::ReadOnly))
        {
            filesList[i].chop(3);

            QScriptValue globalObjectBackup = m_pScriptEngine->newObject();
            globalObjectBackup.setPrototype(m_pScriptEngine->globalObject().prototype());

            QScriptValueIterator it(m_pScriptEngine->globalObject());
            while(it.hasNext())
            {
                it.next();
                globalObjectBackup.setProperty(it.name(), it.value(), it.flags());
            }

            QString strScriptText = file.readAll();
            m_globalObjects[filesList[i]] = m_pScriptEngine->newObject();

            qScriptRegisterMetaType(m_pScriptEngine, toScriptValue_grVertex, fromScriptValue_grVertex);
            QScriptValue ctor_grVertex = m_pScriptEngine->newFunction(new_grVertex);
            m_pScriptEngine->globalObject().setProperty("grVertex", ctor_grVertex);

            qScriptRegisterMetaType(m_pScriptEngine, toScriptValue_grEdge, fromScriptValue_grEdge);
            QScriptValue ctor_grEdge = m_pScriptEngine->newFunction(new_grEdge);
            m_pScriptEngine->globalObject().setProperty("grEdge", ctor_grEdge);

            m_pScriptEngine->globalObject().setProperty("alert", m_pScriptEngine->newFunction(scriptAlert));
            m_pScriptEngine->globalObject().setProperty("confirm", m_pScriptEngine->newFunction(scriptConfirm));
            m_pScriptEngine->globalObject().setProperty("prompt", m_pScriptEngine->newFunction(scriptPrompt));

            m_pScriptEngine->globalObject().setProperty("CONNECTION_BOTH", m_pScriptEngine->toScriptValue(0), QScriptValue::ReadOnly);
            m_pScriptEngine->globalObject().setProperty("CONNECTION_FROM", m_pScriptEngine->toScriptValue(1), QScriptValue::ReadOnly);
            m_pScriptEngine->globalObject().setProperty("CONNECTION_TO", m_pScriptEngine->toScriptValue(2), QScriptValue::ReadOnly);

            QScriptValue valVGE2Namespace = m_pScriptEngine->newObject();
            valVGE2Namespace.setProperty("clearGraph", m_pScriptEngine->newFunction(scriptVGE2ClearGraph));
            valVGE2Namespace.setProperty("addVertices", m_pScriptEngine->newFunction(scriptVGE2AddVertices));
            valVGE2Namespace.setProperty("addEdges", m_pScriptEngine->newFunction(scriptVGE2AddEdges));
            valVGE2Namespace.setProperty("getGraph", m_pScriptEngine->newFunction(scriptVGE2GetGraph));
            valVGE2Namespace.setProperty("setGraph", m_pScriptEngine->newFunction(scriptVGE2SetGraph));
            valVGE2Namespace.setProperty("getEdgesOfVertex", m_pScriptEngine->newFunction(scriptVGE2GetEdgesOfVertex));
            valVGE2Namespace.setProperty("deleteVertex", m_pScriptEngine->newFunction(scriptVGE2DeleteVertex));
            valVGE2Namespace.setProperty("removeEdge", m_pScriptEngine->newFunction(scriptVGE2RemoveEdge));
            valVGE2Namespace.setProperty("getSelectedIndexes", m_pScriptEngine->newFunction(scriptVGE2GetSelectedIndexes));
            valVGE2Namespace.setProperty("getVertexByIndex", m_pScriptEngine->newFunction(scriptVGE2GetVertexByIndex));
            valVGE2Namespace.setProperty("markGraphsPart", m_pScriptEngine->newFunction(scriptVGE2MarkGraphsPart));

            m_pScriptEngine->globalObject().setProperty("VGE2", valVGE2Namespace);

            m_pScriptEngine->evaluate(strScriptText, QString(filesList[i] + QString(".js")));

            if(m_pScriptEngine->hasUncaughtException())
            {
                QMessageBox::critical(g_pWndMain, tr("Visual graph editor"), tr("File: %1\nLine: %2\n%3").arg(filesList[i] + ".js").arg(m_pScriptEngine->uncaughtExceptionLineNumber()).arg(m_pScriptEngine->uncaughtException().toString()));
                m_globalObjects.remove(filesList[i]);
            }
            else
            {
                m_globalObjects[filesList[i]].setPrototype(m_pScriptEngine->globalObject().prototype());

                QScriptValueIterator it(m_pScriptEngine->globalObject());
                while(it.hasNext())
                {
                    it.next();
                    m_globalObjects[filesList[i]].setProperty(it.name(), it.value(), it.flags());
                }
            }

            m_pScriptEngine->setGlobalObject(globalObjectBackup);
        }
        else
        {
            QMessageBox::critical(g_pWndMain, tr("Visual graph editor"), tr("Couldn't open script file: %1!").arg(filesList[i]));
        }
    }
}

void ScriptManager::onMenuClicked(QString strScriptName)
{
    m_pScriptEngine->collectGarbage();
    m_pScriptEngine->clearExceptions();

    m_pScriptEngine->setGlobalObject(m_globalObjects[strScriptName]);
    m_pScriptEngine->evaluate("onMenuClicked();");

    if(m_pScriptEngine->hasUncaughtException())
        QMessageBox::critical(g_pWndMain, tr("Visual graph editor"), tr("File: %1\nLine: %2\n%3").arg(strScriptName + ".js").arg(m_pScriptEngine->uncaughtExceptionLineNumber()).arg(m_pScriptEngine->uncaughtException().toString()));
}

QString ScriptManager::getDescription(QString strScriptName)
{
    m_pScriptEngine->collectGarbage();
    m_pScriptEngine->clearExceptions();

    m_pScriptEngine->setGlobalObject(m_globalObjects[strScriptName]);
    QScriptValue retValue = m_pScriptEngine->evaluate("getDescription();");

    if(m_pScriptEngine->hasUncaughtException())
    {
        QMessageBox::critical(g_pWndMain, tr("Visual graph editor"), tr("File: %1\nLine: %2\n%3").arg(strScriptName + ".js").arg(m_pScriptEngine->uncaughtExceptionLineNumber()).arg(m_pScriptEngine->uncaughtException().toString()));
        m_globalObjects.remove(strScriptName);
    }
    else
        return retValue.toString();

    return strScriptName + ".js";
}

int ScriptManager::getTypeOfScript(QString strScriptName)
{
    m_pScriptEngine->collectGarbage();
    m_pScriptEngine->clearExceptions();

    m_pScriptEngine->setGlobalObject(m_globalObjects[strScriptName]);
    QScriptValue retValue = m_pScriptEngine->evaluate("getTypeOfScript();");

    if(m_pScriptEngine->hasUncaughtException())
    {
        QMessageBox::critical(g_pWndMain, tr("Visual graph editor"), tr("File: %1\nLine: %2\n%3").arg(strScriptName + ".js").arg(m_pScriptEngine->uncaughtExceptionLineNumber()).arg(m_pScriptEngine->uncaughtException().toString()));
        m_globalObjects.remove(strScriptName);
    }
    else
    {
        int retValueInt32 = retValue.toInt32();
        if(retValueInt32 < 0 || retValueInt32 > 2)
        {
            m_globalObjects.remove(strScriptName);
            return -1;
        }
        else
            return retValueInt32;
    }

    return -1;
}

QStringList ScriptManager::getScriptsList()
{
    return m_globalObjects.uniqueKeys();
}
