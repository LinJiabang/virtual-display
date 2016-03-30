/*
 * ljb_dxgk_set_vidpn_source_visibility.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_SetVidPnSourceVisibility)
#endif

static
VOID
LJB_NotifySetVidPnSourceVisibility(
    __in LJB_ADAPTER *  Adapter,
    __in ULONG          ChildUid,
    __in BOOLEAN        Visible
    );

/*
 * Function: LJB_DXGK_SetVidPnSourceVisibility
 *
 * Description:
 * The DxgkDdiSetVidPnSourceVisibility function programs the video output codec
 * that is associated with a specified video present source to either start
 * scanning or stop scanning the source's primary surface.
 *
 * Return Value:
 * DxgkDdiSetVidPnSourceVisibility returns STATUS_SUCCESS if it succeeds; otherwise,
 * it returns one of the error codes defined in Ntstatus.h.
 *
 * Remarks:
 * See requirements on calling this function with multiplane overlays in
 * Multiplane overlay VidPN presentation.
 *
 * DxgkDdiSetVidPnSourceVisibility should be made pageable.
 */
NTSTATUS
LJB_DXGK_SetVidPnSourceVisibility(
    _In_ const HANDLE                           hAdapter,
    _In_ const DXGKARG_SETVIDPNSOURCEVISIBILITY *pSetVidPnSourceVisibility
    )
{
    LJB_ADAPTER * CONST                     Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST          ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST      DriverInitData = &ClientDriverData->DriverInitData;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID CONST    VidPnSourceId = pSetVidPnSourceVisibility->VidPnSourceId;
    BOOLEAN CONST                           IsConnectedInboxTarget = LJB_DXGK_IsSourceConnectedToInboxTarget(Adapter, pSetVidPnSourceVisibility->VidPnSourceId);
    BOOLEAN CONST                           IsConnectedUsbTarget = LJB_DXGK_IsSourceConnectedToUsbTarget(Adapter, pSetVidPnSourceVisibility->VidPnSourceId);
    NTSTATUS                                ntStatus;

    PAGED_CODE();

    DBG_PRINT(Adapter, DBGLVL_FLOW,
        (__FUNCTION__": VidPnSourceId(%u)/Visible(%u)\n",
        VidPnSourceId,
        pSetVidPnSourceVisibility->Visible
        ));

    ntStatus = STATUS_SUCCESS;
    if (IsConnectedInboxTarget)
    {
        ntStatus = (*DriverInitData->DxgkDdiSetVidPnSourceVisibility)(
            hAdapter,
            pSetVidPnSourceVisibility
            );
        if (!NT_SUCCESS(ntStatus))
        {
            DBG_PRINT(Adapter, DBGLVL_ERROR,
                ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        }
    }

    if (IsConnectedUsbTarget)
    {
        UINT    i;

        /*
         * For every USB Target connected to this VidPnSourceId, send a notification
         */
        for (i = 0; i < Adapter->NumPathsCommitted; i++)
        {
            D3DKMDT_VIDPN_PRESENT_PATH * Path = Adapter->PathsCommitted + i;

            if (Path->VidPnSourceId != VidPnSourceId)
                continue;

            if (Path->VidPnSourceId < Adapter->UsbTargetIdBase)
                continue;

            LJB_NotifySetVidPnSourceVisibility(
                Adapter,
                Path->VidPnSourceId,
                pSetVidPnSourceVisibility->Visible
                );

        }
    }

    return ntStatus;
}

static
VOID
LJB_NotifySetVidPnSourceVisibility(
    __in LJB_ADAPTER *  Adapter,
    __in ULONG          ChildUid,
    __in BOOLEAN        Visible
    )
{
    LJB_MONITOR_NODE * MonitorNode;

    MonitorNode = LJB_GetMonitorNodeFromChildUid(Adapter, ChildUid);
    if (MonitorNode == NULL)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?"__FUNCTION__": No MonitorNode found for VidPnTargetId(0x%x)\n",
            ChildUid
            ));
        return;
    }

    if (MonitorNode->MonitorInterface.pfnGenericIoctl != NULL &&
        MonitorNode->MonitorInterface.Context != NULL)
    {
        LJB_MONITOR_INTERFACE* CONST MonitorInterface = &MonitorNode->MonitorInterface;
        NTSTATUS myStatus;

        DBG_PRINT(Adapter, DBGLVL_FLOW,
            (__FUNCTION__": Send LJB_GENERIC_IOCTL_SET_VIDPN_SOURCE_VISIBLE to UsbTargetId(0x%x)\n",
            ChildUid
            ));
        myStatus = (*MonitorInterface->pfnGenericIoctl)(
            MonitorInterface->Context,
            LJB_GENERIC_IOCTL_SET_VIDPN_SOURCE_VISIBLE,
            &Visible,
            sizeof(Visible),
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
