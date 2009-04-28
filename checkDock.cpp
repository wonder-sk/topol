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

  connect(mConfigureButton, SIGNAL(clicked()), this, SLOT(configure()));
  connect(mValidateAllButton, SIGNAL(clicked()), this, SLOT(validateAll()));
  connect(mValidateExtentButton, SIGNAL(clicked()), this, SLOT(validateExtent()));
}

checkDock::~checkDock() {}

void checkDock::initErrorMaps()
{
  mErrorNameMap[ErrorIntersection] = "Intersecting geometries";
  mErrorNameMap[ErrorOverlap] = "Overlapping geometries";
  mErrorNameMap[ErrorTolerance] = "Segment shorter than tolerance";

  /*mErrorFixMap.insertMulti(ErrorIntersection, "Move intersecting geometries");
  mErrorFixMap.insertMulti(ErrorIntersection, "Union intersecting geometries");
  mErrorFixMap.insertMulti(ErrorTolerance, "Increase segment size");
  mErrorFixMap.insertMulti(ErrorTolerance, "Delete segment");
  */
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

  mErrorList->addItem(mErrorNameMap[errorType]);
}

void checkDock::checkForIntersections(QgsFeatureList featureList)
{
  QgsGeometryMap m;

  //std::cout << "#of selected: "<<featureList.size()<<"\n";

  QgsFeature f;
  QgsGeometry *g;

  QgsFeatureList::Iterator it;
  for (it = featureList.begin(); it != featureList.end(); ++it)
  {
    g = it->geometry();
    if (g)
      m[it->id()] = *g;
  }

  //std::cout << "msize: "<<m.size()<<"\n";
  int intersectionCount = 0;

  for (int i = 0; i < m.size(); ++i)
    for (int j = 0; j < m.size(); ++j)
    {
      if (i >= j)
        continue;


      if (m[i].intersects(&m[j]))
      {
	++intersectionCount;
	mErrorRectangleMap[i] = m[i].boundingBox();
        //std::cout << "intersection: " <<i<<"  "<<j<<"\n";
	updateValidationDock(intersectionCount, ErrorIntersection);
      }
    }

    mComment->setText(QString("%1 errors were found").arg(intersectionCount));
}

void checkDock::configure()
{
  mConfigureDialog->show();
}

void checkDock::validate(QgsRectangle rect)
{
  mErrorList->clear();
  mLayer->select(QgsAttributeList(), rect);

  QgsFeatureList featureList;
  QgsFeature f;
  while (mLayer->nextFeature(f))
    featureList << f;

  checkForIntersections(featureList);
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
