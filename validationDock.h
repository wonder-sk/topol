#ifndef VALIDATION_H
#define VALIDATION_H

#include <QDockWidget>

#include <qgsvectorlayer.h>

#include "ui_validationDock.h"
#include "rulesDialog.h"

class validationDock : public QDockWidget, public Ui::validationDock
{
Q_OBJECT

public:
  validationDock(const QString &tableName, QgsVectorLayer *theLayer, rulesDialog* theConfigureDialog, QWidget *parent = 0);
  ~validationDock();

private:
  QgsVectorLayer *mLayer;
  rulesDialog* mConfigureDialog;
};

#endif
