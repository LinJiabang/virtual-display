/*++
Copyright (c) 1990-2000    Microsoft Corporation All Rights Reserved

Module Name:

    driver.h

Abstract:

    This module contains the common declarations for the
    bus, function and filter drivers.

Environment:

    kernel mode only

--*/

#ifndef __DRIVER_H
#define __DRIVER_H

//
// Define Interface reference/dereference routines for
//  Interfaces exported by IRP_MN_QUERY_INTERFACE
//

typedef VOID (*PINTERFACE_REFERENCE)(PVOID Context);
typedef VOID (*PINTERFACE_DEREFERENCE)(PVOID Context);

typedef
BOOLEAN
(*PTOASTER_GET_CRISPINESS_LEVEL)(
                           IN   PVOID Context,
                           OUT  PUCHAR Level
                               );

typedef
BOOLEAN
(*PTOASTER_SET_CRISPINESS_LEVEL)(
                           IN   PVOID Context,
                           OUT  UCHAR Level
                               );

typedef
BOOLEAN
(*PTOASTER_IS_CHILD_PROTECTED)(
                             IN PVOID Context
                             );

//
// Interface for getting and setting power level etc.,
//
typedef struct _TOASTER_INTERFACE_STANDARD {
    INTERFACE                        InterfaceHeader;
    PTOASTER_GET_CRISPINESS_LEVEL    GetCrispinessLevel;
    PTOASTER_SET_CRISPINESS_LEVEL    SetCrispinessLevel;
    PTOASTER_IS_CHILD_PROTECTED      IsSafetyLockEnabled; //):
} TOASTER_INTERFACE_STANDARD, *PTOASTER_INTERFACE_STANDARD;


#endif

