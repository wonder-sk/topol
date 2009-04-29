/***************************************************************************
  topol.cpp 
  Better attribute table
  -------------------
         begin                : 2008
         copyright            : Vita Cizek
         email                : weetya (at) gmail.com

 ***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/*  $Id: plugin.cpp 8053 2008-01-26 13:59:53Z timlinux $ */

//
// QGIS Specific includes
//

#include <qgsmaplayer.h>
#include <qgsapplication.h>
#include <qgisinterface.h>
#include <qgisgui.h>

#include "topol.h"
#include "rulesDialog.h"
#include "checkDock.h"

//
// Qt4 Related Includes
//

#include <QAction>
#include <QToolBar>
#include <QFile>
#include <QMessageBox>

static const char * const sIdent = "$Id: plugin.cpp 8053 2008-01-26 13:59:53Z timlinux $";
static const QString sName = QObject::tr("TOPOL");
static const QString sDescription = QObject::tr("Topology Checker Plugin");
static const QString sPluginVersion = QObject::tr("Version 0.1");
static const QgisPlugin::PLUGINTYPE sPluginType = QgisPlugin::UI;

//////////////////////////////////////////////////////////////////////
//
// THE FOLLOWING METHODS ARE MANDATORY FOR ALL PLUGINS
//
//////////////////////////////////////////////////////////////////////

/**
 * Constructor for the plugin. The plugin is passed a pointer 
 * an interface object that provides access to exposed functions in QGIS.
 * @param theQGisInterface - Pointer to the QGIS interface object
 */
Topol::Topol(QgisInterface * theQgisInterface):
                 QgisPlugin(sName,sDescription,sPluginVersion,sPluginType),
                 mQGisIface(theQgisInterface)
{
	std::cout << "Topol constr\n";
}

Topol::~Topol()
{
}

/*
 * Initialize the GUI interface for the plugin - this is only called once when the plugin is 
 * added to the plugin registry in the QGIS application.
 */
void Topol::initGui()
{
  mQActionPointer = new QAction(QIcon(),tr("Topology Checker"), this);

  QString myCurThemePath = QgsApplication::activeThemePath() + "/plugins/topol.png";
  QString myDefThemePath = QgsApplication::defaultThemePath() + "/plugins/topol.png";
  QString myQrcPath = ":/topol.png";

  if ( QFile::exists( myCurThemePath ) )
  {
    mQActionPointer->setIcon( QIcon( myCurThemePath ) );
  }
  else if ( QFile::exists( myDefThemePath ) )
  {
    mQActionPointer->setIcon( QIcon( myDefThemePath ) );
  }
  else if ( QFile::exists( myQrcPath ) )
  {
    mQActionPointer->setIcon( QIcon( myQrcPath ) );
  }
  else
  {
    mQActionPointer->setIcon( QIcon() );
  }

  // Create the action for tool
  //mQActionPointer = new QAction(QIcon(":/topol_c/topol.png"),tr("Topology Checker"), this);
  // Set the what's this text
  mQActionPointer->setWhatsThis(tr("Open Topology Checker for vector layer"));
  // Connect the action to the run
  connect(mQActionPointer, SIGNAL(triggered()), this, SLOT(run()));
  // Add the icon to the toolbar
  mQGisIface->addToolBarIcon(mQActionPointer);
  mQGisIface->addPluginToMenu(tr("&Topol"), mQActionPointer);

  //mRulesPointer = new QAction(QIcon(":/topol/topol.png"),tr("Topology Checker"), this);
  //connect(mRulesPointer, SIGNAL(triggered()), this, SLOT(rules()));
  //mQGisIface->addToolBarIcon(mRulesPointer);
  //mQGisIface->addPluginToMenu(tr("&Rules"), mRulesPointer);
  //run();

}
//method defined in interface
void Topol::help()
{
  //implement me!
}

// Slot called when the menu item is triggered
// If you created more menu items / toolbar buttons in initiGui, you should 
// create a separate handler for each action - this single run() method will
// not be enough
void Topol::run()
{
  
  std::cout << "to nejde?\n"<<std::flush;
  QgsMapLayer *myLayer = mQGisIface->activeLayer();

  if (myLayer == NULL || myLayer->type() != QgsMapLayer::VectorLayer) {
    QMessageBox::information(mQGisIface->mainWindow(), "No layer", "Select a vector layer!");
    return;
  }

  std::cout << "nejde?\n"<<std::flush;
  rulesDialog* rulesDia = new rulesDialog("Rules", (QgsVectorLayer *)(myLayer));
  //checkDock* chDock = new checkDock("Rules", (QgsVectorLayer *)(myLayer), rulesDia, vDock);
  checkDock* chDock = new checkDock("Rules", (QgsVectorLayer *)(myLayer), rulesDia);
  std::cout << "proc to nejde?\n"<<std::flush;
  mQGisIface->addDockWidget(Qt::RightDockWidgetArea, chDock);
  chDock->show();
}

// Unload the plugin by cleaning up the GUI
void Topol::unload()
{
  // remove the GUI
  std::cout << "proc to unloaduje?\n"<<std::flush;
  //mQGisIface->removePluginMenu("&Topol",mQActionPointer);
  //mQGisIface->removeToolBarIcon(mQActionPointer);
  //mQGisIface->removePluginMenu("&Rules",mRulesPointer);
  //mQGisIface->removeToolBarIcon(mRulesPointer);
  //delete mQActionPointer;
  //delete mRulesPointer;
}


//////////////////////////////////////////////////////////////////////////
//
//
//  THE FOLLOWING CODE IS AUTOGENERATED BY THE PLUGIN BUILDER SCRIPT
//    YOU WOULD NORMALLY NOT NEED TO MODIFY THIS, AND YOUR PLUGIN
//      MAY NOT WORK PROPERLY IF YOU MODIFY THIS INCORRECTLY
//
//
//////////////////////////////////////////////////////////////////////////


/** 
 * Required extern functions needed  for every plugin 
 * These functions can be called prior to creating an instance
 * of the plugin class
 */
// Class factory to return a new instance of the plugin class
QGISEXTERN QgisPlugin * classFactory(QgisInterface * theQgisInterfacePointer)
{
  return new Topol(theQgisInterfacePointer);
}
// Return the name of the plugin - note that we do not user class members as
// the class may not yet be insantiated when this method is called.
QGISEXTERN QString name()
{
  return sName;
}

// Return the description
QGISEXTERN QString description()
{
  return sDescription;
}

// Return the type (either UI or MapLayer plugin)
QGISEXTERN int type()
{
  return sPluginType;
}

// Return the version number for the plugin
QGISEXTERN QString version()
{
  return sPluginVersion;
}

// Delete ourself
QGISEXTERN void unload(QgisPlugin * thePluginPointer)
{
  delete thePluginPointer;
}
