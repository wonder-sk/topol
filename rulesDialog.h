#ifndef RULESDIALOG_H_
#define RULESDIALOG_H_

#include <QTabWidget>

#include <qgsvectorlayer.h>

#include "ui_rulesDialog.h"

class rulesDialog : public QTabWidget, public Ui::rulesDialog
{
Q_OBJECT

public:
  rulesDialog(const QString &tableName, QgsVectorLayer *theLayer, QWidget *parent = 0);
  ~rulesDialog();

private:
  QgsVectorLayer *mLayer;
  //TopolGeom *mGeom;
  //QList<QCheckBox *> mBoxes;
  //QMap<QString, CheckType> mBoxMap;
  //QgsFeatureIds mConflicting;

  //QString addNewLayer(QGis::WkbType geometrytype, QString fileName);

//private slots:
  //void checkGeometry();
};

#endif
