/***************************************************************************
  topolError.h 
  TOPOLogy checker
  -------------------
         begin                : May 2009
         copyright            : Vita Cizek
         email                : weetya (at) gmail.com

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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
  QList<FeatureLayer> mFeaturePairs;
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
  bool fixMove(FeatureLayer fl1, FeatureLayer fl2);
  bool fixUnion(FeatureLayer fl1, FeatureLayer fl2);

public:
  TopolError(QgsRectangle theBoundingBox, QgsGeometry* theConflict, QList<FeatureLayer> theFeaturePairs) : mFeaturePairs(theFeaturePairs), mBoundingBox(theBoundingBox), mConflict(theConflict) {};

  virtual ~TopolError() {}
  virtual bool fix(QString fixName);
  virtual QString name() { return mName; }
  virtual QgsGeometry* conflict() { return mConflict; }
  virtual QgsRectangle boundingBox() { return mBoundingBox; }
  virtual QList<FeatureLayer> featurePairs() { return mFeaturePairs; }
  virtual QStringList fixNames() { return mFixMap.keys(); }
};

class TopolErrorIntersection : public TopolError
{
public:
  TopolErrorIntersection(QgsRectangle theBoundingBox, QgsGeometry* theConflict, QList<FeatureLayer> theFeaturePairs);
};

class TopolErrorClose : public TopolError
{
public:
  TopolErrorClose(QgsRectangle theBoundingBox, QgsGeometry* theConflict, QList<FeatureLayer> theFeaturePairs);
};
/*
class TopolErrorContains : public TopolError
{
public:
  TopolErrorContains(QgsRectangle theBoundingBox, QgsGeometry* theConflict, QList<FeatureLayer> theFeaturePairs);
};
*/

class TopolErrorCovered : public TopolError
{
public:
  TopolErrorCovered(QgsRectangle theBoundingBox, QgsGeometry* theConflict, QList<FeatureLayer> theFeaturePairs);
};

class TopolErrorShort : public TopolError
{
public:
  TopolErrorShort(QgsRectangle theBoundingBox, QgsGeometry* theConflict, QList<FeatureLayer> theFeaturePairs);
  ~TopolErrorShort();
};

class TopolErrorInside : public TopolError
{
public:
  TopolErrorInside(QgsRectangle theBoundingBox, QgsGeometry* theConflict, QList<FeatureLayer> theFeaturePairs);
};

class TopolErrorValid : public TopolError
{
public:
  TopolErrorValid(QgsRectangle theBoundingBox, QgsGeometry* theConflict, QList<FeatureLayer> theFeaturePairs);
};

class TopolErrorDangle : public TopolError
{
public:
  TopolErrorDangle(QgsRectangle theBoundingBox, QgsGeometry* theConflict, QList<FeatureLayer> theFeaturePairs);
};

typedef QList<TopolError*> ErrorList;

#endif
