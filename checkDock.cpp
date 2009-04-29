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
  connect(mErrorList, SIGNAL(clicked(const QModelIndex &)), this, SLOT(errorListClicked(const QModelIndex &)));
}

checkDock::~checkDock()
{
  delete mRubberBand;
}

void checkDock::initErrorMaps()
{
  mErrorNameMap[TopolIntersection] = "Intersecting geometries";
  mErrorNameMap[TopolOverlap] = "Overlapping geometries";
  mErrorNameMap[TopolTolerance] = "Segment shorter than tolerance";
  mErrorNameMap[TopolDangle] = "Point too close to segment";

  /*mErrorFixMap.insertMulti(ErrorIntersection, "Move intersecting geometries");
  mErrorFixMap.insertMulti(ErrorIntersection, "Union intersecting geometries");
  mErrorFixMap.insertMulti(ErrorTolerance, "Increase segment size");
  mErrorFixMap.insertMulti(ErrorTolerance, "Delete segment");
  */
}

void checkDock::errorListClicked(const QModelIndex& index)
{
  //std::cout << index.row() <<" setting Extent: " <<  mErrorRectangleMap[index.row()]<<"\n";

  /*QgsRectangle zoom;
  std::cout << "zoom:"<<zoom;
  QgsFeatureList::ConstIterator it = mErrorMap[index.row()].features.begin();
  for (; it != mErrorMap[index.row()].features.end(); ++it)
    zoom.combineExtentWith(mFeatureMap[it->fid].geometry().boundingBox());
*/
  QgisApp::instance()->mapCanvas()->setExtent(mErrorMap[index.row()].boundingBox);
  QgisApp::instance()->mapCanvas()->refresh();
  //QgsGeometry* g = it.value().intersection(&jit.value());
  //delete g;
  mRubberBand->setToGeometry(&mErrorMap[index.row()].conflict, mLayer);

}

void checkDock::updateValidationDock(int row, TopolErrorType errorType)
{
/*
  QComboBox* cb = new QComboBox();
  QMap<validationError, QString>::const_iterator it = mErrorFixMap.lowerBound(errorType);
  QMap<validationError, QString>::const_iterator upperBound = mErrorFixMap.upperBound(errorType);
  for (; it != upperBound; ++it)
    cb->addItem(it.value());
*/
  //mErrorList->addItem(mErrorNameMap[errorType]);
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
	mErrorMap[mErrorList->count()].boundingBox = r;

	QgsGeometry* c = g1->intersection(g2);
	if (!c)
	  c = new QgsGeometry;

	mErrorMap[mErrorList->count()].conflict = *c;
	delete c;

        mErrorList->addItem(mErrorNameMap[TopolDangle]);
      }
    }

  //fix would be like that
  //snapToGeometry(point, geom, squaredTolerance, QMultiMap<double, QgsSnappingResult>, SnapToVertexAndSegment);
}

void checkDock::checkIntersections()
{
  QMap<int, QgsFeature>::Iterator it, jit;
  for (it = mFeatureMap.begin(); it != mFeatureMap.end(); ++it)
    for (jit = mFeatureMap.begin(); jit != mFeatureMap.end(); ++jit)
    {
      if (it.key() >= jit.key())
        continue;

      QgsGeometry* g1 = it.value().geometry();
      QgsGeometry* g2 = jit.value().geometry();

      if (g1->intersects(g2))
      {
        QgsRectangle r = g1->boundingBox();
	r.combineExtentWith(&g2->boundingBox());
	mErrorMap[mErrorList->count()].boundingBox = r;

	QgsGeometry* c = g1->intersection(g2);
	if (!c)
	  c = new QgsGeometry;

	mErrorMap[mErrorList->count()].conflict = *c;
	delete c;

        mErrorList->addItem(mErrorNameMap[TopolIntersection]);
      }
    }
}

void checkDock::configure()
{
  mConfigureDialog->show();
}

void checkDock::validate(QgsRectangle rect)
{
  mErrorList->clear();
  mFeatureMap.clear();
  mLayer->select(QgsAttributeList(), rect);

  QgsFeature f;
  //QgsGeometry *g;
  while (mLayer->nextFeature(f))
  {
    if (f.geometry())
      mFeatureMap[f.id()] = f;
  }

  checkIntersections();
  checkDanglingEndpoints();

  mComment->setText(QString("%1 errors were found").arg(mErrorList->count()));
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
