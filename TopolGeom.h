#ifndef TOPOLGEOM_H_
#define TOPOLGEOM_H_

#include <QMap>
#include <QSet>
#include <QString>

#include <qgsvectorlayer.h>
#include <qgsline.h>
#include <qgspoint.h>

class QPlainTextEdit;

enum CheckType
{
  Intersection,
  Overlap,
  Multipart
};

class TopolGeom
{
public:
  TopolGeom();
  TopolGeom(QgsVectorLayer *theLayer, QPlainTextEdit *theWindow);
  ~TopolGeom();
  void buildGeometry(QgsVectorLayer *nodeLayer, QgsVectorLayer *wayLayer);

  QgsFeatureIds checkGeometry(CheckType type);

private:
  QgsVectorLayer *mLayer;
  QMap<int, QgsGeometry> mObjects;
  QList<QgsPoint> mNodes;
  QList<QgsLine> mLines;
  QPlainTextEdit *mWindow;
  QgsFeatureIds mConflicting;

  void getFeatures();

  void cgIntersect();
  void cgOverlap();
  void cgMultipart();
};

#endif
