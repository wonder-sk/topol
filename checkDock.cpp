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
  mErrorNameMap[ErrorIntersection] = "Intersecting geometries";
  mErrorNameMap[ErrorOverlap] = "Overlapping geometries";
  mErrorNameMap[ErrorTolerance] = "Segment shorter than tolerance";
  mErrorNameMap[ErrorDangle] = "Point too close to segment";

  /*mErrorFixMap.insertMulti(ErrorIntersection, "Move intersecting geometries");
  mErrorFixMap.insertMulti(ErrorIntersection, "Union intersecting geometries");
  mErrorFixMap.insertMulti(ErrorTolerance, "Increase segment size");
  mErrorFixMap.insertMulti(ErrorTolerance, "Delete segment");
  */
}

void checkDock::errorListClicked(const QModelIndex& index)
{
  std::cout << index.row() <<" setting Extent: " <<  mErrorRectangleMap[index.row()]<<"\n";
  QgisApp::instance()->mapCanvas()->setExtent(mErroMap[index.row()].boundingBox);
  QgisApp::instance()->mapCanvas()->refresh();
}

void checkDock::updateValidationDock(int row, validationError errorType)
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

  QgsGeometryMap::Iterator it, jit;
  for (it = mGeometryMap.begin(); it != mGeometryMap.end(); ++it)
    for (jit = mGeometryMap.begin(); jit != mGeometryMap.end(); ++jit)
    {
      if (it.key() >= jit.key())
        continue;

      if (it.value().distance(jit.value()) < tolerance)
      {
	//mErrorRectangleMap[it.key()] = it.value().boundingBox();
        QgsRectangle r = it.value().boundingBox();
	r.combineExtentWith(&jit.value().boundingBox());
	mErrorRectangleMap[mErrorList->count()] = r;
        std::cout << "point too close: " <<it.key()<<"  "<<jit.key()<<"\n";
        mErrorList->addItem(mErrorNameMap[ErrorDangle]);
      }
    }
   
  //snapToGeometry(point, geom, squaredTolerance, QMultiMap<double, QgsSnappingResult>, SnapToVertexAndSegment);
}

void checkDock::checkIntersections()
{
  QgsGeometryMap::Iterator it, jit;
  for (it = mGeometryMap.begin(); it != mGeometryMap.end(); ++it)
    for (jit = mGeometryMap.begin(); jit != mGeometryMap.end(); ++jit)
    {
      if (it.key() >= jit.key())
        continue;

      if (it.value().intersects(&jit.value()))
      {
        QgsRectangle r = it.value().boundingBox();
	r.combineExtentWith(&jit.value().boundingBox());
	mErrorRectangleMap[mErrorList->count()] = r;
	//r = g->boundingBox();
        QgsGeometry* g = it.value().intersection(&jit.value());
        mRubberBand->setToGeometry(g, mLayer);
        delete g;

	std::cout << "intersection: " <<it.key()<<"  "<<jit.key()<<"\n";
        mErrorList->addItem(mErrorNameMap[ErrorIntersection]);
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
  mGeometryMap.clear();
  mLayer->select(QgsAttributeList(), rect);

  QgsFeature f;
  QgsGeometry *g;
  while (mLayer->nextFeature(f))
  {
    g = f.geometry();
    if (g)
      mGeometryMap[f.id()] = *g;
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
