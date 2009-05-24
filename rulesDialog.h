/***************************************************************************
  rulesDialog.h 
  TOPOLogy checker
  -------------------
         date                 : May 2009
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
  void addLayer(QgsMapLayer* layer);
  void removeLayer(QString layerId);
};

#endif
