/*
 * ljb_dxgk_open_allocation.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_OpenAllocation)
#endif

static VOID
LJB_DXGK_OpenAllocationPostProcessing(
    __in LJB_ADAPTER *                  Adapter,
    __in const DXGKARG_OPENALLOCATION * pOpenAllocation
    );

/*
 * Function: LJB_DXGK_OpenAllocation
 *
 * Description:
 * The DxgkDdiOpenAllocation function binds non-device-specific allocations that
 * the DxgkDdiCreateAllocation function created to allocations that are specific
 * to the specified graphics context device.
 *
 * Return Value:
 * DxgkDdiOpenAllocation returns one of the following values:
 *
 *  STATUS_SUCCESS: DxgkDdiOpenAllocation successfully bound allocations to the
 *  graphics context device that the hDevice parameter specified.
 *
 *  STATUS_INVALID_PARAMETER: Parameters that were passed to DxgkDdiOpenAllocation
 *  contained errors that prevented it from completing.
 *
 *  STATUS_NO_MEMORY: DxgkDdiOpenAllocation could not allocate memory that was
 *  required for it to complete.
 *
 *  STATUS_GRAPHICS_DRIVER_MISMATCH: The display miniport driver is not compatible
 *  with the user-mode display driver that initiated the call to DxgkDdiOpenAllocation
 *  (that is, supplied private data to the display miniport driver).
 *
 * Remarks:
 * The DirectX graphics kernel subsystem calls the display miniport driver's
 * DxgkDdiOpenAllocation function to bind nondevice-specific allocations that the
 * DxgkDdiCreateAllocation function created to allocations that are specific to
 * the graphics context device that the hDevice parameter specifies. The display
 * miniport driver binds allocations to a device so the driver can keep track of
 * allocation data that is specific to a device.
 *
 * The display miniport driver can bind an allocation to any device that any process
 * (on the same graphics adapter) created and not just to a device in the creating
 * process.
 *
 * When DxgkDdiOpenAllocation returns STATUS_SUCCESS, the driver sets the
 * hDeviceSpecificAllocation member of the DXGK_OPENALLOCATIONINFO structure for
 * each allocation to a non-NULL value. The DXGK_OPENALLOCATIONINFO structure for
 * each allocation is an element of the array that the pOpenAllocation member of
 * the DXGKARG_OPENALLOCATION structure specifies.
 *
 * The driver can modify the allocation private driver data that is passed in the
 * pPrivateDriverData member of the DXGK_OPENALLOCATIONINFO structure only when
 * the allocation is created (which is indicated when the Create bit-field flag
 * in the Flags member of the DXGKARG_OPENALLOCATION structure is set). The driver
 * should determine that it can only read the allocation private driver data when
 * the allocation is opened (that is, when the Create bit-field flag is not set).
 *
 * DxgkDdiOpenAllocation should be made pageable.
 */
NTSTATUS
LJB_DXGK_OpenAllocation(
    _In_ const HANDLE                 hDevice,
    _In_ const DXGKARG_OPENALLOCATION *pOpenAllocation
    )
{
    LJB_DEVICE * CONST                  MyDevice = LJB_DXGK_FindDevice(hDevice);
    LJB_ADAPTER * CONST                 Adapter = MyDevice->Adapter;
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiOpenAllocation)(hDevice, pOpenAllocation);
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        return ntStatus;
    }

    LJB_DXGK_OpenAllocationPostProcessing(Adapter, pOpenAllocation);
    return ntStatus;
}

static VOID
LJB_DXGK_OpenAllocationPostProcessing(
    __in LJB_ADAPTER *                  Adapter,
    __in const DXGKARG_OPENALLOCATION * pOpenAllocation
    )
{
    DXGKRNL_INTERFACE * CONST   DxgkInterface = &Adapter->DxgkInterface;
    UINT                        i;

    for (i = 0; i < pOpenAllocation->NumAllocations; i++)
    {
        DXGK_OPENALLOCATIONINFO * CONST OpenAllocationInfo = pOpenAllocation->pOpenAllocation + i;
        LJB_OPENED_ALLOCATION * OpenedAllocation;
        DXGKARGCB_GETHANDLEDATA GetHandleData;
        PVOID                   hAllocation;
        KIRQL                   oldIrql;

        RtlZeroMemory(&GetHandleData, sizeof(GetHandleData));
        GetHandleData.hObject = OpenAllocationInfo->hAllocation;
        GetHandleData.Type = DXGK_HANDLE_ALLOCATION;
        hAllocation = LJB_AcquireHandleData(DxgkInterface, &GetHandleData);

        if (hAllocation == NULL)
        {
            DBG_PRINT(Adapter, DBGLVL_ERROR,
                ("?"__FUNCTION__": LJB_AcquireHandleData failed?\n"));
            break;
        }

        OpenedAllocation = LJB_GetPoolZero(sizeof(LJB_OPENED_ALLOCATION));
        if (OpenedAllocation == NULL)
        {
            DBG_PRINT(Adapter, DBGLVL_ERROR,
                ("?" __FUNCTION__ ": failed to create OpenedAllocation\n"));
            break;
        }

        OpenedAllocation->hKmHandle = OpenAllocationInfo->hAllocation;
        OpenedAllocation->hDeviceSpecificAllocation = OpenAllocationInfo->hDeviceSpecificAllocation;
        OpenedAllocation->hAllocation = hAllocation;
        OpenedAllocation->MyAllocation = LJB_DXGK_FindAllocation(Adapter, hAllocation);
        InitializeListHead(&OpenedAllocation->ListEntry);

        KeAcquireSpinLock(&Adapter->OpenedAllocationListLock, &oldIrql);
        InsertTailList(&Adapter->OpenedAllocationListHead, &OpenedAllocation->ListEntry);
        KeReleaseSpinLock(&Adapter->OpenedAllocationListLock, oldIrql);
        InterlockedIncrement(&Adapter->OpenedAllocationListCount);
    }
}

LJB_OPENED_ALLOCATION *
LJB_DXGK_FindOpenedAllocation(
    __in LJB_ADAPTER*   Adapter,
    __in HANDLE         hDeviceSpecificAllocation
    )
{
    LIST_ENTRY * CONST      listHead = &Adapter->OpenedAllocationListHead;
    LIST_ENTRY *            listEntry;
    LJB_OPENED_ALLOCATION * OpenedAllocation;
    KIRQL                   oldIrql;

    OpenedAllocation = NULL;
    KeAcquireSpinLock(&Adapter->OpenedAllocationListLock, &oldIrql);
    for (listEntry = listHead->Flink;
         listEntry != listHead;
         listEntry = listEntry->Flink)
    {
        LJB_OPENED_ALLOCATION * thisAllocation;

        thisAllocation = CONTAINING_RECORD(listEntry, LJB_OPENED_ALLOCATION, ListEntry);
        if (thisAllocation->hDeviceSpecificAllocation == hDeviceSpecificAllocation)
        {
            OpenedAllocation = thisAllocation;
            break;
        }
    }
    KeReleaseSpinLock(&Adapter->OpenedAllocationListLock, oldIrql);

    if (OpenedAllocation == NULL)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__
            ": no OpenedAllocation found for hDeviceSpecificAllocation(%p)?\n",
            hDeviceSpecificAllocation));
    }
    return OpenedAllocation;
}

PVOID
LJB_AcquireHandleData(
    __in DXGKRNL_INTERFACE *            DxgkInterface,
    IN_CONST_PDXGKARGCB_GETHANDLEDATA   GetHandleData
    )
{
    PVOID   hAllocation;

    /*
     * Started from Win10, a new set of API (DxgkCbAcquireHandleData/DxgkCbReleaseHandleData)
     * is introduced. We first try old DxgkCbGetHandleData. If it does not work,
     * we try new API
     */
    hAllocation = (*DxgkInterface->DxgkCbGetHandleData)(GetHandleData);

    if (hAllocation == NULL)
    {
        if (DxgkInterface->Version >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
        {
            DXGKARG_RELEASE_HANDLE      ReleaseHandle;
            DXGKARGCB_RELEASEHANDLEDATA ReleaseHandleData;

            hAllocation = (*DxgkInterface->DxgkCbAcquireHandleData)(GetHandleData, &ReleaseHandle);

            if (hAllocation != NULL)
            {
                RtlZeroMemory(&ReleaseHandleData, sizeof(ReleaseHandleData));
                ReleaseHandleData.ReleaseHandle = ReleaseHandle;
                ReleaseHandleData.Type = GetHandleData->Type;
                (*DxgkInterface->DxgkCbReleaseHandleData)(ReleaseHandleData);
            }
        }
    }

    return hAllocation;
}