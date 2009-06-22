/***************************************************************************
  topolTest.cpp 
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

#include "topolTest.h"

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

#include "geosFunctions.h"
#include "../../app/qgisapp.h"

/*topolTest::topolTest(const QString &tableName, QgsVectorLayer* theLayer, QWidget* parent)
: QDockWidget(parent), Ui::topolTest()
{
  mLayerRegistry = QgsMapLayerRegistry::instance();

  mTestMap["Test self intersections"] = &topolTest::checkSelfIntersections;
  mTestMap["Test intersections"] = &topolTest::checkIntersections;
  mTestMap["Test feature too close"] = &topolTest::checkCloseFeature;
  mTestMap["Test features inside polygon"] = &topolTest::checkPolygonContains;
  mTestMap["Test points not covered by segments"] = &topolTest::checkPointCoveredBySegment;
  mTestMap["Test segment lengths"] = &topolTest::checkSegmentLength;
  mTestMap["Test geometry validity"] = &topolTest::checkValid;
  mTestMap["Test unconnected lines"] = &topolTest::checkUnconnectedLines;

  QList<QString> layerNames;
  QList<QgsMapLayer*> layers = mLayerRegistry->mapLayers().values();
  for (int i = 0; i < layers.size(); ++i)
    layerNames << layers[i]->name();
  
  mQgisApp = QgisApp::instance();
}
*/

topolTest::topolTest(QList<FeatureLayer> theFeatureList1, QMap<int, FeatureLayer> theFeatureMap2)
{
  mFeatureList1 = theFeatureList1;
  mFeatureMap2 = theFeatureMap2;
  mTestCancelled = false;
}

/*QgsGeometry* checkEndpoints(QgsGeometry* g1, QgsGeometry* g2, double tolerance)
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
}*/

void topolTest::setTestCancelled()
{
  mTestCancelled = true;
}

bool topolTest::testCancelled()
{
  if (mTestCancelled)
  {
    mTestCancelled = false;
    return true;
  }

  return false;
}

ErrorList topolTest::checkCloseFeature(double tolerance, QString layer1Str, QString layer2Str)
{
  ErrorList errorList;
  QgsSpatialIndex* index = mLayerIndexes[layer2Str];
  if (!index)
  {
    std::cout << "No index for layer " << layer2Str.toStdString() << "!\n";
    return errorList;
  }

  QgsVectorLayer* layer2 = (QgsVectorLayer*)mLayerRegistry->mapLayers()[layer2Str];

  int i = 0;
  QList<FeatureLayer>::Iterator it;
  QList<FeatureLayer>::ConstIterator FeatureListEnd = mFeatureList1.end();
  for (it = mFeatureList1.begin(); it != FeatureListEnd; ++it)
  {
    if (!(++i % 100))
      emit progress(i);

    if (testCancelled())
      break;

    QgsGeometry* g1 = it->feature.geometry();
    QgsRectangle bb = g1->boundingBox();

    // increase bounding box by tolerance
    QgsRectangle frame(bb.xMinimum() - tolerance, bb.yMinimum() - tolerance, bb.xMaximum() + tolerance, bb.yMaximum() + tolerance); 

    QList<int> crossingIds;
    crossingIds = index->intersects(frame);
    int crossSize = crossingIds.size();
    
    QList<int>::Iterator cit = crossingIds.begin();
    QList<int>::ConstIterator crossingIdsEnd = crossingIds.end();

    for (; cit != crossingIdsEnd; ++cit)
    {
      QgsFeature& f = mFeatureMap2[*cit].feature;
      QgsGeometry* g2 = f.geometry();

      //why???
      if (!g2) {std::cout << "neeeee!"<<std::flush ;continue;}

      if (g1->distance(*g2) < tolerance)
      {
        QgsRectangle r = g2->boundingBox();
	r.combineExtentWith(&bb);

	QList<FeatureLayer> fls;
	FeatureLayer fl;
	fl.feature = f;
	fl.layer = layer2;
	fls << *it << fl;
	TopolErrorClose* err = new TopolErrorClose(r, g2, fls);

	errorList << err;
      }
    }
  }

  return errorList;
}
    /* ^
      if (g1->distance(*g2) < tolerance)
      {
	QgsGeometry *c, *d;
	if ((c = checkEndpoints(g1, g2, tolerance)) || (d = checkEndpoints(g2, g1, tolerance)))
	{
          QgsRectangle r = g1->boundingBox();
	  QgsRectangle r2 = g2->boundingBox();
	  r.combineExtentWith(&r2);

	  QList<FeatureLayer> fls;
	  TopolErrorClose* err;

          if (c)
	  {
	    fls << *it << *jit;
            err = new TopolErrorClose(r, c, fls);
            mErrorListView->addItem(err->name() + QString(" %1 %2").arg(it->feature.id()).arg(jit->feature.id()));
	    mErrorList << err;
	  }
	  else if (d)
	  {
	    fls << *jit << *it;
            err = new TopolErrorClose(r, d, fls);
	    mErrorList << err;
	  }
	  // TODO: ids from different layers can be same
	  // write id and layer name instead?
	}*/

ErrorList topolTest::checkUnconnectedLines(double tolerance, QString layer1Str, QString layer2Str)
{
	//TODO: multilines, seems to not work even for simple lines, grr
  ErrorList errorList;
  int i = 0;

  QgsSpatialIndex* index = mLayerIndexes[layer2Str];
  if (!index)
  {
    std::cout << "No index for layer " << layer2Str.toStdString() << "!\n";
    return errorList;
  }

  QgsVectorLayer* layer1 = (QgsVectorLayer*)mLayerRegistry->mapLayers()[layer1Str];
  QgsVectorLayer* layer2 = (QgsVectorLayer*)mLayerRegistry->mapLayers()[layer2Str];
  if (layer1->geometryType() != QGis::Line)
    return errorList;

  QList<FeatureLayer>::Iterator it;
  QList<FeatureLayer>::ConstIterator FeatureListEnd = mFeatureList1.end();
  for (it = mFeatureList1.begin(); it != FeatureListEnd; ++it)
  {
    if (!(++i % 100))
      emit progress(i);

    if (testCancelled())
      break;

    QgsGeometry* g1 = it->feature.geometry();
    QgsRectangle bb = g1->boundingBox();

    QList<int> crossingIds;
    crossingIds = index->intersects(bb);
    int crossSize = crossingIds.size();
    
    QList<int>::Iterator cit = crossingIds.begin();
    QList<int>::ConstIterator crossingIdsEnd = crossingIds.end();

    QgsGeometry* startPoint = QgsGeometry::fromPoint(g1->asPolyline().first());
    QgsGeometry* endPoint = QgsGeometry::fromPoint(g1->asPolyline().last());
    bool touchesStart = false;
    bool touchesEnd = false;

    for (; cit != crossingIdsEnd; ++cit)
    {
      QgsFeature& f = mFeatureMap2[*cit].feature;
      QgsGeometry* g2 = f.geometry();

      // find and test endpoints
      if (geosTouches(startPoint, g2))
      {
	touchesStart = true;
      }
      if (geosTouches(endPoint, g2))
      {
	touchesEnd = true;
      }
      if (touchesStart && touchesEnd)
        break;
    }

    if (! (touchesStart && touchesEnd) )
    {
      QList<FeatureLayer> fls;
      fls << *it << *it;
      TopolErrorUnconnected* err = new TopolErrorUnconnected(bb, g1, fls);

      errorList << err;
    }

    delete startPoint, endPoint;
  }

  return errorList;
}

ErrorList topolTest::checkValid(double tolerance, QString layer1Str, QString layer2Str)
{
  int i = 0;

  QList<FeatureLayer>::Iterator it;
  ErrorList errorList;

  for (it = mFeatureList1.begin(); it != mFeatureList1.end(); ++it)
  {
    if (!(++i % 100))
      emit progress(++i);
    if (testCancelled())
      break;

    QgsGeometry* g = it->feature.geometry();
    if (!g->asGeos() || !GEOSisValid(g->asGeos()))
    {
      QgsRectangle r = g->boundingBox();
      QList<FeatureLayer> fls;
      fls << *it << *it;

      TopolErrorValid* err = new TopolErrorValid(r, g, fls);
      errorList << err;
    }
  }

  return errorList;
}

ErrorList topolTest::checkPolygonContains(double tolerance, QString layer1Str, QString layer2Str)
{
  ErrorList errorList;
  int i = 0;

  QgsSpatialIndex* index = mLayerIndexes[layer2Str];
  if (!index)
  {
    std::cout << "No index for layer " << layer2Str.toStdString() << "!\n";
    return errorList;
  }

  QgsVectorLayer* layer2 = (QgsVectorLayer*)mLayerRegistry->mapLayers()[layer2Str];

  QList<FeatureLayer>::Iterator it;
  QList<FeatureLayer>::ConstIterator FeatureListEnd = mFeatureList1.end();
  for (it = mFeatureList1.begin(); it != FeatureListEnd; ++it)
  {
    if (!(++i % 100))
      emit progress(i);

    if (testCancelled())
      break;

    QgsGeometry* g1 = it->feature.geometry();
    QgsRectangle bb = g1->boundingBox();
    if (g1->type() != QGis::Polygon)
      break;

    QList<int> crossingIds;
    crossingIds = index->intersects(bb);
    int crossSize = crossingIds.size();
    
    QList<int>::Iterator cit = crossingIds.begin();
    QList<int>::ConstIterator crossingIdsEnd = crossingIds.end();

    for (; cit != crossingIdsEnd; ++cit)
    {
      QgsFeature& f = mFeatureMap2[*cit].feature;
      QgsGeometry* g2 = f.geometry();

      if (geosContains(g1, g2))
      {
	QList<FeatureLayer> fls;
	FeatureLayer fl;
	fl.feature = f;
	fl.layer = layer2;
	fls << *it << fl;
	TopolErrorInside* err = new TopolErrorInside(bb, g2, fls);

	errorList << err;
      }
    }
  }

  return errorList;
}

ErrorList topolTest::checkPointCoveredBySegment(double tolerance, QString layer1Str, QString layer2Str)
{
  ErrorList errorList;
  int i = 0;

  QgsSpatialIndex* index = mLayerIndexes[layer2Str];
  if (!index)
  {
    std::cout << "No index for layer " << layer2Str.toStdString() << "!\n";
    return errorList;
  }

  QgsVectorLayer* layer1 = (QgsVectorLayer*)mLayerRegistry->mapLayers()[layer1Str];
  QgsVectorLayer* layer2 = (QgsVectorLayer*)mLayerRegistry->mapLayers()[layer2Str];
  if (layer1->geometryType() != QGis::Point)
    return errorList;
  if (layer2->geometryType() == QGis::Point)
    return errorList;

  QList<FeatureLayer>::Iterator it;
  QList<FeatureLayer>::ConstIterator FeatureListEnd = mFeatureList1.end();
  for (it = mFeatureList1.begin(); it != mFeatureList1.end(); ++it)
  {
    if (!(++i % 100))
      emit progress(i);

    if (testCancelled())
      break;

    QgsGeometry* g1 = it->feature.geometry();
    QgsRectangle bb = g1->boundingBox();

    QList<int> crossingIds;
    crossingIds = index->intersects(bb);
    int crossSize = crossingIds.size();
    
    QList<int>::Iterator cit = crossingIds.begin();
    QList<int>::ConstIterator crossingIdsEnd = crossingIds.end();

    bool touched = false;

    for (; cit != crossingIdsEnd; ++cit)
    {
      QgsFeature& f = mFeatureMap2[*cit].feature;
      QgsGeometry* g2 = f.geometry();

      // test if point touches other geometry
      if (geosTouches(g1, g2))
      {
	touched = true;
	break;
      }
    }

    if (!touched)
    {
      QList<FeatureLayer> fls;
      fls << *it << *it;
      TopolErrorCovered* err = new TopolErrorCovered(bb, g1, fls);

      errorList << err;
    }
  }

  return errorList;
}

ErrorList topolTest::checkSegmentLength(double tolerance, QString layer1Str, QString layer2Str)
{
  //TODO: multi versions, distance from settings, more errors for one feature
  ErrorList errorList;
  int i = 0;

  QgsSpatialIndex* index = mLayerIndexes[layer2Str];
  if (!index)
  {
    std::cout << "No index for layer " << layer2Str.toStdString() << "!\n";
    return errorList;
  }

  QList<FeatureLayer>::Iterator it;
  QList<FeatureLayer>::ConstIterator FeatureListEnd = mFeatureList1.end();
  for (it = mFeatureList1.begin(); it != FeatureListEnd; ++it)
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
            errorList << err;
	  }
	}
      break;
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

  return errorList;
}

/*ErrorList topolTest::checkSelfIntersections(double tolerance, QString layer1Str, QString layer2Str)
{
}*/
/*
QList<QgsRectangle> splitRectangle(QgsRectangle r, int split)
{
  QList<QgsRectangle> rs;

  QgsPoint splitPoint = r.center();

  if (--split > 0)
  {
    rs << splitRectangle(QgsRectangle(QgsPoint(r.xMinimum(), r.yMaximum()), splitPoint), split);
    rs << splitRectangle(QgsRectangle(QgsPoint(r.xMinimum(), r.yMinimum()), splitPoint), split);
    rs << splitRectangle(QgsRectangle(QgsPoint(r.xMaximum(), r.yMaximum()), splitPoint), split);
    rs << splitRectangle(QgsRectangle(QgsPoint(r.xMaximum(), r.yMinimum()), splitPoint), split);
  }
  else
  {
    rs << QgsRectangle(QgsPoint(r.xMinimum(), r.yMaximum()), splitPoint);
    rs << QgsRectangle(QgsPoint(r.xMinimum(), r.yMinimum()), splitPoint);
    rs << QgsRectangle(QgsPoint(r.xMaximum(), r.yMaximum()), splitPoint);
    rs << QgsRectangle(QgsPoint(r.xMaximum(), r.yMinimum()), splitPoint);
  }

  return rs;
}

QList<QgsRectangle> crossingRectangles(QgsGeometry* g)
{
  QList<QgsRectangle> crossRectangles;
  QgsRectangle r = g->boundingBox();
  QList<QgsRectangle> tiles = splitRectangle(r, 1);
  QList<QgsRectangle>::ConstIterator it = tiles.begin();
  QList<QgsRectangle>::ConstIterator tilesEnd = tiles.end();

  for (; it != tilesEnd; ++it)
    if (g->intersects(*it))
      crossRectangles << *it;

  return crossRectangles;
}
*/

ErrorList topolTest::checkIntersections(double tolerance, QString layer1Str, QString layer2Str)
{
  ErrorList errorList;
  int i = 0;

  QgsSpatialIndex* index = mLayerIndexes[layer2Str];
  if (!index)
  {
    std::cout << "No index for layer " << layer2Str.toStdString() << "!\n";
    return errorList;
  }

  QList<FeatureLayer>::Iterator it;
  QList<FeatureLayer>::ConstIterator FeatureListEnd = mFeatureList1.end();

  QgsVectorLayer* layer2 = (QgsVectorLayer*)mLayerRegistry->mapLayers()[layer2Str];
  
  for (it = mFeatureList1.begin(); it != FeatureListEnd; ++it)
  {
    if (!(++i % 100))
      emit progress(i);

    if (testCancelled())
      break;

    QgsGeometry* g1 = it->feature.geometry();
    QgsRectangle bb = g1->boundingBox();

    QList<int> crossingIds;
    crossingIds = index->intersects(bb);
    /*
    QSet<int> crossingIds;
    QList<QgsRectangle> tiles = crossingRectangles(g1); 
    QList<QgsRectangle>::ConstIterator tilesIt = tiles.begin();
    QList<QgsRectangle>::ConstIterator tilesEnd = tiles.end();

    for (; tilesIt != tilesEnd; ++tilesIt)
      crossingIds |= index->intersects(*tilesIt).toSet();
      //crossingIds << index->intersects(*tilesIt);
*/
    int crossSize = crossingIds.size();
    //std::cout << "crossingFeatures size: " << crossingFeatures.size() << "\n";

    //QSet<int>::Iterator cit = crossingIds.begin();
    //QSet<int>::ConstIterator crossingIdsEnd = crossingIds.end();
    QList<int>::Iterator cit = crossingIds.begin();
    QList<int>::ConstIterator crossingIdsEnd = crossingIds.end();
    for (; cit != crossingIdsEnd; ++cit)
    {
      //QgsFeature f;
      //layer2->featureAtId(id, f, true, false);
      QgsFeature& f = mFeatureMap2[*cit].feature;
      QgsGeometry* g2 = f.geometry();

      if (g1->intersects(g2))
      {
        QgsRectangle r = bb;
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

	errorList << err;
      }
    }
  }

  return errorList;
}

QgsSpatialIndex* topolTest::createIndex(QgsVectorLayer* layer, QgsRectangle extent)
{
  QgsSpatialIndex* index = new QgsSpatialIndex();
  layer->select(QgsAttributeList(), QgsRectangle());

  int i = 0;
  QgsFeature f;
  while (layer->nextFeature(f))
  {
    if (!(++i % 100))
      emit progress(i);

    if (testCancelled())
    {
      delete index;
      return 0;
    }

    if (f.geometry())
    { 
      index->insertFeature(f);
      //mFeatureMap2[f.id()] = FeatureLayer(layer, f);
    }
  }

  return index;
}

/*
void topolTest::runTests(QgsRectangle extent)
{
  for (int i = 0; i < mTestTable->rowCount(); ++i)
  {
    QString test = mTestTable->itemAt(i, 0)->text();
	  std::cout << test.toStdString();
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

    mFeatureList1.clear();
    mFeatureMap2.clear();

    //if (!mLayerIndexes.contains(layer1Str))
      //mLayerIndexes[layer1Str] = createIndex(layer1, extent);
    if (!mLayerIndexes.contains(layer2Str))
      mLayerIndexes[layer2Str] = createIndex(layer2, extent);

    QgsFeature f;

    layer1->select(QgsAttributeList(), extent);
    while (layer1->nextFeature(f))
      if (f.geometry())
        mFeatureList1 << FeatureLayer(layer1, f);

    //call test routine
    (this->*mTestMap[test])(toleranceStr.toDouble(), layer1Str, layer2Str);
    //checkIntersections(toleranceStr.toDouble(), layer1Str, layer2Str);
  }
}
*/
