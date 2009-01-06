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

struct info
{
	 QgsGeometry *g;
	 int comp;
	 QSet<int> groups;
}; 

// TODO: Polygon version not done
void TopolGeom::buildIntersections()
{
	QMap<int, QgsGeometry>::Iterator obj;
	//QList<QgsGeometry *> queue, parsed;
	QList<info> geoms;

	QgsGeometry *c;
	//int objects = mObjects.size();

	///////////////////////////////////////////////////
	for (obj = mObjects.begin(); obj != mObjects.end(); ++obj)
	{
		struct info inf;
		inf.comp = obj.key();	
		inf.groups.clear();
		inf.g = &obj.value();
		geoms.append(inf);
	}

	for (int a = 0; a < geoms.size();)// ++a)
	{
	std::cout << "QUEUE taken\n" << std::flush;
		bool intersection = false;
		QgsGeometry *lastIntersected;

std::cout << "a: " << geoms[a].g->asPolyline().first().toString().toStdString() << "\n";

		for (int b = 1; b < geoms.size(); ++b)
		{
	std::cout << "geoms size " << geoms.size() <<"\n" << std::flush;
std::cout << "b zas b: " << geoms[b].g->asPolyline().first().toString().toStdString() << "\n";

			// if not intersecting each other
			if ( !(geoms[a].g->boundingBox().intersects(geoms[b].g->boundingBox()))
				|| !(geoms[a].g->intersects(geoms[b].g)) )
			{
	std::cout << "continue\n" << std::flush;
				continue;
			}
	std::cout << "prusecik\n" << std::flush;
			if ((c = geoms[a].g->combine(geoms[b].g)) && geoms[a].g != c)
			{
	std::cout << "combine\n" << std::flush;
	std::cout << "protinajici: " << geoms[b].g->asPolyline().first().toString().toStdString() << "\n";
				QgsMultiPolyline mpol = c->asMultiPolyline();
				QgsMultiPolyline::Iterator mpit = mpol.begin();
				QgsGeometry *ng;
				for (; mpit != mpol.end(); ++mpit)
				{
					std::cout << "polyline\n" << std::flush;
					ng = QgsGeometry::fromPolyline(*mpit);
					struct info inf;
					inf.g = ng;
					inf.comp = 
					geoms.append(ng);			 
				}

	lastIntersected = ng;

	geoms.removeAt(b);
	geoms.pop_front();
	a = 0;
	intersection = true;
	std::cout << "size " << geoms.size() <<"\n" << std::flush;
	break;
			}
			else
				std::cout << "GEOS union not possible!\n";
		} 	

		if (!intersection)
		{
			if (geoms[a].g == lastIntersected)
				break; 

			++a;
			geoms.append(geoms.takeFirst());
		}
	}
	std::cout << "oblouky:\n";

	QList<QgsGeometry *>::Iterator it ; 
	for (it = geoms.begin(); it != geoms.end(); ++it)
	{
		QgsPolyline pol = (*it)->asPolyline();
	std::cout << pol.first().toString().toStdString() << "\n";
		mNodes[pol.first().toString()].point = pol.first();
		mNodes[pol.first().toString()].arcs.append(pol);
		mNodes[pol.last().toString()].point = pol.last();
		mNodes[pol.last().toString()].arcs.append(pol);
	}
	//////////////////////////////////////////////////
	/*
	for (obj = mObjects.begin(); obj != mObjects.end(); ++obj)
		//queue.append(&obj.value());
		parsed.append(&obj.value());

	for (int a = 0; a < parsed.size();)// ++a)
	{
	std::cout << "QUEUE taken\n" << std::flush;
		bool intersection = false;
		QgsGeometry *lastIntersected;

std::cout << "a: " << parsed[a]->asPolyline().first().toString().toStdString() << "\n";

		for (int b = 1; b < parsed.size(); ++b)
		{
	std::cout << "parsed size " << parsed.size() <<"\n" << std::flush;
std::cout << "b zas b: " << parsed[b]->asPolyline().first().toString().toStdString() << "\n";

			// if not intersecting each other
			if ( !(parsed[a]->boundingBox().intersects(parsed[b]->boundingBox()))
				|| !(parsed[a]->intersects(parsed[b])) )
			{
	std::cout << "continue\n" << std::flush;
				continue;
			}
	std::cout << "prusecik\n" << std::flush;
			if ((c = parsed[a]->combine(parsed[b])) && parsed[a] != c)
			{
	std::cout << "combine\n" << std::flush;
	std::cout << "protinajici: " << parsed[b]->asPolyline().first().toString().toStdString() << "\n";
				QgsMultiPolyline mpol = c->asMultiPolyline();
				QgsMultiPolyline::Iterator mpit = mpol.begin();
				QgsGeometry *ng;
				for (; mpit != mpol.end(); ++mpit)
				{
					std::cout << "polyline\n" << std::flush;
					ng = QgsGeometry::fromPolyline(*mpit);
					parsed.append(ng);			 
				}

	lastIntersected = ng;

	parsed.removeAt(b);
	parsed.pop_front();
	a = 0;
	intersection = true;
	std::cout << "size " << parsed.size() <<"\n" << std::flush;
	break;
			}
			else
				std::cout << "GEOS union not possible!\n";
		} 	

		if (!intersection)
		{
			if (parsed[a] == lastIntersected)
				break; 

			++a;
			parsed.append(parsed.takeFirst());
		}
	}
	std::cout << "oblouky:\n";

	QList<QgsGeometry *>::Iterator it ; 
	for (it = parsed.begin(); it != parsed.end(); ++it)
	{
		QgsPolyline pol = (*it)->asPolyline();
	std::cout << pol.first().toString().toStdString() << "\n";
		mNodes[pol.first().toString()].point = pol.first();
		mNodes[pol.first().toString()].arcs.append(pol);
		mNodes[pol.last().toString()].point = pol.last();
		mNodes[pol.last().toString()].arcs.append(pol);
	}*/
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
