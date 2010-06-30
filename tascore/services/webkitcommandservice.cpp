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
 

#include <QApplication>
#include <QListIterator>
#include <QtWebKit/QtWebKit>
#include <QGraphicsWebView>

#include "tascoreutils.h"
#include "tascommandparser.h"
#include "taslogger.h"
#include "webkitcommandservice.h"
#include "tassocket.h"

#include "actionhandler.h"
#include "gesturehandler.h"
#include "keyhandler.h"
#include "mousehandler.h"
#include "multitouchhandler.h"

#include "tasdeviceutils.h"

/*!
  \class WebkitCommandService
  \brief WebkitCommandService manages ui commands send to the app

*/    

WebkitCommandService::WebkitCommandService(QObject* parent)
    :QObject(parent), counter(0)
{}

WebkitCommandService::~WebkitCommandService()
{}

bool WebkitCommandService::executeService(TasCommandModel& model, TasResponse& response)
{    
    TasLogger::logger()->debug("WebkitCommandService::executeService " + model.service());
    //starting new id round
    counter = 0;
    if(model.service() == serviceName() ){
        foreach (TasTarget* target, model.targetList()) {
//            TasLogger::logger()->debug("WebkitCommandService::executeService " + target->type());
//            if (target->type() == TYPE_WEB) {

            	foreach (TasCommand* command, target->commandList()) {
//                    TasLogger::logger()->debug("WebkitCommandService::executeService command name:" + command->name());
                    bool ret = true;

                    if (command->name() == COMMAND_EXEC_JS_ON_OBJ) {
                        ret = executeJavaScriptWebElement(target, command);
                    }
                    if (command->name() == COMMAND_EXEC_JS_ON_QWEBFRAME){
                        ret = executeJavaScriptQWebFrame(target, command);
                    }

                    if(!ret) {
                        TasLogger::logger()->debug("WebkitCommandService::executeService failed, return true");
                        response.setErrorMessage(mErrorMessage);
                        return true;
                    }
                    else if(ret) {
                        TasLogger::logger()->debug("WebkitCommandService::executeService success, return true");
                        return true;
                    }
                }
  //          }
        }
        response.setErrorMessage("Webkit service target/command did not match any existing target or command");
        return false;
    }
    else{
        return false;
    }
}




bool WebkitCommandService::executeJavaScriptWebElement(TasTarget* target, TasCommand* command)
{
    TasLogger::logger()->debug("WebkitCommandService::executeJavaScriptWebElement TasId("+ target->id() + ") JavaScript \"" + command->parameter("java_script")  + "\"");
    int index = command->parameter("index").toInt();
    index = (index == -1) ? 0 : index;

    QList<QWebFrame*> mainFrameList;

    mainFrameList = traverseStart();

//    TasLogger::logger()->debug("  mainFrameList size " + QString::number(mainFrameList.count()) );

    foreach(QWebFrame* frame, mainFrameList)
    {
        bool ret = false;
        ret = traverseJavaScriptToWebElement(frame,
                                             command->parameter("webframe_id").toUInt(),
                                             command->parameter("java_script"),
                                             command->parameter("locator_query"),
                                             index,
                                             command);
        if(ret) return ret;
    }

    mErrorMessage = "When executing JavaScript to WebElement: QWebFrame not found";
    return false;
}

bool WebkitCommandService::executeJavaScriptQWebFrame(TasTarget* target, TasCommand* command)
{
    TasLogger::logger()->debug("WebkitCommandService::executeJavaScriptQWebFrame JavaScript \"" + command->parameter("java_script")  + "\"");
    quint32 id = target->id().toUInt();

    QList<QWebFrame*> mainFrameList;

    mainFrameList = traverseStart();

    foreach(QWebFrame* frame, mainFrameList)
    {
        bool ret = false;
        ret = traverseJavaScriptToQWebFrame(frame,
                                            command->parameter("java_script"),
                                            id);
        if(ret) return ret;
    }

    mErrorMessage = "When executing JavaScript to QWebFrame: QWebFrame not found";
    return false;

}

bool WebkitCommandService::traverseJavaScriptToWebElement(QWebFrame* webFrame,
                                                          quint32 webFrameId,
                                                          QString javaScript,
                                                          QString query,
                                                          int &index,
                                                          TasCommand* command,
                                                          int parentFrames)
{
    TasLogger::logger()->debug("WebkitCommandService::traverseJavaScriptToWebElement index(" + QString::number(index) +
                               ") webFrameId(" + QString::number(webFrameId) +
                               ") webFrame(" + QString::number((quint32)webFrame) + ")");
    QList<QWebElement> element_list;
    int count = 0;

    //find all elements, is matching webframe or if doing search to all webframes
    if(webFrameId == 0 || webFrameId == (quint32)webFrame)
    {
//        TasLogger::logger()->debug("  ok to search elements " );

         //findAllElements
         //check do we have other limitations than tag type
         if(query.indexOf('[') > -1 )
         {

             // query string is like a[id='value'][text='literals']
             // split query to QHash:
             //     "a[id='value'"
             //     "[text='literals'"
             // - then find the attribute between [ and =
             // - then find value between ' and '
             QHash<QString, QString> attribHash;
             attribHash.insert("type", query.left(query.indexOf('[')));
             TasLogger::logger()->debug("    type(" + query.left(query.indexOf('[')) + ")");
             QStringList parameterList = query.split("]", QString::SkipEmptyParts);
             foreach(QString s,parameterList)
             {
                 QString attribute = s.mid(s.indexOf('[')+1, s.indexOf('=')-s.indexOf('[')-1);
                 QString value = s.mid(s.indexOf('\'')+1, s.lastIndexOf('\'')-s.indexOf('\'')-1);

                 if(attribute.size()>0){
                     TasLogger::logger()->debug("    attrib(" + attribute + ") value(" + value + ")");
                     attribHash.insert(attribute, value);
                 }
             }

             QPoint webViewPos(0,0);
             QPoint screenPos(0,0);

             element_list = traverseFrame(webFrame, webViewPos, screenPos,attribHash);

         } else {
             element_list = webFrame->findAllElements(query).toList();
         }

         count = element_list.count();
    }

    TasLogger::logger()->debug("WebkitCommandService::executeJavaScriptQWebFrame JavaScript \"" + javaScript  +
                               "\" query \"" + query +
                               "\" index(" + QString::number(index) +
                               ") count(" + QString::number(count) + ")");

    if(count > index)
    {
        QWebElement element = element_list.at(index);
        element.evaluateJavaScript(javaScript);
        return true;
    }
    else{
        // consume index if any matching objects were found
        index -= count;
        // find all direct children frames and traverse those too, return on first true
        foreach(QWebFrame* childFrame, webFrame->childFrames()) {
           bool ret = false;
           ret = traverseJavaScriptToWebElement(childFrame,
                                                webFrameId,
                                                javaScript,
                                                query,
                                                index,
                                                command,
                                                parentFrames+1);
           if(ret) {
               return ret;
           }
        }
        mErrorMessage = "When executing JavaScript to WebElement: QWebElement not found";
        return false;
    }
}

bool WebkitCommandService::traverseJavaScriptToQWebFrame(QWebFrame* webFrame, QString javaScript, quint32 id)
{
    if((quint32)webFrame == id)
    {
//        TasLogger::logger()->debug("WebkitCommandService::traverseJavaScriptToQWebFrame found");
        webFrame->evaluateJavaScript(javaScript);
        return true;
    }
    else
    {
//        TasLogger::logger()->debug("WebkitCommandService::traverseJavaScriptToQWebFrame not found");
        // find all direct children frames and traverse those too
        foreach(QWebFrame* childFrame, webFrame->childFrames()) {
            bool ret = false;
            ret = traverseJavaScriptToQWebFrame(childFrame,javaScript,id);
            if(ret) {
                return ret;
            }
        }
    }
    mErrorMessage =  "When executing JavaScript to QWebFrame: QWebFrame not found";
    return false;
}

QList<QWebFrame*> WebkitCommandService::traverseObject(QObject* object)
{
    QList<QWebFrame*> list;

    //check decendants
    if(object->inherits("QWebView")){
        TasLogger::logger()->debug(" QWebView found " + QString(object->metaObject()->className()) );
        QWebView* web = qobject_cast<QWebView*>(object);
        if (web) {
            list.append(web->page()->mainFrame());
        }
    }
    else if(object->inherits("QGraphicsWebView")) {
        TasLogger::logger()->debug(" QGraphicsWebView found " + QString(object->metaObject()->className()) );
        QGraphicsWebView* web = qobject_cast<QGraphicsWebView*>(object);
        if (web) {
            list.append(web->page()->mainFrame());
        }
    }
    else // support for Symbian CWRT 9.2 and 10.1 - Fullscreen mode only
    if (object->inherits("WRT__WrtWebView")) {
        TasLogger::logger()->debug("WebKitCommandService::traverseObject WRT__WrtWebView");
        
        QGraphicsWebView* web = 0;    
        QMetaObject::invokeMethod(object, "view", Qt::DirectConnection, Q_RETURN_ARG(QGraphicsWebView*, web));
                
        if (web) {
            TasLogger::logger()->debug(" QGraphicsWebView found " + QString(object->metaObject()->className()) );            
            list.append(web->page()->mainFrame());
        }
    }

    //1. is graphicsview
    if(object->inherits("QGraphicsView")){
        list.append(traverseGraphicsViewItems(qobject_cast<QGraphicsView*>(object)));
    }
    //2. is GraphicsWidget
    QGraphicsWidget* graphicsWidget = qobject_cast<QGraphicsWidget*>(object);
    if(graphicsWidget){
        list.append(traverseGraphicsItemList(graphicsWidget));
    }
    //3. Widget children
    else{
        QObjectList children = object->children();
        if (!children.isEmpty()) {
            for (int i = 0; i < children.size(); ++i){
                QObject *obj = children.at(i);
                //TasLogger::logger()->debug(" "+ QString(obj->metaObject()->className()) );
                //only include widgets
                if (obj->isWidgetType() && obj->parent() == object){
                    QWidget *widget = qobject_cast<QWidget*>(obj);
                    // TODO This (and other similar hacks) needs to be moved to plugins once OSS changes are done
                    list.append(traverseObject(widget));
                }
            }
        }
    }
    return list;
}

QList<QWebFrame*> WebkitCommandService::traverseGraphicsItem(QGraphicsItem* graphicsItem)
{
    QList<QWebFrame*> list;
    if (graphicsItem->isWindow() || graphicsItem->isWidget()) {
        QObject * object = (QObject*)((QGraphicsWidget*)graphicsItem);
        list.append(traverseObject(object));
        // Traverse the actual widget under the proxy, if available
        QGraphicsProxyWidget* proxy = qobject_cast<QGraphicsProxyWidget*>(object);
        if (proxy) {
            list.append(traverseObject(proxy->widget()));
        }
    }
    else{
        list.append(traverseGraphicsItemList(graphicsItem));
    }
    return list;
}

QList<QWebFrame*> WebkitCommandService::traverseGraphicsItemList(QGraphicsItem* graphicsItem)
{
    QList<QWebFrame*> list;
    foreach (QGraphicsItem* item, graphicsItem->childItems()){
//        TasLogger::logger()->debug(" "+ QString(item->metaObject()->className()) );
        if(graphicsItem == item->parentItem()){
            // TODO This needs to be moved to plugins once OSS changes are done
            if(getApplicationName() == "webwidgetrunner" || item->isVisible() ||
               getApplicationName() == "duiappletrunner") {
               list.append(traverseGraphicsItem(item));
            }
        }
    }
    return list;
}

QList<QWebFrame*> WebkitCommandService::traverseGraphicsViewItems(QGraphicsView* view)
{
    QList<QWebFrame*> list;
    foreach(QGraphicsItem* item, view->items()){
//        TasLogger::logger()->debug("  "+ QString(item) );
        if(item->parentItem() == 0){
            // TODO This needs to be moved to plugins once OSS changes are done
            if(getApplicationName() == "webwidgetrunner" || item->isVisible() ||
               getApplicationName() == "duiappletrunner") {
                list.append(traverseGraphicsItem(item));
            }
        }
    }
    return list;
}

QList<QWebFrame*> WebkitCommandService::traverseStart()
{
    QList<QWebFrame*> list;

    foreach (QWidget *object, QApplication::allWidgets())
    {
//        TasLogger::logger()->debug(" "+ QString(object->metaObject()->className()) );
        list.append(traverseObject(object));
    }
    return list;
}


QList<QWebElement> WebkitCommandService::traverseFrame(QWebFrame* webFrame, const QPoint& parentPos, const QPoint& screenPos, QHash<QString, QString> attributeMatchHash)
{
    QList<QWebElement> list;
    if(webFrame) {
//        TasLogger::logger()->debug("WebkitCommandService::traverseFrame webFrame != null");


        QWebElement docElement = webFrame->documentElement();
        list.append(traverseWebElement(parentPos+webFrame->pos()-webFrame->scrollPosition(),
                                       screenPos+webFrame->pos()-webFrame->scrollPosition(),
                                       &docElement,
                                       attributeMatchHash));
   }
   return list;
}

/*!
  Traverse QWebElement
*/
QList<QWebElement> WebkitCommandService::traverseWebElement(QPoint parentPos,
                                                            QPoint screenPos,
                                                            QWebElement* webElement,
                                                            QHash<QString, QString> attributeMatchHash)
{
    QList<QWebElement> list;
    if(webElement == NULL || webElement->isNull()) {
        //TasLogger::logger()->debug("WebkitCommandService::traverseWebElement webElement is null");
        return list;
    }

    // traverse this element and all children

    //check that the parameters match for the web element
    QHashIterator<QString, QString> i(attributeMatchHash);
    QPoint childPos = QPoint(webElement->geometry().x(), webElement->geometry().y());
    counter++;
    bool okElement=true;
    while (i.hasNext() && okElement ) {
        i.next();
        if(webElement->attribute(i.key()) == i.value()){
            TasLogger::logger()->debug("  matched " + i.key() + " value \"" + i.value() + "\"" );
//        }else if(i.key() == "x" && i.value().toInt() == childPos.x()+parentPos.x()){
//            TasLogger::logger()->debug("  matched " + i.key());
//        }else if(i.key() == "y" && i.value().toInt() == childPos.y()+parentPos.y()){
//            TasLogger::logger()->debug("  matched " + i.key());
//        }else if(i.key() == "x_absolute" && i.value().toInt() == childPos.x()+screenPos.x()){
//            TasLogger::logger()->debug("  matched " + i.key());
//        }else if(i.key() == "y_absolute" && i.value().toInt() == childPos.y()+screenPos.y()){
//            TasLogger::logger()->debug("  matched " + i.key());
        }else if(i.key() == "width" && i.value().toInt() == webElement->geometry().width()){
            TasLogger::logger()->debug("  matched " + i.key() + " " + i.value());
        }else if(i.key() == "height" && i.value().toInt() == webElement->geometry().height()){
            TasLogger::logger()->debug("  matched " + i.key() + " " + i.value());
        }else if(i.key() == "objectType" && i.value() ==  "Web"){
            TasLogger::logger()->debug("  matched " + i.key() + " " + i.value());
        }else if(i.key() == "innerText" && i.value() ==  webElement->toPlainText()){
            TasLogger::logger()->debug("  matched " + i.key() + " " + i.value());
        }else if(i.key() == "elementText" && i.value() ==  webElement->toPlainText()){
            TasLogger::logger()->debug("  matched " + i.key() + " " + i.value());
        }else if(i.key() == "name" && i.value() ==  webElement->localName().toLower()){
            TasLogger::logger()->debug("  matched " + i.key() + " " + i.value());
        }else if(i.key() == "id" && i.value().toInt() == counter  ){
            TasLogger::logger()->debug("  matched " + i.key() + " " + i.value());
        }else if(i.key() == "type" && i.value() == webElement->tagName().toLower()){
            TasLogger::logger()->debug("  matched " + i.key() + " " + i.value());
        }else if(i.key() == "visible" && ((i.value() == "true") == (webElement->styleProperty("visibility", QWebElement::ComputedStyle).toLower() == "visible"))){
            TasLogger::logger()->debug("  matched " + i.key() + " " + i.value());
        }else if(i.key() == "hasFocus" && ((i.value() == "true") == (webElement->hasFocus()))){
            TasLogger::logger()->debug("  matched " + i.key() + " " + i.value());
        }else {
            //did not match webelement, skip to next
            okElement = false;
//            TasLogger::logger()->debug("WebkitCommandService::traverseWebElement no element for key(" + i.key() + ") and value(" + i.value() + ")");
        }
    }
    if(okElement){
        TasLogger::logger()->debug("WebkitCommandService::traverseWebElement found element(" + webElement->tagName().toLower() + ")");
        list.append(*webElement);
    }
    //childInfo.addBooleanAttribute("visible", webElement->styleProperty("visibility", QWebElement::ComputedStyle).toLower() == "visible");


    // traverse first child
    QWebElement firstChild = webElement->firstChild();

    if(!firstChild.isNull()) {
        //TasLogger::logger()->debug("WebkitCommandService::traverseWebElement " + webElement->localName() + " traverse first child");
        list.append(traverseWebElement(parentPos, screenPos, &firstChild, attributeMatchHash));
    }

    // check if this node has siblings and traverse them
    QWebElement sibling = webElement->nextSibling();
    if(!sibling.isNull()) {
        //TasLogger::logger()->debug("WebkitCommandService::traverseWebElement " + webElement->localName() + " traverse sibling");
        list.append(traverseWebElement(parentPos, screenPos, &sibling, attributeMatchHash));
    }
    return list;
}
