#ifndef TOPOLERROR_H
#define TOPOLERROR_H

#include <qgsvectorlayer.h>
#include <qgsgeometry.h>
#include <qgsrectangle.h>

class TopolError;

enum TopolErrorType
{
  TopolErrorIntersection = 1,
  TopolErrorSelfIntersection,
  TopolErrorOverlap,
  TopolErrorTolerance,
  TopolErrorDangle
};

typedef bool (TopolError::*fixFunction)();
/*
enum TopolFixType
{
  TopolFixMoveFirst = 1,
  TopolFixMoveSecond,
  TopolFixUnionFirst,
  TopolFixUnionSecond,
  TopolFixDeleteFirst,
  TopolFixDeleteSecond,
  TopolFixSnap,
};*/

//const StringList FixType = 

class TopolError
{
protected:
  TopolErrorType mType;
  QgsRectangle mBoundingBox;  
  QgsGeometry* mConflict;
  QgsFeatureIds mFids;
  QStringList mFixNames;
  QMap<QString, fixFunction> mFixMap;

  bool fixMoveFirst();
  bool fixMoveSecond();
  bool fixUnionFirst();
  bool fixUnionSecond();
  bool fixDeleteFirst();
  bool fixDeleteSecond();

public:
  TopolError(QgsRectangle theBoundingBox, QgsGeometry* theConflict, QgsFeatureIds theFids) : mBoundingBox(theBoundingBox), mConflict(theConflict), mFids(theFids) {};

  virtual ~TopolError() {}
  virtual bool fixIt(QString fixName);
  virtual QStringList fixNames() { return mFixNames; }
};

class TopolErrorIntersection : TopolError
{
  TopolErrorIntersection(QgsRectangle theBoundingBox, QgsGeometry* theConflict, QgsFeatureIds theFids);
  //TopolErrorIntersection(QgsRectangle theBoundingBox, QgsGeometry* theConflict, QgsFeatureIds theFids) : mBoundingBox(theBoundingBox), mConflict(theConflict), mFids(theFids);
};

#endif
