/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef DEBUGEVENTCALLBACK_H
#define DEBUGEVENTCALLBACK_H

#include "debugeventcallbackbase.h"
#include <QtCore/QObject>

class DebugEventCallback :
        public QObject,
        public CdbCore::DebugEventCallbackBase

{
    Q_OBJECT
public:
    DebugEventCallback();

    STDMETHOD(GetInterestMask)(
          THIS_
          __out PULONG mask
          );

    STDMETHOD(Breakpoint)(
            THIS_
            __in PDEBUG_BREAKPOINT2 Bp
            );

    STDMETHOD(Exception)(
            THIS_
            __in PEXCEPTION_RECORD64 Exception,
            __in ULONG FirstChance
            );

    STDMETHOD(CreateThread)(
            THIS_
            __in ULONG64 Handle,
            __in ULONG64 DataOffset,
            __in ULONG64 StartOffset
            );
    STDMETHOD(ExitThread)(
            THIS_
            __in ULONG ExitCode
            );

    STDMETHOD(CreateProcess)(
            THIS_
            __in ULONG64 ImageFileHandle,
            __in ULONG64 Handle,
            __in ULONG64 BaseOffset,
            __in ULONG ModuleSize,
            __in_opt PCWSTR ModuleName,
            __in_opt PCWSTR ImageName,
            __in ULONG CheckSum,
            __in ULONG TimeDateStamp,
            __in ULONG64 InitialThreadHandle,
            __in ULONG64 ThreadDataOffset,
            __in ULONG64 StartOffset
            );

    STDMETHOD(ExitProcess)(
            THIS_
            __in ULONG ExitCode
            );

    STDMETHOD(LoadModule)(
            THIS_
            __in ULONG64 ImageFileHandle,
            __in ULONG64 BaseOffset,
            __in ULONG ModuleSize,
            __in_opt PCWSTR ModuleName,
            __in_opt PCWSTR ImageName,
            __in ULONG CheckSum,
            __in ULONG TimeDateStamp
            );

    STDMETHOD(UnloadModule)(
            THIS_
            __in_opt PCWSTR ImageBaseName,
            __in ULONG64 BaseOffset
            );

    STDMETHOD(SystemError)(
            THIS_
            __in ULONG Error,
            __in ULONG Level
            );

    STDMETHOD(ChangeDebuggeeState)(
            THIS_
            __in ULONG Flags,
            __in ULONG64 Argument
            );

signals:
    void processAttached(void *h);
};

#endif // DEBUGEVENTCALLBACK_H
