// Qt
#include <QtGui>
#include <QVector>
#include <QMap>
#include <QMessageBox>
#include <QFile>
#include <QInputDialog>
#include <QFileDialog>
#include <QTranslator>
#include <QSettings>
#include <QtScript/QScriptEngine>

// Project
#include "wptdefs.h"
#include "wndmain.h"
#include "gldrawer3d.h"
#include "ui_wndmain.h"
#include "scriptmanager.h"

QVector<grVertex> qvGraphVertices;
QVector<grEdge> qvGraphEdges;
QVector<int> qvMarkedVerticesIndexes;
QVector<int> qvMarkedEdgesIndexes;

int selectedPointNumFst;
int selectedPointNumSnd;
unsigned int lastVertexIndex;

bool needToRebuildRD;

WndMain *g_pWndMain;
GLDrawer3D *g_pGLDrawer3D;

int globalStackIsModified;

extern QTranslator g_translator;
extern QSettings g_settings;

bool actionTypeLessThan(const QAction *a1, const QAction *a2)
{
    return a1->property("type").toInt() < a2->property("type").toInt();
}

WndMain::WndMain(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::WndMain)
{
    g_pWndMain = this;
    ui->setupUi(this);

    lastVertexIndex = 0;
    globalStackIsModified = 0;

    ui->glDrawerTop->setViewportType(GLDrawer2D::Top);
    ui->glDrawerFront->setViewportType(GLDrawer2D::Front);
    ui->glDrawerSide->setViewportType(GLDrawer2D::Side);

    ui->vSBTop->setValue(32768 - 128);
    on_vSBTop_valueChanged(32768 - 128);
    ui->hSBTop->setValue(32768 - 128);
    on_hSBTop_valueChanged(32768 - 128);

    ui->vSBFront->setValue(32768 - 128);
    on_vSBFront_valueChanged(32768 - 128);
    ui->hSBFront->setValue(32768 - 128);
    on_hSBFront_valueChanged(32768 - 128);

    ui->vSBSide->setValue(32768 - 128);
    on_vSBSide_valueChanged(32768 - 128);
    ui->hSBSide->setValue(32768 - 128);
    on_hSBSide_valueChanged(32768 - 128);

    needToRebuildRD = false;

    connect(ui->actionConnect, SIGNAL(triggered()), ui->glDrawerTop, SLOT(connectVertices()));
    connect(ui->actionDisconnect, SIGNAL(triggered()), ui->glDrawerTop, SLOT(disconnectVertices()));
    connect(ui->actionDelete, SIGNAL(triggered()), ui->glDrawerTop, SLOT(deleteVertex()));
    connect(ui->actionAlignAllToGrid, SIGNAL(triggered()), ui->glDrawerTop, SLOT(alignAllToGrid()));

    setCurrentFile(QString());

    m_pScriptManager = new ScriptManager(this);
    m_qvGraphMenuItems.clear();

    QStringList lstScripts = m_pScriptManager->getScriptsList();
    ui->menuGraph->setEnabled(lstScripts.size() > 0);

    for(int i = 0; i < lstScripts.size(); i++)
    {
        QAction *pGraphAction = ui->menuGraph->addAction(lstScripts[i]);
        pGraphAction->setEnabled(m_pScriptManager->getTypeOfScript(lstScripts[i]) == 0);
        pGraphAction->setText(m_pScriptManager->getDescription(lstScripts[i]));
        pGraphAction->setProperty("tag", lstScripts[i]);
        pGraphAction->setProperty("type", m_pScriptManager->getTypeOfScript(lstScripts[i]));
        connect(pGraphAction, SIGNAL(triggered()), this, SLOT(launchPlugin()));
        m_qvGraphMenuItems.push_back(pGraphAction);
    }

    QList<QAction *> actions = ui->menuGraph->actions();
    qSort(actions.begin(), actions.end(), actionTypeLessThan);

    for(int i = 0; i < actions.size(); i++)
        ui->menuGraph->removeAction(actions[i]);

    for(int i = 0; i < actions.size(); i++)
    {
        ui->menuGraph->addAction(actions[i]);
        if(i < actions.size() - 1)
        {
            if(actions[i]->property("type").toInt() != actions[i + 1]->property("type").toInt())
            {
                QAction *pSeparatorAction = ui->menuGraph->addAction(QString());
                pSeparatorAction->setSeparator(true);
                ui->menuGraph->addAction(pSeparatorAction);
            }
        }
    }

    QDir appDirectory = QDir();
    QStringList langList;

    langList = appDirectory.entryList(QStringList("grapheditor2_*.qm"), QDir::Files | QDir::NoSymLinks);
    langList.replaceInStrings(QString(".qm"), QString());

    if(!langList.empty())
        ui->actionEnglish->setVisible(false);

    m_qvLangMenuItems.clear();
    for(int i = 0; i < langList.size(); i++)
    {
        QAction *pLangAction = ui->menuLanguage->addAction(langList[i]);
        pLangAction->setCheckable(true);
        pLangAction->setChecked(g_settings.value("language", "grapheditor2_en").toString() == langList[i]);
        pLangAction->setText(tr(langList[i].toLocal8Bit()));
        pLangAction->setProperty("tag", langList[i]);
        connect(pLangAction, SIGNAL(triggered()), this, SLOT(changeLanguage()));
        m_qvLangMenuItems.push_back(pLangAction);
    }

    QString dummyForTranslation[4] = {tr("grapheditor2_en"), tr("grapheditor2_ru"), tr("grapheditor2_ua")};
    dummyForTranslation[3] = QString();

    // Don't use the StyleSheet on non-Windows OS.
#ifndef _WIN32
    this->setStyleSheet(QString());
#endif // _WIN32
}

WndMain::~WndMain()
{
    delete ui;
}

void WndMain::updateIsModified(bool isModified)
{
    m_isModified = isModified;

    QString shownName = m_curFile;
    if(m_curFile.isEmpty())
        shownName = "untitled.gef";
    else
        shownName = strippedName(shownName);

    setWindowTitle(tr("Visual graph editor - %1 %2").arg(shownName).arg(m_isModified ? "*" : QString()));

    qvMarkedVerticesIndexes.clear();
    qvMarkedEdgesIndexes.clear();
    needToRebuildRD = true;
}

void WndMain::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        updateIsModified(m_isModified);
        break;
    default:
        break;
    }
}

void WndMain::changeLanguage()
{
    qApp->removeTranslator(&g_translator);
    g_translator.load(sender()->property("tag").toString());
    g_settings.setValue("language", sender()->property("tag").toString());
    qApp->installTranslator(&g_translator);

    for(int i = 0; i < m_qvLangMenuItems.size(); i++)
    {
        m_qvLangMenuItems[i]->setChecked(false);
        m_qvLangMenuItems[i]->setText(tr(m_qvLangMenuItems[i]->property("tag").toString().toLocal8Bit()));
    }

    sender()->setProperty("checked", true);
}

void WndMain::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
        event->accept();
    else
        event->ignore();
}

bool WndMain::maybeSave()
{
    if(m_isModified)
    {
        QMessageBox::StandardButton ret;
        ret = QMessageBox::warning(this, tr("Visual graph editor"),
                     tr("The document has been modified.\n"
                        "Do you want to save your changes?"),
                     QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

        if(ret == QMessageBox::Save)
            return on_actionSave_triggered();
        else if(ret == QMessageBox::Cancel)
            return false;
    }
    return true;
}

void WndMain::loadFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Visual graph editor"),
                             tr("Cannot read file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return;
    }

    QByteArray arrBytes = fileName.toLocal8Bit();

    char *szFileName = new char[2048];
    memset(szFileName, 0, sizeof(char) * 2048);
    memcpy(szFileName, arrBytes.data(), arrBytes.size());

    FILE *f_in;
    f_in = fopen(szFileName, "rb");
    if(f_in)
    {
        selectedPointNumFst = -1;
        selectedPointNumSnd = -1;
        qvGraphVertices.clear();
        qvGraphEdges.clear();
        qvMarkedVerticesIndexes.clear();
        qvMarkedEdgesIndexes.clear();

        int dummy = 0;

        header wptHeader;
        dummy = fread(&wptHeader, sizeof(wptHeader), 1, f_in);

        for(int i = 0; i < wptHeader.iPointsCount; i++)
        {
            grVertex pTmp;
            dummy = fread(&pTmp, sizeof(grVertex), 1, f_in);
            qvGraphVertices.push_back(pTmp);
        }
        for(int i = 0; i < wptHeader.iLinksCount; i++)
        {
            grEdge lTmp;
            dummy = fread(&lTmp, sizeof(grEdge), 1, f_in);
            qvGraphEdges.push_back(lTmp);
        }
        fclose(f_in);

        needToRebuildRD = true;
        on_hSBTop_valueChanged(wptHeader.iHTopOffset + 32768);
        ui->hSBTop->setValue(wptHeader.iHTopOffset + 32768);
        on_vSBTop_valueChanged(wptHeader.iVTopOffset + 32768);
        ui->vSBTop->setValue(wptHeader.iVTopOffset + 32768);
        on_hSBFront_valueChanged(wptHeader.iHFrontOffset + 32768);
        ui->hSBFront->setValue(wptHeader.iHFrontOffset + 32768);
        on_vSBFront_valueChanged(wptHeader.iVFrontOffset + 32768);
        ui->vSBFront->setValue(wptHeader.iVFrontOffset + 32768);
        on_hSBSide_valueChanged(wptHeader.iHSideOffset + 32768);
        ui->hSBSide->setValue(wptHeader.iHSideOffset + 32768);
        on_vSBSide_valueChanged(wptHeader.iVSideOffset + 32768);
        ui->vSBSide->setValue(wptHeader.iVSideOffset + 32768);
    }

    delete szFileName;

    setCurrentFile(fileName);
}

bool WndMain::saveFile(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
        QMessageBox::warning(this, tr("Visual graph editor"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(file.errorString()));
        return false;
    }

    QByteArray arrBytes = fileName.toLocal8Bit();

    char *szFileName = new char[2048];
    memset(szFileName, 0, sizeof(char) * 2048);
    memcpy(szFileName, arrBytes.data(), arrBytes.size());

    FILE *f_out;
    f_out = fopen(szFileName, "wb");
    if(f_out)
    {
        header wptHeader;
        wptHeader.iPointsCount = qvGraphVertices.size();
        wptHeader.iLinksCount = qvGraphEdges.size();
        wptHeader.iHTopOffset = ui->hSBTop->value() - 32768;
        wptHeader.iVTopOffset = ui->vSBTop->value() - 32768;
        wptHeader.iHFrontOffset = ui->hSBFront->value() - 32768;
        wptHeader.iVFrontOffset = ui->vSBFront->value() - 32768;
        wptHeader.iHSideOffset = ui->hSBSide->value() - 32768;
        wptHeader.iVSideOffset = ui->vSBSide->value() - 32768;
        (void)fwrite(&wptHeader, sizeof(wptHeader), 1, f_out);
        for(int i = 0; i < qvGraphVertices.size(); i++)
                (void)fwrite(&qvGraphVertices[i], sizeof(grVertex), 1, f_out);
        for(int i = 0; i < qvGraphEdges.size(); i++)
                (void)fwrite(&qvGraphEdges[i], sizeof(grEdge), 1, f_out);
        fclose(f_out);
    }

    delete szFileName;

    setCurrentFile(fileName);
    return true;
}

void WndMain::setCurrentFile(const QString &fileName)
{
    m_curFile = fileName;
    updateIsModified(false);
}

QString WndMain::strippedName(const QString &fullFileName)
{
    QString strResult = QFileInfo(fullFileName).fileName();
    return strResult;
}

void WndMain::setOneVertexActionsEnabled(bool enabled)
{
    this->ui->actionDelete->setEnabled(enabled);
    this->ui->actionRenameVertex->setEnabled(enabled);

    for(int i = 0; i < m_qvGraphMenuItems.size(); i++)
    {
        if(m_pScriptManager->getTypeOfScript(m_qvGraphMenuItems[i]->property("tag").toString()) == 1)
            m_qvGraphMenuItems[i]->setEnabled(enabled);
    }
}

void WndMain::setTwoVertexActionsEnabled(bool enabled)
{
    this->ui->actionConnect->setEnabled(enabled);
    this->ui->actionDisconnect->setEnabled(enabled);

    for(int i = 0; i < m_qvGraphMenuItems.size(); i++)
    {
        if(m_pScriptManager->getTypeOfScript(m_qvGraphMenuItems[i]->property("tag").toString()) == 2)
            m_qvGraphMenuItems[i]->setEnabled(enabled);
    }
}

bool WndMain::getIsGridModeEnabled()
{
    return this->ui->actionAlignToGridMode->isChecked();
}

void WndMain::setIsGridModeEnabled(bool enabled)
{
    this->ui->actionAlignToGridMode->setChecked(enabled);
}

bool WndMain::getIsAutoconnectModeEnabled()
{
    return this->ui->actionAutoconnect->isChecked();
}

void WndMain::on_actionAboutQt_triggered()
{
    qApp->aboutQt();
}

void WndMain::on_vSBTop_valueChanged(int value)
{
    ui->glDrawerTop->setVOffset(value - 32768);
}

void WndMain::on_hSBTop_valueChanged(int value)
{
    ui->glDrawerTop->setHOffset(value - 32768);
}

void WndMain::on_vSBFront_valueChanged(int value)
{
    ui->glDrawerFront->setVOffset(value - 32768);
}

void WndMain::on_hSBFront_valueChanged(int value)
{
    ui->glDrawerFront->setHOffset(value - 32768);
}

void WndMain::on_vSBSide_valueChanged(int value)
{
    ui->glDrawerSide->setVOffset(value - 32768);
}

void WndMain::on_hSBSide_valueChanged(int value)
{
    ui->glDrawerSide->setHOffset(value - 32768);
}

void WndMain::on_actionNew_triggered()
{
    if(maybeSave())
    {
        qvGraphVertices.clear();
        qvGraphEdges.clear();
        qvMarkedVerticesIndexes.clear();
        qvMarkedEdgesIndexes.clear();
        lastVertexIndex = 0;

        needToRebuildRD = true;
        on_hSBTop_valueChanged(32768 - 128);
        ui->hSBTop->setValue(32768 - 128);
        on_vSBTop_valueChanged(32768 - 128);
        ui->vSBTop->setValue(32768 - 128);

        on_hSBFront_valueChanged(32768 - 128);
        ui->hSBFront->setValue(32768 - 128);
        on_vSBFront_valueChanged(32768 - 128);
        ui->vSBFront->setValue(32768 - 128);

        on_hSBSide_valueChanged(32768 - 128);
        ui->hSBSide->setValue(32768 - 128);
        on_vSBSide_valueChanged(32768 - 128);
        ui->vSBSide->setValue(32768 - 128);

        setCurrentFile(QString());
    }
}

void WndMain::on_actionOpen_triggered()
{
    if(maybeSave())
    {
        QString fileName = QFileDialog::getOpenFileName(this, QString(), m_curFile, QString("GraphEditor2 file (*.gef)"));
        if(!fileName.isEmpty())
            loadFile(fileName);
    }
}

bool WndMain::on_actionSave_triggered()
{
    if(m_curFile.isEmpty())
        return on_actionSaveAs_triggered();
    else
        return saveFile(m_curFile);
}

bool WndMain::on_actionSaveAs_triggered()
{
    QString fileName = QFileDialog::getSaveFileName(this, QString(), m_curFile, QString("GraphEditor2 file (*.gef)"));
    if (fileName.isEmpty())
        return false;

    return saveFile(fileName);
}

void WndMain::launchPlugin()
{
    m_pScriptManager->onMenuClicked(sender()->property("tag").toString());

    selectedPointNumFst = -1;
    selectedPointNumSnd = -1;
    setOneVertexActionsEnabled(false);
    setTwoVertexActionsEnabled(false);
}

void WndMain::on_toolBox_currentChanged(int index)
{
    needToRebuildRD = true;

    if(index == 3)
        g_pGLDrawer3D->grabKeyboard();
    else
        g_pGLDrawer3D->releaseKeyboard();
}

void WndMain::on_actionRenameVertex_triggered()
{
    int index = selectedPointNumFst;
    selectedPointNumFst = -1;

    QString strInfo = QString().fromUtf8(qvGraphVertices[index].szInfo);

    bool isOKPressed;
    QString strResult = QInputDialog::getText(this, tr("Visual graph editor"),
                                              tr("Please, enter new name for vertex here:\t"), QLineEdit::Normal,
                                      strInfo, &isOKPressed);
    if(isOKPressed)
        strcpy(qvGraphVertices[index].szInfo, strResult.toUtf8().constData());
}
