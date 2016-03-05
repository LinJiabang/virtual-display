/*
 * ljb_dxgk_query_child_status.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_QueryChildStatus)
#endif

static
NTSTATUS
LJB_DXGK_QueryChildStatusForUsbMonitor(
    _In_    LJB_ADAPTER *       Adapter,
    _Inout_ PDXGK_CHILD_STATUS  ChildStatus
    );

/*
 * Function: LJB_DXGK_QueryChildStatus
 *
 * Description:
 * The DxgkDdiQueryChildStatus function returns the status of an individual child
 * device of a display adapter.
 *
 * Return value
 * DxgkDdiQueryChildStatus returns STATUS_SUCCESS if it succeeds; otherwise, it
 * returns one of the error codes defined in Ntstatus.h.
 *
 * Remarks
 * During initialization, the display port driver calls DxgkDdiQueryChildRelations
 * to get a list of devices that are children of the display adapter represented
 * by MiniportDeviceContext. Then for each child that has an HPD awareness value
 * of HpdAwarenessPolled or HpdAwarenessInterruptible, the display port driver
 * calls DxgkDdiQueryChildStatus to determine whether the child currently has
 * hardware (for example a monitor) connected to it.
 *
 * DxgkDdiQueryChildStatus must perform the following actions:
 * If ChildStatus->Type is equal to StatusConnection, return a Boolean value in
 * ChildStatus->HotPlug.Connected. Return TRUE if the child device identified by
 * ChildStatus->ChildUid has external hardware connected to it; otherwise return
 * FALSE.
 *
 * If ChildStatus->Type is equal to StatusRotation, return (in ChildStatus->Rotation.Angle)
 * the angle of rotation for the display connected to the child device identified
 * by ChildStatus->ChildUid.
 *
 * DxgkDdiQueryChildStatus should be made pageable.
 */
NTSTATUS
LJB_DXGK_QueryChildStatus(
    _In_    const PVOID              MiniportDeviceContext,
    _Inout_       PDXGK_CHILD_STATUS ChildStatus,
    _In_          BOOLEAN            NonDestructiveOnly
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(MiniportDeviceContext);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    /*
     * if the runtime is query our child status, don't pass it to inbox driver
     */
    if (ChildStatus->ChildUid >= Adapter->UsbTargetIdBase)
    {
        return LJB_DXGK_QueryChildStatusForUsbMonitor(
            Adapter,
            ChildStatus
            );
    }

    /*
     * pass the call to inbox driver
     */
    ntStatus = (*DriverInitData->DxgkDdiQueryChildStatus)(
        MiniportDeviceContext,
        ChildStatus,
        NonDestructiveOnly
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        return ntStatus;
    }

    /*
     * update Adapter->ChildConnectionStatus
     */
    if (ChildStatus->Type == StatusConnection)
    {
        UINT    i;

        for (i = 0; i < Adapter->ActualNumberOfChildren; i++)
        {
            PDXGK_CHILD_DESCRIPTOR CONST ChildDescriptor = Adapter->ChildRelations + i;

            if (ChildDescriptor->ChildUid != ChildStatus->ChildUid)
                continue;

            Adapter->ChildConnectionStatus[i] = ChildStatus->HotPlug.Connected;
            DBG_PRINT(Adapter, DBGLVL_FLOW,
                (__FUNCTION__ ": Child[%u]/ChildUid(0x%x)/Connected(%u)\n",
                i,
                ChildStatus->ChildUid,
                ChildStatus->HotPlug.Connected
                ));
            break;
        }
    }

    return ntStatus;
}

static
NTSTATUS
LJB_DXGK_QueryChildStatusForUsbMonitor(
    _In_    LJB_ADAPTER *       Adapter,
    _Inout_ PDXGK_CHILD_STATUS  ChildStatus
    )
{
    /*
     * NOT YET IMPLEMENTED
     */
    UNREFERENCED_PARAMETER(Adapter);
    UNREFERENCED_PARAMETER(ChildStatus);

    return STATUS_SUCCESS;
}