/*
 * ljb_dxgk_close_allocation.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_CloseAllocation)
#endif

static
VOID
LJB_DXGK_CloseAllocationPostProcessing(
    __in LJB_ADAPTER *                  Adapter,
    _In_ const DXGKARG_CLOSEALLOCATION *pCloseAllocation
    );

/*
 * Function: LJB_DXGK_CloseAllocation
 *
 * Description:
 * The DxgkDdiCloseAllocation function unbinds device-specific allocations that
 * the DxgkDdiOpenAllocation function created.
 *
 * Return Value:
 * DxgkDdiCloseAllocation returns STATUS_SUCCESS, or an appropriate error result
 * if the allocations are not successfully unbound from the graphics context device.
 *
 * Remarks:
 * Before the display miniport driver receives a call to its DxgkDdiDestroyAllocation
 * function to release allocations, the driver calls the DxgkDdiCloseAllocation
 * function to close all bindings to those allocations.
 *
 * For a resource that contains multiple allocations, the DirectX graphics kernel
 * subsystem directs DxgkDdiCloseAllocation to simultaneously close all of the
 * allocations by specifying the handles to the device-specific allocations in
 * the pOpenHandleList member of the DXGKARG_CLOSEALLOCATION structure.
 *
 * DxgkDdiCloseAllocation should be made pageable.
 */
NTSTATUS
LJB_DXGK_CloseAllocation(
    _In_ const HANDLE                  hDevice,
    _In_ const DXGKARG_CLOSEALLOCATION *pCloseAllocation
    )
{
    LJB_DEVICE * CONST                  MyDevice = LJB_DXGK_FindDevice(hDevice);
    LJB_ADAPTER * CONST                 Adapter = MyDevice->Adapter;
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiCloseAllocation)(hDevice, pCloseAllocation);
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        return ntStatus;
    }

    LJB_DXGK_CloseAllocationPostProcessing(Adapter, pCloseAllocation);
    return ntStatus;
}

static
VOID
LJB_DXGK_CloseAllocationPostProcessing(
    __in LJB_ADAPTER *                  Adapter,
    _In_ const DXGKARG_CLOSEALLOCATION *pCloseAllocation
    )
{
    UINT    i;

    for (i = 0; i < pCloseAllocation->NumAllocations; i++)
    {
        HANDLE CONST hDeviceSpecificAllocation = pCloseAllocation->pOpenHandleList[i];
        LJB_OPENED_ALLOCATION * CONST OpenedAllocation =
            LJB_DXGK_FindOpenedAllocation(Adapter, hDeviceSpecificAllocation);
        KIRQL   oldIrql;

        if (OpenedAllocation == NULL)
        {
            DBG_PRINT(Adapter, DBGLVL_ERROR,
                ("?" __FUNCTION__
                ": unable to find OpenedAllocation on hDeviceSpecificAllocation(0x%p)\n",
                hDeviceSpecificAllocation
                ));
            continue;
        }

        KeAcquireSpinLock(&Adapter->OpenedAllocationListLock, &oldIrql);
        RemoveEntryList(&OpenedAllocation->ListEntry);
        KeReleaseSpinLock(&Adapter->OpenedAllocationListLock, oldIrql);
        InterlockedDecrement(&Adapter->OpenedAllocationListCount);

        LJB_FreePool(OpenedAllocation);
    }
}
