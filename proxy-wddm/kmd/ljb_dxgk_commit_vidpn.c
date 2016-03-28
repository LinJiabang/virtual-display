/*
 * ljb_dxgk_commit_vidpn.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"
#include "ljb_dxgk_vidpn_interface.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_CommitVidPn)
#pragma alloc_text (PAGE, LJB_DXGK_UpdateActiveVidPnPresentPath)
#endif

CONST CHAR * MonitorConnectivityCheckString[] =
{
    "D3DKMDT_MCC_UNINITIALIZED",
    "D3DKMDT_MCC_IGNORE",
    "D3DKMDT_MCC_ENFORCE",
};

static
VOID
LJB_NotifyCommitVidPnToSingleTarget(
    __in LJB_ADAPTER *                      Adapter,
    __in PVOID                              hPrimaryAllocation,
    __in CONST D3DKMDT_VIDPN_PRESENT_PATH*  Path
    );

static
VOID
LJB_NotifyCommitVidPnToAllUsbTargets(
    __in LJB_ADAPTER *  Adapter,
    __in PVOID          hPrimaryAllocation
    );

/*
 * Function: LJB_DXGK_CommitVidPn
 *
 * Description:
 * The DxgkDdiCommitVidPn function makes a specified video present network (VidPN)
 * active on a display adapter.
 *
 * Return Value:
 * DxgkDdiCommitVidPn returns one of the values in the following list. The VidPN
 * referred to in the list is the VidPN represented by pCommitVidPnArg->hFunctionalVidPn.
 *
 *  STATUS_SUCCESS: The driver successfully handled the function call.
 *
 *  STATUS_GRAPHICS_GAMMA_RAMP_NOT_SUPPORTED: The display adapter does not support
 *  all of the path gamma ramps in the VidPN.
 *
 *  STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY: The topology of the VidPN is invalid.
 *  In particular, DxgkDdiCommitVidPn must return this value if pCommitVidPnArg->
 *  MonitorConnectivityChecks is equal to D3DKMDT_MCC_ENFORCE and one of the video
 *  outputs in the new VidPN's topology does not have a monitor connected.
 *
 *  STATUS_GRAPHICS_PATH_CONTENT_GEOMETRY_TRANSFORMATION_NOT_SUPPORTED:
 *  One of the present paths in the topology does not support the specified content
 *  transformation.
 *
 *  STATUS_GRAPHICS_VIDPN_MODALITY_NOT_SUPPORTED:  The display adapter does not
 *  currently support the set of modes that are pinned in the VidPN.
 *
 *  STATUS_NO_MEMORY: The driver could not complete this request because of
 *  insufficient memory.
 *
 * Remarks:
 * For more information about how the display miniport driver should handle calls
 * to DxgkDdiCommitVidPn, see DXGKARG_COMMITVIDPN.
 *
 * Beginning with Windows 8, if the display miniport driver sets the
 * SupportSmoothRotation member of the DXGK_DRIVERCAPS structure, it must support
 * updating the path rotation on the adapter using the DxgkDdiUpdateActiveVidPnPresentPath
 * function. The driver must always be able to set the path rotation during a call
 * to the DxgkDdiCommitVidPn function.
 */
NTSTATUS
LJB_DXGK_CommitVidPn(
    IN_CONST_HANDLE                         hAdapter,
    IN_CONST_PDXGKARG_COMMITVIDPN_CONST     pCommitVidPn
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    DXGKARG_COMMITVIDPN                 MyCommitVidpn;
    LJB_VIDPN *                         MyVidPn;
    NTSTATUS                            ntStatus;
    UINT                                NumOfInboxTarget;
    UINT                                NumOfUsbTarget;
    BOOLEAN                             SourceIsConnectedToInboxTarget;
    BOOLEAN                             SourceIsConnectedToUsbTarget;
    UINT                                i;

    PAGED_CODE();

    DBG_PRINT(Adapter, DBGLVL_FLOW,
        (__FUNCTION__
        ": hFunctionalVidPn(%p),AffectedVidPnSourceId(0x%x),MonitorConnectivityChecks(%s),"
        "hPrimaryAllocation(%p),PathPowerTransition(%u),PathPoweredOff(%u)\n",
        pCommitVidPn->hFunctionalVidPn,
        pCommitVidPn->AffectedVidPnSourceId,
        MonitorConnectivityCheckString[pCommitVidPn->MonitorConnectivityChecks],
        pCommitVidPn->hPrimaryAllocation,
        pCommitVidPn->Flags.PathPowerTransition,
        pCommitVidPn->Flags.PathPoweredOff
        ));

    Adapter->LastCommitVidPn = *pCommitVidPn;
    MyCommitVidpn = *pCommitVidPn;
    if (!Adapter->FirstVidPnArrived)
    {
        Adapter->FirstVidPnArrived = TRUE;
    }

    RtlZeroMemory(Adapter->PathsCommitted, sizeof(Adapter->PathsCommitted));
    Adapter->NumPathsCommitted = 0;

    /*
     * If pCommitVidPn->hFunctionalVidPn is NULL, it is probably the system
     * is going to S3/S4 state
     */
    if (pCommitVidPn->hFunctionalVidPn == NULL)
    {
        /*
         * Check AffectedVidPnSourceId.
         * The constant D3DDDI_ID_ALL or the identifier of a particular video present
         * source in the VidPN. If this member is a source identifier, DxgkDdiCommitVidPn
         * updates only the modes of the video present paths that originate at that
         * source -- DxgkDdiCommitVidPn does not have to inspect paths that originate
         * from other sources, because those paths are the same in the new VidPN
         * as they are in the currently active VidPN. If this member is equal to
         * D3DDDI_ID_ALL, DxgkDdiCommitVidPn must inspect and update the entire
         * VidPN as a single transaction; that is, the entire new VidPN must be
         * made active or the entire current VidPN must remain active.
         *
         * Since hFunctionalVidPn is NULL, we have no way to check if the AffectedVidPnSourceId
         * connectivity.
         */
        ntStatus = (*DriverInitData->DxgkDdiCommitVidPn)(
            hAdapter,
            pCommitVidPn
            );
        if (!NT_SUCCESS(ntStatus))
        {
            DBG_PRINT(Adapter, DBGLVL_ERROR,
                ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        }
        return ntStatus;
    }

    /*
     * now the hFunctionalVidPn is valid, we further check AffectedVidPnSourceId.
     * If the AffectedVidPnSourceId is connected ONLY to USB target (such as extended
     * desktop mode), we don't want inbox driver to see this request.
     * If AffectedVidPnSourceId is connected to both USB and inbox target,
     * we need inbox driver to handle it as well.
     * If AffectedVidPnSourceId isn't connected to USB target, just let inbox
     * driver handle it.
     * If AffectedVidPnSourceId is D3DDDI_ID_ALL, check if there is inbox target or not.
     */
    MyVidPn = LJB_VIDPN_CreateVidPn(Adapter, pCommitVidPn->hFunctionalVidPn);
    if (MyVidPn == NULL)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?"__FUNCTION__": no MyVidPn allocated.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /*
     * Copy Paths from VidPn topology to PathsCommitted
     */
    Adapter->NumPathsCommitted = MyVidPn->NumPaths;
    for (i = 0; i < MyVidPn->NumPaths; i++)
    {
        Adapter->PathsCommitted[i] = MyVidPn->Paths[i];
    }

    if (pCommitVidPn->AffectedVidPnSourceId == D3DDDI_ID_ALL)
    {
        NumOfInboxTarget = LJB_VIDPN_GetNumberOfInboxTarget(MyVidPn);
        NumOfUsbTarget = LJB_VIDPN_GetNumberOfUsbTarget(MyVidPn);
        DBG_PRINT(Adapter, DBGLVL_FLOW,
            (__FUNCTION__": NumOfInboxTarget(%u), NumOfUsbTarget(%u)\n",
            NumOfInboxTarget, NumOfUsbTarget));

        if (NumOfInboxTarget != 0)
        {
            MyCommitVidpn.hFunctionalVidPn = (D3DKMDT_HVIDPN) MyVidPn;
            ntStatus = (*DriverInitData->DxgkDdiCommitVidPn)(
                hAdapter,
                &MyCommitVidpn
                );
            if (!NT_SUCCESS(ntStatus))
            {
                DBG_PRINT(Adapter, DBGLVL_ERROR,
                    ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
                goto Exit;
            }
        }

        if (NumOfUsbTarget != 0)
        {
            LJB_NotifyCommitVidPnToAllUsbTargets(Adapter, pCommitVidPn->hPrimaryAllocation);
        }
        ntStatus = STATUS_SUCCESS;
        goto Exit;
    }

    /*
     * AffectedVidPnSourceId != D3DDDI_ID_ALL
     */
    SourceIsConnectedToInboxTarget = LJB_DXGK_IsSourceConnectedToInboxTarget(
        Adapter,
        pCommitVidPn->AffectedVidPnSourceId
        );
    SourceIsConnectedToUsbTarget = LJB_DXGK_IsSourceConnectedToUsbTarget(
        Adapter,
        pCommitVidPn->AffectedVidPnSourceId
        );

    DBG_PRINT(Adapter, DBGLVL_FLOW,
        (__FUNCTION__": SourceIsConnectedToInboxTarget(%u),SourceIsConnectedToUsbTarget(%u)\n",
        SourceIsConnectedToInboxTarget,
        SourceIsConnectedToUsbTarget
        ));

    /*
     * If the affected Source is not connected to inbox target, don't let inbox driver
     * see it.
     * FIXME: need to take care of topology transition: from extended view to
     * 2nd screen only view??
     */
    ntStatus = STATUS_SUCCESS;
    if (SourceIsConnectedToInboxTarget)
    {
        MyCommitVidpn.hFunctionalVidPn = (D3DKMDT_HVIDPN) MyVidPn;
        ntStatus = (*DriverInitData->DxgkDdiCommitVidPn)(
            hAdapter,
            &MyCommitVidpn
            );
        if (!NT_SUCCESS(ntStatus))
        {
            DBG_PRINT(Adapter, DBGLVL_ERROR,
                ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
            goto Exit;
        }
    }

    if (SourceIsConnectedToUsbTarget)
    {
        LJB_NotifyCommitVidPnToAllUsbTargets(Adapter, pCommitVidPn->hPrimaryAllocation);
    }

Exit:
    LJB_VIDPN_DestroyVidPn(MyVidPn);
    return ntStatus;
}

BOOLEAN
LJB_DXGK_IsSourceConnectedToInboxTarget(
    __in LJB_ADAPTER *                  Adapter,
    __in D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId
    )
{
    UINT    i;

    for (i = 0; i < Adapter->NumPathsCommitted; i++)
    {
        D3DKMDT_VIDPN_PRESENT_PATH * Path = Adapter->PathsCommitted + i;

        if (Path->VidPnSourceId != VidPnSourceId)
            continue;

        if (Path->VidPnSourceId < Adapter->UsbTargetIdBase)
            return TRUE;
    }

    return FALSE;
}

BOOLEAN
LJB_DXGK_IsSourceConnectedToUsbTarget(
    __in LJB_ADAPTER *                  Adapter,
    __in D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId
    )
{
    UINT    i;

    for (i = 0; i < Adapter->NumPathsCommitted; i++)
    {
        D3DKMDT_VIDPN_PRESENT_PATH * Path = Adapter->PathsCommitted + i;

        if (Path->VidPnSourceId != VidPnSourceId)
            continue;

        if (Path->VidPnSourceId >= Adapter->UsbTargetIdBase)
            return TRUE;
    }

    return FALSE;
}

/*
 * Function: LJB_DXGK_UpdateActiveVidPnPresentPath
 *
 * Description:
 * The DxgkDdiUpdateActiveVidPnPresentPath function updates one of the video
 * present paths that is currently active on the display adapter.
 *
 * Return Value:
 * DxgkDdiUpdateActiveVidPnPresentPathreturns one of the following values:
 *
 *  STATUS_SUCCESS: The function succeeded.
 *
 *  STATUS_GRAPHICS_PATH_NOT_IN_TOPOLOGY: The path specified by
 *  pUpdateActiveVidPnPresentPathArg->VidPnPresentPathInfo is not in the topology
 *  of the active VidPN.
 *
 *  STATUS_GRAPHICS_PATH_CONTENT_GEOMETRY_TRANSFORMATION_NOT_SUPPORTED:
 *  The path does not support the content transformation specified by
 *  pUpdateActiveVidPnPresentPathArg->VidPnPresentPathInfo.ContentTransformation.
 *
 *  The path does not support the gamma ramp specified by pUpdateActiveVidPnPresentPathArg->
 *  VidPnPresentPathInfo.GammaRamp.
 *
 * Remarks:
 * The operating system calls the DxgkDdiUpdateActiveVidPnPresentPath function to
 * control the settings of video present paths, such as path rotation, a presented
 * content's geometry transformations, gamma ramps that are used to adjust the
 * presented content's brightness, and so on.
 *
 * Note   The display miniport driver's DxgkDdiUpdateActiveVidPnPresentPath
 * function must support gamma ramps.
 *
 * Beginning with Windows 8, if the display miniport driver sets the SupportSmoothRotation
 * member of the DXGK_DRIVERCAPS structure, it must support updating the path rotation
 * on the adapter using the DxgkDdiUpdateActiveVidPnPresentPath function. The
 * driver must always be able to set the path rotation during a call to the
 * DxgkDdiCommitVidPn function.
 *
 * The DxgkDdiUpdateActiveVidPnPresentPath function should be made pageable.
 */
NTSTATUS
LJB_DXGK_UpdateActiveVidPnPresentPath(
    IN_CONST_HANDLE                                         hAdapter,
    IN_CONST_PDXGKARG_UPDATEACTIVEVIDPNPRESENTPATH_CONST    pUpdateActiveVidPnPresentPath
    )
{
    LJB_ADAPTER * CONST                     Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST          ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST      DriverInitData = &ClientDriverData->DriverInitData;
    CONST D3DKMDT_VIDPN_PRESENT_PATH* CONST Path = &pUpdateActiveVidPnPresentPath->VidPnPresentPathInfo;
    NTSTATUS                                ntStatus;

    PAGED_CODE();

    if (Path->VidPnTargetId < Adapter->UsbTargetIdBase)
    {
        /*
         * the target is to inbox monitor
         */
        ntStatus = (*DriverInitData->DxgkDdiUpdateActiveVidPnPresentPath)(
            hAdapter,
            pUpdateActiveVidPnPresentPath
            );
        if (!NT_SUCCESS(ntStatus))
        {
            DBG_PRINT(Adapter, DBGLVL_ERROR,
                ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        }
    }
    else
    {
        /*
         * targeting at USB monitor
         */
        DBG_PRINT(Adapter, DBGLVL_FLOW,
            (__FUNCTION__": FIXME, send notification to USB driver\n"));
        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
}

static
VOID
LJB_NotifyCommitVidPnToSingleTarget(
    __in LJB_ADAPTER *                      Adapter,
    __in PVOID                              hPrimaryAllocation,
    __in CONST D3DKMDT_VIDPN_PRESENT_PATH*  Path
    )
{
    LJB_MONITOR_NODE * MonitorNode;
    LJB_COMMIT_VIDPN CommitVidPn;

    MonitorNode = LJB_GetMonitorNodeFromChildUid(
        Adapter, Path->VidPnTargetId
        );
    if (MonitorNode == NULL)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": couldn't find MonitorNode for ChildUid(0x%x)?\n",
            Path->VidPnTargetId
            ));
        return;
    }

    if (MonitorNode->MonitorInterface.pfnGenericIoctl != NULL &&
        MonitorNode->MonitorInterface.Context != NULL)
    {
        LJB_MONITOR_INTERFACE* CONST MonitorInterface = &MonitorNode->MonitorInterface;
        NTSTATUS myStatus;

        RtlZeroMemory(&CommitVidPn, sizeof(CommitVidPn));
        CommitVidPn.CommitPath = *Path;

        /*
         * FIXME: setup Width, Height, Bpp according to hPrimaryAllocation
         */
        UNREFERENCED_PARAMETER(hPrimaryAllocation);
        DBG_PRINT(Adapter, DBGLVL_FLOW,
            (__FUNCTION__": Send LJB_GENERIC_IOCTL_COMMIT_VIDPN UsbTargetId(0x%x)\n",
            Path->VidPnTargetId
            ));
        myStatus = (*MonitorInterface->pfnGenericIoctl)(
            MonitorInterface->Context,
            LJB_GENERIC_IOCTL_COMMIT_VIDPN,
            &CommitVidPn,
            sizeof(CommitVidPn),
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

static
VOID
LJB_NotifyCommitVidPnToAllUsbTargets(
    __in LJB_ADAPTER *  Adapter,
    __in PVOID          hPrimaryAllocation
    )
{
    CONST D3DKMDT_VIDPN_PRESENT_PATH* Path;
    UINT i;

    /*
     * For every Path connected to USB target, send LJB_COMMIT_VIDPN
     */
    for (i = 0; i < Adapter->NumPathsCommitted; i++)
    {
        Path = Adapter->PathsCommitted + i;
        if (Path->VidPnTargetId < Adapter->UsbTargetIdBase)
            continue;

        LJB_NotifyCommitVidPnToSingleTarget(Adapter, hPrimaryAllocation, Path);
    }
}