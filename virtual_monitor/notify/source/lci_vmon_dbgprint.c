/*!
    \file       lci_vmon_dbgprint.c
    \brief      LCI_USBAV_DbgPrint() implementation.
    \details    LCI_USBAV_DbgPrint() implementation.
    \authors    lucaslin
    \version    0.01a
    \date       June 01, 2013
    \todo       (Optional)
    \bug        (Optional)
    \warning    (Optional)
    \copyright  (c) 2013 Luminon Core Incorporated. All Rights Reserved.

    Revision Log
    + 0.01a;    June 01, 2013;   lucaslin
     - Created.

 */

#include "lci_vmon.h"
#include <stdarg.h>

VOID
__cdecl
LCI_VMON_DbgPrint(
    __in_z __drv_formatString(printf) PCSTR format,
    ...
    )
    {
    va_list arg;
    CHAR    OutputString[1024];

    va_start (arg, format);
    (VOID) _vsnprintf_s(
        OutputString,
        sizeof(OutputString),
        _TRUNCATE,
        format,
        arg
        );
    va_end (arg);
    
    OutputDebugString(OutputString);
    }