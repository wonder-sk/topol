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

#include <qgsproviderregistry.h>
#include <qgslogger.h>

#include "rulesDialog.h"
#include "validationDock.h"
#include "../../app/qgisapp.h"

int rectIndex = 0;

checkDock::checkDock(const QString &tableName, QgsVectorLayer* theLayer, rulesDialog* theConfigureDialog, validationDock* theValidationDock, QWidget* parent)
: QDockWidget(parent), Ui::checkDock()
{
  setupUi(this);
  //std::cout << "check dock\n\n\n";
  mLayer = theLayer;
  mConfigureDialog = theConfigureDialog;
  mValidationDock = theValidationDock;

  initErrorMaps();

  connect(configureButton, SIGNAL(clicked()), this, SLOT(configure()));
  connect(validateButton, SIGNAL(clicked()), this, SLOT(validate()));
  //connect(validationDock->errorTable, SIGNAL(clicked(const QModelIndex&)), this, SLOT(clkd(const QModelIndex&)));
}

checkDock::~checkDock() {}

/*
void clkd(const QModelIndex& index)
{
  if (!index.column())
    
}*/

void checkDock::initErrorMaps()
{
  mErrorNameMap[ErrorIntersection] = "Intersection";
  mErrorNameMap[ErrorOverlap] = "Overlap";
  mErrorNameMap[ErrorTolerance] = "Tolerance";

  mErrorFixMap.insertMulti(ErrorIntersection, "Move intersecting geometries");
  mErrorFixMap.insertMulti(ErrorIntersection, "Union intersecting geometries");
  mErrorFixMap.insertMulti(ErrorTolerance, "Increase segment size");
  mErrorFixMap.insertMulti(ErrorTolerance, "Delete segment");
}


void checkDock::updateValidationDock(int row, validationError errorType)
{
  
  //std::cout << mErrorNameMap[errorType].toStdString()<<"\n";
  //std::cout << row<<"\n";
//std::cout << "1st fix: "<<mErrorFixMap[errorType].toStdString();

  mValidationDock->errorTable->setItem(row - 1, 0, new QTableWidgetItem(mErrorNameMap[errorType]));
  //std::cout << "item: "<<mValidationDock->errorTable->itemAt(row - 1, 0)->text().toStdString();
  
  QComboBox* cb = new QComboBox();
  QMap<validationError, QString>::const_iterator it = mErrorFixMap.lowerBound(errorType);
  QMap<validationError, QString>::const_iterator upperBound = mErrorFixMap.upperBound(errorType);
  for (; it != upperBound; ++it)
    cb->addItem(it.value());

  //std::cout << "cb future text: " <<it.value().toStdString();
  //std::cout << "cb text: "<<cb->currentText().toStdString();
  mValidationDock->errorTable->setCellWidget(row - 1, 1, cb);
  //mValidationDock->errorTable->setItem(row - 1, 1, new QTableWidgetItem("No fix"));
  //mValidationDock->errorTable->setItem(row - 1, 1, new QTableWidgetItem(QString("%1").arg(row - 1)));

/*  QComboBox* pb = new QPushButton();
  connect(pb, SIGNAL(clicked()), this, SLOT(zoomTo()));
  mValidationDock->errorTable->setCellWidget(row - 1, 2, pb);
*/
  mValidationDock->errorComment->setText(QString("%1 errors were found").arg(row));
  //mValidationDock->errorTable->setRowCount(row);
  QStringList labels;
  labels << "Error Type" << "Suggested Fixes";
  mValidationDock->errorTable->setHorizontalHeaderLabels(labels);
  mValidationDock->errorTable->resizeColumnsToContents();
}

void checkDock::checkForIntersections()
{
  QgsGeometryMap m;
  QgsRectangle r;
  mLayer->select(QgsAttributeList());

  //std::cout << "#of selected: "<<mLayer->selectedFeatureCount()<<"\n";

  QgsFeature f;
  QgsGeometry *g;

  while (mLayer->nextFeature(f))
  {
    g = f.geometryAndOwnership();
    if (g)
      m[f.id()] = *g;
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
}

void checkDock::configure()
{
  mConfigureDialog->show();
}

void checkDock::validate()
{
  mValidationDock->errorTable->clear();
  checkForIntersections();
  QgisApp::instance()->addDockWidget(Qt::RightDockWidgetArea, mValidationDock);
  mValidationDock->show();
}
