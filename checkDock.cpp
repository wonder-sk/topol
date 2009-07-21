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
#include "dockModel.h"
//#include "geosFunctions.h"

class QgisInterface;

checkDock::checkDock(QgisInterface* qIface, QWidget* parent)
: QDockWidget(parent), Ui::checkDock()
{
  setupUi(this);
  mErrorListModel = new DockModel(mErrorList, parent);
  mErrorTableView->setModel(mErrorListModel);
  mErrorTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
  mErrorTableView->verticalHeader()->setDefaultSectionSize( 20 );

  mLayerRegistry = QgsMapLayerRegistry::instance();
  mConfigureDialog = new rulesDialog(mLayerRegistry->mapLayers().keys(), mTest.testMap(), qIface, parent);
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
  connect(mValidateSelectedButton, SIGNAL(clicked()), this, SLOT(validateSelected()));
  connect(mValidateExtentButton, SIGNAL(clicked()), this, SLOT(validateExtent()));

  connect(mFixButton, SIGNAL(clicked()), this, SLOT(fix()));
  connect(mErrorTableView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(errorListClicked(const QModelIndex &)));

  connect(mLayerRegistry, SIGNAL(layerWasAdded(QgsMapLayer*)), mConfigureDialog, SLOT(addLayer(QgsMapLayer*)));
  connect(mLayerRegistry, SIGNAL(layerWillBeRemoved(QString)), mConfigureDialog, SLOT(removeLayer(QString)));
  connect(mLayerRegistry, SIGNAL(layerWillBeRemoved(QString)), this, SLOT(parseErrorListByLayer(QString)));

  connect(this, SIGNAL(visibilityChanged(bool)), this, SLOT(updateRubberBands(bool)));
  //connect(canvas, SIGNAL(scaleChanged(double)), this, SLOT(updateRubberBand(double)));
}

checkDock::~checkDock()
{
  /*mRBConflict->reset();
  mRBFeature1->reset();
  mRBFeature2->reset();
  mQgisApp->mapCanvas()->scene()->removeItem(mRBConflict);
  mQgisApp->mapCanvas()->scene()->removeItem(mRBFeature1);
  mQgisApp->mapCanvas()->scene()->removeItem(mRBFeature2);
  */

  delete mRBConflict, mRBFeature1, mRBFeature2;
  delete mConfigureDialog;
  delete mErrorListModel;

  // delete errors in list
  deleteErrors();
}

void checkDock::updateRubberBands(bool visible)
{
  if (!visible)
  {
    mRBConflict->reset();
    mRBFeature1->reset();
    mRBFeature2->reset();
  }
}

void checkDock::deleteErrors()
{
  QList<TopolError*>::Iterator it = mErrorList.begin();
  for (; it != mErrorList.end(); ++it)
    delete *it;

  mErrorList.clear();
  mErrorListModel->resetModel();
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

  mErrorListModel->resetModel();
  mComment->setText(QString("No errors were found"));
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
  mErrorListModel->resetModel();
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
  //std::cout << "1s\n";
  //std::cout << "g1 "<<g<<std::flush;
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
  //std::cout << "2s\n";
  //std::cout << "g2 "<<g<<std::flush;
  mRBFeature2->setToGeometry(g, fl.layer);

  if (!mErrorList[row]->conflict())
  {
    std::cout << "invalid conflict\n" << std::flush;
    return;
  }
  /*
  std::cout << "3s\n";
  std::cout << fl.layer<<std::flush;

    std::cout << "\nelc pol\n";
    //std::cout << "char1: "<<*(char *)(mErrorList[row]->conflict() + 1)<<"\n";
    //std::cout << std::flush;
    QgsGeometry* ggg = mErrorList[row]->conflict();
    */
  /*
  std::cout << "conflict "<<ggg<<std::flush;
    std::cout << "wkb size: "<<ggg->wkbSize()<<"\n";
    std::cout << std::flush;
    std::cout << "char: "<<*(char *)(ggg->asWkb() + 1)<<"\n";
    std::cout << std::flush;
    for (int i = 0; i < ggg->asPolygon().size();++i)
      for (int j = 0; j<ggg->asPolygon()[i].size();++j)
        std::cout<<ggg->asPolygon()[i][j].toString().toStdString()<<" | ";
    std::cout << std::flush;
    */

  mRBConflict->setToGeometry(mErrorList[row]->conflict(), fl.layer);
}

void checkDock::fix()
{
  int row = mErrorTableView->currentIndex().row();
  QString fixName = mFixBox->currentText();

  if (row == -1)
    return;

  mRBFeature1->reset();
  mRBFeature2->reset();
  mRBConflict->reset();

  if (mErrorList[row]->fix(fixName))
  {
    mErrorList.removeAt(row);
    mErrorListModel->resetModel();
    //parseErrorListByFeature();
    mComment->setText(QString("%1 errors were found").arg(mErrorList.count()));
    mQgisApp->mapCanvas()->refresh();
  }
  else
    QMessageBox::information(this, "Topology fix error", "Fixing failed!");
}

void checkDock::runTests(ValidateType type)
{
  for (int i = 0; i < mTestTable->rowCount(); ++i)
  {
    QString testName = mTestTable->item(i, 0)->text();
    QString toleranceStr = mTestTable->item(i, 3)->text();
    QString layer1Str = mTestTable->item(i, 4)->text();
    QString layer2Str = mTestTable->item(i, 5)->text();

    // test if layer1 is in the registry
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

    ErrorList errors = mTest.runTest(testName, layer1, layer2, type, toleranceStr.toDouble());
    disconnect(&progress, SIGNAL(canceled()), &mTest, SLOT(setTestCancelled()));
    disconnect(&mTest, SIGNAL(progress(int)), &progress, SLOT(setValue(int)));
    mErrorList << errors;
  }
  mErrorListModel->resetModel();
}

void checkDock::validate(ValidateType type)
{
  mErrorList.clear();

  runTests(type);
  mComment->setText(QString("%1 errors were found").arg(mErrorList.count()));

  mRBFeature1->reset();
  mRBFeature2->reset();
  mRBConflict->reset();
  mErrorTableView->resizeColumnsToContents();
}

void checkDock::validateExtent()
{
  validate(ValidateExtent);
}

void checkDock::validateAll()
{
  validate(ValidateAll);
}

void checkDock::validateSelected()
{
  validate(ValidateSelected);
}
