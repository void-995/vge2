// Qt
#include <QtOpenGL>

// Project
#include "wptdefs.h"
#include "gldrawer3d.h"

extern QVector<grVertex> qvGraphVertices;
extern QVector<grEdge> qvGraphEdges;
extern QVector<int> qvMarkedVerticesIndexes;
extern QVector<int> qvMarkedEdgesIndexes;

extern int selectedPointNumFst;
extern int selectedPointNumSnd;

extern bool needToRebuildRD;

extern WndMain *g_pWndMain;
extern GLDrawer3D *g_pGLDrawer3D;

GLDrawer3D::GLDrawer3D(QWidget *parent): QGLWidget(parent)
{
    m_qvGraphLinesVA.clear();
    m_renderTime.start();
    startTimer(25);

    m_quadric = gluNewQuadric();

    m_pCameraPosition[0] = 0.0f;
    m_pCameraPosition[1] = 0.0f;
    m_pCameraPosition[2] = 0.0f;
    m_pCameraPosition[3] = 1.0f; // const!

    m_pColorCurrent[3] = 1.0f; // const!

    m_flAngleH = 0.0f;
    m_flAngleV = 0.0f;
    recalcAxis();

    g_pGLDrawer3D = this;
}

void GLDrawer3D::initializeGL()
{
    glShadeModel(GL_SMOOTH);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnableClientState(GL_VERTEX_ARRAY);
}

void GLDrawer3D::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (GLfloat)w/(GLfloat)h, 0.1f, 8192.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glScalef(1.0f, -1.0f, 1.0f);

    gluLookAt(m_pCameraPosition[0],
              m_pCameraPosition[1],
              m_pCameraPosition[2],
              m_pCameraPosition[0] + m_vecForwardMod.x(),
              m_pCameraPosition[1] + m_vecForwardMod.y(),
              m_pCameraPosition[2] + m_vecForwardMod.z(),
              m_vecUpMod.x(),
              m_vecUpMod.y(),
              m_vecUpMod.z());
}

void GLDrawer3D::recalcAxis()
{
    QMatrix4x4 matRotation;
    matRotation.rotate(m_flAngleH, 0.0f, 0.0f, 1.0f);

    m_vecForwardMod = vecForward * matRotation;
    m_vecRightMod = vecRight * matRotation;
    m_vecUpMod = vecUp * matRotation;
}

void GLDrawer3D::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if(needToRebuildRD)
    {
        rebuildLinesVA();
        recalcAxis();
        resizeGL(this->width(), this->height());
        needToRebuildRD = false;
    }

    glLightfv(GL_LIGHT0, GL_POSITION, m_pCameraPosition);

    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);

    glLightfv(GL_LIGHT0, GL_DIFFUSE, pConstColorWhite);
    glLightfv(GL_LIGHT0, GL_AMBIENT, pConstColorWhite);
    if(!qvGraphEdges.empty())
    {
        glVertexPointer(3, GL_FLOAT, 0, (GLfloat *)m_qvGraphLinesVA.data());
        for(int i = 0; i < qvGraphEdges.size(); i++)
        {
            glDrawArrays(GL_LINES, i * 2, 2);
        }
    }

    glLightfv(GL_LIGHT0, GL_DIFFUSE, pConstColorRed);
    glLightfv(GL_LIGHT0, GL_AMBIENT, pConstColorBlack);
    for(int i = 0; i < qvGraphVertices.size(); i++)
    {
        glPushMatrix();
            glTranslatef(qvGraphVertices[i].vecOrigin.x(), qvGraphVertices[i].vecOrigin.y(), qvGraphVertices[i].vecOrigin.z());
            gluSphere(m_quadric, 4.0, 16, 16);
        glPopMatrix();
    }

    if(!qvMarkedEdgesIndexes.empty())
    {
        glLightfv(GL_LIGHT0, GL_DIFFUSE, pConstColorBlue);
        glLightfv(GL_LIGHT0, GL_AMBIENT, pConstColorGreen);

        for(int i = 0; i < qvMarkedEdgesIndexes.size(); i++)
            glDrawArrays(GL_LINES, qvMarkedEdgesIndexes[i] * 2, 2);
    }

    if(!qvMarkedVerticesIndexes.empty())
    {
        m_flColorBlendK1 = fabs(sin(m_renderTime.elapsed() * 0.00390625f));
        m_flColorBlendK2 = fabs(cos(m_renderTime.elapsed() * 0.00390625f));

        m_pColorCurrent[0] = (pConstColorBlue[0] * m_flColorBlendK1) + (pConstColorRed[0] * m_flColorBlendK2);
        m_pColorCurrent[1] = (pConstColorBlue[1] * m_flColorBlendK1) + (pConstColorRed[1] * m_flColorBlendK2);
        m_pColorCurrent[2] = (pConstColorBlue[2] * m_flColorBlendK1) + (pConstColorRed[2] * m_flColorBlendK2);

        glLightfv(GL_LIGHT0, GL_DIFFUSE, m_pColorCurrent);
        glLightfv(GL_LIGHT0, GL_AMBIENT, pConstColorBlack);
        for(int i = 0; i < qvMarkedVerticesIndexes.size(); i++)
        {
            glPushMatrix();
                glTranslatef(qvGraphVertices[qvMarkedVerticesIndexes[i]].vecOrigin.x(), qvGraphVertices[qvMarkedVerticesIndexes[i]].vecOrigin.y(), qvGraphVertices[qvMarkedVerticesIndexes[i]].vecOrigin.z());
                gluSphere(m_quadric, 4.0, 16, 16);
            glPopMatrix();
        }
    }

    if(selectedPointNumFst != -1 && selectedPointNumSnd == -1)
    {
        m_flColorBlendK1 = fabs(sin(m_renderTime.elapsed() * 0.00390625f));
        m_flColorBlendK2 = fabs(cos(m_renderTime.elapsed() * 0.00390625f));

        m_pColorCurrent[0] = (pConstColorYellow[0] * m_flColorBlendK1) + (pConstColorRed[0] * m_flColorBlendK2);
        m_pColorCurrent[1] = (pConstColorYellow[1] * m_flColorBlendK1) + (pConstColorRed[1] * m_flColorBlendK2);
        m_pColorCurrent[2] = (pConstColorYellow[2] * m_flColorBlendK1) + (pConstColorRed[2] * m_flColorBlendK2);

        glLightfv(GL_LIGHT0, GL_DIFFUSE, m_pColorCurrent);

        glPushMatrix();
            glTranslatef(qvGraphVertices[selectedPointNumFst].vecOrigin.x(), qvGraphVertices[selectedPointNumFst].vecOrigin.y(), qvGraphVertices[selectedPointNumFst].vecOrigin.z());
            gluSphere(m_quadric, 4.0, 16, 16);
        glPopMatrix();
    }

    if(selectedPointNumFst != -1 && selectedPointNumSnd != -1)
    {
        m_flColorBlendK1 = fabs(sin(m_renderTime.elapsed() * 0.00390625f));
        m_flColorBlendK2 = fabs(cos(m_renderTime.elapsed() * 0.00390625f));

        m_pColorCurrent[0] = (pConstColorGreen[0] * m_flColorBlendK1) + (pConstColorRed[0] * m_flColorBlendK2);
        m_pColorCurrent[1] = (pConstColorGreen[1] * m_flColorBlendK1) + (pConstColorRed[1] * m_flColorBlendK2);
        m_pColorCurrent[2] = (pConstColorGreen[2] * m_flColorBlendK1) + (pConstColorRed[2] * m_flColorBlendK2);

        glLightfv(GL_LIGHT0, GL_DIFFUSE, m_pColorCurrent);

        glPushMatrix();
            glTranslatef(qvGraphVertices[selectedPointNumFst].vecOrigin.x(), qvGraphVertices[selectedPointNumFst].vecOrigin.y(), qvGraphVertices[selectedPointNumFst].vecOrigin.z());
            gluSphere(m_quadric, 4.0, 16, 16);
        glPopMatrix();

        glPushMatrix();
            glTranslatef(qvGraphVertices[selectedPointNumSnd].vecOrigin.x(), qvGraphVertices[selectedPointNumSnd].vecOrigin.y(), qvGraphVertices[selectedPointNumSnd].vecOrigin.z());
            gluSphere(m_quadric, 4.0, 16, 16);
        glPopMatrix();
    }
}

void GLDrawer3D::keyPressEvent(QKeyEvent *event)
{
    int keyPressed = event->key();
    switch(keyPressed)
    {
    case Qt::Key_Left:
        m_flAngleH -= 5.0f;
        needToRebuildRD = true;
        break;

    case Qt::Key_Right:
        m_flAngleH += 5.0f;
        needToRebuildRD = true;
        break;

    case Qt::Key_Up:
        m_flAngleV += 5.0f;
        needToRebuildRD = true;
        break;

    case Qt::Key_Down:
        m_flAngleV -= 5.0f;
        needToRebuildRD = true;
        break;

    case Qt::Key_A:
        m_pCameraPosition[0] -= m_vecRightMod.x() * 10;
        m_pCameraPosition[1] -= m_vecRightMod.y() * 10;
        m_pCameraPosition[2] -= m_vecRightMod.z() * 10;
        needToRebuildRD = true;
        break;

    case Qt::Key_D:
        m_pCameraPosition[0] += m_vecRightMod.x() * 10;
        m_pCameraPosition[1] += m_vecRightMod.y() * 10;
        m_pCameraPosition[2] += m_vecRightMod.z() * 10;
        needToRebuildRD = true;
        break;

    case Qt::Key_W:
        m_pCameraPosition[0] += m_vecForwardMod.x() * 10;
        m_pCameraPosition[1] += m_vecForwardMod.y() * 10;
        m_pCameraPosition[2] += m_vecForwardMod.z() * 10;
        needToRebuildRD = true;
        break;

    case Qt::Key_S:
        m_pCameraPosition[0] -= m_vecForwardMod.x() * 10;
        m_pCameraPosition[1] -= m_vecForwardMod.y() * 10;
        m_pCameraPosition[2] -= m_vecForwardMod.z() * 10;
        needToRebuildRD = true;
        break;

    case Qt::Key_C:
        m_pCameraPosition[0] += m_vecUpMod.x() * 10;
        m_pCameraPosition[1] += m_vecUpMod.y() * 10;
        m_pCameraPosition[2] += m_vecUpMod.z() * 10;
        needToRebuildRD = true;
        break;

    case Qt::Key_Space:
        m_pCameraPosition[0] -= m_vecUpMod.x() * 10;
        m_pCameraPosition[1] -= m_vecUpMod.y() * 10;
        m_pCameraPosition[2] -= m_vecUpMod.z() * 10;
        needToRebuildRD = true;
        break;
    }
}

void GLDrawer3D::timerEvent(QTimerEvent *)
{
    update();
}

void GLDrawer3D::rebuildLinesVA()
{
    m_qvGraphLinesVA.clear();
    for(int i = 0; i < qvGraphEdges.size(); i++)
    {
        QVector3D vecA = qvGraphVertices[qvGraphEdges[i].iFrom].vecOrigin;
        QVector3D vecB = qvGraphVertices[qvGraphEdges[i].iTo].vecOrigin;

        m_qvGraphLinesVA.push_back(vecA.x());
        m_qvGraphLinesVA.push_back(vecA.y());
        m_qvGraphLinesVA.push_back(vecA.z());

        m_qvGraphLinesVA.push_back(vecB.x());
        m_qvGraphLinesVA.push_back(vecB.y());
        m_qvGraphLinesVA.push_back(vecB.z());
    }
}
