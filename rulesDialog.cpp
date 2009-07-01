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

#include "../../app/qgisapp.h"

#include "rulesDialog.h"
#include "topolTest.h"

//TODO check when layer add/deleted
rulesDialog::rulesDialog(const QString &tableName, QList<QString> layerList, QMap<QString, test> testMap, QWidget *parent)
: QDialog(parent), Ui::rulesDialog()
{
  setupUi(this);

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
  connect(mDeleteTestButton, SIGNAL(clicked()), this, SLOT(deleteTest()));
  connect(mTestBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(showControls(const QString&)));
}

rulesDialog::~rulesDialog() {}

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

  //mLayer1Box->removeItem(mLayer1Box->findText(layerId));
  //mLayer2Box->removeItem(mLayer2Box->findText(layerId));
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

 std::cout << "index: "<<mLayer1Box->currentIndex();
 std::cout << "index2: "<<mLayer2Box->currentIndex();
 std::cout << "size: "<<mLayerIds.size()<<std::flush;
 // add layer ids to hidden columns
 // -1 for "No layer"
 newItem = new QTableWidgetItem(mLayerIds[mLayer1Box->currentIndex() - 1]);
 mTestTable->setItem(row, 4, newItem);
 newItem = new QTableWidgetItem(mLayerIds[mLayer2Box->currentIndex() - 1]);
 mTestTable->setItem(row, 5, newItem);

 mTestBox->setCurrentIndex(0);
 mLayer1Box->setCurrentIndex(0);
 mLayer2Box->setCurrentIndex(0);
}

void rulesDialog::deleteTest()
{
  int row = mTestTable->currentRow();
  if (0 <= row && row < mTestTable->rowCount()) 
    mTestTable->removeRow(row);
}
