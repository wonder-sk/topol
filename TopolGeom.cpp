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
	QMap<int, QgsGeometry*>::Iterator obj;
	for (obj = mObjects.begin(); obj != mObjects.end(); ++obj)
		delete(obj.value());
}

/**
 * This function updates QList arcs in this way:
 * Adds 2 or more new arcs, which were part of arc a to the start of the list. 
 *   (So that we can use the information about the number of already tested arcs from the start of the list.)
 * Adds 2 or more new arcs, which were part of arc arcs[b] to the end of the list.
 * Removes the arcs[b] arc and deletes it.
 */
void TopolGeom::updateArcs(Arc *a, int b, QgsGeometry *comb, QList<Arc*> *arcs)
{ 
  QgsMultiPolyline mpol = comb->asMultiPolyline();
  QgsGeometry *newGeom;
  int tested = b;
	
  // get pointer to the b arc, and then remove it from the list, 
  // while we did not change the index by adding new arcs 
  Arc *bb = (*arcs)[b];
  arcs->removeAt(b);
	
  for (QgsMultiPolyline::Iterator mpit = mpol.begin(); mpit != mpol.end(); ++mpit) {

    // get the polyline and its geometry
    newGeom = QgsGeometry::fromPolyline(*mpit);
    
    if (belongsTo(newGeom, a)) {
     // the polyline belongs to arc a, so it goes to the start of list
      arcs->prepend(new Arc(newGeom, a->featureID, tested));

      // the next polylines will not need to test against its "siblings" already inserted to arcs
      tested++;
      //std::cout << "newGeom: " << newGeom->asPolyline().first().toString().toStdString() << "\n" << std::flush;
      //std::cout << "arcs[0]: " << (*arcs)[0]->geom->asPolyline().first().toString().toStdString() << "\n" << std::flush;
    }
    else if (belongsTo(newGeom, bb)) {
      // the polyline belongs to arc b, so it goes to the end of list
      //std::cout << "patrim k B\n" << std::flush;
      arcs->append(new Arc(newGeom, bb->featureID, 0));
    }  
    else {
      std::cout << "nepatrim nikam!\n" << std::flush;
	  }  
  }
  
  delete bb;
}

bool TopolGeom::belongsTo(QgsGeometry *newGeom, Arc *originalArc)
{
  double intersectThreshold =  0.0000001;
		        
  QgsPolyline polyline = newGeom->asPolyline();
  QgsPoint point;
  if (polyline.size() > 2 ) {
    point.set( polyline[1].x(), polyline[1].y() );
//    std::cout << "> 2: " << point.x() << ";" << point.y() << "\n";
  }
  else if (polyline.size() == 2 ) {
    point.set( (polyline[0].x()+polyline[1].x())/2, (polyline[0].y()+polyline[1].y())/2 );
//    std::cout << "= 2: " << point.x() << ";" << point.y() << "\n";
  }
  else {
    return false;
  }
				  
  // the newGeom belongs to originalArc, if the originalArc contains the point
  // for implementation reasons we allow some threshold
  return (originalArc->geom->distance(*QgsGeometry::fromPoint(point)) < intersectThreshold);
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
			mObjects[f.id()] = g;
	}
}


void TopolGeom::buildIntersections()
{
  QList<Arc*> arcs;
  QgsGeometry *comb;

  // convert all initial geometries to polylines and put them to a list
  for (QMap<int, QgsGeometry*>::Iterator obj = mObjects.begin(); obj != mObjects.end(); ++obj)
  {
    arcs.append(new Arc(obj.value(), obj.key()));
  }

  while (!arcs.isEmpty()) {
    // get the first arc

    //for (int j = 0; j < arcs.size(); ++j)
      //std::cout << arcs[j]->featureID << ", "<< std::flush;
    //std::cout << "\n"<< std::flush; 

    Arc *a = arcs.takeFirst();

    // find if it intersects with any other arc
    bool intersects = false;
    for (int b = 0; b < arcs.size(); ++b)
    {
      if ( a->intersects(&arcs, b) )
      {
        comb = a->geom->combine(arcs[b]->geom);
        if ((a->geom == comb) || comb->asMultiPolyline().size() <= 2)
          continue;
          
        intersects = true;

        // this function updates arcs in this way
        // adds 4 or more new arcs 
        // removes the arcs[b] arc and deletes it
        updateArcs(a, b, comb, &arcs);
                
        break;
      }    
    }

    if (intersects) {
      // we have to delete the arc a, it was replaced by its child arcs
      delete a;
    }
    else {
      // we have not found any intersection of arc a with any other arc
      // so a is finished and goes to global list
      mArcs.append(*a); 
    }
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

	QMap<QString, TopolNode>::ConstIterator node = mNodes.begin();
	for (; node != mNodes.end(); ++node)
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
	node = mNodes.begin();
	for (; node != mNodes.end(); ++node)
	{
		QList<QgsPolyline>::ConstIterator arc = node.value().arcs.begin();
		for (; arc != node.value().arcs.end(); ++arc)
		{
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
	/*
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
	mWindow->append(text);*/
}

void TopolGeom::cgOverlap()
{
	/*
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
	mWindow->append(text);*/
}

void TopolGeom::cgMultipart()
{/*
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
	mWindow->append(text);*/
}
