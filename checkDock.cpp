#include <QtGui>

#include "checkDock.h"

#include <qgsvectordataprovider.h>
#include <qgsvectorlayer.h>
#include <qgssearchstring.h>
#include <qgssearchtreenode.h>
#include <qgsmaplayer.h>
#include <qgsmaplayerregistry.h>

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

  connect(configureButton, SIGNAL(clicked()), this, SLOT(configure()));
  connect(validateButton, SIGNAL(clicked()), this, SLOT(validate()));
}

void checkDock::configure()
{
  mConfigureDialog->show();
}

void checkDock::validate()
{
  mValidationDock->show();
}

checkDock::~checkDock() {}
