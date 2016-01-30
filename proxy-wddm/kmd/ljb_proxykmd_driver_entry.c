/*
 * ljb_proxykmd_driver_entry.c, adapted from WDK toaster sample code.
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, LJB_PROXYKMD_Unload)
#endif

LJB_GLOBAL_DRIVER_DATA   GlobalDriverData;

/*
 * Name:  DriverEntry
 *
 * Definition:
 *    NTSTATUS
 *    DriverEntry(
 *        __in PDRIVER_OBJECT  DriverObject,
 *        __in PUNICODE_STRING RegistryPath
 *        );
 *
 * Description:
 *    DriverEntry initializes the driver and is the first routine called by the
 *    system after the driver is loaded.
 *
 *    DriverObject - represents the instance of the function driver that is loaded
 *    into memory. DriverEntry must initialize members of DriverObject before it
 *    returns to the caller. DriverObject is allocated by the system before the
 *    driver is loaded, and it is released by the system after the system unloads
 *    the function driver from memory.
 *
 *    RegistryPath - represents the driver specific path in the Registry.
 *    The function driver can use the path to store driver related data between
 *    reboots. The path does not store hardware instance specific data.
 *
 * Return Value:
 *    DriverEntry returns STATUS_SUCCESS. NTSTATUS values are defined in the DDK
 *    header file, Ntstatus.h.
 *
 *    Drivers execute in the context of other threads in the system. They do not
 *    receive their own threads. This is different than with applications. After
 *    DriverEntry returns, the Toaster function driver executes in an asynchronous,
 *    event driven, manner.
 *
 *    The system schedules other threads to execute on behalf of the driver. Every
 *    thread has a user-mode stack and a kernel-mode stack. When the system schedules
 *    a thread to execute in a driver, that thread's kernel stack is used.
 *
 *    The I/O manager calls the function driver's routines to process events and I/O
 *    operations as needed. Multiple threads can execute simultaneously throughout the
 *    driver, thus special attention to how the driver operates and processes events
 *    and I/O operations is required
 *
 */

NTSTATUS
DriverEntry(
    __in PDRIVER_OBJECT  DriverObject,
    __in PUNICODE_STRING RegistryPath
    )
    {
    ULONG           i;

    UNREFERENCED_PARAMETER(RegistryPath);

    KdPrint((__FUNCTION__ ": Built %s %s\n", __DATE__, __TIME__));

    RtlZeroMemory(&GlobalDriverData, sizeof(GlobalDriverData));
    InitializeListHead(&GlobalDriverData.ClientDriverListHead);
    GlobalDriverData.ClientDriverListCount = 0;

    InitializeListHead(&GlobalDriverData.ClientAdapterListHead);
    GlobalDriverData.ClientAdapterListCount = 0;

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
        {
        DriverObject->MajorFunction[i] = LJB_PROXYKMD_PassDown;
        }

    DriverObject->MajorFunction[IRP_MJ_CREATE]                  = &LJB_PROXYKMD_DispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]                   = &LJB_PROXYKMD_DispatchClose;
    DriverObject->MajorFunction[IRP_MJ_PNP]                     = &LJB_PROXYKMD_DispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]                   = &LJB_PROXYKMD_DispatchPower;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]          = &LJB_PROXYKMD_DispatchIoctl;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = &LJB_PROXYKMD_DispatchInternalIoctl;
    DriverObject->DriverExtension->AddDevice                    = &LJB_PROXYKMD_AddDevice;
    DriverObject->DriverUnload                                  = &LJB_PROXYKMD_Unload;

    return STATUS_SUCCESS;
    }

/*++

Routine Description:

    Free all the allocated resources in DriverEntry, etc.

Arguments:

    DriverObject - pointer to a driver object.

Return Value:

    VOID.

--*/
VOID
LJB_PROXYKMD_Unload(
    __in PDRIVER_OBJECT DriverObject
    )
{
    PAGED_CODE ();

    //
    // The device object(s) should be NULL now
    // (since we unload, all the devices objects associated with this
    // driver must be deleted.
    //
    if (DriverObject->DeviceObject != NULL)
    {
        KdPrint(("DeviceObject is not deleted\n"));
    }

    //
    // We should not be unloaded until all the devices we control
    // have been removed from our queue.
    //
    KdPrint((__FUNCTION__ "\n"));

    return;
}

