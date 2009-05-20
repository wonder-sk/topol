#ifndef TOPOLERROR_H
#define TOPOLERROR_H

#include <qgsvectorlayer.h>
#include <qgsgeometry.h>
#include <qgsrectangle.h>

class TopolError;
typedef bool (TopolError::*fixFunction)();

class FeatureLayer
{
public:
  FeatureLayer() : 
    layer(0), feature(QgsFeature()) {};
  FeatureLayer(QgsVectorLayer* theLayer, QgsFeature theFeature) :
    layer(theLayer), feature(theFeature) {};

  QgsVectorLayer* layer;
  QgsFeature feature;
};

class TopolError
{
protected:
  QString mName;
  QgsRectangle mBoundingBox;  
  QgsGeometry* mConflict;
  //QgsFeatureIds mFids;
  QList<FeatureLayer> mFeaturePairs;
  //QgsVectorLayer* mLayer;
  QMap<QString, fixFunction> mFixMap;

  bool fixDummy() { return false; }
  bool fixSnap();
  bool fixMoveFirst();
  bool fixMoveSecond();
  bool fixUnionFirst();
  bool fixUnionSecond();
  bool fixDeleteFirst();
  bool fixDeleteSecond();

  //helper fix functions
  bool fixMove(int id1, int id2);
  bool fixUnion(int id1, int id2);

public:
  //TopolError(QgsVectorLayer* theLayer, QgsRectangle theBoundingBox, QgsGeometry* theConflict, QgsFeatureIds theFids) : mLayer(theLayer), mBoundingBox(theBoundingBox), mConflict(theConflict), mFids(theFids) {};
  TopolError(QgsRectangle theBoundingBox, QgsGeometry* theConflict, QList<FeatureLayer> theFeaturePairs) : mFeaturePairs(theFeaturePairs), mBoundingBox(theBoundingBox), mConflict(theConflict) {};

  virtual ~TopolError() {}
  virtual bool fix(QString fixName);
  virtual QString name() { return mName; }
  virtual QgsGeometry* conflict() { return mConflict; }
  virtual QgsRectangle boundingBox() { return mBoundingBox; }
  //virtual QgsFeatureIds fids() { return mFids; }
  virtual QList<FeatureLayer> featurePairs() { return mFeaturePairs; }
  virtual QStringList fixNames() { return mFixMap.keys(); }
};

class TopolErrorIntersection : public TopolError
{
public:
  TopolErrorIntersection(QgsRectangle theBoundingBox, QgsGeometry* theConflict, QList<FeatureLayer> theFeaturePairs);
};

class TopolErrorDangle : public TopolError
{
public:
  TopolErrorDangle(QgsRectangle theBoundingBox, QgsGeometry* theConflict, QList<FeatureLayer> theFeaturePairs);
};

#endif
