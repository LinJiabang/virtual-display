#ifndef _LJB_MONITOR_INTERFACE_H_
#define _LJB_MONITOR_INTERFACE_H_
#include <ntddk.h>

typedef struct _LJB_MONITOR_INTERFACE
{
    USHORT                  Size;
    USHORT                  Version;
    PVOID                   Context;
    PINTERFACE_REFERENCE    InterfaceReference;
    PINTERFACE_DEREFERENCE  InterfaceDereference;
    //  interface-specific  entries go here
} LJB_MONITOR_INTERFACE, *PLJB_MONITOR_INTERFACE;

#define LJB_MONITOR_INTERFACE_V0    0

#endif