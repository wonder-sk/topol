#ifndef CHECKDOCK_H
#define CHECKDOCK_H

#include <QDockWidget>

#include <qgsvectorlayer.h>

#include "ui_checkDock.h"
#include "rulesDialog.h"

class checkDock : public QDockWidget, public Ui::checkDock
{
Q_OBJECT

public:
  checkDock(const QString &tableName, QgsVectorLayer *theLayer, rulesDialog* theConfigureDialog, QWidget *parent = 0);
  ~checkDock();

private:
  QgsVectorLayer *mLayer;
  rulesDialog* mConfigureDialog;

private slots:
  void configure();
};

#endif
