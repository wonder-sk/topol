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
  //mValidationDock = theValidationDock;

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
  //mErrorList->setItem(row - 1, 0, new QTableWidgetItem(mErrorNameMap[errorType]));
  //std::cout << "item: "<<mValidationDock->errorTable->itemAt(row - 1, 0)->text().toStdString();
 /* 
  QComboBox* cb = new QComboBox();
  QMap<validationError, QString>::const_iterator it = mErrorFixMap.lowerBound(errorType);
  QMap<validationError, QString>::const_iterator upperBound = mErrorFixMap.upperBound(errorType);
  for (; it != upperBound; ++it)
    cb->addItem(it.value());
*/

	std::cout << "updating\n";
  mErrorList->addItem(mErrorNameMap[errorType]);
  mComment->setText(QString("%1 errors were found").arg(row));
  //std::cout << "cb future text: " <<it.value().toStdString();
  //std::cout << "cb text: "<<cb->currentText().toStdString();
  //mValidationDock->errorTable->setCellWidget(row - 1, 1, cb);
  //mValidationDock->errorTable->setItem(row - 1, 1, new QTableWidgetItem("No fix"));
  //mValidationDock->errorTable->setItem(row - 1, 1, new QTableWidgetItem(QString("%1").arg(row - 1)));

/*  QComboBox* pb = new QPushButton();
  connect(pb, SIGNAL(clicked()), this, SLOT(zoomTo()));
  mValidationDock->errorTable->setCellWidget(row - 1, 2, pb);
*/
  //mValidationDock->errorTable->setRowCount(row);
  //QStringList labels;
  //labels << "Error Type" << "Suggested Fixes";
  //mValidationDock->errorTable->setHorizontalHeaderLabels(labels);
  //mValidationDock->errorTable->resizeColumnsToContents();
}

void checkDock::checkForIntersections(QgsFeatureList featureList)
{
  QgsGeometryMap m;
  //QgsRectangle r;
  //mLayer->select(QgsAttributeList(), QgsRectangle());

  //std::cout << "#of selected: "<<mLayer->selectedFeatureCount()<<"\n";
  std::cout << "#of selected: "<<featureList.size()<<"\n";

  QgsFeature f;
  QgsGeometry *g;

  /*
  while (mLayer->nextFeature(f))
  {
    g = f.geometryAndOwnership();
    if (g)
      m[f.id()] = *g;
  }
  */
  
  QgsFeatureList::Iterator it;
  for (it = featureList.begin(); it != featureList.end(); ++it)
  {
    g = it->geometry();
    if (g)
      m[it->id()] = *g;
  }

  std::cout << "msize: "<<m.size()<<"\n";
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
        std::cout << "intersection: " <<i<<"  "<<j<<"\n";
	updateValidationDock(intersectionCount, ErrorIntersection);
      }
    }
}

void checkDock::configure()
{
  mConfigureDialog->show();
}

void checkDock::validateExtent()
{
  mErrorList->clear();
  QgsRectangle extent = QgisApp::instance()->mapCanvas()->extent();
  //std::cout << extent;
  mLayer->select(QgsAttributeList(), extent);
  QgsFeatureList featureList = mLayer->selectedFeatures();

  checkForIntersections(featureList);
}

void checkDock::validateAll()
{
  mErrorList->clear();
  QgsFeature f;
  QgsRectangle r;

  mLayer->select(QgsAttributeList(), QgsRectangle(), true, false);
  QgsFeatureList featureList = mLayer->selectedFeatures();
  //QgsFeatureList featureList;
  //while (mLayer->nextFeature(f))
    //featureList << f;

  checkForIntersections(featureList);
}
