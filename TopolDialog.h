#ifndef TOPOLDIALOG_H_
#define TOPOLDIALOG_H_

#include <QDialog>
#include <QString>
#include <QModelIndex>

#include <qgsvectorlayer.h>

#include "ui_TopolDialog.h"
#include "TopolGeom.h"

class QDialogButtonBox;
class QPushButton;
class QLineEdit;
class QComboBox;

class TopolDialog : public QDialog, public Ui::TopolDialog
{
Q_OBJECT

public:
  TopolDialog(const QString &tableName, QgsVectorLayer *theLayer, QWidget *parent = 0);
  ~TopolDialog();

private:
  QgsVectorLayer *mLayer;
  TopolGeom *mGeom;
  QList<QCheckBox *> mBoxes;
  QMap<QString, CheckType> mBoxMap;
  QgsFeatureIds mConflicting;

  QString addNewLayer(QGis::WkbType geometrytype, QString fileName);

private slots:
  void checkGeometry();
};

#endif
