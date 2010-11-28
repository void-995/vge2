#ifndef WPTDEFS_H
#define WPTDEFS_H

// Qt
#include <QVector2D>
#include <QVector3D>

struct grVertexTAG
{
        QVector3D vecOrigin;
        char szInfo[256];
};

struct grEdgeTAG
{
        int iFrom;
        int iTo;
        float flReservedValue;
};

struct headerTAG
{
        int iPointsCount;
        int iLinksCount;
        int iHTopOffset;
        int iVTopOffset;
        int iHFrontOffset;
        int iVFrontOffset;
        int iHSideOffset;
        int iVSideOffset;
};

typedef grVertexTAG grVertex;
typedef grEdgeTAG grEdge;
typedef headerTAG header;

#endif // WPTDEFS_H
