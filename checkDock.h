#ifndef CHECKDOCK_H
#define CHECKDOCK_H

#include <QDockWidget>

#include <qgsvectorlayer.h>
#include <qgsgeometry.h>

#include "ui_checkDock.h"
#include "rulesDialog.h"
#include "topolError.h"

class QgsRubberBand;

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
  QMap<TopolErrorType, QString> mErrorNameMap;
  QMap<TopolFixType, QString> mFixNameMap;
  QMap<int, QgsFeature> mFeatureMap;
  //QMap<validationError, QString> mErrorFixMap;
  //QMap<int, QgsRectangle> mErrorRectangleMap;
  QList<TopolError> mErrorList;
  QgsGeometryMap mGeometryMap;
  QgsRubberBand* mRubberBand;

  void initErrorMaps();
  void checkIntersections();
  void checkSelfIntersections();
  void checkDanglingEndpoints();
  //void updateValidationDock(int row, TopolErrorType errorType);
  void validate(QgsRectangle rect);
};

#endif
