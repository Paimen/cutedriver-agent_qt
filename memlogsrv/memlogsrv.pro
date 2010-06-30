############################################################################
## 
## Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies). 
## All rights reserved. 
## Contact: Nokia Corporation (testabilitydriver@nokia.com) 
## 
## This file is part of Testability Driver Qt Agent
## 
## If you have questions regarding the use of this file, please contact 
## Nokia at testabilitydriver@nokia.com . 
## 
## This library is free software; you can redistribute it and/or 
## modify it under the terms of the GNU Lesser General Public 
## License version 2.1 as published by the Free Software Foundation 
## and appearing in the file LICENSE.LGPL included in the packaging 
## of this file. 
## 
############################################################################


TEMPLATE = app

mac {
   CONFIG -= app_bundle
}


include(../tasbase.pri)

TARGET = qttasmemlog_srv
DESTDIR = bin
target.path = $$TAS_TARGET_BIN

symbian: {
    TARGET.CAPABILITY = ReadUserData WriteUserData ReadDeviceData WriteDeviceData SwEvent PowerMgmt
	TARGET.VID = VID_DEFAULT
  	TARGET.EPOCALLOWDLLDATA = 1 
	TARGET.EPOCHEAPSIZE = 0x20000 0x1400000
    LIBS += -lMemSpyDriverClient
    INCLUDEPATH += /epoc32/include/platform/memspy/driver
}

win32: {
	LIBS +=  -lUser32
}

INCLUDEPATH += .
INCLUDEPATH += services corelib
INCLUDEPATH += ../tascore/corelib

DEPENDPATH += . services corelib

# Input
SOURCES += main.cpp

include(corelib/corelib.pri)
include(services/services.pri)

QT -= gui
QT += network xml 
INSTALLS += target

LIBS += -L../tascore/lib/ -lqttestability

unix:!symbian:!macx {
  LIBS += -lX11 -lXtst
}


