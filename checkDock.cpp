#include <QtGui>

#include "checkDock.h"

#include <qgsvectordataprovider.h>
#include <qgsvectorlayer.h>
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

#include "rulesDialog.h"
#include "geosFunctions.h"
#include "../../app/qgisapp.h"

checkDock::checkDock(const QString &tableName, QgsVectorLayer* theLayer, rulesDialog* theConfigureDialog, QWidget* parent)
: QDockWidget(parent), Ui::checkDock()
{
  setupUi(this);
  mLayer = theLayer;
  mConfigureDialog = theConfigureDialog;
  mQgisApp = QgisApp::instance();

  mValidateExtentButton->setIcon(QIcon(":/topol_c/topol.png"));
  mValidateAllButton->setIcon(QIcon(":/topol_c/topol.png"));
  mConfigureButton->setIcon(QIcon(":/topol_c/topol.png"));

  rub1 = new QgsRubberBand(mQgisApp->mapCanvas(), mLayer);
  rub2 = new QgsRubberBand(mQgisApp->mapCanvas(), mLayer);
  mRubberBand = new QgsRubberBand(mQgisApp->mapCanvas(), mLayer);

  rub1->setColor("blue");
  rub2->setColor("red");
  mRubberBand->setColor("gold");

  rub1->setWidth(3);
  rub2->setWidth(3);
  mRubberBand->setWidth(3);

  connect(mConfigureButton, SIGNAL(clicked()), this, SLOT(configure()));
  connect(mValidateAllButton, SIGNAL(clicked()), this, SLOT(validateAll()));
  connect(mValidateExtentButton, SIGNAL(clicked()), this, SLOT(validateExtent()));
  connect(mFixButton, SIGNAL(clicked()), this, SLOT(fix()));
  connect(mErrorListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(errorListClicked(const QModelIndex &)));
}

checkDock::~checkDock()
{
  delete mRubberBand;
  delete rub1;
  delete rub2;

  QList<TopolError*>::Iterator it = mErrorList.begin();
  for (; it != mErrorList.end(); ++it)
    delete *it;
}

void checkDock::configure()
{
  mConfigureDialog->show();
}

void checkDock::errorListClicked(const QModelIndex& index)
{
  int row = index.row();
  mQgisApp->mapCanvas()->setExtent(mErrorList[row]->boundingBox());
  mQgisApp->mapCanvas()->refresh();

  mFixBox->clear();
  mFixBox->addItem("Select automatic fix");
  mFixBox->addItems(mErrorList[row]->fixNames());

  QgsFeature f;
  QgsGeometry* g;
  FeatureLayer fl = mErrorList[row]->featurePairs().first();
  fl.layer->featureAtId(fl.feature.id(), f, true, false);
  g = f.geometry();
  rub1->setToGeometry(g, mLayer);

  fl = mErrorList[row]->featurePairs()[1];
  fl.layer->featureAtId(fl.feature.id(), f, true, false);
  g = f.geometry();
  rub2->setToGeometry(g, mLayer);

  mRubberBand->setToGeometry(mErrorList[row]->conflict(), mLayer);
}

void checkDock::fix()
{
  int row = mErrorListView->currentRow();
  QString fixName = mFixBox->currentText();

  if (row == -1)
    return;

  rub1->reset();
  rub2->reset();
  mRubberBand->reset();

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

QgsGeometry* checkEndpoints(QgsGeometry* g1, QgsGeometry* g2)
{
	//TODO:MultiLines
  if (!g1 || !g2)
    return 0;

  if (g1->type() != QGis::Line || g2->type() != QGis::Line)
    return 0;

  QgsPoint endPoint = g1->asPolyline().last();
  QgsGeometry *g = QgsGeometry::fromPoint(endPoint);
  if (g2->distance(*g) < 0.1)
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

void checkDock::checkDanglingEndpoints()
{
  QList<FeatureLayer>::Iterator it, jit;
  for (it = mFeatureList.begin(); it != mFeatureList.end(); ++it)
  {
    QgsGeometry* g1 = it->feature.geometry();

    for (jit = mFeatureList.begin(); jit != mFeatureList.end(); ++jit)
    {
      if (it->feature.id() >= jit->feature.id())
        continue;

      QgsGeometry* g2 = jit->feature.geometry();

      //TODO: read from settings
      if (g1->distance(*g2) < 0.1)
      {
	QgsGeometry *c, *d;
	if ((c = checkEndpoints(g1, g2)) || (d = checkEndpoints(g2, g1)))
	{
          QgsRectangle r = g1->boundingBox();
	  r.combineExtentWith(&g2->boundingBox());

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

void checkDock::checkIntersections()
{
  QList<FeatureLayer>::Iterator it, jit;
  for (it = mFeatureList.begin(); it != mFeatureList.end(); ++it)
  {
    QgsGeometry* g1 = it->feature.geometry();

    for (jit = mFeatureList.begin(); jit != mFeatureList.end(); ++jit)
    {
	    //TODO: ids could be same in different layers
      if (it->feature.id() >= jit->feature.id())
        continue;

      QgsGeometry* g2 = jit->feature.geometry();
      if (g1->intersects(g2))
      {
        QgsRectangle r = g1->boundingBox();
	r.combineExtentWith(&g2->boundingBox());

	QgsGeometry* c = g1->intersection(g2);
	if (!c)
	  c = new QgsGeometry;

	QList<FeatureLayer> fls;
	fls << *it << *jit;
	TopolErrorIntersection* err = new TopolErrorIntersection(r, c, fls);

	mErrorList << err;
        mErrorListView->addItem(err->name() + QString(" %1 %2").arg(it->feature.id()).arg(jit->feature.id()));
      }
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

void checkDock::checkPolygonContains()
{
  QList<FeatureLayer>::Iterator it, jit;
  for (it = mFeatureList.begin(); it != mFeatureList.end(); ++it)
  {
    QgsGeometry* g1 = it->feature.geometry();
    if (g1->type() != QGis::Polygon)
      continue;

    for (jit = mFeatureList.begin(); jit != mFeatureList.end(); ++jit)
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

void checkDock::checkPointCoveredBySegment()
{
  QList<FeatureLayer>::Iterator it, jit;
  for (it = mFeatureList.begin(); it != mFeatureList.end(); ++it)
  {
    bool touched = false;
    QgsGeometry* g1 = it->feature.geometry();

    if (g1->type() != QGis::Point)
      continue;

    for (jit = mFeatureList.begin(); jit != mFeatureList.end(); ++jit)
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

void checkDock::checkSegmentLength()
{
  //TODO: multi versions, distance from settings, more errors for one feature
  QList<FeatureLayer>::Iterator it, jit;
  for (it = mFeatureList.begin(); it != mFeatureList.end(); ++it)
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
	  if (ls[i-1].sqrDist(ls[i]) < 0.1)
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
	    if (pol[i][j-1].sqrDist(pol[i][j]) < 0.1)
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

void checkDock::checkSelfIntersections()
{
}

void checkDock::validate(QgsRectangle rect)
{
  mErrorListView->clear();
  mFeatureList.clear();

  QgsMapLayerRegistry *reg = QgsMapLayerRegistry::instance();
  QList<QgsMapLayer *> layerList = reg->mapLayers().values();
  QList<QgsMapLayer *>::ConstIterator it = layerList.begin();
  QgsFeature f;

  for (; it != layerList.end(); ++it)
  {
    ((QgsVectorLayer*)(*it))->select(QgsAttributeList(), rect);
    while (((QgsVectorLayer*)(*it))->nextFeature(f))
      if (f.geometry())
        mFeatureList << FeatureLayer((QgsVectorLayer*)*it, f);
  }

  checkIntersections();
  //checkPointInsidePolygon();
  checkPolygonContains();
  checkSegmentLength();
  checkDanglingEndpoints();
  checkPointCoveredBySegment();
  //checkSelfIntersections();

  mComment->setText(QString("%1 errors were found").arg(mErrorListView->count()));
  rub1->reset();
  rub2->reset();
  mRubberBand->reset();
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
