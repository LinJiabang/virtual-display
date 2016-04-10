/*
 * ljb_dxgk_get_std_allocation_drv_data.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_GetStdAllocationDrvData)
#endif

CONST CHAR * StdAllocationTypeString[] =
{
    "Unknown",
    "D3DKMDT_STANDARDALLOCATION_SHAREDPRIMARYSURFACE",
    "D3DKMDT_STANDARDALLOCATION_SHADOWSURFACE",
    "D3DKMDT_STANDARDALLOCATION_STAGINGSURFACE",
    "D3DKMDT_STANDARDALLOCATION_GDISURFACE"
};

static
VOID
LJB_DXGK_GetStdAllocationDrvDataPostProcessing(
    __in LJB_ADAPTER *  Adapter,
    __in DXGKARG_GETSTANDARDALLOCATIONDRIVERDATA * pGetStandardAllocationDriverData
    );

/*
 * Function: LJB_DXGK_GetStdAllocationDrvData
 *
 * Description:
 * The DxgkDdiGetStandardAllocationDriverData function returns a description of
 * a standard allocation type.
 *
 * Return Value:
 * DxgkDdiGetStandardAllocationDriverData returns one of the following values:
 *
 *  STATUS_SUCCESS: DxgkDdiGetStandardAllocationDriverData successfully returned
 *  a description of the standard allocation type.
 *
 *  STATUS_NO_MEMORY: DxgkDdiGetStandardAllocationDriverData could not allocate
 *  memory that was required for it to complete.
 *
 * Remarks:
 * Standard allocation types are allocations that must be created in kernel mode
 * without communication from the user-mode display driver. The DirectX graphics
 * kernel subsystem calls the DxgkDdiGetStandardAllocationDriverData function to
 * generate a description of the standard allocation type that the
 * pGetStandardAllocationDriverData parameter specifies. The display miniport
 * driver returns the description of the allocation type in the pAllocationPrivateDriverData
 * and pResourcePrivateDriverData members of the DXGKARG_GETSTANDARDALLOCATIONDRIVERDATA
 * structure that the pGetStandardAllocationDriverData parameter points to. The
 * DirectX graphics kernel subsystem subsequently passes the description to the
 * DxgkDdiCreateAllocation function to actually create the allocation.
 *
 * Beginning with Windows 7, if a display miniport driver processes a call to the
 * DxgkDdiGetStandardAllocationDriverData function to create allocations for GDI
 * hardware acceleration, the driver should set the pitch of the allocation for
 * CPU visible allocations, pGetStandardAllocationDriverData->pCreateGdiSurfaceData->Pitch.
 *
 * DxgkDdiGetStandardAllocationDriverData should be made pageable.
 */
NTSTATUS
LJB_DXGK_GetStdAllocationDrvData(
    _In_    const HANDLE                                  hAdapter,
    _Inout_       DXGKARG_GETSTANDARDALLOCATIONDRIVERDATA *pGetStandardAllocationDriverData
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiGetStandardAllocationDriverData)(
        hAdapter,
        pGetStandardAllocationDriverData
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        return ntStatus;
    }

    LJB_DXGK_GetStdAllocationDrvDataPostProcessing(Adapter, pGetStandardAllocationDriverData);
    return ntStatus;
}

static
VOID
LJB_DXGK_GetStdAllocationDrvDataPostProcessing(
    __in LJB_ADAPTER *  Adapter,
    __in DXGKARG_GETSTANDARDALLOCATIONDRIVERDATA * pGetStandardAllocationDriverData
    )
{
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    LJB_STD_ALLOCATION_INFO *           StdAllocationInfo;
    PVOID                               PrivateDriverData;
    KIRQL                               oldIrql;

    if (pGetStandardAllocationDriverData->pAllocationPrivateDriverData == NULL)
        return;

    StdAllocationInfo = LJB_GetPoolZero(sizeof(LJB_STD_ALLOCATION_INFO) +
        pGetStandardAllocationDriverData->AllocationPrivateDriverDataSize);

    if (StdAllocationInfo == NULL)
    {
        /*
         * nothing you can do.
         */
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__": unable to get StdAllocationInfo\n"));
        return;
    }

    InitializeListHead(&StdAllocationInfo->ListEntry);

    /*
     * be careful on DXGKARG_GETSTANDARDALLOCATIONDRIVERDATA size. This is WDK
     * dependent. Do NOT USE structure assignment.
     */
    StdAllocationInfo->DriverData.StandardAllocationType = pGetStandardAllocationDriverData->StandardAllocationType;
    StdAllocationInfo->DriverData.pCreateSharedPrimarySurfaceData = pGetStandardAllocationDriverData->pCreateSharedPrimarySurfaceData;
    StdAllocationInfo->DriverData.pAllocationPrivateDriverData = pGetStandardAllocationDriverData->pAllocationPrivateDriverData;
    StdAllocationInfo->DriverData.AllocationPrivateDriverDataSize = pGetStandardAllocationDriverData->AllocationPrivateDriverDataSize;

    if (pGetStandardAllocationDriverData->StandardAllocationType == D3DKMDT_STANDARDALLOCATION_SHAREDPRIMARYSURFACE)
    {
        StdAllocationInfo->PrimarySurfaceData = *pGetStandardAllocationDriverData->pCreateSharedPrimarySurfaceData;
    }

    // Not interested in pResourcePrivateDriverData

    if (DriverInitData->Version >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    {
        StdAllocationInfo->DriverData.PhysicalAdapterIndex = pGetStandardAllocationDriverData->PhysicalAdapterIndex;
    }

    // Now copy pAllocationPrivateDriverData to our cache area
    PrivateDriverData = StdAllocationInfo + 1;
    RtlCopyMemory(
        PrivateDriverData,
        pGetStandardAllocationDriverData->pAllocationPrivateDriverData,
        pGetStandardAllocationDriverData->AllocationPrivateDriverDataSize
        );

    KeAcquireSpinLock(&Adapter->StdAllocationInfoListLock, &oldIrql);
    InsertTailList(&Adapter->StdAllocationInfoListHead, &StdAllocationInfo->ListEntry);
    KeReleaseSpinLock(&Adapter->StdAllocationInfoListLock, oldIrql);
    InterlockedIncrement(&Adapter->StdAllocationInfoListCount);

    DBG_PRINT(Adapter, DBGLVL_FLOW,
        (__FUNCTION__": %s pAllocationPrivateDriverData(%p), AllocationPrivateDriverDataSize(%u) tracked\n",
        StdAllocationTypeString[pGetStandardAllocationDriverData->StandardAllocationType],
        pGetStandardAllocationDriverData->pAllocationPrivateDriverData,
        pGetStandardAllocationDriverData->AllocationPrivateDriverDataSize
        ));
}

LJB_STD_ALLOCATION_INFO *
LJB_FindStdAllocationInfo(
    __in LJB_ADAPTER *  Adapter,
    __in PVOID          pAllocationPrivateDriverData,
    __in UINT           AllocationPrivateDriverDataSize
    )
{
    LIST_ENTRY * CONST          ListHead = &Adapter->StdAllocationInfoListHead;
    LIST_ENTRY *                ListEntry;
    LJB_STD_ALLOCATION_INFO *   StdAllocationInfo;
    KIRQL                       oldIrql;

    StdAllocationInfo = NULL;
    KeAcquireSpinLock(&Adapter->StdAllocationInfoListLock, &oldIrql);
    for (ListEntry = ListHead->Flink;
         ListEntry != ListHead;
         ListEntry = ListEntry->Flink)
    {
        LJB_STD_ALLOCATION_INFO *   ThisInfo;
        PVOID                       PrivateDriverData;
        DXGKARG_GETSTANDARDALLOCATIONDRIVERDATA* DriverData;

        ThisInfo = CONTAINING_RECORD(ListEntry, LJB_STD_ALLOCATION_INFO, ListEntry);

        DriverData = &ThisInfo->DriverData;
        if (DriverData->AllocationPrivateDriverDataSize != AllocationPrivateDriverDataSize)
            continue;

        // Match by pointer. Assume the pAllocationPrivateDriverData is maintained
        // by Dxgk runtime, and wouldn't change across DDI call.
        if (DriverData->pAllocationPrivateDriverData == pAllocationPrivateDriverData)
        {
            StdAllocationInfo = ThisInfo;
            break;
        }

        // Match by memory content. This might not work since target driver would
        // change the content of private data during DxgkDdiCreateAllocation.
        // When this happens, we wouldn't find the original StdAllocationDriverData.
        PrivateDriverData = ThisInfo + 1;
        if (RtlEqualMemory(PrivateDriverData, pAllocationPrivateDriverData, AllocationPrivateDriverDataSize))
        {
            StdAllocationInfo = ThisInfo;
            break;
        }
    }
    KeReleaseSpinLock(&Adapter->StdAllocationInfoListLock, oldIrql);

    return StdAllocationInfo;
}