#ifndef CHECKDOCK_H
#define CHECKDOCK_H

#include <QDockWidget>

#include <qgsvectorlayer.h>
#include <qgsgeometry.h>

#include "ui_checkDock.h"
#include "rulesDialog.h"
#include "topolError.h"

class QgsRubberBand;
class QgisApp;

typedef QList<TopolError*> ErrorList;

class FeatureLayer
{
public:
  FeatureLayer(QgsVectorLayer* theLayer, QgsFeature theFeature)
  {
    layer = theLayer; feature = theFeature;
  }

  QgsVectorLayer* layer;
  QgsFeature feature;
};

class checkDock : public QDockWidget, public Ui::checkDock
{
Q_OBJECT

public:
  checkDock(const QString &tableName, QgsVectorLayer *theLayer, rulesDialog* theConfigureDialog, QWidget *parent = 0);
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

  QList<FeatureLayer> mFeatureList;
  ErrorList mErrorList;
  QgsGeometryMap mGeometryMap;

  void checkIntersections();
  void checkSelfIntersections();
  void checkDanglingEndpoints();
  void validate(QgsRectangle rect);
};

#endif
