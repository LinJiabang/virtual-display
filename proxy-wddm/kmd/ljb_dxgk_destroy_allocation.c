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

static
VOID
DestroyKmBuffer(
    __in LJB_ADAPTER *      Adapter,
    __in LJB_ALLOCATION *   MyAllocation
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
        HANDLE CONST hAllocation = pDestroyAllocation->pAllocationList[i];

        KeAcquireSpinLock(&Adapter->AllocationListLock, &oldIrql);
        for (listEntry = listHead->Flink;
             listEntry != listHead;
             listEntry = listNext)
        {
            listNext = listEntry->Flink;
            MyAllocation = CONTAINING_RECORD(listEntry, LJB_ALLOCATION, ListEntry);
            if (MyAllocation->hAllocation == hAllocation)
            {
                if (MyAllocation->KmBuffer != NULL)
                {
                    DestroyKmBuffer(Adapter, MyAllocation);
                    MyAllocation->KmBuffer = NULL;
                }

                DBG_PRINT(Adapter, DBGLVL_ALLOCATION,
                    (__FUNCTION__ ": MyAllocation(%p)/hAllocation(%p) released\n",
                    MyAllocation,
                    MyAllocation->hAllocation
                    ));
                RemoveEntryList(listEntry);
                LJB_FreePool(MyAllocation);
            }
        }
        KeReleaseSpinLock(&Adapter->AllocationListLock, oldIrql);
    }
}

static
VOID
DestroyKmBuffer(
    __in LJB_ADAPTER *      Adapter,
    __in LJB_ALLOCATION *   MyAllocation
    )
{
    LJB_STD_ALLOCATION_INFO* CONST  StdAllocationInfo = MyAllocation->StdAllocationInfo;
    UINT                            i;

    if (MyAllocation->StdAllocationInfo == NULL)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__": no StdAllocationInfo in MyAllocation(0x%p)?\n",
            MyAllocation
            ));
        return;
    }

    if (StdAllocationInfo->DriverData.StandardAllocationType != D3DKMDT_STANDARDALLOCATION_SHAREDPRIMARYSURFACE)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__": not SharedPrimarySurface?\n"));
        return;
    }

    /*
     * For each USB target associated with MyAllocation->VidPnSourceId in the
     * current topology, send LJB_GENERIC_IOCTL_DESTROY_PRIMARY_SURFACE
     */
    for (i = 0; i < Adapter->NumPathsCommitted; i++)
    {
        D3DKMDT_VIDPN_PRESENT_PATH* CONST   Path = &Adapter->PathsCommitted[i];
        LJB_MONITOR_NODE *                  MonitorNode;

        if (Path->VidPnTargetId < Adapter->UsbTargetIdBase)
            continue;

        if (Path->VidPnSourceId != MyAllocation->VidPnSourceId)
            continue;

        MonitorNode = LJB_GetMonitorNodeFromChildUid(Adapter, Path->VidPnTargetId);
        if (MonitorNode->MonitorInterface.pfnGenericIoctl != NULL &&
            MonitorNode->MonitorInterface.Context != NULL)
        {
            LJB_MONITOR_INTERFACE* CONST MonitorInterface = &MonitorNode->MonitorInterface;
            D3DKMDT_SHAREDPRIMARYSURFACEDATA * CONST PrimarySurfaceData = &StdAllocationInfo->PrimarySurfaceData;
            LJB_PRIMARY_SURFACE LjbPrimarySurface;
            NTSTATUS myStatus;

            /*
             * send LJB_GENERIC_IOCTL_DESTROY_PRIMARY_SURFACE to monitor driver
             */
            RtlZeroMemory(&LjbPrimarySurface, sizeof(LjbPrimarySurface));
            LjbPrimarySurface.Width = PrimarySurfaceData->Width;
            LjbPrimarySurface.Height = PrimarySurfaceData->Height;
            LjbPrimarySurface.BitPerPixel = 32;
            LjbPrimarySurface.BufferSize = MyAllocation->KmBufferSize;
            LjbPrimarySurface.RemoteBuffer = MyAllocation->KmBuffer;
            LjbPrimarySurface.SurfaceHandle = MyAllocation;

            DBG_PRINT(Adapter, DBGLVL_FLOW,
                (__FUNCTION__
                ": Send LJB_GENERIC_IOCTL_DESTROY_PRIMARY_SURFACE UsbTargetId(0x%x)/Width(%u)/Height(%u)\n",
                Path->VidPnTargetId,
                LjbPrimarySurface.Width,
                LjbPrimarySurface.Height
                ));
            myStatus = (*MonitorInterface->pfnGenericIoctl)(
                MonitorInterface->Context,
                LJB_GENERIC_IOCTL_DESTROY_PRIMARY_SURFACE,
                &LjbPrimarySurface,
                sizeof(LjbPrimarySurface),
                NULL,
                0,
                NULL
                );
            if (!NT_SUCCESS(myStatus))
            {
                DBG_PRINT(Adapter, DBGLVL_ERROR,
                    ("?" __FUNCTION__": failed with ntStatus(0x%x)?\n",
                    myStatus));
            }
        }
        LJB_DereferenceMonitorNode(MonitorNode);
    }

    LJB_FreePool(MyAllocation->KmBuffer);
    MyAllocation->KmBuffer = NULL;
}