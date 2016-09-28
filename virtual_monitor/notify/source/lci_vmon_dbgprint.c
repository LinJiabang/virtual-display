#include "ljb_vmon.h"
#include <stdarg.h>

VOID
__cdecl
LJB_VMON_DbgPrint(
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