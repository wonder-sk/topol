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

class QgisInterface;

//TODO: get rid of those global variables (eg. mFeatureList)
checkDock::checkDock(const QString &tableName, QgisInterface* qIface, QWidget* parent)
: QDockWidget(parent), Ui::checkDock()
{
  setupUi(this);

  mLayerRegistry = QgsMapLayerRegistry::instance();
  mConfigureDialog = new rulesDialog("Rules", mLayerRegistry->mapLayers().keys(), mTest.testMap(), qIface, parent);
  mTestTable = mConfigureDialog->testTable();
  
  mValidateExtentButton->setIcon(QIcon(":/topol_c/topol.png"));
  mValidateAllButton->setIcon(QIcon(":/topol_c/topol.png"));
  mConfigureButton->setIcon(QIcon(":/topol_c/topol.png"));

  mQgisApp = QgisApp::instance();
  QgsMapCanvas* canvas = mQgisApp->mapCanvas();
  mRBFeature1 = new QgsRubberBand(canvas);
  mRBFeature2 = new QgsRubberBand(canvas);
  mRBConflict = new QgsRubberBand(canvas);

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

  connect(mLayerRegistry, SIGNAL(layerWasAdded(QgsMapLayer*)), mConfigureDialog, SLOT(addLayer(QgsMapLayer*)));
  connect(mLayerRegistry, SIGNAL(layerWillBeRemoved(QString)), mConfigureDialog, SLOT(removeLayer(QString)));
  connect(mLayerRegistry, SIGNAL(layerWillBeRemoved(QString)), this, SLOT(parseErrorListByLayer(QString)));

  //connect(canvas, SIGNAL(scaleChanged(double)), this, SLOT(updateRubberBand(double)));
}

checkDock::~checkDock()
{
  //TODO: doesn't work, rubberbands stay on canvas after plugin unloaded
  //TODO clear canvas from rubberbands 
  mRBConflict->reset();
  mRBFeature1->reset();
  mRBFeature2->reset();
  mQgisApp->mapCanvas()->scene()->removeItem(mRBConflict);
  mQgisApp->mapCanvas()->scene()->removeItem(mRBFeature1);
  mQgisApp->mapCanvas()->scene()->removeItem(mRBFeature2);

  delete mRBConflict, mRBFeature1, mRBFeature2;
  delete mConfigureDialog;

  // delete errors in list
  deleteErrors();

  // delete spatial indexes
  QMap<QString, QgsSpatialIndex*>::Iterator lit = mLayerIndexes.begin();
  for (; lit != mLayerIndexes.end(); ++lit)
    delete *lit;
}

/*void checkDock::updateRubberBand(double scale)
{
	std::cout << "updateRub\n";
  mRBConflict->updateRect();
  mRBFeature1->updateRect();
  mRBFeature2->updateRect();
}*/

void checkDock::deleteErrors()
{
  QList<TopolError*>::Iterator it = mErrorList.begin();
  for (; it != mErrorList.end(); ++it)
    delete *it;

  mErrorList.clear();
  mErrorListView->clear();
}

void checkDock::parseErrorListByLayer(QString layerId)
{
  QgsVectorLayer* layer = (QgsVectorLayer*)mLayerRegistry->mapLayers()[layerId];
  QList<TopolError*>::Iterator it = mErrorList.begin();
  QList<TopolError*>::Iterator end = mErrorList.end();

  while (it != mErrorList.end())
  {
    FeatureLayer fl1 = (*it)->featurePairs().first();
    FeatureLayer fl2 = (*it)->featurePairs()[1];
    if (fl1.layer == layer || fl2.layer == layer)
    {
      it = mErrorList.erase(it);
    }
    else
      ++it;
  }

  mComment->setText(QString("No errors were found"));
  mErrorListView->clear();

  ErrorList::ConstIterator lit = mErrorList.begin();
  ErrorList::ConstIterator errorsEnd = mErrorList.end();
  for (; lit != errorsEnd; ++lit)
    mErrorListView->addItem((*lit)->name() + QString(" %1").arg((*lit)->featurePairs().first().feature.id()));
}

void checkDock::parseErrorListByFeature(int featureId)
{
  QList<TopolError*>::Iterator it = mErrorList.begin();
  QList<TopolError*>::Iterator end = mErrorList.end();

  while (it != mErrorList.end())
  {
    FeatureLayer fl1 = (*it)->featurePairs().first();
    FeatureLayer fl2 = (*it)->featurePairs()[1];
    if (fl1.feature.id() == featureId || fl2.feature.id() == featureId)
    {
      it = mErrorList.erase(it);
    }
    else
      ++it;
  }

  mComment->setText(QString("No errors were found"));
  mErrorListView->clear();

  ErrorList::ConstIterator lit = mErrorList.begin();
  ErrorList::ConstIterator errorsEnd = mErrorList.end();
  for (; lit != errorsEnd; ++lit)
    mErrorListView->addItem((*lit)->name() + QString(" %1").arg((*lit)->featurePairs().first().feature.id()));
}

void checkDock::configure()
{
  mConfigureDialog->show();
}

void checkDock::errorListClicked(const QModelIndex& index)
{
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
  if (!fl.layer)
  {
    std::cout << "invalid layer 1\n";
    return;
  }

  fl.layer->featureAtId(fl.feature.id(), f, true, false);
  g = f.geometry();
  if (!g)
  {
    std::cout << "invalid geometry 1\n"<<std::flush;
    QMessageBox::information(this, "Topology test", "Feature not found in the layer.\nThe layer has probably changed.\nRun topology check again.");
    return;
  }
  mRBFeature1->setToGeometry(g, fl.layer);

  fl = mErrorList[row]->featurePairs()[1];
  if (!fl.layer)
  {
    std::cout << "invalid layer 2\n";
    return;
  }

  fl.layer->featureAtId(fl.feature.id(), f, true, false);
  g = f.geometry();
  if (!g)
  {
    std::cout << "invalid geometry 2\n" << std::flush;
    QMessageBox::information(this, "Topology test", "Feature not found in the layer.\nThe layer has probably changed.\nRun topology check again.");
    return;
  }
  mRBFeature2->setToGeometry(g, fl.layer);

  if (!mErrorList[row]->conflict())
  {
    std::cout << "invalid conflict\n" << std::flush;
    return;
  }
  mRBConflict->setToGeometry(mErrorList[row]->conflict(), fl.layer);
}

//TODO: delete items in errorListView
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
    //parseErrorListByFeature();
    mComment->setText(QString("%1 errors were found").arg(mErrorListView->count()));
    mQgisApp->mapCanvas()->refresh();
  }
  else
    QMessageBox::information(this, "Topology fix error", "Fixing failed!");
}

void checkDock::runTests(QgsRectangle extent)
{
  for (int i = 0; i < mTestTable->rowCount(); ++i)
  {
    QString testName = mTestTable->item(i, 0)->text();
    QString toleranceStr = mTestTable->item(i, 3)->text();
    QString layer1Str = mTestTable->item(i, 4)->text();
    QString layer2Str = mTestTable->item(i, 5)->text();

    std::cout << testName.toStdString();

    // test if layer1  is in the registry
    if (!((QgsVectorLayer*)mLayerRegistry->mapLayers().contains(layer1Str)))
    {
      std::cout << "CheckDock: layer " << layer1Str.toStdString() << " not found in registry!" << std::flush;
      return;
    }

    QgsVectorLayer* layer1 = (QgsVectorLayer*)mLayerRegistry->mapLayers()[layer1Str];
    QgsVectorLayer* layer2 = 0;

    if ((QgsVectorLayer*)mLayerRegistry->mapLayers().contains(layer2Str))
      layer2 = (QgsVectorLayer*)mLayerRegistry->mapLayers()[layer2Str];

    QProgressDialog progress(testName, "Abort", 0, layer1->featureCount(), this);
    progress.setWindowModality(Qt::WindowModal);

    connect(&progress, SIGNAL(canceled()), &mTest, SLOT(setTestCancelled()));
    connect(&mTest, SIGNAL(progress(int)), &progress, SLOT(setValue(int)));
    // run the test

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
