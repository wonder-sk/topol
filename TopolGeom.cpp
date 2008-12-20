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
  QMap<int, QgsGeometry>::Iterator obj2;
  for (; obj1 != mObjects.end(); ++obj1)
    for (obj2 = mObjects.begin(); obj2 != mObjects.end(); ++obj2)
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
  QMap<int, QgsGeometry>::Iterator obj2;
  for (; obj1 != mObjects.end(); ++obj1)
    for (obj2 = mObjects.begin(); obj2 != mObjects.end(); ++obj2)
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

//TODO: split or simplify, lines with >=2 intersections doesn't work yet
void TopolGeom::buildIntersections()
{
  QMap<int, QgsGeometry>::Iterator obj1 = mObjects.begin();
  QMap<int, QgsGeometry>::Iterator obj2;

  // add all intersection points to nodes list
  for (; obj1 != mObjects.end(); ++obj1)
    for (obj2 = mObjects.begin(); obj2 != mObjects.end(); ++obj2)
    {
      if (obj1.key() >= obj2.key())
	continue;

      QgsGeometry *intersection = obj1.value().intersection(&obj2.value());
      QgsPoint point = intersection->asPoint();
    
      if (point != QgsPoint(0,0))
      {
	mIntersections.append(point);

        int at, before, after;
	double dist;

	// TODO: this should work on temporary objects, if not solved in a more clever way
        obj1.value().closestVertex(point, at, before, after, dist);
	obj1.value().insertVertex(point.x(), point.y(), after);
        
        TopolNode node;
	node.point = point;

        QgsPolyline polyline = obj1.value().asPolyline();
	QgsPolyline::Iterator it =  polyline.begin();

	QgsPolyline arc;

        for (; it != polyline.end(); ++it)
	{
	  arc.append(*it);

	  if (*it == point) 
	  {
	    // add first arc
	    node.arcs.append(arc);
            arc.clear();
	  }
	}

	// add second arc
	node.arcs.append(arc);

        obj2.value().closestVertex(point, at, before, after, dist);
	obj2.value().insertVertex(point.x(), point.y(), after);

        polyline = obj2.value().asPolyline();
	it =  polyline.begin();
	node.arcs.clear();

        for (; it != polyline.end(); ++it)
	{
	  arc.append(*it);

	  if (*it == point) 
	  {
	    // add third arc
	    node.arcs.append(arc);
            arc.clear();
	  }
	}

	// add fourth arc
	node.arcs.append(arc);

	// node is done -> add it to the list
	mNodes.append(node);
      }
    }
}

// create layers containing nodes and arcs
void TopolGeom::buildGeometry(QgsVectorLayer *nodeLayer, QgsVectorLayer *arcLayer)
{
  mNodes.clear();
  buildIntersections();

  TopolNode tempNode;

  QMap<int, QgsGeometry>::Iterator obj = mObjects.begin();
  // add beginnings and ends of lines to nodes list
  for (obj = mObjects.begin(); obj != mObjects.end(); ++obj)
  {
    QgsPolyline polyline = obj->asPolyline();

    tempNode.point = polyline.first();
    QgsPolyline::Iterator it =  polyline.begin();
    QgsPolyline arc;
    bool intersected = false;

    for (; it != polyline.end(); ++it)
    {
      arc.append(*it);

      if (mIntersections.contains(*it)) 
      {
	intersected = true;
        // add first node
        tempNode.arcs.append(arc);
        mNodes.append(tempNode);

	tempNode.point = polyline.last();
	arc.clear();
        arc.append(*it);
      }
    }

    // add second node
    tempNode.arcs.append(arc);
    mNodes.append(tempNode);

    // not intersected -> need to add the other end node
    if (!intersected)
    {
      //reverse arc
      QgsPolyline cra;
      for (it = arc.begin(); it != arc.end(); ++it)
        cra.prepend(*it);

      tempNode.arcs.append(cra);
      tempNode.point = polyline.last();
      mNodes.append(tempNode);
    }
  }

  // create node and arc layers
  QgsFeature f;
  int id = 0;

  // node layer
  nodeLayer->startEditing();

  QList<TopolNode>::ConstIterator node = mNodes.begin();
  for (; node != mNodes.end(); ++node)
  {
    f.setGeometry(QgsGeometry::fromPoint(node->point));
    f.addAttribute(0, id);
    nodeLayer->addFeature(f);
    ++id;
  }

  // arc layer
  // TODO: arcs added more than once
  arcLayer->startEditing();

  id = 0;
  node = mNodes.begin();
  for (; node != mNodes.end(); ++node)
  {
    QList<QgsPolyline>::ConstIterator arc = node->arcs.begin();
    for (; arc != node->arcs.end(); ++arc)
    {
      f.setGeometry(QgsGeometry::fromPolyline(*arc));
      f.addAttribute(0, id);
      arcLayer->addFeature(f);
      ++id;
    }
  }
}
