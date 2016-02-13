/*
 * ljb_dxgk_destroy_allocation.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_DestroyAllocation)
#endif

/*
 * forward declaration
 */
static VOID
LJB_DXGK_DestroyAllocationPostProcessing(
    __in LJB_ADAPTER *                      Adapter,
    _In_ const DXGKARG_DESTROYALLOCATION *  pDestroyAllocation
    );

/*
 * Function: LJB_DXGK_DestroyAllocation
 *
 * Description:
 * The DxgkDdiDestroyAllocation function releases allocations.
 *
 * Return Value:
 * DxgkDdiDestroyAllocation returns STATUS_SUCCESS, or an appropriate error
 * result if the allocations are not successfully released.
 *
 * Remarks:
 * When the user-mode display driver calls the pfnDeallocateCb function, the
 * DirectX graphics kernel subsystem (which is part of Dxgkrnl.sys) calls the
 * display miniport driver's DxgkDdiDestroyAllocation function to release the
 * allocations. The display miniport driver should clean up its internal data
 * structures and references to the allocations. The Microsoft Direct3D runtime
 * initiates calls to the video memory manager (which is also part of Dxgkrnl.sys),
 * which then calls the GPU scheduler (which is also part of Dxgkrnl.sys) to
 * synchronize before video memory is actually released.
 *
 * The display miniport driver can release the entire resource as well as
 * allocations. To determine whether the resource should be released, the display
 * miniport driver can check whether the DestroyResource flag is set in the Flags
 * member of the DXGKARG_DESTROYALLOCATION structure that the pDestroyAllocation
 * parameter points to. To release the resource, the display miniport driver must
 * clean up the handle that the hResource member of DXGKARG_DESTROYALLOCATION
 * specifies. If the display miniport driver does not release the resource, the
 * driver can change the value in hResource if necessary.
 *
 * DxgkDdiDestroyAllocation should be made pageable.
 */
NTSTATUS
LJB_DXGK_DestroyAllocation(
    _In_ const HANDLE                    hAdapter,
    _In_ const DXGKARG_DESTROYALLOCATION *pDestroyAllocation
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiDestroyAllocation)(
        hAdapter,
        pDestroyAllocation
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    LJB_DXGK_DestroyAllocationPostProcessing(Adapter, pDestroyAllocation);

    return ntStatus;
}

static VOID
LJB_DXGK_DestroyAllocationPostProcessing(
    __in LJB_ADAPTER *                      Adapter,
    _In_ const DXGKARG_DESTROYALLOCATION *  pDestroyAllocation
    )
{
    LIST_ENTRY * CONST  listHead = &Adapter->AllocationListHead;;
    LJB_ALLOCATION *    MyAllocation;
    LIST_ENTRY *        listNext;
    LIST_ENTRY *        listEntry;
    KIRQL               oldIrql;
    UINT                i;

    /*
     * remove any LBJ_ALLOCATION associated with hAllocation
     */
    for (i = 0; i < pDestroyAllocation->NumAllocations; i++)
    {
        HANDLE CONST hAllocation = pDestroyAllocation->pAllocationList + i;

        KeAcquireSpinLock(&Adapter->AllocationListLock, &oldIrql);
        for (listEntry = listHead->Flink;
             listEntry != listHead;
             listEntry = listNext)
        {
            listNext = listEntry->Flink;
            MyAllocation = CONTAINING_RECORD(listEntry, LJB_ALLOCATION, ListEntry);
            if (MyAllocation->hAllocation == hAllocation)
            {
                RemoveEntryList(listEntry);
                LJB_PROXYKMD_FreePool(MyAllocation);
            }
        }
        KeReleaseSpinLock(&Adapter->AllocationListLock, oldIrql);
    }

}
