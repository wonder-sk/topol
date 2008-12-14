#include <QSet>
#include <QList>
#include <QPlainTextEdit>

#include <qgsgeometry.h>
#include <qgspoint.h>

#include "TopolGeom.h"

TopolGeom::TopolGeom()
{
}

TopolGeom::TopolGeom(QgsVectorLayer *theLayer, QPlainTextEdit *theWindow)
{
  mLayer = theLayer;
  mWindow = theWindow;
    
  getFeatures();
}	

TopolGeom::~TopolGeom()
{
  //TODO: free geometries in mObjects
}

void TopolGeom::getFeatures()
{
  mLayer->select(QgsAttributeList());

  QgsFeature f;

  while (mLayer->nextFeature(f))
    mObjects[f.id()] = *f.geometryAndOwnership();
}

QgsFeatureIds TopolGeom::checkGeometry(CheckType type)
{
  mWindow->appendPlainText("Check for: " + type);
  QGis::GeometryType layertype = mLayer->geometryType();

  switch (type)
  {
    case Intersection:
      if (layertype != QGis::Point)
        cgIntersect();
      break;
    case Overlap:
      if (layertype != QGis::Point)
        cgOverlap();
      break;
    case Multipart:
      if (layertype != QGis::Point)
        cgMultipart();
      break;
  }

  return mConflicting;
}

void TopolGeom::cgIntersect()
{
  int conflicts = 0;
  QMap<int, QgsGeometry>::Iterator obj1 = mObjects.begin();
  QMap<int, QgsGeometry>::Iterator obj2 = mObjects.begin();

  for (; obj1 != mObjects.end(); ++obj1)
    for (; obj2 != mObjects.end(); ++obj2)
    {
      if (obj1.key() >= obj2.key())
	continue;

      if (obj1.value().intersects(&obj2.value()))
      {
        ++conflicts;
	mConflicting |= obj1.key();
	mConflicting |= obj2.key();
      }
    }

  QString text = QString("# of intersections: %1\n").arg(conflicts);
  mWindow->appendPlainText(text);
}

void TopolGeom::cgOverlap()
{
  int conflicts = 0;
  QMap<int, QgsGeometry>::Iterator obj1 = mObjects.begin();
  QMap<int, QgsGeometry>::Iterator obj2 = mObjects.begin();

  for (; obj1 != mObjects.end(); ++obj1)
    for (; obj2 != mObjects.end(); ++obj2)
    {
      if (obj1.key() >= obj2.key())
	continue;

      QgsGeometry *intersection = obj1.value().intersection(&obj2.value());
      QGis::GeometryType type = intersection->type();

      if (type != QGis::Point && type != QGis::UnknownGeometry)
      {
        ++conflicts;
	mConflicting |= obj1.key();
	mConflicting |= obj2.key();

        QString text = QString("Conflict: %1 %2\n").arg(obj1.key()).arg(obj2.key());
        mWindow->appendPlainText(text);
      }
    }

  QString text = QString("# of intersections: %1\n").arg(conflicts);
  mWindow->appendPlainText(text);
}

void TopolGeom::cgMultipart()
{
  int conflicts = 0;
  QMap<int, QgsGeometry>::Iterator obj = mObjects.begin();

  for (; obj != mObjects.end(); ++obj)
    if (obj.value().isMultipart())
    {
      ++conflicts;
      mConflicting |= obj.key();
      
      QString text = QString("Conflict: %1\n").arg(obj.key());
      mWindow->appendPlainText(text);
    }  

  QString text = QString("# of intersections: %1\n").arg(conflicts);
  mWindow->appendPlainText(text);
}

void TopolGeom::buildGeometry(QgsVectorLayer *nodeLayer, QgsVectorLayer *wayLayer)
{
//TODO: mNodes and mLines are not sets, QgsLine code not finished, split to more sub-functions
  std::cout << "Intersections:\n";

  QMap<int, QgsGeometry>::Iterator obj1 = mObjects.begin();
  QMap<int, QgsGeometry>::Iterator obj2 = mObjects.begin();

  // add all intersection points to nodes list
  for (; obj1 != mObjects.end(); ++obj1)
    for (; obj2 != mObjects.end(); ++obj2)
    {
      if (obj1.key() >= obj2.key())
	continue;

      QgsGeometry *intersection = obj1.value().intersection(&obj2.value());
    
      if (intersection->asPoint() != QgsPoint(0,0))
      {
        QgsPoint point = intersection->asPoint();
        std::cout << point.x() << ", " << point.y();

        int at, before, after;
	double dist;

	// this should work on temporary objects, if not solved in a more clever way
        obj1.value().closestVertex(point, at, before, after, dist);
	obj1.value().insertVertex(point.x(), point.y(), after);
        obj2.value().closestVertex(point, at, before, after, dist);
	obj2.value().insertVertex(point.x(), point.y(), after);
      }
    }

  std::cout << "Line Points:\n";

  mNodes.clear();
  mLines.clear();

  QMap<int, QgsGeometry>::Iterator obj = mObjects.begin();
  for (; obj != mObjects.end(); ++obj)
  {
    QgsPolyline polyline = obj->asPolyline();

    QgsPolyline::Iterator pt = polyline.begin();
    QgsPolyline::Iterator ppt = polyline.end();

    for (; pt != polyline.end(); ++pt)
    {
      if (ppt != polyline.end())
        mLines.append(QgsLine(*ppt, *pt));

      mNodes.append(*pt);
      ppt = pt;
    }

    //  mNodes |= *pt;

    //QList<QgsPoint> pointList = polyline.toList();
    //QSet<QgsPoint> pointSet = pointList.toSet(); 
    //mNodes |= pointSet;
    //mNodes |= polyline.toList().toSet();
    //mNodes |= QSet<QgsPoint>::fromList(polyline.toList());
      
    QgsPolyline::ConstIterator it = polyline.begin();
    for (; it != polyline.end(); ++it)
      std::cout << it->x() << ", " << it->y() << "\n";
  }

  /*
    std::cout << "All Nodes:\n";

    QList<QgsPoint>::ConstIterator node = mNodes.begin();
    for (; node != mNodes.end(); ++node)
      std::cout << node->x() << ", " << node->y() << "\n";
*/

  // create node and way layers
  QgsFeature f;
  int id = 0;

  // node layer
  nodeLayer->startEditing();

  QList<QgsPoint>::ConstIterator node = mNodes.begin();
  for (; node != mNodes.end(); ++node)
  {
    f.setGeometry(QgsGeometry::fromPoint(*node));
    f.addAttribute(0, id);
    nodeLayer->addFeature(f);
    ++id;
  }

  // way layer
  wayLayer->startEditing();
  id = 0;
/*
  QList<QgsLine>::Iterator line = mLines.begin();
  for (; line != mLines.end(); ++line)
  {
    f.setGeometry(QgsGeometry::fromWkt(line->wellKnownText()));
    //QString wkt = line->wellKnownText();
    //f.setGeometry(QgsGeometry::fromWkt(wkt));
    f.addAttribute(0, id);
    wayLayer->addFeature(f);
    ++id;
  }
*/
}
