#ifndef TOPOLERROR_H
#define TOPOLERROR_H

#include <qgsvectorlayer.h>
#include <qgsgeometry.h>
#include <qgsrectangle.h>

class TopolError;
typedef bool (TopolError::*fixFunction)();

class TopolError
{
protected:
  QString mName;
  QgsRectangle mBoundingBox;  
  QgsGeometry* mConflict;
  QgsFeatureIds mFids;
  QgsVectorLayer* mLayer;
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
  TopolError(QgsVectorLayer* theLayer, QgsRectangle theBoundingBox, QgsGeometry* theConflict, QgsFeatureIds theFids) : mLayer(theLayer), mBoundingBox(theBoundingBox), mConflict(theConflict), mFids(theFids) {};

  virtual ~TopolError() {}
  virtual bool fix(QString fixName);
  virtual QString name() { return mName; }
  virtual QgsGeometry* conflict() { return mConflict; }
  virtual QgsRectangle boundingBox() { return mBoundingBox; }
  virtual QStringList fixNames() { return mFixMap.keys(); }
};

class TopolErrorIntersection : public TopolError
{
public:
  TopolErrorIntersection(QgsVectorLayer* theLayer, QgsRectangle theBoundingBox, QgsGeometry* theConflict, QgsFeatureIds theFids);
};

class TopolErrorDangle : public TopolError
{
public:
  TopolErrorDangle(QgsVectorLayer* theLayer, QgsRectangle theBoundingBox, QgsGeometry* theConflict, QgsFeatureIds theFids);
};

#endif
