/***************************************************************************
  checkDock.cpp 
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

#include "checkDock.h"

#include <qgsvectordataprovider.h>
#include <qgsvectorlayer.h>
#include <qgsmaplayer.h>
#include <qgssearchstring.h>
#include <qgssearchtreenode.h>
#include <qgsmaplayer.h>
#include <qgsmaplayerregistry.h>
#include <qgsgeometry.h>
#include <qgsfeature.h>
#include <qgsmapcanvas.h>
#include <qgsrubberband.h>
#include <qgsproviderregistry.h>
#include <qgslogger.h>
#include <spatialindex/qgsspatialindex.h>

#include "../../app/qgisapp.h"

#include "topolTest.h"
#include "rulesDialog.h"
//#include "geosFunctions.h"

//TODO: get rid of those global variables (, mFeatureList, ...
checkDock::checkDock(const QString &tableName, QgsVectorLayer* theLayer, QWidget* parent)
: QDockWidget(parent), Ui::checkDock()
{
  setupUi(this);

  mLayer = theLayer;
  mLayerRegistry = QgsMapLayerRegistry::instance();

/*
  QList<QString> layerNames;
  QList<QgsMapLayer*> layers = mLayerRegistry->mapLayers().values();
  for (int i = 0; i < layers.size(); ++i)
    layerNames << layers[i]->name();

  mConfigureDialog = new rulesDialog("Rules", mTestMap.keys(), layerNames, parent);
  */
  mConfigureDialog = new rulesDialog("Rules", mLayerRegistry->mapLayers().keys(), mTest.testMap(), parent);
  mTestTable = mConfigureDialog->testTable();
  
  mQgisApp = QgisApp::instance();

  mValidateExtentButton->setIcon(QIcon(":/topol_c/topol.png"));
  mValidateAllButton->setIcon(QIcon(":/topol_c/topol.png"));
  mConfigureButton->setIcon(QIcon(":/topol_c/topol.png"));

  mRBFeature1 = new QgsRubberBand(mQgisApp->mapCanvas(), mLayer);
  mRBFeature2 = new QgsRubberBand(mQgisApp->mapCanvas(), mLayer);
  mRBConflict = new QgsRubberBand(mQgisApp->mapCanvas(), mLayer);

  mRBFeature1->setColor("blue");
  mRBFeature2->setColor("red");
  mRBConflict->setColor("gold");

  mRBFeature1->setWidth(5);
  mRBFeature2->setWidth(5);
  mRBConflict->setWidth(5);

  connect(mConfigureButton, SIGNAL(clicked()), this, SLOT(configure()));

  connect(mValidateAllButton, SIGNAL(clicked()), this, SLOT(validateAll()));
  connect(mValidateExtentButton, SIGNAL(clicked()), this, SLOT(validateExtent()));
  connect(mFixButton, SIGNAL(clicked()), this, SLOT(fix()));
  connect(mErrorListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(errorListClicked(const QModelIndex &)));

  connect(mLayerRegistry, SIGNAL(layerWillBeRemoved(QString)), mConfigureDialog, SLOT(removeLayer(QString)));
  connect(mLayerRegistry, SIGNAL(layerWasAdded(QgsMapLayer*)), mConfigureDialog, SLOT(addLayer(QgsMapLayer*)));
}

checkDock::~checkDock()
{
  delete mRBConflict, mRBFeature1, mRBFeature2;
  delete mConfigureDialog;

  // delete errors in list
  QList<TopolError*>::Iterator it = mErrorList.begin();
  for (; it != mErrorList.end(); ++it)
    delete *it;

  // delete spatial indexes
  QMap<QString, QgsSpatialIndex*>::Iterator lit = mLayerIndexes.begin();
  for (; lit != mLayerIndexes.end(); ++lit)
    delete *lit;
}

void checkDock::configure()
{
  mConfigureDialog->show();
}

void checkDock::errorListClicked(const QModelIndex& index)
{
  //assert(mErrorList.count() != mErrorListView->count());
	std::cout << "sizes: " <<mErrorList.count() << " =?= "<< mErrorListView->count()<<std::flush;
  int row = index.row();
  QgsRectangle r = mErrorList[row]->boundingBox();
  r.scale(1.5);
  mQgisApp->mapCanvas()->setExtent(r);
  mQgisApp->mapCanvas()->refresh();

  mFixBox->clear();
  mFixBox->addItem("Select automatic fix");
  mFixBox->addItems(mErrorList[row]->fixNames());

  QgsFeature f;
  QgsGeometry* g;
  FeatureLayer fl = mErrorList[row]->featurePairs().first();
  fl.layer->featureAtId(fl.feature.id(), f, true, false);
  g = f.geometry();
  mRBFeature1->setToGeometry(g, mLayer);

  fl = mErrorList[row]->featurePairs()[1];
  fl.layer->featureAtId(fl.feature.id(), f, true, false);
  g = f.geometry();
  mRBFeature2->setToGeometry(g, mLayer);

  mRBConflict->setToGeometry(mErrorList[row]->conflict(), mLayer);
}

void checkDock::fix()
{
  int row = mErrorListView->currentRow();
  QString fixName = mFixBox->currentText();

  if (row == -1)
    return;

  mRBFeature1->reset();
  mRBFeature2->reset();
  mRBConflict->reset();

  if (mErrorList[row]->fix(fixName))
  {
    mErrorList.removeAt(row);
    delete mErrorListView->takeItem(row);
    mComment->setText(QString("%1 errors were found").arg(mErrorListView->count()));
    mLayer->triggerRepaint();
  }
  else
    QMessageBox::information(this, "Topology fix error", "Fixing failed!");
}

void checkDock::runTests(QgsRectangle extent)
{
  for (int i = 0; i < mTestTable->rowCount(); ++i)
  {

    QString testName = mTestTable->itemAt(i, 0)->text();
    QString layer1Str = mTestTable->item(i, 1)->text();
    QString layer2Str = mTestTable->item(i, 2)->text();

    std::cout << testName.toStdString();

    QString toleranceStr = mTestTable->itemAt(i, 3)->text();

    QgsVectorLayer* layer1 = (QgsVectorLayer*)mLayerRegistry->mapLayers()[layer1Str];
    QgsVectorLayer* layer2 = (QgsVectorLayer*)mLayerRegistry->mapLayers()[layer2Str];
/*
    if (!layer1)
    {
      std::cout << "CheckDock: layer " << layer1Str.toStdString() << " not found in registry!" << std::flush;
      return;
    }

    if (!layer2 && mTest.testMap()[testName].showSecondLayer)
    {
      std::cout << "CheckDock: layer " << layer2Str.toStdString() << " not found in registry!" << std::flush;
      return;
    }*/

    QProgressDialog progress(testName, "Abort", 0, layer1->featureCount(), this);
    progress.setWindowModality(Qt::WindowModal);

    connect(&progress, SIGNAL(canceled()), &mTest, SLOT(setTestCancelled()));
    connect(&mTest, SIGNAL(progress(int)), &progress, SLOT(setValue(int)));
    ErrorList errors = mTest.runTest(testName, layer1, layer2, extent, toleranceStr.toDouble());
    disconnect(&progress, SIGNAL(canceled()), &mTest, SLOT(setTestCancelled()));
    disconnect(&mTest, SIGNAL(progress(int)), &progress, SLOT(setValue(int)));

    ErrorList::ConstIterator it = errors.begin();
    ErrorList::ConstIterator errorsEnd = errors.end();
    for (; it != errorsEnd; ++it)
      mErrorListView->addItem((*it)->name() + QString(" %1").arg((*it)->featurePairs().first().feature.id()));

    mErrorList << errors;
  }
}

void checkDock::validate(QgsRectangle extent)
{
  mErrorList.clear();
  mErrorListView->clear();

  runTests(extent);
  mComment->setText(QString("%1 errors were found").arg(mErrorListView->count()));

  mRBFeature1->reset();
  mRBFeature2->reset();
  mRBConflict->reset();
}

void checkDock::validateExtent()
{
  QgsRectangle extent = mQgisApp->mapCanvas()->extent();
  validate(extent);
}

void checkDock::validateAll()
{
  validate(QgsRectangle());
}
