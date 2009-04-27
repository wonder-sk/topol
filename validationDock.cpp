#include <QtGui>

#include "validationDock.h"

#include <qgsvectordataprovider.h>
#include <qgsvectorlayer.h>
#include <qgssearchstring.h>
#include <qgssearchtreenode.h>
#include <qgsmaplayer.h>
#include <qgsmaplayerregistry.h>

#include <qgsproviderregistry.h>
#include <qgslogger.h>

#include "rulesDialog.h"
#include "../../app/qgisapp.h"

validationDock::validationDock(const QString &tableName, QgsVectorLayer* theLayer, rulesDialog* theConfigureDialog, QWidget* parent)
: QDockWidget(parent), Ui::validationDock()
{
  setupUi(this);

  std::cout << "check dock\n\n\n";
  mLayer = theLayer;
  mConfigureDialog = theConfigureDialog;

  connect(configureButton, SIGNAL(clicked()), this, SLOT(configure()));
}

validationDock::~validationDock() {}
