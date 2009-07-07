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

#include <qgsvectorlayer.h>
#include <qgsmaplayer.h>
#include <qgsgeometry.h>
#include <qgsfeature.h>
#include <spatialindex/qgsspatialindex.h>

#include "geosFunctions.h"
#include "../../app/qgisapp.h"

topolTest::topolTest()
{
  mTestCancelled = false;

  // one layer tests
  mTestMap["Test geometry validity"].f = &topolTest::checkValid;
  mTestMap["Test geometry validity"].useSecondLayer = false;

  mTestMap["Test segment lengths"].f = &topolTest::checkSegmentLength;
  mTestMap["Test segment lengths"].useTolerance = true;
  mTestMap["Test segment lengths"].useSecondLayer = false;

  mTestMap["Test unconnected lines"].f = &topolTest::checkDanglingLines;
  mTestMap["Test unconnected lines"].useSecondLayer = false;

  // two layer tests
  mTestMap["Test intersections"].f = &topolTest::checkIntersections;
  mTestMap["Test features inside polygon"].f = &topolTest::checkPolygonContains;
  mTestMap["Test points not covered by segments"].f = &topolTest::checkPointCoveredBySegment;
  mTestMap["Test feature too close"].f = &topolTest::checkCloseFeature;
  mTestMap["Test feature too close"].useTolerance = true;
}

topolTest::~topolTest()
{
  QMap<QString, QgsSpatialIndex*>::Iterator lit = mLayerIndexes.begin();
  for (; lit != mLayerIndexes.end(); ++lit)
    delete *lit;
}

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

ErrorList topolTest::checkCloseFeature(double tolerance, QgsVectorLayer* layer1, QgsVectorLayer* layer2)
{
  ErrorList errorList;
  QString secondLayerId = layer2->getLayerID();
  QgsSpatialIndex* index = mLayerIndexes[secondLayerId];
  if (!index)
  {
    std::cout << "No index for layer " << secondLayerId.toStdString() << "!\n";
    return errorList;
  }

  bool badG1 = false, badG2 = false;

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
    //QgsRectangle frame(bb.xMinimum() - tolerance, bb.yMinimum() - tolerance, bb.xMaximum() + tolerance, bb.yMaximum() + tolerance); 
    QgsRectangle frame(bb);

    QList<int> crossingIds;
    crossingIds = index->intersects(frame);
    int crossSize = crossingIds.size();
    
    QList<int>::Iterator cit = crossingIds.begin();
    QList<int>::ConstIterator crossingIdsEnd = crossingIds.end();

    for (; cit != crossingIdsEnd; ++cit)
    {
      QgsFeature& f = mFeatureMap2[*cit].feature;
      QgsGeometry* g2 = f.geometry();

      if (!g2 || !g2->asGeos())
      {
	badG2 = true;
        continue;
      }

      if (!g1 || !g1->asGeos())
      {
	badG1 = true;
        continue;
      }

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

  if (badG2)
    std::cout << "g2 or g2->asGeos() == NULL in close\n" << std::flush;

  if (badG1)
    std::cout << "g1 or g1->asGeos() == NULL in close\n" << std::flush;

  return errorList;
}

ErrorList topolTest::checkDanglingLines(double tolerance, QgsVectorLayer* layer1, QgsVectorLayer* layer2)
{
  //TODO: multilines
  int i = 0;
  ErrorList errorList;
  QString layerId = layer1->getLayerID();
  QgsSpatialIndex* index = mLayerIndexes[layerId];
  if (!index)
  {
    // attempt to create new index - it was not built in runtest()
    mLayerIndexes[layerId] = createIndex(layer1);
    index = mLayerIndexes[layerId];

    if (!index)
    {
      std::cout << "No index for layer " << layerId.toStdString() << "!\n";
      return errorList;
    }
  }
  else // map was not filled in runtest()
    fillFeatureMap(layer1);
  
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
    if (!startPoint || !endPoint)
      continue;

    bool touches = false;
    for (; cit != crossingIdsEnd; ++cit)
    {
      // skip itself
      if (mFeatureMap2[*cit].feature.id() == it->feature.id())
	continue;

      QgsGeometry* g2 = mFeatureMap2[*cit].feature.geometry();
      if (!g2)
      {
	std::cout << "g2 == NULL in unconnected\n" << std::flush;
        continue;
      }

      if (!g2->asGeos())
      {
	std::cout << "g2->asGeos() == NULL in unconnected\n" << std::flush;
        continue;
      }
	
      // test both endpoints
      if (geosTouches(startPoint, g2) || geosTouches(endPoint, g2))
      {
	touches = true;
	break;
      }
    }

    if (!touches)
    {
      QList<FeatureLayer> fls;
      fls << *it << *it;
      TopolErrorDangle* err = new TopolErrorDangle(bb, g1, fls);

      errorList << err;
    }

    delete startPoint, endPoint;
  }

  return errorList;
}

ErrorList topolTest::checkValid(double tolerance, QgsVectorLayer* layer1, QgsVectorLayer* layer2)
{
  //TODO: crashes when clicked on error | maybe only when more tests ran
  int i = 0;
  ErrorList errorList;
  QList<FeatureLayer>::Iterator it;

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

ErrorList topolTest::checkPolygonContains(double tolerance, QgsVectorLayer* layer1, QgsVectorLayer* layer2)
{
  int i = 0;
  ErrorList errorList;
  QString secondLayerId = layer2->getLayerID();
  QgsSpatialIndex* index = mLayerIndexes[secondLayerId];

  bool skipItself = layer1 == layer2;

  if (!index)
  {
    std::cout << "No index for layer " << secondLayerId.toStdString() << "!\n";
    return errorList;
  }

  if (layer1->geometryType() != QGis::Polygon)
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

    for (; cit != crossingIdsEnd; ++cit)
    {
      QgsFeature& f = mFeatureMap2[*cit].feature;
      QgsGeometry* g2 = f.geometry();

      // skip itself, when invoked with the same layer
      if (skipItself && f.id() == it->feature.id())
        continue;

      if (!g2)
      {
	std::cout << "g2 == NULL in contains\n" << std::flush;
        continue;
      }

      if (!g2->asGeos())
      {
	std::cout << "g2->asGeos() == NULL in contains\n" << std::flush;
        continue;
      }

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

ErrorList topolTest::checkPointCoveredBySegment(double tolerance, QgsVectorLayer* layer1, QgsVectorLayer* layer2)
{
  int i = 0;
  ErrorList errorList;
  QString secondLayerId = layer2->getLayerID();
  QgsSpatialIndex* index = mLayerIndexes[secondLayerId];

  if (!index)
  {
    std::cout << "No index for layer " << secondLayerId.toStdString() << "!\n";
    return errorList;
  }

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

      if (!g2 || !g2->asGeos())
      {
	std::cout << "g2 or g2->asGeos() == NULL in covered\n" << std::flush;
        continue;
      }

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
      //bb.scale(10);
      TopolErrorCovered* err = new TopolErrorCovered(bb, g1, fls);

      errorList << err;
    }
  }

  return errorList;
}

ErrorList topolTest::checkSegmentLength(double tolerance, QgsVectorLayer* layer1, QgsVectorLayer* layer2)
{
  //TODO: multi versions, use map units
  //TODO: move the type-switch from the cycle - it will be faster
  int i = 0;
  ErrorList errorList;
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

	for (int i = 0; i < pol.size(); ++i)
	  for (int j = 1; j < pol[i].size(); ++j)
	    if (pol[i][j-1].sqrDist(pol[i][j]) < tolerance)
	    {
	      fls.clear();
              fls << *it << *it;
	      segm.clear();
	      segm << pol[i][j-1] << pol[i][j];
              err = new TopolErrorShort(g1->boundingBox(), QgsGeometry::fromPolyline(segm), fls);
              errorList << err;
	    }
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

ErrorList topolTest::checkIntersections(double tolerance, QgsVectorLayer* layer1, QgsVectorLayer* layer2)
{
  int i = 0;
  ErrorList errorList;
  QString secondLayerId = layer2->getLayerID();
  QgsSpatialIndex* index = mLayerIndexes[secondLayerId];

  bool skipItself = layer1 == layer2;

  if (!index)
  {
    std::cout << "No index for layer " << secondLayerId.toStdString() << "!\n";
    return errorList;
  }

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
    for (; cit != crossingIdsEnd; ++cit)
    {
      QgsFeature& f = mFeatureMap2[*cit].feature;
      QgsGeometry* g2 = f.geometry();

      // skip itself, when invoked with the same layer
      if (skipItself && f.id() == it->feature.id())
	continue;

      if (!g2)
      {
        std::cout << "no second geometry\n";
	continue;
      }

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

void topolTest::fillFeatureMap(QgsVectorLayer* layer)
{
  layer->select(QgsAttributeList(), QgsRectangle());

  QgsFeature f;
  while (layer->nextFeature(f))
  {
    if (f.geometry())
    { 
      mFeatureMap2[f.id()] = FeatureLayer(layer, f);
    }
  }
}

QgsSpatialIndex* topolTest::createIndex(QgsVectorLayer* layer)
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
      mFeatureMap2[f.id()] = FeatureLayer(layer, f);
    }
  }

  return index;
}

ErrorList topolTest::runTest(QString testName, QgsVectorLayer* layer1, QgsVectorLayer* layer2, QgsRectangle extent, double tolerance)
{
  std::cout << testName.toStdString();
  ErrorList errors;

  if (!layer1)
  {
    std::cout << "First layer not found in registry!\n" << std::flush;
    return errors;
  }

  if (!layer2 && mTestMap[testName].useSecondLayer)
  {
    std::cout << "Second layer not found in registry!\n" << std::flush;
    return errors;
  }

  QString secondLayerId;
  mFeatureList1.clear();
  mFeatureMap2.clear();
  QgsFeature f;

  if (layer2)
  {
    secondLayerId = layer2->getLayerID();
    if (!mLayerIndexes.contains(secondLayerId))
      mLayerIndexes[secondLayerId] = createIndex(layer2);
    else
      fillFeatureMap(layer2);
  }

  layer1->select(QgsAttributeList(), extent);
  while (layer1->nextFeature(f))
    if (f.geometry())
      mFeatureList1 << FeatureLayer(layer1, f);

  //call test routine
  return (this->*(mTestMap[testName].f))(tolerance, layer1, layer2);
}
