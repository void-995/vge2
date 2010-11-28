#ifndef SCRIPTMANAGER_H
#define SCRIPTMANAGER_H

// Qt
#include <QObject>
#include <QScriptEngine>
#include <QScriptContext>
#include <QFile>

// Project
#include "wptdefs.h"

class ScriptManager : public QObject
{
    Q_OBJECT
public:
    explicit ScriptManager(QObject *parent = 0);

    void onMenuClicked(QString strScriptName);
    QString getDescription(QString strScriptName);
    int getTypeOfScript(QString strScriptName);

    QStringList getScriptsList();

private:
    QMap<QString, QScriptValue> m_globalObjects;
    QScriptEngine *m_pScriptEngine;
};

#endif // SCRIPTMANAGER_H
