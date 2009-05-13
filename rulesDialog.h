#ifndef RULESDIALOG_H_
#define RULESDIALOG_H_

#include <QDialog>

#include <qgsvectorlayer.h>

#include "ui_rulesDialog.h"

class rulesDialog : public QDialog, public Ui::rulesDialog
{
Q_OBJECT

public:
  rulesDialog(const QString &tableName, QgsVectorLayer *theLayer, QWidget *parent = 0);
  ~rulesDialog();

private:
  QgsVectorLayer *mLayer;
  //QList<QCheckBox *> mBoxes;
  //QMap<QString, CheckType> mBoxMap;
};

#endif
