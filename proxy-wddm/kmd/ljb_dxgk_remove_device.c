/*
 * ljb_dxgk_remove_device.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_RemoveDevice)
#endif

/*
 * forward declaration
 */
static VOID
LJB_DXGK_RemoveDevicePostProcessing(
    __in LJB_ADAPTER *  Adaper
    );

/*
 * Function: LJB_DXGK_RemoveDevice
 *
 * Description:
 * The DxgkDdiRemoveDevice function frees any resources allocated during
 * DxgkDdiAddDevice.
 *
 * Return Value:
 * DxgkDdiRemoveDevice returns STATUS_SUCCESS if it succeeds; otherwise, it returns
 * one of the error codes defined in Ntstatus.h.
 *
 * Remarks:
 * DxgkDdiRemoveDevice must free the context block represented by
 * MiniportDeviceContext.
 *
 * The DxgkDdiRemoveDevice function should be made pageable.
 */
NTSTATUS
LJB_DXGK_RemoveDevice(
    _In_  const PVOID   MiniportDeviceContext
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(MiniportDeviceContext);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiRemoveDevice)(MiniportDeviceContext);
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }
    LJB_DXGK_RemoveDevicePostProcessing(Adapter);

    return ntStatus;
}

static VOID
LJB_DXGK_RemoveDevicePostProcessing(
    __in LJB_ADAPTER *  Adapter
    )
{
    KIRQL           oldIrql;
    LJB_DEVICE *    MyDevice;
    LIST_ENTRY *    listHead;
    LIST_ENTRY *    listNext;
    LIST_ENTRY *    listEntry;

    /*
     * remove any LBJ_DEVICE associated with Adapter
     */
    listHead = &GlobalDriverData.ClientDeviceListHead;
    KeAcquireSpinLock(&GlobalDriverData.ClientDeviceListLock, &oldIrql);
    for (listEntry = listHead->Flink;
         listEntry != listHead;
         listEntry = listNext)
    {
        listNext = listEntry->Flink;
        MyDevice = CONTAINING_RECORD(listEntry, LJB_DEVICE, ListEntry);
        if (MyDevice->Adapter == Adapter)
        {
            RemoveEntryList(listEntry);
            LJB_FreePool(MyDevice);
        }
    }
    KeReleaseSpinLock(&GlobalDriverData.ClientDeviceListLock, oldIrql);

    /*
     * released any allocation
     */
    listHead = &Adapter->AllocationListHead;
    KeAcquireSpinLock(&Adapter->AllocationListLock, &oldIrql);
    while (!IsListEmpty(listHead))
    {
        LJB_ALLOCATION *    MyAllocation;

        listEntry = RemoveHeadList(listHead);
        InterlockedDecrement(&Adapter->AllocationListCount);
        MyAllocation = CONTAINING_RECORD(listEntry, LJB_ALLOCATION, ListEntry);

        DBG_PRINT(Adapter, DBGLVL_ALLOCATION,
            (__FUNCTION__ ": MyAllocation(%p)/hAllocation(%p) released\n",
            MyAllocation,
            MyAllocation->hAllocation
            ));
        LJB_FreePool(MyAllocation);
    }
    KeReleaseSpinLock(&Adapter->AllocationListLock, oldIrql);

    /*
     * released any LJB_STD_ALLOCATION_INFO
     */
    listHead = &Adapter->StdAllocationInfoListHead;
    KeAcquireSpinLock(&Adapter->StdAllocationInfoListLock, &oldIrql);
    while (!IsListEmpty(listHead))
    {
        LJB_STD_ALLOCATION_INFO *    StdAllocationInfo;

        listEntry = RemoveHeadList(listHead);
        InterlockedDecrement(&Adapter->StdAllocationInfoListCount);
        StdAllocationInfo = CONTAINING_RECORD(listEntry, LJB_STD_ALLOCATION_INFO, ListEntry);

        LJB_FreePool(StdAllocationInfo);
    }
    KeReleaseSpinLock(&Adapter->StdAllocationInfoListLock, oldIrql);

    /*
     * released any LJB_OPENED_ALLOCATION
     */
    listHead = &Adapter->OpenedAllocationListHead;
    KeAcquireSpinLock(&Adapter->OpenedAllocationListLock, &oldIrql);
    while (!IsListEmpty(listHead))
    {
        LJB_OPENED_ALLOCATION *    OpenedAllocationInfo;

        listEntry = RemoveHeadList(listHead);
        InterlockedDecrement(&Adapter->OpenedAllocationListCount);
        OpenedAllocationInfo = CONTAINING_RECORD(listEntry, LJB_OPENED_ALLOCATION, ListEntry);

        LJB_FreePool(OpenedAllocationInfo);
    }
    KeReleaseSpinLock(&Adapter->OpenedAllocationListLock, oldIrql);

    /*
     * FIXME: Release ClientDriverData should occur when the last driver instance is removed.
     * For now, assume there is only one device per driver.
     */
    KeAcquireSpinLock(&GlobalDriverData.ClientDriverListLock, &oldIrql);
    RemoveEntryList(&Adapter->ClientDriverData->ListEntry);
    LJB_FreePool(Adapter->ClientDriverData);
    KeReleaseSpinLock(&GlobalDriverData.ClientDriverListLock, oldIrql);

    KeAcquireSpinLock(&GlobalDriverData.ClientAdapterListLock, &oldIrql);
    RemoveEntryList(&Adapter->ListEntry);
    KeReleaseSpinLock(&GlobalDriverData.ClientAdapterListLock, oldIrql);
    InterlockedDecrement(&GlobalDriverData.ClientAdapterListCount);
    KdPrint((__FUNCTION__ ": Adapter(%p) released, ClientAdapterListCount=%u\n",
        Adapter,
        GlobalDriverData.ClientAdapterListCount
        ));

    LJB_FreePool(Adapter);
}