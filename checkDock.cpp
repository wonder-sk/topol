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
#include "../../app/qgisapp.h"

//TODO: fix crashing when no layer under
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
  /*QMap<int, QgsFeature>::Iterator it, jit;
  for (it = mFeatureList.begin(); it != mFeatureList.end(); ++it)
    for (jit = mFeatureList.begin(); jit != mFeatureList.end(); ++jit)
    {
      if (it.key() >= jit.key())
        continue;

      QgsGeometry* g1 = it.value().geometry();
      QgsGeometry* g2 = jit.value().geometry();

      if (g1->distance(*g2) < 0.1)
      {
	QgsGeometry *c, *d;
	if ((c = checkEndpoints(g1, g2)) || (d = checkEndpoints(g2, g1)))
	{
          QgsRectangle r = g1->boundingBox();
	  r.combineExtentWith(&g2->boundingBox());

	  QgsFeatureIds fids;
	  fids << it.key() << jit.key();
	  
	  TopolErrorDangle* err;
          if (c)
	  {
	    fids << it.key() << jit.key();
            err = new TopolErrorDangle(mLayer, r, c, fids);
            mErrorListView->addItem(err->name());
	    mErrorList << err;
	  }

          if (d)
	  {
	    fids << jit.key() << it.key();
            err = new TopolErrorDangle(mLayer, r, d, fids);
            mErrorListView->addItem(err->name());
	    mErrorList << err;
	  }
	}
      }
    } */
}

void checkDock::checkIntersections()
{
  QList<FeatureLayer>::Iterator it, jit;
  for (it = mFeatureList.begin(); it != mFeatureList.end(); ++it)
    for (jit = mFeatureList.begin(); jit != mFeatureList.end(); ++jit)
    {
      if (it->feature.id() >= jit->feature.id())
        continue;

      QgsGeometry* g1 = it->feature.geometry();
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

void checkDock::checkSelfIntersections()
{
  /*QList<TopolError>::Iterator it = mErrorList.begin();
  QList<TopolError>::Iterator end_it = mErrorList.end();
  QSet<TopolError> set;
  QList<TopolError> tempList;

  for (; it != end_it; ++it)
  {
    //set = it->fids.toSet();
    //if (set.size() < it->fids.size())
    if (it->fids.size() == 1)
    {
      tempList << TopolError();
      tempList.last().boundingBox = it->boundingBox;
      tempList.last().fids << it->fids;

      tempList.last().conflict = it->conflict;
      mErrorListView->addItem(mErrorNameMap[TopolSelfIntersection]);
    }
  }

  mErrorList << tempList;
  */
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
  //checkSelfIntersections();
  //checkDanglingEndpoints();

  /* TODO: doesn't work yet
  QgsRectangle zoom;
  ErrorList::ConstIterator it = mErrorList.begin();
  for (; it != mErrorList.end(); ++it)
    zoom.combineExtentWith(&(*it)->boundingBox());

  std::cout << "zoom:"<<zoom;
  mQgisApp->mapCanvas()->setExtent(zoom);
  */

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
