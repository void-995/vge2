#ifndef GLDRAWER3D_H
#define GLDRAWER3D_H

// Qt
#include <QTime>
#include <QGLWidget>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QMatrix>

// Project
#include "wndmain.h"

// GLU
#include <GL/glu.h>

static const float pConstColorRed[4] = {1.0f, 0.0f, 0.0f, 1.0f};
static const float pConstColorGreen[4] = {0.0f, 1.0f, 0.0f, 1.0f};
static const float pConstColorBlue[4] = {0.0f, 0.0f, 1.0f, 1.0f};
static const float pConstColorYellow[4] = {1.0f, 1.0f, 0.0f, 1.0f};
static const float pConstColorBlack[4] = {0.0f, 0.0f, 0.0f, 1.0f};
static const float pConstColorWhite[4] = {1.0f, 1.0f, 1.0f, 1.0f};

static const QVector3D vecForward(0.0f, 1.0f, 0.0f);
static const QVector3D vecRight(1.0f, 0.0f, 0.0f);
static const QVector3D vecUp(0.0f, 0.0f, 1.0f);

class GLDrawer3D : public QGLWidget
{
    Q_OBJECT
public:
    explicit GLDrawer3D(QWidget *parent = 0);

private:
    void rebuildLinesVA();
    void recalcAxis();

    QTime m_renderTime;
    QVector<GLfloat> m_qvGraphLinesVA;
    GLUquadric *m_quadric;
    float m_pCameraPosition[4];
    float m_pColorCurrent[4];
    float m_flColorBlendK1;
    float m_flColorBlendK2;
    float m_flAngleH, m_flAngleV;

    QVector3D m_vecForwardMod;
    QVector3D m_vecRightMod;
    QVector3D m_vecUpMod;

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();
    void timerEvent(QTimerEvent *);
    void keyPressEvent(QKeyEvent *);

signals:

public slots:

};

#endif // GLDRAWER3D_H
