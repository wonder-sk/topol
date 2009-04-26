#include <QtGui>

#include "TopolDialog.h"
#include "TopolGeom.h"

#include <qgsvectordataprovider.h>
#include <qgsvectorlayer.h>
#include <qgssearchstring.h>
#include <qgssearchtreenode.h>
#include <qgsmaplayer.h>
#include <qgsmaplayerregistry.h>

#include <qgsproviderregistry.h>
#include <qgslogger.h>

#include "../../app/qgisapp.h"

TopolDialog::TopolDialog(const QString &tableName, QgsVectorLayer *theLayer, QWidget *parent)
: QDialog(parent), Ui::TopolDialog()
{
  setupUi(this);
  std::cout << "XXXXXXX\n\n\n";
  mLayer = theLayer;
  //mGeom = new TopolGeom(theLayer, resultWindow);

  QgsVectorLayer *nodeLayer, *wayLayer;

  mBoxes.append(intersectBox);
  mBoxes.append(overlapBox);
  mBoxes.append(multipartBox);

  mBoxMap[intersectBox->objectName()] = Intersection;
  mBoxMap[overlapBox->objectName()] = Overlap;
  mBoxMap[multipartBox->objectName()] = Multipart;

  connect(checkButton, SIGNAL(clicked()), this, SLOT(checkGeometry()));

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

  QString path = addNewLayer(QGis::WKBPoint, "nodes");
  nodeLayer = new QgsVectorLayer(path, "nodes", "ogr" );
  QgsMapLayerRegistry::instance()->addMapLayer(nodeLayer);

  path = addNewLayer(QGis::WKBLineString, "arcs");
  wayLayer = new QgsVectorLayer(path, "arcs", "ogr" );
  QgsMapLayerRegistry::instance()->addMapLayer(wayLayer);

  std::cout << "buildge\n\n\n";
  //mGeom->buildGeometry(nodeLayer, wayLayer);
  std::cout << "buildge hotov\n\n\n";
}

TopolDialog::~TopolDialog()
{
  delete mGeom;
}

void TopolDialog::checkGeometry()
{
  resultWindow->clear();
  mConflicting.clear();

  QgsFeatureIds conflicting;

  QList<QCheckBox*>::Iterator it = mBoxes.begin();
  for (; it != mBoxes.end(); ++it)
  {
    if ((*it)->isChecked())
      conflicting = mGeom->checkGeometry(mBoxMap[(*it)->objectName()]);

    mConflicting |= conflicting;
  }

  // select conflicting features
  mLayer->setSelectedFeatures(conflicting);
  QgisApp::instance()->zoomToSelected();
}

QString TopolDialog::addNewLayer(QGis::WkbType geometrytype, QString fileName)
{
  QString fileformat = "ESRI Shapefile";

  std::list<std::pair<QString, QString> > attributes;
  attributes.push_back(std::make_pair("name", "String"));
  QString enc = "UTF-8";

  //try to create the new layer with OGRProvider instead of QgsVectorFileWriter
  QgsProviderRegistry *pReg = QgsProviderRegistry::instance();
  QString ogrlib = pReg->library( "ogr" );

  // load the data provider
  QLibrary* myLib = new QLibrary( ogrlib );
  bool loaded = myLib->load();

  if ( loaded )
  {
    QgsDebugMsg( "ogr provider loaded" );

    typedef bool ( *createEmptyDataSourceProc )( const QString&, const QString&, const QString&, QGis::WkbType,
        const std::list<std::pair<QString, QString> >& );
    createEmptyDataSourceProc createEmptyDataSource = ( createEmptyDataSourceProc ) cast_to_fptr( myLib->resolve( "createEmptyDataSource" ) );
    if ( createEmptyDataSource )
    {
      if ( geometrytype != QGis::WKBUnknown )
      {
        createEmptyDataSource( fileName, fileformat, enc, geometrytype, attributes );
      }
      else
      {
        QgsDebugMsg( "geometry type not recognised" );
        return QString();
      }
    }
    else
    {
      QgsDebugMsg( "Resolving newEmptyDataSource(...) failed" );
    }
  }

  return fileName;
}
