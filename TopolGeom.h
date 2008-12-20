#ifndef TOPOLGEOM_H_
#define TOPOLGEOM_H_

#include <QMap>
#include <QSet>
#include <QString>

#include <qgsvectorlayer.h>
#include <qgsgeometry.h>
#include <qgspoint.h>

class QPlainTextEdit;

enum CheckType
{
  Intersection,
  Overlap,
  Multipart
};

class TopolNode
{
public:
  QgsPoint point;
  QList<QgsPolyline> arcs;
};

class TopolArc
{
public:
  // beginning and end of line
  int beg, end;

  QgsPolyline arc;
};

class TopolGeom
{
public:
  TopolGeom();
  TopolGeom(QgsVectorLayer *theLayer, QPlainTextEdit *theWindow);
  ~TopolGeom();
  void buildGeometry(QgsVectorLayer *nodeLayer, QgsVectorLayer *arcLayer);

  QgsFeatureIds checkGeometry(CheckType type);

private:
  QgsVectorLayer *mLayer;
  QMap<int, QgsGeometry> mObjects;
  QPlainTextEdit *mWindow;

  QgsPolyline mIntersections;
  QList<TopolNode> mNodes;
  QList<TopolArc> mArcs;
  QgsFeatureIds mConflicting;

  void getFeatures();
  void buildIntersections();

  void cgIntersect();
  void cgOverlap();
  void cgMultipart();
};

#endif
