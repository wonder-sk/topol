/***************************************************************************
  topolTest.h 
  TOPOLogy checker
  -------------------
         date                 : May 2009
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

#ifndef TOPOLTEST_H
#define TOPOLTEST_H

#include <QObject>

#include <qgsvectorlayer.h>
#include <qgsgeometry.h>
#include <spatialindex/qgsspatialindex.h>

#include "topolError.h"

class topolTest;
class QgsMapLayerRegistry;

typedef ErrorList (topolTest::*testFunction)(double, QgsVectorLayer*, QgsVectorLayer*);

class test
{
public:
  bool useSecondLayer;
  bool useTolerance;
  testFunction f;

  test()
  {
    useSecondLayer = true;
    useTolerance = false;
    f = 0;
  }
};

class topolTest: public QObject
{
Q_OBJECT

public:
  topolTest();
  ~topolTest();

  QMap<QString, test> testMap() { return mTestMap; }
  ErrorList runTest(QString testName, QgsVectorLayer* layer1, QgsVectorLayer* layer2, QgsRectangle extent, double tolerance);

  ErrorList checkIntersections(double tolerance, QgsVectorLayer* layer1, QgsVectorLayer* layer2);
  ErrorList checkSelfIntersections(double tolerance, QgsVectorLayer* layer1, QgsVectorLayer* layer2);
  ErrorList checkCloseFeature(double tolerance, QgsVectorLayer* layer1, QgsVectorLayer* layer2);
  ErrorList checkPolygonContains(double tolerance, QgsVectorLayer* layer1, QgsVectorLayer* layer2);
  ErrorList checkSegmentLength(double tolerance, QgsVectorLayer* layer1, QgsVectorLayer* layer2);
  ErrorList checkUnconnectedLines(double tolerance, QgsVectorLayer* layer1, QgsVectorLayer* layer2);
  ErrorList checkPointCoveredBySegment(double tolerance, QgsVectorLayer* layer1, QgsVectorLayer* layer2);
  ErrorList checkValid(double tolerance, QgsVectorLayer* layer1, QgsVectorLayer* layer2);

public slots:
  void setTestCancelled();

private:
  QMap<QString, QgsSpatialIndex*> mLayerIndexes;
  QMap<QString, test> mTestMap;
  QgsMapLayerRegistry* mLayerRegistry;

  QList<FeatureLayer> mFeatureList1;
  QMap<int, FeatureLayer> mFeatureMap2;
  bool mTestCancelled;

  QgsSpatialIndex* createIndex(QgsVectorLayer* layer);
  void fillFeatureMap(QgsVectorLayer* layer);
  bool testCancelled();

signals:
  void progress(int value);
};

#endif
