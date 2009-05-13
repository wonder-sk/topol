#include <QtGui>

#include "rulesDialog.h"

#include <qgsvectordataprovider.h>
#include <qgsvectorlayer.h>
#include <qgssearchstring.h>
#include <qgssearchtreenode.h>
#include <qgsmaplayer.h>
#include <qgsmaplayerregistry.h>

#include <qgsproviderregistry.h>
#include <qgslogger.h>

#include "../../app/qgisapp.h"

rulesDialog::rulesDialog(const QString &tableName, QgsVectorLayer *theLayer, QWidget *parent)
: QDialog(parent), Ui::rulesDialog()
{
  setupUi(this);
  mLayer = theLayer;
/*
  QGis::GeometryType type = mLayer->geometryType();
  switch (type)
  {
    case QGis::Point:
      // hide all boxes
      for (QList<QCheckBox *>::Iterator it = mBoxes.begin(); it != mBoxes.end(); ++it)
        (*it)->setHidden(true);
      break;

    case QGis::Line:
    case QGis::Polygon:
      // all boxes remain shown
      ;
  }
*/
}

rulesDialog::~rulesDialog() {}
