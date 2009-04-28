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

checkDock::checkDock(const QString &tableName, QgsVectorLayer* theLayer, rulesDialog* theConfigureDialog, validationDock* theValidationDock, QWidget* parent)
: QDockWidget(parent), Ui::checkDock()
{
  setupUi(this);
  std::cout << "check dock\n\n\n";
  mLayer = theLayer;
  mConfigureDialog = theConfigureDialog;
  mValidationDock = theValidationDock;

  errorMap[ErrorIntersection] = "Intersection";
  errorMap[ErrorOverlap] = "Overlap";
  errorMap[ErrorTolerance] = "Tolerance";

  connect(configureButton, SIGNAL(clicked()), this, SLOT(configure()));
  connect(validateButton, SIGNAL(clicked()), this, SLOT(validate()));
}

checkDock::~checkDock() {}


void checkDock::updateValidationDock(int row, validationError errorType)
{
  
  //std::cout << errorMap[errorType].toStdString()<<"\n";
  //std::cout << row<<"\n";
  //QTableWidgetItem* tbi = new QTableWidgetItem(errorMap[errorType]);
  //std::cout << "tbi: "<<tbi->text().toStdString();
  //mValidationDock->errorTable->setItem(row - 1, 0, tbi);
  mValidationDock->errorTable->setItem(row - 1, 0, new QTableWidgetItem(errorMap[errorType]));
  //std::cout << "item: "<<mValidationDock->errorTable->itemAt(row - 1, 0)->text().toStdString();
  mValidationDock->errorTable->setItem(row - 1, 1, new QTableWidgetItem("No fix"));
  //mValidationDock->errorTable->setItem(row - 1, 1, new QTableWidgetItem(QString("%1").arg(row - 1)));

  mValidationDock->errorComment->setText(QString("%1 errors were found").arg(row));
  //mValidationDock->errorTable->setRowCount(row);
}

void checkDock::checkForIntersections()
{
  QgsGeometryMap m;
  QgsRectangle r;
  mLayer->select(QgsAttributeList());

  std::cout << "#of selected: "<<mLayer->selectedFeatureCount()<<"\n";

  QgsFeature f;
  QgsGeometry *g;

  while (mLayer->nextFeature(f))
  {
    g = f.geometryAndOwnership();
    if (g)
      m[f.id()] = *g;
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
        std::cout << "intersection: " <<i<<"  "<<j<<"\n";
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
  mValidationDock->show();
}
