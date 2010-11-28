#ifndef GLDRAWER_H
#define GLDRAWER_H

// Qt
#include <QTime>
#include <QGLWidget>
#include <QVector>
#include <QVector2D>
#include <QVector3D>

// Project
#include "wndmain.h"

class GLDrawer2D : public QGLWidget
{
    Q_OBJECT
    Q_PROPERTY(VType viewportType READ viewportType WRITE setViewportType)
    Q_ENUMS(VType)
public:
    explicit GLDrawer2D(QWidget *parent = 0);

    void setVOffset(int);
    void setHOffset(int);

    enum VType { Top, Front, Side };

    void setViewportType(VType viewportType) { m_viewportType = viewportType; }
    VType viewportType() const { return m_viewportType; }

private:
    void rebuildVerticesVA();
    void rebuildLinesVA();

    QTime m_renderTime;

    QVector<GLfloat> m_qvGraphVerticesVA;
    QVector<GLfloat> m_qvGraphVerticesTCA;
    QVector<GLfloat> m_qvGraphArrowsVA;
    QVector<GLfloat> m_qvGraphArrowsTCA;
    QVector<GLfloat> m_qvGraphLinesVA;
    QVector<GLfloat> m_qvGraphGridBackgroundVA;
    QVector<GLfloat> m_qvGraphGridBackgroundTCA;

    GLuint m_idTextureCircle;
    GLuint m_idTextureArrow;

    GLuint m_idTextureCircleRed;
    GLuint m_idTextureArrowRed;

    GLuint m_idTextureCircleYellow;
    GLuint m_idTextureArrowYellow;

    GLuint m_idTextureCircleGreen;
    GLuint m_idTextureArrowGreen;

    GLuint m_idTextureCircleBlue;
    GLuint m_idTextureArrowBlue;

    GLuint m_idTextureGrid;

    int m_VOffset;
    int m_HOffset;

    VType m_viewportType;

protected:
    void initializeGL();
    void resizeGL(int w, int h);
    void paintGL();
    void timerEvent(QTimerEvent *);
    void mousePressEvent(QMouseEvent *);

signals:

public slots:
    void connectVertices();
    void disconnectVertices();
    void deleteVertex();
    void alignAllToGrid();
};

#endif // GLDRAWER_H
