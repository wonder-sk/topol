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
#include "topolTest.h"
#include "dockModel.h"

class QgsMapLayerRegistry;
class QgsRubberBand;
class QgisApp;
class QgisInterface;
class checkDock;

class checkDock : public QDockWidget, public Ui::checkDock
{
Q_OBJECT

public:
  checkDock(const QString &tableName, QgisInterface* qIface, QWidget *parent = 0);
  ~checkDock();

private slots:
  void configure();
  void fix();
  void validateAll();
  void validateExtent();
  void validateSelected();
  void errorListClicked(const QModelIndex& index);
  void deleteErrors();
  void parseErrorListByLayer(QString layerId);

private:
  rulesDialog* mConfigureDialog;
  QgsRubberBand* mRBConflict;
  QgsRubberBand* mRBFeature1;
  QgsRubberBand* mRBFeature2;
  QgisApp* mQgisApp;

  ErrorList mErrorList;
  DockModel* mErrorListModel;

  //pointer to topology tests table
  QTableWidget* mTestTable;

  topolTest mTest;
  QgsMapLayerRegistry* mLayerRegistry;

  void runTests(ValidateType type);
  void validate(ValidateType type);
  void parseErrorListByFeature(int featureId);
};

#endif
