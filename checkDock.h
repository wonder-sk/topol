#ifndef CHECKDOCK_H
#define CHECKDOCK_H

#include <QDockWidget>

#include <qgsvectorlayer.h>
#include <qgsgeometry.h>

#include "ui_checkDock.h"
#include "rulesDialog.h"
#include "topolError.h"

class QgsMapLayerRegistry;
class QgsRubberBand;
class QgisApp;
class checkDock;

typedef QList<TopolError*> ErrorList;
typedef void (checkDock::*testFunction)();

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
  QgsRubberBand* mRubberBand;
  QgsRubberBand* rub1;
  QgsRubberBand* rub2;
  QgisApp* mQgisApp;

  double mTolerance;

  QList<FeatureLayer> mFeatureList;
  ErrorList mErrorList;
  QgsGeometryMap mGeometryMap;

  //pointers to topology test table
  QTableWidget* mTestTable;

  QMap<QString, testFunction> mTestMap;
  QgsMapLayerRegistry* mLayerRegistry;

  void checkIntersections();
  void checkSelfIntersections();
  void checkDanglingEndpoints();
  void checkPolygonContains();
  void checkSegmentLength();
  void checkPointCoveredBySegment();

  void runTests(QgsRectangle extent);
  void validate(QgsRectangle extent);
};

#endif
