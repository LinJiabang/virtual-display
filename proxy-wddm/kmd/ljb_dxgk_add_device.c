/*
 * ljb_dxgk_add_device.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

/*
 * Function: LJB_DXGK_AddDevice
 *
 * Description:
 * The DxgkDdiAddDevice function creates a context block for a display adapter and
 * returns a handle that represents the display adapter.
 *
 * Return Value:
 * DxgkDdiAddDevice returns STATUS_SUCCESS if it succeeds; otherwise, it returns
 * one of the error codes defined in Ntstatus.h.
 *
 * Remarks:
 * The DxgkDdiAddDevice function allocates a private context block that is
 * associated with the display adapter identified by PhysicalDeviceObject. You
 * can think of the handle returned in MiniportDeviceContext as a handle to the
 * display adapter or as a handle to the context block associated with the display
 * adapter. The DirectX graphics kernel subsystem (Dxgkrnl.sys) will supply the
 * handle in subsequent calls to the display miniport driver. The following list
 * gives examples of various components of Dxgkrnl.sys passing the handle to
 * functions implemented by the display miniport driver.
 *
 * The display port driver supplies the handle in the MiniportDeviceContext
 * parameter of the DxgkDdiStartDevice function.
 *
 * The VidPN manager supplies the handle in the hAdapter parameter of the
 * DxgkDdiIsSupportedVidPn function.
 *
 * The DirectX graphics core supplies the handle in the hAdapter parameter of
 * the DxgkDdiQueryAdapterInfo function.
 *
 * Do not be confused by the fact that sometimes the handle is named
 * MiniportDeviceContext and sometimes it is named hAdapter. Also, do not confuse
 * this handle with the hDevice parameter that is passed to certain display
 * miniport driver functions.
 *
 * Some display adapter cards have two or more PCI functions that play the role
 * of display adapter. For example, certain older cards implement multiple views
 * by having a separate PCI function for each view. The display port driver calls
 * DxgkDdiAddDevice once for each of those PCI functions, at which time the
 * display miniport driver can indicate that it supports the PCI function (by
 * setting MiniportDeviceContext to a nonzero value) or that it does not support
 * the PCI function (by setting MiniportDeviceContext to NULL). To get information
 * about a particular PCI function, the display miniport driver can pass
 * PhysicalDeviceObject to IoGetDeviceProperty.
 *
 * In DxgkDdiRemoveDevice, free your context block and any other resources you
 * allocate during DxgkDdiAddDevice.
 *
 * The DxgkDdiAddDevice function should be made pageable.
 */
NTSTATUS
LJB_DXGK_AddDevice(
    _In_ LJB_CLIENT_DRIVER_DATA *ClientDriverData,
    _In_  const PDEVICE_OBJECT  PhysicalDeviceObject,
    _Out_       PVOID           *MiniportDeviceContext
    )
{
    DRIVER_INITIALIZATION_DATA * CONST DriverInitData = &ClientDriverData->DriverInitData;
    LJB_ADAPTER * Adapter;
    NTSTATUS ntStatus;
    KIRQL oldIrql;
    HANDLE hKey;
    ULONG ResultLength;

    ntStatus =  (*DriverInitData->DxgkDdiAddDevice)(PhysicalDeviceObject, MiniportDeviceContext);

    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__ ": failed with ntStatus(0x%08x)\n", ntStatus));
        return ntStatus;
    }

    /*
     * do post processing here
     */
    if (*MiniportDeviceContext == NULL)
        return ntStatus;

    Adapter = LJB_PROXYKMD_GetPoolZero(sizeof(LJB_ADAPTER));
    if (Adapter == NULL)
    {
        KdPrint(("?" __FUNCTION__ ": unable to allocate Adapter?\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    InitializeListHead(&Adapter->ListEntry);
    Adapter->PhysicalDeviceObject = PhysicalDeviceObject;
    Adapter->hAdapter = *MiniportDeviceContext;
    Adapter->ClientDriverData = ClientDriverData;
    Adapter->DebugMask = DBGLVL_DEFAULT;

    InitializeListHead(&Adapter->AllocationListHead);
    KeInitializeSpinLock(&Adapter->AllocationListLock);

    KeAcquireSpinLock(&GlobalDriverData.ClientAdapterListLock, &oldIrql);
    InsertTailList(&GlobalDriverData.ClientAdapterListHead, &Adapter->ListEntry);
    KeReleaseSpinLock(&GlobalDriverData.ClientAdapterListLock, oldIrql);
    InterlockedIncrement(&GlobalDriverData.ClientAdapterListCount);

    /*
     * Get Driver key eg. HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Control\Class\{4d36e968-e325-11ce-bfc1-08002be10318}\0000
     */
    ntStatus = IoOpenDeviceRegistryKey(
        PhysicalDeviceObject,
        PLUGPLAY_REGKEY_DRIVER,
        GENERIC_READ,
        &hKey
        );
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__ ": "
            "IoOpenDeviceRegistryKey failed with ntStatus(0x%08x)\n",
            ntStatus
            ));
        return ntStatus;
    }

    ntStatus = ZwQueryKey(
        hKey,
        KeyNameInformation,
        &Adapter->DriverKeyNameInfo,
        sizeof(Adapter->DriverKeyNameInfo) + sizeof(Adapter->DriverKeyNameBuffer0),
        &ResultLength
        );
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__ ": "
            "ZwQueryValueKey failed with ntStatus(0x%08x), "
            "ResultLength(0x%x)\n",
            ntStatus,
            ResultLength
            ));
    }
    ZwClose(hKey);
    Adapter->DriverKeyNameBuffer = Adapter->DriverKeyNameInfo.Name;
    KdPrint((__FUNCTION__ ": DriverKeyName (%ws)\n", Adapter->DriverKeyNameBuffer));

    return ntStatus;
}

NTSTATUS
LJB_DXGK_AddDevice0(
    _In_  const PDEVICE_OBJECT  PhysicalDeviceObject,
    _Out_       PVOID           *MiniportDeviceContext
    )
{
    LJB_CLIENT_DRIVER_DATA *    ClientDriverData;
    LIST_ENTRY * CONST          listHead = &GlobalDriverData.ClientDriverListHead;
    LIST_ENTRY *                listEntry;
    KIRQL                       oldIrql;
    NTSTATUS                    ntStatus;

    /*
     * find the ClientDriverData with DxgkAddDeviceTag == 0
     */
    ClientDriverData = NULL;
    KeAcquireSpinLock(&GlobalDriverData.ClientDriverListLock, &oldIrql);
    for (listEntry = listHead->Flink;
         listEntry != listHead;
         listEntry = listEntry->Flink)
    {
        LJB_CLIENT_DRIVER_DATA * thisData;

        thisData = CONTAINING_RECORD(
            listEntry,
            LJB_CLIENT_DRIVER_DATA,
            ListEntry
            );
        if (thisData->DxgkAddDeviceTag == 0)
        {
            ClientDriverData = thisData;
            break;
        }
    }
    KeReleaseSpinLock(&GlobalDriverData.ClientDriverListLock, oldIrql);

    ntStatus = STATUS_UNSUCCESSFUL;
    if (ClientDriverData != NULL)
    {
        ntStatus = LJB_DXGK_AddDevice(ClientDriverData, PhysicalDeviceObject, MiniportDeviceContext);
    }
    return ntStatus;
}

NTSTATUS
LJB_DXGK_AddDevice1(
    _In_  const PDEVICE_OBJECT  PhysicalDeviceObject,
    _Out_       PVOID           *MiniportDeviceContext
    )
{
    LJB_CLIENT_DRIVER_DATA *    ClientDriverData;
    LIST_ENTRY * CONST          listHead = &GlobalDriverData.ClientDriverListHead;
    LIST_ENTRY *                listEntry;
    KIRQL                       oldIrql;
    NTSTATUS                    ntStatus;

    /*
     * find the ClientDriverData with DxgkAddDeviceTag == 1
     */
    ClientDriverData = NULL;
    KeAcquireSpinLock(&GlobalDriverData.ClientDriverListLock, &oldIrql);
    for (listEntry = listHead->Flink;
         listEntry != listHead;
         listEntry = listEntry->Flink)
    {
        LJB_CLIENT_DRIVER_DATA * thisData;

        thisData = CONTAINING_RECORD(
            listEntry,
            LJB_CLIENT_DRIVER_DATA,
            ListEntry
            );
        if (thisData->DxgkAddDeviceTag == 1)
        {
            ClientDriverData = thisData;
            break;
        }
    }
    KeReleaseSpinLock(&GlobalDriverData.ClientDriverListLock, oldIrql);

    ntStatus = STATUS_UNSUCCESSFUL;
    if (ClientDriverData != NULL)
    {
        ntStatus = LJB_DXGK_AddDevice(ClientDriverData, PhysicalDeviceObject, MiniportDeviceContext);
    }
    return ntStatus;
}

NTSTATUS
LJB_DXGK_AddDevice2(
    _In_  const PDEVICE_OBJECT  PhysicalDeviceObject,
    _Out_       PVOID           *MiniportDeviceContext
    )
{
    LJB_CLIENT_DRIVER_DATA *    ClientDriverData;
    LIST_ENTRY * CONST          listHead = &GlobalDriverData.ClientDriverListHead;
    LIST_ENTRY *                listEntry;
    KIRQL                       oldIrql;
    NTSTATUS                    ntStatus;

    /*
     * find the ClientDriverData with DxgkAddDeviceTag == 2
     */
    ClientDriverData = NULL;
    KeAcquireSpinLock(&GlobalDriverData.ClientDriverListLock, &oldIrql);
    for (listEntry = listHead->Flink;
         listEntry != listHead;
         listEntry = listEntry->Flink)
    {
        LJB_CLIENT_DRIVER_DATA * thisData;

        thisData = CONTAINING_RECORD(
            listEntry,
            LJB_CLIENT_DRIVER_DATA,
            ListEntry
            );
        if (thisData->DxgkAddDeviceTag == 2)
        {
            ClientDriverData = thisData;
            break;
        }
    }
    KeReleaseSpinLock(&GlobalDriverData.ClientDriverListLock, oldIrql);

    ntStatus = STATUS_UNSUCCESSFUL;
    if (ClientDriverData != NULL)
    {
        ntStatus = LJB_DXGK_AddDevice(ClientDriverData, PhysicalDeviceObject, MiniportDeviceContext);
    }
    return ntStatus;
}

NTSTATUS
LJB_DXGK_AddDevice3(
    _In_  const PDEVICE_OBJECT  PhysicalDeviceObject,
    _Out_       PVOID           *MiniportDeviceContext
    )
{
    LJB_CLIENT_DRIVER_DATA *    ClientDriverData;
    LIST_ENTRY * CONST          listHead = &GlobalDriverData.ClientDriverListHead;
    LIST_ENTRY *                listEntry;
    KIRQL                       oldIrql;
    NTSTATUS                    ntStatus;

    /*
     * find the ClientDriverData with DxgkAddDeviceTag == 2
     */
    ClientDriverData = NULL;
    KeAcquireSpinLock(&GlobalDriverData.ClientDriverListLock, &oldIrql);
    for (listEntry = listHead->Flink;
         listEntry != listHead;
         listEntry = listEntry->Flink)
    {
        LJB_CLIENT_DRIVER_DATA * thisData;

        thisData = CONTAINING_RECORD(
            listEntry,
            LJB_CLIENT_DRIVER_DATA,
            ListEntry
            );
        if (thisData->DxgkAddDeviceTag == 3)
        {
            ClientDriverData = thisData;
            break;
        }
    }
    KeReleaseSpinLock(&GlobalDriverData.ClientDriverListLock, oldIrql);

    ntStatus = STATUS_UNSUCCESSFUL;
    if (ClientDriverData != NULL)
    {
        ntStatus = LJB_DXGK_AddDevice(ClientDriverData, PhysicalDeviceObject, MiniportDeviceContext);
    }
    return ntStatus;
}

LJB_ADAPTER *
LJB_DXGK_FindAdapterByDriverAdapter(
    __in PVOID hAdapter
    )
{
    LIST_ENTRY * CONST  listHead = &GlobalDriverData.ClientAdapterListHead;
    LIST_ENTRY *        listEntry;
    LJB_ADAPTER *       Adapter;
    KIRQL               oldIrql;

    Adapter = NULL;
    KeAcquireSpinLock(&GlobalDriverData.ClientAdapterListLock, &oldIrql);
    for (listEntry = listHead->Flink;
         listEntry != listHead;
         listEntry = listEntry->Flink)
    {
        LJB_ADAPTER * thisAdapter;

        thisAdapter = CONTAINING_RECORD(listEntry, LJB_ADAPTER, ListEntry);
        if (thisAdapter->hAdapter == hAdapter)
        {
            Adapter = thisAdapter;
            break;
        }
    }
    KeReleaseSpinLock(&GlobalDriverData.ClientAdapterListLock, oldIrql);

    return Adapter;
}
