#ifndef RULESDIALOG_H_
#define RULESDIALOG_H_

#include <QDialog>

#include <qgsvectorlayer.h>

#include "ui_rulesDialog.h"

class rulesDialog : public QDialog, public Ui::rulesDialog
{
Q_OBJECT

public:
  rulesDialog(const QString &tableName, QList<QString> tests, QList<QString>layerList, QWidget *parent);
  ~rulesDialog();
  QTableWidget* testTable() { return mTestTable; }
private slots:
  void addTest();
  void deleteTest();
};

#endif
