/*************************************************************************** 
** 
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies). 
** All rights reserved. 
** Contact: Nokia Corporation (testabilitydriver@nokia.com) 
** 
** This file is part of Testability Driver Qt Agent
** 
** If you have questions regarding the use of this file, please contact 
** Nokia at testabilitydriver@nokia.com . 
** 
** This library is free software; you can redistribute it and/or 
** modify it under the terms of the GNU Lesser General Public 
** License version 2.1 as published by the Free Software Foundation 
** and appearing in the file LICENSE.LGPL included in the packaging 
** of this file. 
** 
****************************************************************************/ 
 


#ifndef TASBASETRAVERSE_H
#define TASBASETRAVERSE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QPair>

#include "tasqtdatamodel.h"
#include "tasconstants.h"

class TraverseFilter;
class TasCommand;
class QGraphicsItem;
class QFont;

/*!
 If used directly the filter must be set or reset before use.
 e.g. 
    TasBaseTraverse traverse;
    traverse.resetFilter();
	...
 Normally the plugin loader takes care of this.
 */
class TAS_EXPORT TasBaseTraverse
{
public:
    virtual ~TasBaseTraverse();

	void resetFilter();
	void setFilter(TraverseFilter* traverseFilter);
    void addObjectDetails(TasObject* objectData, QObject* object);

	//protected:
    quint32 getParentId(QObject* object);
    void printProperties(TasObject* objectData, QObject* object);    
    void addFont(TasObject* objectInfo, QFont font);
    void addVariantValue(TasAttribute& attr, const QVariant& value);
    bool includeAttribute(const QString& attributeName);
    /*! return the 1) xy coodrinates of the item, 2) the absolute xy coordinate.
     Coordinate 0,0 is returned if data not available. */
    QPair<QPoint,QPoint> addGraphicsItemCoordinates(TasObject* objectInfo, QGraphicsItem* graphicsItem, 
                                    TasCommand* command);
    void addTextInfo(TasObject* objectInfo, const QString& text, const QFont& font, qreal width, Qt::TextElideMode mode=Qt::ElideRight);
private:
	TraverseFilter* mTraverseFilter;
};

class TAS_EXPORT TraverseFilter
{
public:
  TraverseFilter();
  ~TraverseFilter();

  void initialize(bool excludeProperties, QStringList attrBlackList, QStringList attrWhiteList, 
				  QStringList pluginBlackList, QStringList pluginWhiteList);
  void clear();
  bool filterPlugin(const QString& pluginName);
  bool includeAttribute(const QString& attributeName);
  bool filterProperties();

private:	
  QStringList mAttributeWhiteList;
  QStringList mAttributeBlackList;
  QStringList mPluginBlackList;
  QStringList mPluginWhiteList;
  bool mExcludeProperties;

};

#endif
 
