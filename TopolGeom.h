#ifndef TOPOLGEOM_H_
#define TOPOLGEOM_H_

#include <QMap>
#include <QSet>
#include <QString>

#include <qgsvectorlayer.h>
#include <qgsgeometry.h>
#include <qgspoint.h>

class QTextEdit;

enum CheckType
{
  Intersection,
  Overlap,
  Multipart
};

class Arc
{
  public:
  QgsGeometry *geom;
  // id of the QgsFeature to which the arc belongs
  int featureID;

  // helper value - number of arcs with which there is no need to test for intersections
  // starting from the beginning of the arcs QList
  int alreadyTested;

  // sign if this is the first level arc contained in mObjects
  bool firstLevel;
  
  /** Contructor for the first level of arcs. */
  Arc(QgsGeometry *g, int f) {
    geom = g;
    featureID = f;
    alreadyTested = 0;  // no knowledge about the first level arcs 
    firstLevel = true;   
  }
  
  /** Contructor for the arcs created by intersections. */
  Arc(QgsGeometry *g, int f, int at) {
    geom = g;
    featureID = f;
    alreadyTested = at;  // we know how many arcs do NOT have intersection with the arc's parent
    firstLevel = false;   
  }
  
  ~Arc() {
    // the geometry may be freed only if it is not in the mObjects too
    if (firstLevel == false) 
      delete(geom);
  }
  
  bool intersects(QList<Arc*> *arcs, int i) {
    // first the optimalization
    if (alreadyTested > i)
      return false;
      
    // real test for intersection
    return geom->intersects((*arcs)[i]->geom);  
  }
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
  int beg, end;

  QgsPolyline arc;
};

class TopolGeom
{
public:
  TopolGeom();
  TopolGeom(QgsVectorLayer *theLayer, QTextEdit *theWindow);
  ~TopolGeom();
  void buildGeometry(QgsVectorLayer *nodeLayer, QgsVectorLayer *arcLayer);
  void updateArcs(Arc *a, int b, QgsGeometry *comb, QList<Arc*> *arcs);
  bool belongsTo(QgsGeometry *newGeom, Arc *originalArc);

  QgsFeatureIds checkGeometry(CheckType type);

private:
  QgsVectorLayer *mLayer;
  QMap<int, QgsGeometry*> mObjects;
  QTextEdit *mWindow;

  QMap<QString, TopolNode> mNodes;
  QList<Arc> mArcs;
  QgsFeatureIds mConflicting;

  void getFeatures();
  void buildIntersections();

  void cgIntersect();
  void cgOverlap();
  void cgMultipart();
};

#endif
