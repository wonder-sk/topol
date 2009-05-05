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

int rectIndex = 0;

checkDock::checkDock(const QString &tableName, QgsVectorLayer* theLayer, rulesDialog* theConfigureDialog, QWidget* parent)
: QDockWidget(parent), Ui::checkDock()
{
  setupUi(this);
  mLayer = theLayer;
  mConfigureDialog = theConfigureDialog;

  initErrorMaps();

  mValidateExtentButton->setIcon(QIcon(":/topol_c/topol.png"));
  mValidateAllButton->setIcon(QIcon(":/topol_c/topol.png"));
  mConfigureButton->setIcon(QIcon(":/topol_c/topol.png"));
  mRubberBand = new QgsRubberBand(QgisApp::instance()->mapCanvas(), mLayer);

  connect(mConfigureButton, SIGNAL(clicked()), this, SLOT(configure()));
  connect(mValidateAllButton, SIGNAL(clicked()), this, SLOT(validateAll()));
  connect(mValidateExtentButton, SIGNAL(clicked()), this, SLOT(validateExtent()));
  connect(mFixButton, SIGNAL(clicked()), this, SLOT(fix()));
  connect(mErrorListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(errorListClicked(const QModelIndex &)));
}

checkDock::~checkDock()
{
  delete mRubberBand;
}

void checkDock::initErrorMaps()
{
  mErrorNameMap[TopolErrorSelfIntersection] = "Intersecting itself";
  mErrorNameMap[TopolErrorIntersection] = "Intersecting geometries";
  mErrorNameMap[TopolErrorOverlap] = "Overlapping geometries";
  mErrorNameMap[TopolErrorTolerance] = "Segment shorter than tolerance";
  mErrorNameMap[TopolErrorDangle] = "Point too close to segment";

  mFixNameMap[TopolFixMoveFirst] = "Move first geometry";
  mFixNameMap[TopolFixMoveSecond] = "Move second geometry";
  mFixNameMap[TopolFixDeleteFirst] = "Delete first geometry";
  mFixNameMap[TopolFixDeleteSecond] = "Delete second geometry";
  mFixNameMap[TopolFixUnionFirst] = "Union in first";
  mFixNameMap[TopolFixUnionSecond] = "Union in second";
  mFixNameMap[TopolFixSnap] = "Snap to segment";

  /*mErrorFixMap.insertMulti(ErrorIntersection, "Move intersecting geometries");
  mErrorFixMap.insertMulti(ErrorIntersection, "Union intersecting geometries");
  mErrorFixMap.insertMulti(ErrorTolerance, "Increase segment size");
  mErrorFixMap.insertMulti(ErrorTolerance, "Delete segment");
  */
}

void checkDock::configure()
{
  mConfigureDialog->show();
}

void checkDock::errorListClicked(const QModelIndex& index)
{
  //std::cout << index.row() <<" setting Extent: " <<  mErrorRectangleMap[index.row()]<<"\n";

  /*QgsRectangle zoom;
  std::cout << "zoom:"<<zoom;
  QgsFeatureList::ConstIterator it = mErrorList[index.row()].features.begin();
  for (; it != mErrorList[index.row()].features.end(); ++it)
    zoom.combineExtentWith(mFeatureMap[it->fid].geometry().boundingBox());
*/
  QgisApp::instance()->mapCanvas()->setExtent(mErrorList[index.row()].boundingBox);
  QgisApp::instance()->mapCanvas()->refresh();
  //QgsGeometry* g = it.value().intersection(&jit.value());
  //delete g;
  mRubberBand->setToGeometry(&mErrorList[index.row()].conflict, mLayer);

}

//void checkDock::updateValidationDock(int row, TopolErrorType errorType)
//{
/*
  QComboBox* cb = new QComboBox();
  QMap<validationError, QString>::const_iterator it = mErrorFixMap.lowerBound(errorType);
  QMap<validationError, QString>::const_iterator upperBound = mErrorFixMap.upperBound(errorType);
  for (; it != upperBound; ++it)
    cb->addItem(it.value());
*/
  //mErrorListView->addItem(mErrorNameMap[errorType]);
//}

void checkDock::fix()
{
  //mFixBox->currentText();
  int row = mErrorListView->currentRow();

  //mErrorFixMap[row];

  /*switch (mErrorList[row].type())
  {
    

  }*/

  //QMap<validationError, QString>::const_iterator it = mErrorFixMap.lowerBound(errorType);
  //QMap<validationError, QString>::const_iterator upperBound = mErrorFixMap.upperBound(errorType);
  //for (; it != upperBound; ++it)
    //cb->addItem(it.value());

}

void checkDock::checkDanglingEndpoints()
{
  double tolerance = 0.1;
  QMap<int, QgsFeature>::Iterator it, jit;
  for (it = mFeatureMap.begin(); it != mFeatureMap.end(); ++it)
    for (jit = mFeatureMap.begin(); jit != mFeatureMap.end(); ++jit)
    {
      if (it.key() >= jit.key())
        continue;

      QgsGeometry* g1 = it.value().geometry();
      QgsGeometry* g2 = jit.value().geometry();

      if (g1->distance(*g2) < tolerance)
      {
        QgsRectangle r = g1->boundingBox();
	r.combineExtentWith(&g2->boundingBox());
	mErrorList << TopolError();
	mErrorList.last().boundingBox = r;
	mErrorList.last().fids << it.key() << jit.key();
	//mErrorList[mErrorListView->count()].fids << it.key() << jit.key();

	QgsGeometry* c = g1->intersection(g2);
	if (!c)
	  c = new QgsGeometry;

	mErrorList.last().conflict = *c;
	delete c;

        mErrorListView->addItem(mErrorNameMap[TopolErrorDangle]);
      }
    }

  //fix would be like that
  //snapToGeometry(point, geom, squaredTolerance, QMultiMap<double, QgsSnappingResult>, SnapToVertexAndSegment);
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

void checkDock::checkIntersections()
{
  QMap<int, QgsFeature>::Iterator it, jit;
  for (it = mFeatureMap.begin(); it != mFeatureMap.end(); ++it)
    for (jit = mFeatureMap.begin(); jit != mFeatureMap.end(); ++jit)
    {
      if (it.key() > jit.key())
        continue;

      QgsGeometry* g1 = it.value().geometry();
      QgsGeometry* g2 = jit.value().geometry();

      if (g1->intersects(g2))
      {
        QgsRectangle r = g1->boundingBox();
	r.combineExtentWith(&g2->boundingBox());
	mErrorList << TopolError();
	mErrorList.last().boundingBox = r;
	mErrorList.last().fids << it.key() << jit.key();

	QgsGeometry* c = g1->intersection(g2);
	if (!c)
	  c = new QgsGeometry;

	mErrorList.last().conflict = *c;
	delete c;

        mErrorListView->addItem(mErrorNameMap[TopolErrorIntersection] + QString(" %1 %2").arg(it.key()).arg(jit.key()));
	mFixBox->addItems
      }
    }
}

void checkDock::validate(QgsRectangle rect)
{
  mErrorListView->clear();
  mFeatureMap.clear();
  //mLayer = QgisApp::instance()->mapLegend()->currentLayer();
  mLayer->select(QgsAttributeList(), rect);

  QgsFeature f;
  //QgsGeometry *g;
  while (mLayer->nextFeature(f))
  {
    if (f.geometry())
      mFeatureMap[f.id()] = f;
  }

  checkIntersections();
  checkSelfIntersections();
  checkDanglingEndpoints();

  mComment->setText(QString("%1 errors were found").arg(mErrorListView->count()));
}

void checkDock::validateExtent()
{
  QgsRectangle extent = QgisApp::instance()->mapCanvas()->extent();
  validate(extent);
}

void checkDock::validateAll()
{
  validate(QgsRectangle());
}
