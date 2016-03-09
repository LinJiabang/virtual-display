/*
 * ljb_dxgk_create_allocation.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_CreateAllocation)
#endif

static VOID
LJB_DXGK_CreateAllocationPostProcessing(
    _In_    const HANDLE                   hAdapter,
    _Inout_       DXGKARG_CREATEALLOCATION *pCreateAllocation
    );

/*
 * Function: LJB_DXGK_CreateAllocation
 *
 * Description:
 * The DxgkDdiCreateAllocation function creates allocations of system or video memory.
 *
 * Return value
 * DxgkDdiCreateAllocation returns one of the following values:
 *
 * STATUS_SUCCESS
 *  DxgkDdiCreateAllocation successfully created the allocation.
 *
 * STATUS_INVALID_PARAMETER
 *  Parameters that were passed to DxgkDdiCreateAllocation contained errors that
 *  prevented it from completing.
 *
 * STATUS_NO_MEMORY
 *  DxgkDdiCreateAllocation could not allocate memory that was required for it
 *  to complete.
 *
 * STATUS_GRAPHICS_DRIVER_MISMATCH
 *  The display miniport driver is not compatible with the user-mode display
 *  driver that initiated the call to DxgkDdiCreateAllocation.
 *
 * Remarks
 * After the user-mode display driver calls the pfnAllocateCb function, the
 * DirectX graphics kernel subsystem calls the display miniport driver's
 * DxgkDdiCreateAllocation function to create the allocations. The display
 * miniport driver must interpret the private data (in the pPrivateDriverData
 * member of the DXGK_ALLOCATIONINFO structure for each allocation) that is passed
 * from the user-mode display driver and must determine the list of parameters
 * that are required to create each allocation through the video memory manager.
 * For more information about how the display miniport driver supplies these
 * parameters to the video memory manager, see Specifying Segments When Creating
 * Allocations.
 *
 * The display miniport driver must return an allocation handle, which is typically
 * a pointer to a private driver data structure that contains information about
 * the allocation. The display miniport driver can call the DxgkCbGetHandleData
 * function anytime to retrieve the private data for an allocation handle. Therefore,
 * the display miniport driver is not required to maintain a private allocation
 * handle table. In fact, we strongly discourage private handle tables because
 * they could become stale or out of sync with the DirectX graphics kernel subsystem
 * in display-mode-switch scenarios such as fast user switch, hot unplug, and so
 * on.
 *
 * The user-mode display driver assigns an allocation to either a resource or a
 * device. To determine whether the allocation belongs to a resource, the display
 * miniport driver can check whether the Resource bit-field flag is set in the
 * Flags member of the DXGKARG_CREATEALLOCATION structure that the pCreateAllocation
 * parameter of DxgkDdiCreateAllocation points to. If the allocation belongs to
 * a resource, the display miniport driver can (but is not required to) return a
 * resource handle, which is typically a pointer to a private driver data structure
 * that describes the resource. If the DirectX graphics kernel subsystem calls
 * DxgkDdiCreateAllocation to create an additional allocation for an existing
 * resource, the hResource member of DXGKARG_CREATEALLOCATION contains the handle
 * that was returned by the previous DxgkDdiCreateAllocation call for that
 * resource. If necessary, the display miniport driver can change the resource
 * handle during a call to DxgkDdiCreateAllocation.
 *
 * If the user-mode display driver places a resource handle in a command buffer,
 * the display miniport driver can retrieve the private data by calling
 * DxgkCbGetHandleData. The display miniport driver can also enumerate all of the
 * resource's child allocations by calling the DxgkCbEnumHandleChildren function.
 * Beginning with Windows 7, if a display miniport driver processes a call to the
 * DxgkDdiCreateAllocation function to create allocations for GDI hardware
 * acceleration, the driver should set the size of the allocation (including the
 * pitch value for CPU visible allocations), pCreateAllocation->pAllocationInfo->Size.
 *
 * The resources that are created in the DxgkDdiCreateAllocation call belong to
 * the adapter and not to the device. The display miniport driver should not
 * reference the device data anywhere within the private allocation and resource
 * data structures. Because of surface sharing, a resource might be in use after
 * the destruction of the device that the user-mode display driver created the
 * resource from.
 *
 * DxgkDdiCreateAllocation should be made pageable.
 *
 * Allocating history buffers
 * Starting in Windows 8.1, when DxgkDdiCreateAllocation is called, the display
 * miniport driver can set the DXGK_ALLOCATIONINFOFLAGS.HistoryBuffer member to
 * indicate that the user-mode driver can manage the creation and destruction of
 * history buffers.
 */
NTSTATUS
LJB_DXGK_CreateAllocation(
    _In_    const HANDLE                   hAdapter,
    _Inout_       DXGKARG_CREATEALLOCATION *pCreateAllocation
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    /*
     * pass the call to inbox driver
     */
    ntStatus = (*DriverInitData->DxgkDdiCreateAllocation)(hAdapter, pCreateAllocation);
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        return ntStatus;
    }

    LJB_DXGK_CreateAllocationPostProcessing(hAdapter, pCreateAllocation);

    return ntStatus;
}

static VOID
LJB_DXGK_CreateAllocationPostProcessing(
    _In_    const HANDLE                   hAdapter,
    _Inout_       DXGKARG_CREATEALLOCATION *pCreateAllocation
    )
{
    LJB_ADAPTER * CONST Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    KIRQL               oldIrql;
    UINT                i;

    for (i = 0; i < pCreateAllocation->NumAllocations; i++)
    {
        DXGK_ALLOCATIONINFO * CONST AllocationInfo = pCreateAllocation->pAllocationInfo;
        LJB_ALLOCATION *            MyAllocation;

        MyAllocation = LJB_PROXYKMD_GetPoolZero(sizeof(LJB_ALLOCATION));
        if (MyAllocation == NULL)
        {
            DBG_PRINT(Adapter, DBGLVL_ERROR,
                ("?" __FUNCTION__": unable to allocat MyAllocation?\n"));
            break;
        }

        MyAllocation->Adapter = Adapter;
        MyAllocation->hAllocation = AllocationInfo->hAllocation;
        MyAllocation->AllocationInfo = *AllocationInfo;
        InitializeListHead(&MyAllocation->ListEntry);

        KeAcquireSpinLock(&Adapter->AllocationListLock, &oldIrql);
        InsertTailList(&Adapter->AllocationListHead, &MyAllocation->ListEntry);
        KeReleaseSpinLock(&Adapter->AllocationListLock, oldIrql);

        DBG_PRINT(Adapter, DBGLVL_ALLOCATION,
            (__FUNCTION__ ": MyAllocation(%p)/hAllocation(%p) allocated\n",
            MyAllocation,
            MyAllocation->hAllocation
            ));
    }
}

LJB_ALLOCATION *
LJB_DXGK_FindAllocation(
    __in LJB_ADAPTER*   Adapter,
    __in HANDLE         hAllocation
    )
{
    LIST_ENTRY * CONST  listHead = &Adapter->AllocationListHead;
    LIST_ENTRY *        listEntry;
    LJB_ALLOCATION *    MyAllocation;
    KIRQL               oldIrql;

    MyAllocation = NULL;
    KeAcquireSpinLock(&Adapter->AllocationListLock, &oldIrql);
    for (listEntry = listHead->Flink;
         listEntry != listHead;
         listEntry = listEntry->Flink)
    {
        LJB_ALLOCATION * thisAllocation;

        thisAllocation = CONTAINING_RECORD(listEntry, LJB_ALLOCATION, ListEntry);
        if (thisAllocation->hAllocation == hAllocation)
        {
            MyAllocation = thisAllocation;
            break;
        }
    }
    KeReleaseSpinLock(&Adapter->AllocationListLock, oldIrql);

    if (MyAllocation == NULL)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": no allocation found for hAllocation(%p)?\n",
            hAllocation));
    }
    return MyAllocation;
}