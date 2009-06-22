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

typedef void (topolTest::*testFunction)(double, QString, QString);

class test
{

public:
  bool showSecondLayer;
  bool showTolerance;
  testFunction f;

  test()
  {
    showSecondLayer = false;
    showTolerance = false;
    f = 0;
  }
};

class topolTest: public QObject
{
Q_OBJECT

public:
  topolTest(QList<FeatureLayer> theFeatureList1, QMap<int, FeatureLayer> theFeatureMap2);

  ErrorList checkIntersections(double tolerance, QString layer1str, QString layer2Str);
  ErrorList checkSelfIntersections(double tolerance, QString layer1str, QString layer2Str);
  ErrorList checkCloseFeature(double tolerance, QString layer1str, QString layer2Str);
  ErrorList checkPolygonContains(double tolerance, QString layer1str, QString layer2Str);
  ErrorList checkSegmentLength(double tolerance, QString layer1str, QString layer2Str);
  ErrorList checkUnconnectedLines(double tolerance, QString layer1str, QString layer2Str);
  ErrorList checkPointCoveredBySegment(double tolerance, QString layer1str, QString layer2Str);
  ErrorList checkValid(double tolerance, QString layer1str, QString layer2Str);

public slots:
  void setTestCancelled();

private:
  QMap<QString, QgsSpatialIndex*> mLayerIndexes;
  QMap<QString, test> mTestMap;
  QgsMapLayerRegistry* mLayerRegistry;

  QList<FeatureLayer> mFeatureList1;
  QMap<int, FeatureLayer> mFeatureMap2;
  bool mTestCancelled;

  QgsSpatialIndex* createIndex(QgsVectorLayer* layer, QgsRectangle extent);
  bool testCancelled();

signals:
  void progress(int value);
};

#endif
