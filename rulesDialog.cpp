/***************************************************************************
  rulesDialog.cpp 
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

#include <QtGui>

#include <qgsvectordataprovider.h>
#include <qgsvectorlayer.h>
#include <qgssearchstring.h>
#include <qgssearchtreenode.h>
#include <qgsmaplayer.h>
#include <qgsmaplayerregistry.h>

#include <qgsproviderregistry.h>
#include <qgsmaplayerregistry.h>
#include <qgslogger.h>
#include <qgisinterface.h>
#include <qgsproject.h>

#include "../../app/qgisapp.h"

#include "rulesDialog.h"
#include "topolTest.h"

//TODO check when layer add/deleted
rulesDialog::rulesDialog(const QString &tableName, QList<QString> layerList, QMap<QString, test> testMap, QgisInterface* theQgisIface, QWidget *parent)
: QDialog(parent), Ui::rulesDialog()
{
  setupUi(this);

  mQgisIface = theQgisIface;
  mTestTable->hideColumn(4);
  mTestTable->hideColumn(5);

  mTestConfMap = testMap;
  mTestTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  mTestBox->addItems(mTestConfMap.keys());

  QgsMapLayerRegistry* layerRegistry = QgsMapLayerRegistry::instance();

  for (int i = 0; i < layerList.size(); ++i)
  {
    // add layer ID to the layerId list
    mLayerIds << layerList[i];

    // add layer name to the layer combo boxes
    mLayer1Box->addItem(((QgsVectorLayer*)layerRegistry->mapLayers()[layerList[i]])->name());
    mLayer2Box->addItem(((QgsVectorLayer*)layerRegistry->mapLayers()[layerList[i]])->name());
  }

  connect(mAddTestButton, SIGNAL(clicked()), this, SLOT(addTest()));
  connect(mAddTestButton, SIGNAL(clicked()), mTestTable, SLOT(resizeColumnsToContents()));
  // attempt to add new test when Ok clicked
  connect(buttonBox, SIGNAL(accepted()), this, SLOT(addTest()));
  connect(mDeleteTestButton, SIGNAL(clicked()), this, SLOT(deleteTest()));
  connect(mTestBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(showControls(const QString&)));

  //this resets this plugin up if a project is loaded
  connect( mQgisIface->mainWindow(), SIGNAL( projectRead() ), this, SLOT( projectRead() ) );
  projectRead();
}

rulesDialog::~rulesDialog() {}

void rulesDialog::projectRead()
{
	//TODO
  QString testName, layer1Id, layer2Id, tolerance;

  testName = QgsProject::instance()->readEntry( "Topol", "/testName", "" );
  tolerance = QgsProject::instance()->readEntry( "Topol", "/tolerance", "" );
  layer1Id = QgsProject::instance()->readEntry( "Topol", "/layer1", "" );
  layer2Id = QgsProject::instance()->readEntry( "Topol", "/layer2", "" );
}

void rulesDialog::showControls(const QString& testName)
{
  std::cout << mTestConfMap[testName].useSecondLayer << " "<< mTestConfMap[testName].useTolerance;
  mLayer2Box->setVisible(mTestConfMap[testName].useSecondLayer);

  bool useTolerance = mTestConfMap[testName].useTolerance;
  mToleranceBox->setVisible(useTolerance);
  mToleranceLabel->setVisible(useTolerance);
}

void rulesDialog::addLayer(QgsMapLayer* layer)
{
  mLayerIds << layer->getLayerID();

  // add layer name to the layer combo boxes
  mLayer1Box->addItem(layer->name());
  mLayer1Box->addItem(layer->name());
}

void rulesDialog::removeLayer(QString layerId)
{
  int index = mLayerIds.indexOf(layerId);
  mLayerIds.removeAt(index);
  mLayer1Box->removeItem(index);
  mLayer2Box->removeItem(index);
}

void rulesDialog::addTest()
{
  //sanity checks
  QString test = mTestBox->currentText();
  if (test == "Select test for addition")
    return;

  QString layer1 = mLayer1Box->currentText();
  if (layer1 == "Layer")
    return;

  QString layer2 = mLayer2Box->currentText();
  if (layer2 == "Layer" && mTestConfMap[test].useSecondLayer)
    return;

  //is inserting to qtablewidget really this stupid or am i missing something?
  int row = mTestTable->rowCount();
  mTestTable->insertRow(row);
 
  QTableWidgetItem* newItem;
  newItem = new QTableWidgetItem(test);
  mTestTable->setItem(row, 0, newItem);
  newItem = new QTableWidgetItem(layer1);
  mTestTable->setItem(row, 1, newItem);
  newItem = new QTableWidgetItem(layer2);
  mTestTable->setItem(row, 2, newItem);
 
  if (mTestConfMap[test].useTolerance)
    newItem = new QTableWidgetItem(QString("%1").arg(mToleranceBox->value()));
  else
    newItem = new QTableWidgetItem(QString("None"));

  mTestTable->setItem(row, 3, newItem);
 
  // add layer ids to hidden columns
  // -1 for "No layer"s string
  QString layer1ID = mLayerIds[mLayer1Box->currentIndex() - 1];
  QString layer2ID = mLayerIds[mLayer2Box->currentIndex() - 1];
  newItem = new QTableWidgetItem(layer1ID);
  mTestTable->setItem(row, 4, newItem);
  newItem = new QTableWidgetItem(layer2ID);
  mTestTable->setItem(row, 5, newItem);

  mTestBox->setCurrentIndex(0);
  mLayer1Box->setCurrentIndex(0);
  mLayer2Box->setCurrentIndex(0);

  // reset controls to default 
  mTestBox->setCurrentIndex(0);
  mLayer1Box->setCurrentIndex(0);
  mLayer2Box->setCurrentIndex(0);
  mToleranceBox->setValue(0);

  // save state to the project file.....
  QgsProject::instance()->writeEntry( "Topol", "/testname", test );
  QgsProject::instance()->writeEntry( "Topol", "/tolerance", mToleranceBox->value());
  QgsProject::instance()->writeEntry( "Topol", "/layer1", layer1ID );
  QgsProject::instance()->writeEntry( "Topol", "/layer2", layer2ID );
}

void rulesDialog::deleteTest()
{
  int row = mTestTable->currentRow();
  if (0 <= row && row < mTestTable->rowCount()) 
    mTestTable->removeRow(row);
}
