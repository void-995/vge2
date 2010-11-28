// Qt
#include <QtOpenGL>

// Project
#include "wptdefs.h"
#include "gldrawer.h"

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

GLDrawer2D::GLDrawer2D(QWidget *parent) :
        QGLWidget(parent)
{
    m_qvGraphVerticesVA.clear();
    m_qvGraphVerticesTCA.clear();
    m_qvGraphArrowsVA.clear();
    m_qvGraphArrowsTCA.clear();
    m_qvGraphLinesVA.clear();

    selectedPointNumFst = -1;
    selectedPointNumSnd = -1;
    m_VOffset = 0;
    m_HOffset = 0;
    startTimer(25);

    m_renderTime.start();
}

void GLDrawer2D::setVOffset(int value)
{
    m_VOffset = value;
    resizeGL(this->width(), this->height());
}

void GLDrawer2D::setHOffset(int value)
{
    m_HOffset = value;
    resizeGL(this->width(), this->height());
}

void GLDrawer2D::initializeGL()
{
    glClearColor(0.125f, 0.125f, 0.125f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_TEXTURE_2D);

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    m_idTextureCircle = bindTexture(QPixmap(QString(":/resource/images/graph-vertex.png")), GL_TEXTURE_2D);
    m_idTextureArrow = bindTexture(QPixmap(QString(":/resource/images/graph-arrow.png")), GL_TEXTURE_2D);

    m_idTextureCircleRed = bindTexture(QPixmap(QString(":/resource/images/graph-vertex-red.png")), GL_TEXTURE_2D);
    m_idTextureArrowRed = bindTexture(QPixmap(QString(":/resource/images/graph-arrow-red.png")), GL_TEXTURE_2D);

    m_idTextureCircleYellow = bindTexture(QPixmap(QString(":/resource/images/graph-vertex-yellow.png")), GL_TEXTURE_2D);
    m_idTextureArrowYellow = bindTexture(QPixmap(QString(":/resource/images/graph-arrow-yellow.png")), GL_TEXTURE_2D);

    m_idTextureCircleGreen = bindTexture(QPixmap(QString(":/resource/images/graph-vertex-green.png")), GL_TEXTURE_2D);
    m_idTextureArrowGreen = bindTexture(QPixmap(QString(":/resource/images/graph-arrow-green.png")), GL_TEXTURE_2D);

    m_idTextureCircleBlue = bindTexture(QPixmap(QString(":/resource/images/graph-vertex-blue.png")), GL_TEXTURE_2D);
    m_idTextureArrowBlue = bindTexture(QPixmap(QString(":/resource/images/graph-arrow-blue.png")), GL_TEXTURE_2D);

    m_idTextureGrid = bindTexture(QPixmap(QString(":/resource/images/graph-grid.png")), GL_TEXTURE_2D);
}

void GLDrawer2D::resizeGL(int w, int h)
{
    m_qvGraphGridBackgroundVA.clear();
    m_qvGraphGridBackgroundTCA.clear();

    m_qvGraphGridBackgroundTCA.push_back(m_HOffset * 0.01f);
    m_qvGraphGridBackgroundTCA.push_back(m_VOffset * 0.01f);
    m_qvGraphGridBackgroundVA.push_back(0.0f);
    m_qvGraphGridBackgroundVA.push_back(0.0f);

    m_qvGraphGridBackgroundTCA.push_back((float)this->width() * 0.01f + m_HOffset * 0.01f);
    m_qvGraphGridBackgroundTCA.push_back(m_VOffset * 0.01f);
    m_qvGraphGridBackgroundVA.push_back((float)this->width());
    m_qvGraphGridBackgroundVA.push_back(0.0f);

    m_qvGraphGridBackgroundTCA.push_back((float)this->width() * 0.01f + m_HOffset * 0.01f);
    m_qvGraphGridBackgroundTCA.push_back((float)this->height() * 0.01f + m_VOffset * 0.01f);
    m_qvGraphGridBackgroundVA.push_back((float)this->width());
    m_qvGraphGridBackgroundVA.push_back((float)this->height());

    m_qvGraphGridBackgroundTCA.push_back(m_HOffset * 0.01f);
    m_qvGraphGridBackgroundTCA.push_back(m_VOffset * 0.01f);
    m_qvGraphGridBackgroundVA.push_back(0.0f);
    m_qvGraphGridBackgroundVA.push_back(0.0f);

    m_qvGraphGridBackgroundTCA.push_back(m_HOffset * 0.01f);
    m_qvGraphGridBackgroundTCA.push_back((float)this->height() * 0.01f + m_VOffset * 0.01f);
    m_qvGraphGridBackgroundVA.push_back(0.0f);
    m_qvGraphGridBackgroundVA.push_back((float)this->height());

    m_qvGraphGridBackgroundTCA.push_back((float)this->width() * 0.01f + m_HOffset * 0.01f);
    m_qvGraphGridBackgroundTCA.push_back((float)this->height() * 0.01f + m_VOffset * 0.01f);
    m_qvGraphGridBackgroundVA.push_back((float)this->width());
    m_qvGraphGridBackgroundVA.push_back((float)this->height());

    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (double)w, (double)h, 0.0, 0.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(-(float)m_HOffset, -(float)m_VOffset, 0.0f);
}

void GLDrawer2D::paintGL()
{
    if(needToRebuildRD)
    {
        rebuildVerticesVA();
        rebuildLinesVA();
        resizeGL(this->width(), this->height());
        needToRebuildRD = false;
    }

    glClear(GL_COLOR_BUFFER_BIT);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

    glPushMatrix();
    glLoadIdentity();

    glVertexPointer(2, GL_FLOAT, 0, (GLfloat *)m_qvGraphGridBackgroundVA.data());
    glTexCoordPointer(2, GL_FLOAT, 0, (GLfloat *)m_qvGraphGridBackgroundTCA.data());

    glBindTexture(GL_TEXTURE_2D, m_idTextureGrid);
    glDrawArrays(GL_TRIANGLES, 0, m_qvGraphGridBackgroundVA.size() / 2);

    glPopMatrix();

    if(!qvGraphEdges.empty())
    {
        glVertexPointer(2, GL_FLOAT, 0, (GLfloat *)m_qvGraphLinesVA.data());
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glBindTexture(GL_TEXTURE_2D, 0);
        for(int i = 0; i < qvGraphEdges.size(); i++)
        {
            glDrawArrays(GL_LINES, i * 2, 2);
        }

        if(!qvMarkedEdgesIndexes.empty())
        {
            glColor4f(0.65f, 0.65f, 1.0f, 1.0f);
            for(int i = 0; i < qvMarkedEdgesIndexes.size(); i++)
                glDrawArrays(GL_LINES, qvMarkedEdgesIndexes[i] * 2, 2);
        }

        glVertexPointer(2, GL_FLOAT, 0, (GLfloat *)m_qvGraphArrowsVA.data());
        glTexCoordPointer(2, GL_FLOAT, 0, (GLfloat *)m_qvGraphArrowsTCA.data());

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glBindTexture(GL_TEXTURE_2D, m_idTextureArrowRed);
        glDrawArrays(GL_TRIANGLES, 0, m_qvGraphArrowsVA.size() / 2);

        if(!qvMarkedEdgesIndexes.empty())
        {
            glColor4f(1.0f, 1.0f, 1.0f, fabs(sin(m_renderTime.elapsed() * 0.0078125f)));
            glBindTexture(GL_TEXTURE_2D, m_idTextureArrowBlue);
            for(int i = 0; i < qvMarkedEdgesIndexes.size(); i++)
                glDrawArrays(GL_TRIANGLES, qvMarkedEdgesIndexes[i] * 6, 6);
        }
    }

    if(!qvGraphVertices.empty())
    {
        glVertexPointer(2, GL_FLOAT, 0, (GLfloat *)m_qvGraphVerticesVA.data());
        glTexCoordPointer(2, GL_FLOAT, 0, (GLfloat *)m_qvGraphVerticesTCA.data());

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glBindTexture(GL_TEXTURE_2D, m_idTextureCircleRed);
        glDrawArrays(GL_TRIANGLES, 0, m_qvGraphVerticesVA.size() / 2);
    }

    if(!qvMarkedVerticesIndexes.empty())
    {
        glColor4f(1.0f, 1.0f, 1.0f, fabs(sin(m_renderTime.elapsed() * 0.0078125f)));

        glVertexPointer(2, GL_FLOAT, 0, (GLfloat *)m_qvGraphVerticesVA.data());
        glTexCoordPointer(2, GL_FLOAT, 0, (GLfloat *)m_qvGraphVerticesTCA.data());

        glBindTexture(GL_TEXTURE_2D, m_idTextureCircleBlue);
        for(int i = 0; i < qvMarkedVerticesIndexes.size(); i++)
            glDrawArrays(GL_TRIANGLES, qvMarkedVerticesIndexes[i] * 6, 6);
    }

    if(selectedPointNumFst != -1 && selectedPointNumSnd == -1)
    {
        glColor4f(1.0f, 1.0f, 1.0f, fabs(sin(m_renderTime.elapsed() * 0.00390625f)));

        glVertexPointer(2, GL_FLOAT, 0, (GLfloat *)m_qvGraphVerticesVA.data());
        glTexCoordPointer(2, GL_FLOAT, 0, (GLfloat *)m_qvGraphVerticesTCA.data());

        glBindTexture(GL_TEXTURE_2D, m_idTextureCircleYellow);
        glDrawArrays(GL_TRIANGLES, selectedPointNumFst * 6, 6);
    }

    if(selectedPointNumFst != -1 && selectedPointNumSnd != -1)
    {
        glColor4f(1.0f, 1.0f, 1.0f, fabs(sin(m_renderTime.elapsed() * 0.00390625f)));

        glVertexPointer(2, GL_FLOAT, 0, (GLfloat *)m_qvGraphVerticesVA.data());
        glTexCoordPointer(2, GL_FLOAT, 0, (GLfloat *)m_qvGraphVerticesTCA.data());

        glBindTexture(GL_TEXTURE_2D, m_idTextureCircleGreen);
        glDrawArrays(GL_TRIANGLES, selectedPointNumFst * 6, 6);
        glDrawArrays(GL_TRIANGLES, selectedPointNumSnd * 6, 6);

        for(int i = 0; i < qvGraphEdges.size(); i++)
        {
            if(qvGraphEdges[i].iFrom == selectedPointNumFst && qvGraphEdges[i].iTo == selectedPointNumSnd)
            {
                glVertexPointer(2, GL_FLOAT, 0, (GLfloat *)m_qvGraphArrowsVA.data());
                glTexCoordPointer(2, GL_FLOAT, 0, (GLfloat *)m_qvGraphArrowsTCA.data());

                glBindTexture(GL_TEXTURE_2D, m_idTextureArrowGreen);
                glDrawArrays(GL_TRIANGLES, i * 6, 6);
            }
        }
    }

//    for(int i = 0; i < qvGraphVertices.size(); i++)
//    {
//        GLfloat x = 0.0f;
//        GLfloat y = 0.0f;

//        switch(m_viewportType)
//        {
//            case GLDrawer2D::Top:
//                x = qvGraphVertices[i].vecOrigin.x();
//                y = qvGraphVertices[i].vecOrigin.y();
//                break;

//            case GLDrawer2D::Front:
//                x = qvGraphVertices[i].vecOrigin.x();
//                y = qvGraphVertices[i].vecOrigin.z();
//                break;

//            case GLDrawer2D::Side:
//                x = qvGraphVertices[i].vecOrigin.y();
//                y = qvGraphVertices[i].vecOrigin.z();
//                break;
//        }

//        glColor4f(0.0f, 1.0f, 0.0f, 1.0f);
//        renderText((int)x + 6 - m_HOffset, (int)y - 6 - m_VOffset, QString().fromUtf8(qvGraphVertices[i].szInfo), QFont("Tahoma", 10));
//    }
}

void GLDrawer2D::timerEvent(QTimerEvent *)
{
    if(globalStackIsModified > 0)
    {
        g_pWndMain->updateIsModified(true);
        globalStackIsModified--;
    }

    update();
}

void GLDrawer2D::mousePressEvent(QMouseEvent *event)
{
    Qt::MouseButton btnPressed = event->button();
    if(btnPressed == Qt::LeftButton)
    {
        QVector2D vecClickPos(event->posF());
        vecClickPos += QVector2D(m_HOffset, m_VOffset);

        if(g_pWndMain->getIsGridModeEnabled())
        {
            vecClickPos.setX((float)floor(vecClickPos.x() / 25.0f + 0.5f) * 25.0f);
            vecClickPos.setY((float)floor(vecClickPos.y() / 25.0f + 0.5f) * 25.0f);
        }

        if(selectedPointNumFst == -1)
        {
                for(int i = 0; i < qvGraphVertices.size(); i++)
                {
                    GLfloat x = 0.0f;
                    GLfloat y = 0.0f;

                    switch(m_viewportType)
                    {
                        case GLDrawer2D::Top:
                            x = qvGraphVertices[i].vecOrigin.x();
                            y = qvGraphVertices[i].vecOrigin.y();
                            break;

                        case GLDrawer2D::Front:
                            x = qvGraphVertices[i].vecOrigin.x();
                            y = qvGraphVertices[i].vecOrigin.z();
                            break;

                        case GLDrawer2D::Side:
                            x = qvGraphVertices[i].vecOrigin.y();
                            y = qvGraphVertices[i].vecOrigin.z();
                            break;
                    }

                    QVector2D vecDiference = QVector2D(x, y) - vecClickPos;
                    if(vecDiference.length() < 15.0f)
                            return;
                }
                grVertex newPoint;

                switch(m_viewportType)
                {
                    case GLDrawer2D::Top:
                        newPoint.vecOrigin.setX(vecClickPos.x());
                        newPoint.vecOrigin.setY(vecClickPos.y());
                        newPoint.vecOrigin.setZ(0.0f);
                        break;

                    case GLDrawer2D::Front:
                        newPoint.vecOrigin.setX(vecClickPos.x());
                        newPoint.vecOrigin.setZ(vecClickPos.y());
                        newPoint.vecOrigin.setY(0.0f);
                        break;

                    case GLDrawer2D::Side:
                        newPoint.vecOrigin.setY(vecClickPos.x());
                        newPoint.vecOrigin.setZ(vecClickPos.y());
                        newPoint.vecOrigin.setX(0.0f);
                        break;
                }

                sprintf(newPoint.szInfo, "%u", lastVertexIndex);
                qvGraphVertices.push_back(newPoint);
                lastVertexIndex++;

                if(g_pWndMain->getIsAutoconnectModeEnabled())
                {
                     if(qvGraphVertices.size() > 1)
                     {
                            grEdge newLink;
                            newLink.iFrom = qvGraphVertices.size() - 2;
                            newLink.iTo = qvGraphVertices.size() - 1;
                            newLink.flReservedValue = 10.0f;
                            qvGraphEdges.push_back(newLink);
                     }
                }
                needToRebuildRD = true;
                g_pWndMain->updateIsModified(true);
            }
            else if(selectedPointNumFst != -1 && selectedPointNumSnd == -1)
            {
                switch(m_viewportType)
                {
                    case GLDrawer2D::Top:
                        qvGraphVertices[selectedPointNumFst].vecOrigin.setX(vecClickPos.x());
                        qvGraphVertices[selectedPointNumFst].vecOrigin.setY(vecClickPos.y());
                        break;

                    case GLDrawer2D::Front:
                        qvGraphVertices[selectedPointNumFst].vecOrigin.setX(vecClickPos.x());
                        qvGraphVertices[selectedPointNumFst].vecOrigin.setZ(vecClickPos.y());
                        break;

                    case GLDrawer2D::Side:
                        qvGraphVertices[selectedPointNumFst].vecOrigin.setY(vecClickPos.x());
                        qvGraphVertices[selectedPointNumFst].vecOrigin.setZ(vecClickPos.y());
                        break;
                }

                selectedPointNumFst = -1;

                g_pWndMain->setOneVertexActionsEnabled(false);
                g_pWndMain->setTwoVertexActionsEnabled(false);

                needToRebuildRD = true;
                g_pWndMain->updateIsModified(true);
            }
            else if(selectedPointNumFst != -1 && selectedPointNumSnd != -1)
            {
                selectedPointNumFst = -1;
                selectedPointNumSnd = -1;

                g_pWndMain->setOneVertexActionsEnabled(false);
                g_pWndMain->setTwoVertexActionsEnabled(false);
            }
    }
    else if(btnPressed == Qt::RightButton)
    {
        QVector2D vecClickPos((float)event->x(), (float)event->y());
        vecClickPos += QVector2D(m_HOffset, m_VOffset);

        qvMarkedVerticesIndexes.clear();
        qvMarkedEdgesIndexes.clear();

        for(int i = 0; i < qvGraphVertices.size(); i++)
        {
            GLfloat x = 0.0f;
            GLfloat y = 0.0f;

            switch(m_viewportType)
            {
                case GLDrawer2D::Top:
                    x = qvGraphVertices[i].vecOrigin.x();
                    y = qvGraphVertices[i].vecOrigin.y();
                    break;

                case GLDrawer2D::Front:
                    x = qvGraphVertices[i].vecOrigin.x();
                    y = qvGraphVertices[i].vecOrigin.z();
                    break;

                case GLDrawer2D::Side:
                    x = qvGraphVertices[i].vecOrigin.y();
                    y = qvGraphVertices[i].vecOrigin.z();
                    break;
            }

            QVector2D vecDiference = QVector2D(x, y) - vecClickPos;
            if(vecDiference.length() < 10.0f)
            {
                if(selectedPointNumFst == -1)
                {
                    selectedPointNumFst = i;
                    g_pWndMain->setOneVertexActionsEnabled(true);
                    g_pWndMain->setTwoVertexActionsEnabled(false);
                    return;
                }
                else if(selectedPointNumFst != i && selectedPointNumSnd == -1)
                {
                    selectedPointNumSnd = i;
                    g_pWndMain->setOneVertexActionsEnabled(false);
                    g_pWndMain->setTwoVertexActionsEnabled(true);
                }
            }
        }
    }
}

void GLDrawer2D::rebuildVerticesVA()
{
    m_qvGraphVerticesVA.clear();
    m_qvGraphVerticesTCA.clear();
    for(int i = 0; i < qvGraphVertices.size(); i++)
    {
        GLfloat x = 0.0f;
        GLfloat y = 0.0f;

        switch(m_viewportType)
        {
            case GLDrawer2D::Top:
                x = qvGraphVertices[i].vecOrigin.x();
                y = qvGraphVertices[i].vecOrigin.y();
                break;

            case GLDrawer2D::Front:
                x = qvGraphVertices[i].vecOrigin.x();
                y = qvGraphVertices[i].vecOrigin.z();
                break;

            case GLDrawer2D::Side:
                x = qvGraphVertices[i].vecOrigin.y();
                y = qvGraphVertices[i].vecOrigin.z();
                break;
        }

        m_qvGraphVerticesTCA.push_back(1.0f); m_qvGraphVerticesTCA.push_back(1.0f);
        m_qvGraphVerticesVA.push_back(x + 4.0f);
        m_qvGraphVerticesVA.push_back(y + 4.0f);

        m_qvGraphVerticesTCA.push_back(0.0f); m_qvGraphVerticesTCA.push_back(1.0f);
        m_qvGraphVerticesVA.push_back(x - 4.0f);
        m_qvGraphVerticesVA.push_back(y + 4.0f);

        m_qvGraphVerticesTCA.push_back(0.0f); m_qvGraphVerticesTCA.push_back(0.0f);
        m_qvGraphVerticesVA.push_back(x - 4.0f);
        m_qvGraphVerticesVA.push_back(y - 4.0f);

        m_qvGraphVerticesTCA.push_back(1.0f); m_qvGraphVerticesTCA.push_back(1.0f);
        m_qvGraphVerticesVA.push_back(x + 4.0f);
        m_qvGraphVerticesVA.push_back(y + 4.0f);

        m_qvGraphVerticesTCA.push_back(1.0f); m_qvGraphVerticesTCA.push_back(0.0f);
        m_qvGraphVerticesVA.push_back(x + 4.0f);
        m_qvGraphVerticesVA.push_back(y - 4.0f);

        m_qvGraphVerticesTCA.push_back(0.0f); m_qvGraphVerticesTCA.push_back(0.0f);
        m_qvGraphVerticesVA.push_back(x - 4.0f);
        m_qvGraphVerticesVA.push_back(y - 4.0f);
    }
}

void GLDrawer2D::rebuildLinesVA()
{
    m_qvGraphLinesVA.clear();
    m_qvGraphArrowsVA.clear();
    m_qvGraphArrowsTCA.clear();
    for(int i = 0; i < qvGraphEdges.size(); i++)
    {
        QVector2D vecA;
        QVector2D vecB;

        switch(m_viewportType)
        {
            case GLDrawer2D::Top:
                vecA = QVector2D(qvGraphVertices[qvGraphEdges[i].iFrom].vecOrigin.x(), qvGraphVertices[qvGraphEdges[i].iFrom].vecOrigin.y());
                vecB = QVector2D(qvGraphVertices[qvGraphEdges[i].iTo].vecOrigin.x(), qvGraphVertices[qvGraphEdges[i].iTo].vecOrigin.y());
                break;

            case GLDrawer2D::Front:
                vecA = QVector2D(qvGraphVertices[qvGraphEdges[i].iFrom].vecOrigin.x(), qvGraphVertices[qvGraphEdges[i].iFrom].vecOrigin.z());
                vecB = QVector2D(qvGraphVertices[qvGraphEdges[i].iTo].vecOrigin.x(), qvGraphVertices[qvGraphEdges[i].iTo].vecOrigin.z());
                break;

            case GLDrawer2D::Side:
                vecA = QVector2D(qvGraphVertices[qvGraphEdges[i].iFrom].vecOrigin.y(), qvGraphVertices[qvGraphEdges[i].iFrom].vecOrigin.z());
                vecB = QVector2D(qvGraphVertices[qvGraphEdges[i].iTo].vecOrigin.y(), qvGraphVertices[qvGraphEdges[i].iTo].vecOrigin.z());
                break;
        }

        m_qvGraphLinesVA.push_back(vecA.x());
        m_qvGraphLinesVA.push_back(vecA.y());

        m_qvGraphLinesVA.push_back(vecB.x());
        m_qvGraphLinesVA.push_back(vecB.y());

        QVector2D vecC = vecB - vecA;
        vecC *= 12.0f / vecC.length();
        vecC = vecB - vecC;

        float flAngle = atan((vecB.y() - vecA.y()) / (vecB.x() - vecA.x() == 0 ? 1 : vecB.x() - vecA.x()));
        flAngle += vecB.x() - vecA.x() >= 0 ? 0 : M_PI;

        QVector2D vecPM, vecPP, vecMM, vecMP;
        vecPM.setX(+8.0f * cos(flAngle) - -8.0f * sin(flAngle));
        vecPM.setY(+8.0f * sin(flAngle) + -8.0f * cos(flAngle));

        vecPP.setX(+8.0f * cos(flAngle) - +8.0f * sin(flAngle));
        vecPP.setY(+8.0f * sin(flAngle) + +8.0f * cos(flAngle));

        vecMM.setX(-8.0f * cos(flAngle) - -8.0f * sin(flAngle));
        vecMM.setY(-8.0f * sin(flAngle) + -8.0f * cos(flAngle));

        vecMP.setX(-8.0f * cos(flAngle) - +8.0f * sin(flAngle));
        vecMP.setY(-8.0f * sin(flAngle) + +8.0f * cos(flAngle));

        vecPM += vecC;
        vecPP += vecC;
        vecMM += vecC;
        vecMP += vecC;

        m_qvGraphArrowsTCA.push_back(1.0f); m_qvGraphArrowsTCA.push_back(1.0f);
        m_qvGraphArrowsVA.push_back(vecPP.x());
        m_qvGraphArrowsVA.push_back(vecPP.y());

        m_qvGraphArrowsTCA.push_back(0.0f); m_qvGraphArrowsTCA.push_back(1.0f);
        m_qvGraphArrowsVA.push_back(vecMP.x());
        m_qvGraphArrowsVA.push_back(vecMP.y());

        m_qvGraphArrowsTCA.push_back(0.0f); m_qvGraphArrowsTCA.push_back(0.0f);
        m_qvGraphArrowsVA.push_back(vecMM.x());
        m_qvGraphArrowsVA.push_back(vecMM.y());

        m_qvGraphArrowsTCA.push_back(1.0f); m_qvGraphArrowsTCA.push_back(1.0f);
        m_qvGraphArrowsVA.push_back(vecPP.x());
        m_qvGraphArrowsVA.push_back(vecPP.y());

        m_qvGraphArrowsTCA.push_back(1.0f); m_qvGraphArrowsTCA.push_back(0.0f);
        m_qvGraphArrowsVA.push_back(vecPM.x());
        m_qvGraphArrowsVA.push_back(vecPM.y());

        m_qvGraphArrowsTCA.push_back(0.0f); m_qvGraphArrowsTCA.push_back(0.0f);
        m_qvGraphArrowsVA.push_back(vecMM.x());
        m_qvGraphArrowsVA.push_back(vecMM.y());
    }
}

void GLDrawer2D::connectVertices()
{
    if(selectedPointNumFst != -1 && selectedPointNumSnd != -1)
    {
            grEdge newLink;
            newLink.iFrom = selectedPointNumFst;
            newLink.iTo = selectedPointNumSnd;
            newLink.flReservedValue = 10.0f;

            selectedPointNumFst = -1;
            selectedPointNumSnd = -1;

            g_pWndMain->setOneVertexActionsEnabled(false);
            g_pWndMain->setTwoVertexActionsEnabled(false);

            for(int i = 0; i < qvGraphEdges.size(); i++)
            {
                    if(qvGraphEdges[i].iFrom == newLink.iFrom && qvGraphEdges[i].iTo == newLink.iTo)
                            return;
            }

            qvGraphEdges.push_back(newLink);
            qvMarkedVerticesIndexes.clear();
            qvMarkedEdgesIndexes.clear();
            needToRebuildRD = true;
            g_pWndMain->updateIsModified(true);
    }
}

void GLDrawer2D::disconnectVertices()
{
    if(selectedPointNumFst != -1 && selectedPointNumSnd != -1)
    {
            int iFrom = selectedPointNumFst;
            int iTo = selectedPointNumSnd;

            selectedPointNumFst = -1;
            selectedPointNumSnd = -1;

            g_pWndMain->setOneVertexActionsEnabled(false);
            g_pWndMain->setTwoVertexActionsEnabled(false);

            for(int i = 0; i < qvGraphEdges.size(); i++)
            {
                    if(qvGraphEdges[i].iFrom == iFrom && qvGraphEdges[i].iTo == iTo)
                    {
                            QVector<grEdge>::iterator qviTmp = qvGraphEdges.begin();
                            qviTmp += i;
                            qvGraphEdges.erase(qviTmp);
                            qvMarkedVerticesIndexes.clear();
                            qvMarkedEdgesIndexes.clear();
                            needToRebuildRD = true;
                            g_pWndMain->updateIsModified(true);
                            return;
                    }
            }
    }
}

void GLDrawer2D::deleteVertex()
{
    if(selectedPointNumFst != -1 && selectedPointNumSnd == -1)
    {
            int iSelTmp = selectedPointNumFst;
            selectedPointNumFst = -1;

            QVector<grVertex>::iterator qviTmpP = qvGraphVertices.begin();
            qviTmpP += iSelTmp;
            qvGraphVertices.erase(qviTmpP);

            for(int i = 0; i < qvGraphEdges.size(); i++)
            {
                    if(qvGraphEdges[i].iFrom == iSelTmp || qvGraphEdges[i].iTo == iSelTmp)
                    {
                            QVector<grEdge>::iterator qviTmpL = qvGraphEdges.begin();
                            qviTmpL += i;
                            qvGraphEdges.erase(qviTmpL);
                            i--;
                    }
            }

            for(int i = 0; i < qvGraphEdges.size(); i++)
            {
            if(qvGraphEdges[i].iFrom >= iSelTmp)
                    qvGraphEdges[i].iFrom--;
            if(qvGraphEdges[i].iTo >= iSelTmp)
                    qvGraphEdges[i].iTo--;
            }
            qvMarkedVerticesIndexes.clear();
            qvMarkedEdgesIndexes.clear();
            needToRebuildRD = true;
            g_pWndMain->updateIsModified(true);
    }

    g_pWndMain->setOneVertexActionsEnabled(false);
    g_pWndMain->setTwoVertexActionsEnabled(false);
}

void GLDrawer2D::alignAllToGrid()
{
    if(qvGraphVertices.empty())
             return;
    else
    {
        bool isGridModeOn = g_pWndMain->getIsGridModeEnabled();
        g_pWndMain->setIsGridModeEnabled(true);
        int iTmp1 = selectedPointNumFst;
        int iTmp2 = selectedPointNumSnd;
        selectedPointNumSnd = -1;
        for(int i = 0; i < qvGraphVertices.size(); i++)
        {
             selectedPointNumFst = i;
             mousePressEvent(new QMouseEvent(QMouseEvent::MouseButtonPress, QPoint((int)qvGraphVertices[i].vecOrigin.x() - m_HOffset, (int)qvGraphVertices[i].vecOrigin.y() - m_VOffset), Qt::LeftButton, NULL, NULL));
        }
        selectedPointNumFst = iTmp1;
        selectedPointNumSnd = iTmp2;
        g_pWndMain->setIsGridModeEnabled(isGridModeOn);
    }
}
