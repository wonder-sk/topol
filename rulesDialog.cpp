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

#include "rulesDialog.h"

#include <qgsvectordataprovider.h>
#include <qgsvectorlayer.h>
#include <qgssearchstring.h>
#include <qgssearchtreenode.h>
#include <qgsmaplayer.h>
#include <qgsmaplayerregistry.h>

#include <qgsproviderregistry.h>
#include <qgslogger.h>

#include "../../app/qgisapp.h"

rulesDialog::rulesDialog(const QString &tableName, QList<QString> tests, QList<QString>layerList, QWidget *parent)
: QDialog(parent), Ui::rulesDialog()
{
  setupUi(this);

  mTestTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  mTestBox->addItems(QStringList(tests));
  mLayer1Box->addItems(QStringList(layerList));
  mLayer2Box->addItems(QStringList(layerList));

  mTestConfMap["Select test for addition"];
  mTestConfMap["Test intersections"].showLayer2 = true;
  mTestConfMap["Test dangling endpoints"].showTolerance = true;
  mTestConfMap["Test features inside polygon"];
  mTestConfMap["Test points not covered by segments"];
  mTestConfMap["Test segment lengths"].showTolerance = true;
  mTestConfMap["Test geometry validity"];

  connect(mAddTestButton, SIGNAL(clicked()), this, SLOT(addTest()));
  connect(mAddTestButton, SIGNAL(clicked()), mTestTable, SLOT(resizeColumnsToContents()));
  connect(mDeleteTestButton, SIGNAL(clicked()), this, SLOT(deleteTest()));
  connect(mTestBox, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(showControls(const QString&)));
}

rulesDialog::~rulesDialog() {}

void rulesDialog::showControls(const QString& testName)
{
std::cout << mTestConfMap[testName].showLayer2 << " "<<mTestConfMap[testName].showTolerance;
  mLayer2Box->setVisible(mTestConfMap[testName].showLayer2);
  bool showTolerance = mTestConfMap[testName].showTolerance;
  mToleranceBox->setVisible(showTolerance);
  mToleranceLabel->setVisible(showTolerance);
}

void rulesDialog::addLayer(QgsMapLayer* layer)
{
  mLayer1Box->addItem(layer->getLayerID());
  mLayer2Box->addItem(layer->getLayerID());
}

void rulesDialog::removeLayer(QString layerId)
{
  mLayer1Box->removeItem(mLayer1Box->findText(layerId));
  mLayer2Box->removeItem(mLayer2Box->findText(layerId));
}

void addTest() {}

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
  if (layer2 == "Layer" && mTestConfMap[test].showLayer2)
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
 
 if (mTestConfMap[test].showTolerance)
   newItem = new QTableWidgetItem(QString("%1").arg(mToleranceBox->value()));
 else
   newItem = new QTableWidgetItem(QString("None"));

 mTestTable->setItem(row, 3, newItem);

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
