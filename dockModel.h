/***************************************************************************
  dockModel.h 
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

#ifndef DOCKMODEL_H
#define DOCKMODEL_H

#include <QAbstractTableModel>
#include <QModelIndex>
#include <QObject>

#include "topolError.h"

class DockModel: public QAbstractTableModel
{
Q_OBJECT

public:
  DockModel(ErrorList& theErrorList, QObject *parent);
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  virtual QVariant data(const QModelIndex &index, int role) const;
  virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
  Qt::ItemFlags flags(const QModelIndex &index) const;

  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;

  void reload(const QModelIndex &index1, const QModelIndex &index2);
  void resetModel();

private:
  ErrorList& mErrorlist;
  QList<QString> mHeader;
};

#endif
