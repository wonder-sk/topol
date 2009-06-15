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

#include "rulesDialog.h"
#include "geosFunctions.h"
#include "../../app/qgisapp.h"

//TODO: get rid of those global variables (, mFeatureList, ...
checkDock::checkDock(const QString &tableName, QgsVectorLayer* theLayer, QWidget* parent)
: QDockWidget(parent), Ui::checkDock()
{
  setupUi(this);

  mLayer = theLayer;
  mLayerRegistry = QgsMapLayerRegistry::instance();

  //mTestMap["Test self intersections"] = &checkDock::checkSelfIntersections;
  mTestMap["Test intersections"] = &checkDock::checkIntersections;
  mTestMap["Test dangling endpoints"] = &checkDock::checkDanglingEndpoints;
  mTestMap["Test features inside polygon"] = &checkDock::checkPolygonContains;
  mTestMap["Test points not covered by segments"] = &checkDock::checkPointCoveredBySegment;
  mTestMap["Test segment lengths"] = &checkDock::checkSegmentLength;
  mTestMap["Test geometry validity"] = &checkDock::checkValid;

/*
  QList<QString> layerNames;
  QList<QgsMapLayer*> layers = mLayerRegistry->mapLayers().values();
  for (int i = 0; i < layers.size(); ++i)
    layerNames << layers[i]->name();

  mConfigureDialog = new rulesDialog("Rules", mTestMap.keys(), layerNames, parent);
  */
  mConfigureDialog = new rulesDialog("Rules", mTestMap.keys(), mLayerRegistry->mapLayers().keys(), parent);
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
  delete mRBConflict;
  delete mRBFeature1;
  delete mRBFeature2;
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

/*void checkDock::showControls(QString& testString)
{

}*/

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

QgsGeometry* checkEndpoints(QgsGeometry* g1, QgsGeometry* g2, double tolerance)
{
	//TODO:MultiLines
  if (!g1 || !g2)
    return 0;

  if (g1->type() != QGis::Line || g2->type() != QGis::Line)
    return 0;

  QgsPoint endPoint = g1->asPolyline().last();
  QgsGeometry *g = QgsGeometry::fromPoint(endPoint);
  if (g2->distance(*g) < tolerance)
  {
    int before;
    QgsPoint minDistPoint;  
    g2->closestSegmentWithContext(endPoint, minDistPoint, before);
    delete g;
    
    QgsPolyline ls;
    ls << endPoint << minDistPoint;
    g = QgsGeometry::fromPolyline(ls);
    return g;
  }

  delete g;
  return 0;
}

void checkDock::checkDanglingEndpoints(double tolerance, QString layer1Str, QString layer2Str)
{
  QList<FeatureLayer>::Iterator it, jit;
  for (it = mFeatureList1.begin(); it != mFeatureList1.end(); ++it)
  {
    QgsGeometry* g1 = it->feature.geometry();

    for (jit = mFeatureList2.begin(); jit != mFeatureList2.end(); ++jit)
    {
      //if (it->feature.id() >= jit->feature.id())
        //continue;

      QgsGeometry* g2 = jit->feature.geometry();

      if (g1->distance(*g2) < tolerance)
      {
	QgsGeometry *c, *d;
	if ((c = checkEndpoints(g1, g2, tolerance)) || (d = checkEndpoints(g2, g1, tolerance)))
	{
          QgsRectangle r = g1->boundingBox();
	  QgsRectangle r2 = g2->boundingBox();
	  r.combineExtentWith(&r2);

	  QList<FeatureLayer> fls;
	  TopolErrorDangle* err;

          if (c)
	  {
	    fls << *it << *jit;
            err = new TopolErrorDangle(r, c, fls);
            mErrorListView->addItem(err->name() + QString(" %1 %2").arg(it->feature.id()).arg(jit->feature.id()));
	    mErrorList << err;
	  }
	  else if (d)
	  {
	    fls << *jit << *it;
            err = new TopolErrorDangle(r, d, fls);
	    mErrorList << err;
	  }
	  // TODO: ids from different layers can be same
	  // write id and layer name instead?
	}
      }
    }
  }
}

void checkDock::checkUnconnectedLines(double tolerance, QString layer1Str, QString layer2Str)
{
  QList<FeatureLayer>::Iterator it, jit;
  for (it = mFeatureList1.begin(); it != mFeatureList1.end(); ++it)
  {
    QgsGeometry* g1 = it->feature.geometry();

    for (jit = mFeatureList2.begin(); jit != mFeatureList2.end(); ++jit)
    {
      //if (it->feature.id() >= jit->feature.id())
        //continue;

      QgsGeometry* g2 = jit->feature.geometry();

      if (g1->distance(*g2) < tolerance)
      {
	QgsGeometry *c, *d;
	if ((c = checkEndpoints(g1, g2, tolerance)) || (d = checkEndpoints(g2, g1, tolerance)))
	{
          QgsRectangle r = g1->boundingBox();
	  QgsRectangle r2 = g2->boundingBox();
	  r.combineExtentWith(&r2);

	  QList<FeatureLayer> fls;
	  TopolErrorDangle* err;

          if (c)
	  {
	    fls << *it << *jit;
            err = new TopolErrorDangle(r, c, fls);
            mErrorListView->addItem(err->name() + QString(" %1 %2").arg(it->feature.id()).arg(jit->feature.id()));
	    mErrorList << err;
	  }
	  else if (d)
	  {
	    fls << *jit << *it;
            err = new TopolErrorDangle(r, d, fls);
	    mErrorList << err;
	  }
	}
      }
    }
  }
}

void checkDock::checkValid(double tolerance, QString layer1Str, QString layer2Str)
{
  QProgressDialog progress("Checking for intersections", "Abort", 0, mFeatureList1.size(), this);
  progress.setWindowModality(Qt::WindowModal);
  int i = 0;

  QList<FeatureLayer>::Iterator it;

  for (it = mFeatureList1.begin(); it != mFeatureList1.end(); ++it)
  {
    progress.setValue(++i);
    if (progress.wasCanceled())
      break;

    QgsGeometry* g = it->feature.geometry();
    if (!g)
    {
      g = new QgsGeometry;
      std::cout <<"creating new geometry\n";
    }

    if (!g->asGeos())
    {
      std::cout << "Geos geometry is NULL\n";
      continue;
    }

    std::cout<<"geos "<< it->feature.id() <<std::flush;

    if (!GEOSisValid(g->asGeos()))
    {
      QgsRectangle r = g->boundingBox();
      QList<FeatureLayer> fls;
      fls << *it << *it;

      TopolErrorValid* err = new TopolErrorValid(r, g, fls);
      mErrorList << err;
      mErrorListView->addItem(err->name() + QString(" %1").arg(it->feature.id()));
    }
  }
}

/*
void checkDock::checkPointInsidePolygon()
{
  QList<FeatureLayer>::Iterator it, jit;
  for (it = mFeatureList.begin(); it != mFeatureList.end(); ++it)
  {
    QgsGeometry* g1 = it->feature.geometry();
    if (g1->type() != QGis::Polygon)
      continue;

    for (jit = mFeatureList.begin(); jit != mFeatureList.end(); ++jit)
    {
      QgsGeometry* g2 = jit->feature.geometry();

      if (g2->type() != QGis::Point)
        continue;

      QgsPoint pt  = g2->asPoint();
      if (g1->contains(&pt))
      {
	QList<FeatureLayer> fls;
	fls << *it << *jit;
	TopolErrorContains* err = new TopolErrorContains(g1->boundingBox(), g2, fls);

	mErrorList << err;
        mErrorListView->addItem(err->name() + QString(" %1 %2").arg(it->feature.id()).arg(jit->feature.id()));
      }
    }
  }
}*/

void checkDock::checkPolygonContains(double tolerance, QString layer1Str, QString layer2Str)
{
  QList<FeatureLayer>::Iterator it, jit;
  for (it = mFeatureList1.begin(); it != mFeatureList1.end(); ++it)
  {
    QgsGeometry* g1 = it->feature.geometry();
    if (g1->type() != QGis::Polygon)
      continue;

    for (jit = mFeatureList2.begin(); jit != mFeatureList2.end(); ++jit)
    {
      //if (it->feature.id() == jit->feature.id())
        //continue;

      QgsGeometry* g2 = jit->feature.geometry();
      if (contains(g1, g2))
      {
	QList<FeatureLayer> fls;
	fls << *it << *jit;
	TopolErrorInside* err = new TopolErrorInside(g1->boundingBox(), g2, fls);

	mErrorList << err;
        mErrorListView->addItem(err->name() + QString(" %1 %2").arg(it->feature.id()).arg(jit->feature.id()));
      }
    }
  }
}

void checkDock::checkPointCoveredBySegment(double tolerance, QString layer1Str, QString layer2Str)
{
  QList<FeatureLayer>::Iterator it, jit;
  for (it = mFeatureList1.begin(); it != mFeatureList1.end(); ++it)
  {
    bool touched = false;
    QgsGeometry* g1 = it->feature.geometry();

    if (g1->type() != QGis::Point)
      continue;

    for (jit = mFeatureList2.begin(); jit != mFeatureList2.end(); ++jit)
    {
      QgsGeometry* g2 = jit->feature.geometry();

      if (g2->type() == QGis::Point)
        continue;

      // test if point touches other geometry
      if (touches(g1, g2))
      {
	touched = true;
	break;
      }
    }

    if (!touched)
    {
      QList<FeatureLayer> fls;
      fls << *it << *it;
      TopolErrorCovered* err = new TopolErrorCovered(g1->boundingBox(), g1, fls);

      mErrorList << err;
      mErrorListView->addItem(err->name() + QString(" %1").arg(it->feature.id()));
    }
  }
}

void checkDock::checkSegmentLength(double tolerance, QString layer1Str, QString layer2Str)
{
  //TODO: multi versions, distance from settings, more errors for one feature
  QList<FeatureLayer>::Iterator it, jit;
  for (it = mFeatureList1.begin(); it != mFeatureList1.end(); ++it)
  {
    QgsGeometry* g1 = it->feature.geometry();
    QgsPolygon pol;
    QgsPolyline segm;
    QgsPolyline ls;
    QList<FeatureLayer> fls;
    TopolErrorShort* err;

    switch (g1->type()) {
      case QGis::Line:
        ls = g1->asPolyline();

	for (int i = 1; i < ls.size(); ++i)
	{
	  if (ls[i-1].sqrDist(ls[i]) < tolerance)
	  {
	    fls.clear();
            fls << *it << *it;
	    segm.clear();
	    segm << ls[i-1] << ls[i];
            err = new TopolErrorShort(g1->boundingBox(), QgsGeometry::fromPolyline(segm), fls);
            mErrorList << err;
            mErrorListView->addItem(err->name() + QString(" %1").arg(it->feature.id()));
	  }
	}
      case QGis::Polygon:
        pol = g1->asPolygon();

	/*//TODO: jump out of outer cycle
	for (int i = 0; i < pol.size(); ++i)
	  for (int j = 1; j < pol[i].size(); ++j)
	    if (pol[i][j-1].sqrDist(pol[i][j]) < tolerance)
	    {
	      fls.clear();
              fls << *it << *it;
	      segm.clear();
	      segm << pol[i][j-1] << pol[i][j];
              err = new TopolErrorShort(g1->boundingBox(), QgsGeometry::fromPolyline(segm), fls);
              mErrorList << err;
              mErrorListView->addItem(err->name() + QString(" %1").arg(it->feature.id()));
	    }*/
      break;
      default:
        continue;
    }
  }
}

/*void checkDock::checkSelfIntersections(double tolerance, QString layer1Str, QString layer2Str)
{
}*/

void checkDock::checkIntersections(double tolerance, QString layer1Str, QString layer2Str)
{
  int i = 0;
  QgsSpatialIndex* index = mLayerIndexes[layer2Str];
  if (!index)
  {
    std::cout << "No index for layer " << layer2Str << "!\n";
    return;
  }

  QProgressDialog progress("Checking for intersections", "Abort", 0, mFeatureList1.size(), this);
  progress.setWindowModality(Qt::WindowModal);

  QList<FeatureLayer>::Iterator it;
  QList<FeatureLayer>::ConstIterator FeatureListEnd = mFeatureList1.end();

  QgsVectorLayer* layer2 = (QgsVectorLayer*)mLayerRegistry->mapLayers()[layer2Str];
  QStringList itemList;
  
  for (it = mFeatureList1.begin(); it != FeatureListEnd; ++it)
  {
    if (!(++i % 50))
      progress.setValue(i);

    if (progress.wasCanceled())
      break;

    QgsGeometry* g1 = it->feature.geometry();
    QList<int> crossingIds = index->intersects(g1->boundingBox());
    int crossSize = crossingIds.size();

    for (int i = 0; i < crossSize; ++i)
    {
      int id = crossingIds[i];

      if (it->feature.id() == id)
	continue;

      //QgsFeature f;
      //layer2->featureAtId(id, f, true, false);
      QgsFeature& f = mFeatureMap2[id].feature;
      QgsGeometry* g2 = f.geometry();

      if (g1->intersects(g2))
      {
        QgsRectangle r = g1->boundingBox();
	QgsRectangle r2 = g2->boundingBox();
	r.combineExtentWith(&r2);

	QgsGeometry* c = g1->intersection(g2);
	// could this for some reason return NULL?
	//if (!c)
	  //c = new QgsGeometry;

	QList<FeatureLayer> fls;
	FeatureLayer fl;
	fl.feature = f;
	fl.layer = layer2;
	fls << *it << fl;
	TopolErrorIntersection* err = new TopolErrorIntersection(r, c, fls);

	mErrorList << err;
	itemList << err->name() + QString(" %1").arg(it->feature.id());
      }
    }
  }

  mErrorListView->addItems(itemList);
}

QgsSpatialIndex* checkDock::createIndex(QgsVectorLayer* layer, QgsRectangle extent)
{
  //TODO: progressbar 
  QgsSpatialIndex* index = new QgsSpatialIndex();
  layer->select(QgsAttributeList(), extent);

  QgsFeature f;
  while (layer->nextFeature(f))
    if (f.geometry())
    { 
      index->insertFeature(f);
      mFeatureMap2[f.id()] = FeatureLayer(layer, f);
    }

  return index;
}

void checkDock::runTests(QgsRectangle extent)
{
  for (int i = 0; i < mTestTable->rowCount(); ++i)
  {
    QString test = mTestTable->itemAt(i, 0)->text();
    QString layer1Str = mTestTable->item(i, 1)->text();
    QString layer2Str = mTestTable->item(i, 2)->text();

    QString toleranceStr = mTestTable->itemAt(i, 3)->text();

    QgsVectorLayer* layer1 = (QgsVectorLayer*)mLayerRegistry->mapLayers()[layer1Str];
    QgsVectorLayer* layer2 = (QgsVectorLayer*)mLayerRegistry->mapLayers()[layer2Str];

    if (!layer1 || !layer2)
    {
      std::cout << "layer " << layer1Str.toStdString() << " or " << layer2Str.toStdString() << " not found in registry!" << std::flush;
      return;
    }

    //if (!mLayerIndexes.contains(layer1Str))
      //mLayerIndexes[layer1Str] = createIndex(layer1, extent);
    if (!mLayerIndexes.contains(layer2Str))
      mLayerIndexes[layer2Str] = createIndex(layer2, extent);

    mFeatureList1.clear();
    mFeatureMap2.clear();

    QgsFeature f;

    //TODO: ?
    layer1->select(QgsAttributeList(), extent);
    while (layer1->nextFeature(f))
      if (f.geometry())
        mFeatureList1 << FeatureLayer(layer1, f);

    //call test routine
    (this->*mTestMap[test])(toleranceStr.toDouble(), layer1Str, layer2Str);
    //checkIntersections(toleranceStr.toDouble(), layer1Str, layer2Str);
  }
}

void checkDock::validate(QgsRectangle extent)
{
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
