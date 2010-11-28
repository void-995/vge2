#ifndef WNDMAIN_H
#define WNDMAIN_H

// Qt
#include <QString>
#include <QVector>
#include <QMainWindow>

// Project
#include "scriptmanager.h"

namespace Ui {
    class WndMain;
}

class WndMain : public QMainWindow {
    Q_OBJECT
public:
    WndMain(QWidget *parent = 0);
    ~WndMain();

    void setOneVertexActionsEnabled(bool);
    void setTwoVertexActionsEnabled(bool);

    bool getIsGridModeEnabled();
    void setIsGridModeEnabled(bool);
    bool getIsAutoconnectModeEnabled();

    void updateIsModified(bool);

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *);

private:
    bool maybeSave();
    void loadFile(const QString &fileName);
    bool saveFile(const QString &fileName);
    void setCurrentFile(const QString &fileName);
    QString strippedName(const QString &fullFileName);

    ScriptManager *m_pScriptManager;
    Ui::WndMain *ui;
    bool m_isModified;
    QString m_curFile;
    QVector<QAction *> m_qvGraphMenuItems;
    QVector<QAction *> m_qvLangMenuItems;

private slots:
    void on_actionRenameVertex_triggered();
    void on_toolBox_currentChanged(int index);
    bool on_actionSaveAs_triggered();
    bool on_actionSave_triggered();
    void on_actionOpen_triggered();
    void on_actionNew_triggered();
    void on_hSBTop_valueChanged(int value);
    void on_vSBTop_valueChanged(int value);
    void on_hSBFront_valueChanged(int value);
    void on_vSBFront_valueChanged(int value);
    void on_hSBSide_valueChanged(int value);
    void on_vSBSide_valueChanged(int value);
    void on_actionAboutQt_triggered();
    void launchPlugin();
    void changeLanguage();
};

#endif // WNDMAIN_H
