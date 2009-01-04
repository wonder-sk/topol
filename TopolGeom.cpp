#include <QSet>
#include <QList>
#include <QTextEdit>

#include <qgsgeometry.h>
#include <qgspoint.h>

#include "TopolGeom.h"
#include "show.h"

TopolGeom::TopolGeom()
{
}

TopolGeom::TopolGeom(QgsVectorLayer *theLayer, QTextEdit *theWindow)
{
  mLayer = theLayer;
  mWindow = theWindow;
    
  getFeatures();
}	

TopolGeom::~TopolGeom()
{
  QMap<int, QgsGeometry>::Iterator obj;
  for (obj = mObjects.begin(); obj != mObjects.end(); ++obj)
    free(&obj.value());
}

void TopolGeom::getFeatures()
{
  mLayer->select(QgsAttributeList());

  QgsFeature f;
  QgsGeometry *g;

  while (mLayer->nextFeature(f))
  {
    g = f.geometryAndOwnership();
    if (g)
      mObjects[f.id()] = *g;
  }
}

  // TODO: Polygon version not done
void TopolGeom::buildIntersections()
{
  QMap<int, QgsGeometry>::Iterator obj;

 QgsGeometry *ge = &mObjects.begin().value();
 obj = mObjects.begin();
  for (++obj; obj != mObjects.end(); ++obj)
    ge = ge->combine(&obj.value());

  QgsMultiPolyline mpol = ge->asMultiPolyline();
  QgsMultiPolyline::Iterator mpit = mpol.begin();

  for (; mpit != mpol.end(); ++mpit)
  {	  
//    show_line(*mpit);

    mmNodes[mpit->first().toString()].point = mpit->first();
    mmNodes[mpit->first().toString()].arcs.append(*mpit);
    mmNodes[mpit->last().toString()].point = mpit->last();
    mmNodes[mpit->last().toString()].arcs.append(*mpit);
  }
}

// create layers containing nodes and arcs
void TopolGeom::buildGeometry(QgsVectorLayer *nodeLayer, QgsVectorLayer *arcLayer)
{
  mNodes.clear();
  buildIntersections();

  // create node and arc layers
  QgsFeature f;
  int id = 0;

  // node layer
  nodeLayer->startEditing();

  QMap<QString, TopolNode>::ConstIterator node = mmNodes.begin();
  for (; node != mmNodes.end(); ++node)
  {
    f.setGeometry(QgsGeometry::fromPoint(node.value().point));
    f.addAttribute(0, id);
    nodeLayer->addFeature(f, false);
    ++id;
  }

  nodeLayer->updateExtents();

  // arc layer
  arcLayer->startEditing();
  id = 0;
  node = mmNodes.begin();
  for (; node != mmNodes.end(); ++node)
  {
    QList<QgsPolyline>::ConstIterator arc = node.value().arcs.begin();
    for (; arc != node.value().arcs.end(); ++arc)
    {
	QgsPolyline::ConstIterator it =  arc->begin();
        for (; it != arc->end(); ++it)
	    std::cout << it->x() << ", ";
      f.setGeometry(QgsGeometry::fromPolyline(*arc));
      if (!f.geometry())
	      std::cout << "kruci\n";
      else
      {
        f.addAttribute(0, id);
        arcLayer->addFeature(f, false);
        ++id;
      }
    }
  }

  arcLayer->updateExtents();
}

QgsFeatureIds TopolGeom::checkGeometry(CheckType type)
{
  mWindow->append("Check for: " + type);
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
  mWindow->append(text);
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
        mWindow->append(text);
      }
    }

  QString text = QString("# of intersections: %1\n").arg(conflicts);
  mWindow->append(text);
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
      mWindow->append(text);
    }  

  QString text = QString("# of intersections: %1\n").arg(conflicts);
  mWindow->append(text);
}
