/***************************************************************************
  checkDock.h 
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

#ifndef CHECKDOCK_H
#define CHECKDOCK_H

#include <QDockWidget>

#include <qgsvectorlayer.h>
#include <qgsgeometry.h>
#include <spatialindex/qgsspatialindex.h>

#include "ui_checkDock.h"
#include "rulesDialog.h"
#include "topolError.h"

class QgsMapLayerRegistry;
class QgsRubberBand;
class QgisApp;
class checkDock;

typedef QList<TopolError*> ErrorList;
typedef void (checkDock::*testFunction)(double, QString, QString);

class checkDock : public QDockWidget, public Ui::checkDock
{
Q_OBJECT

public:
  checkDock(const QString &tableName, QgsVectorLayer *theLayer, QWidget *parent = 0);
  ~checkDock();

private slots:
  void configure();
  void fix();
  void validateAll();
  void validateExtent();
  void errorListClicked(const QModelIndex& index);

private:
  QgsVectorLayer *mLayer;
  rulesDialog* mConfigureDialog;
  QgsRubberBand* mRBConflict;
  QgsRubberBand* mRBFeature1;
  QgsRubberBand* mRBFeature2;
  QgisApp* mQgisApp;

  QList<FeatureLayer> mFeatureList1;
  QList<FeatureLayer> mFeatureList2;
  QMap<int, FeatureLayer> mFeatureMap2;
  QList<QString> mLayerNameList;

  ErrorList mErrorList;
  QgsGeometryMap mGeometryMap;

  //pointer to topology tests table
  QTableWidget* mTestTable;

  QMap<QString, QgsSpatialIndex*> mLayerIndexes;
  QMap<int, QgsGeometry*> mLayerGeometries;
  QMap<QString, testFunction> mTestMap;
  QgsMapLayerRegistry* mLayerRegistry;

  void checkIntersections(double tolerance, QString layer1str, QString layer2Str);
  void checkSelfIntersections(double tolerance, QString layer1str, QString layer2Str);
  void checkCloseFeature(double tolerance, QString layer1str, QString layer2Str);
  void checkPolygonContains(double tolerance, QString layer1str, QString layer2Str);
  void checkSegmentLength(double tolerance, QString layer1str, QString layer2Str);
  void checkUnconnectedLines(double tolerance, QString layer1str, QString layer2Str);
  void checkPointCoveredBySegment(double tolerance, QString layer1str, QString layer2Str);
  void checkValid(double tolerance, QString layer1str, QString layer2Str);

  QgsSpatialIndex* createIndex(QgsVectorLayer* layer, QgsRectangle extent);
  void runTests(QgsRectangle extent);
  void validate(QgsRectangle extent);
};

#endif
