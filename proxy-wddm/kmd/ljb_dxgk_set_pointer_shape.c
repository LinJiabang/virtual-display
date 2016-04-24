/*
 * ljb_dxgk_set_pointer_shape.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_SetPointerShape)
#endif

/*
 * Function: LJB_DXGK_SetPointerShape
 *
 * Description:
 * The DxgkDdiSetPointerShape function sets the appearance and location of the
 * mouse pointer.
 *
 * Return Value:
 * DxgkDdiSetPointerShape returns one of the following values:
 *
 *  STATUS_SUCCESS: The mouse pointer is successfully drawn.
 *
 *  STATUS_NO_MEMORY: DxgkDdiSetPointerShape could not allocate memory that was
 *  required for it to complete.
 *
 * Remarks:
 * The DirectX graphics kernel subsystem calls the display miniport driver's
 * DxgkDdiSetPointerShape function to set information about the mouse pointer.
 * The DxgkDdiSetPointerShape function is called independently of all of the
 * other display miniport driver functions. Therefore, a DxgkDdiSetPointerShape
 * thread can run simultaneously with another display miniport driver thread.
 * However, the system ensures that DxgkDdiSetPointerShape and DxgkDdiSetPointerPosition
 * threads cannot run simultaneously.
 *
 * If you run a DxgkDdiSetPointerShape thread simultaneouly with another display
 * miniport driver thread, the display miniport driver should be able to program
 * the mouse pointer hardware independently of other activities, such as operations
 * that send a command buffer through direct memory access (DMA) to the graphics
 * hardware, operations that program the graphics hardware by using memory-mapped
 * I/O (MMIO), and so on.
 *
 * DxgkDdiSetPointerShape is not called if the video present network (VidPN)
 * topology that is associated with the VidPnSourceId member of the
 * DXGKARG_SETPOINTERSHAPE structure that the pSetPointerShape parameter points
 * to is disabled.
 *
 * DxgkDdiSetPointerShape should be made pageable.
 */
NTSTATUS
LJB_DXGK_SetPointerShape(
    _In_ const HANDLE                       hAdapter,
    _In_ const DXGKARG_SETPOINTERSHAPE *    pSetPointerShape
    )
{
    LJB_ADAPTER * CONST                     Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST          ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST      DriverInitData = &ClientDriverData->DriverInitData;
    D3DDDI_VIDEO_PRESENT_SOURCE_ID CONST    VidPnSourceId = pSetPointerShape->VidPnSourceId;
    BOOLEAN                                 IsConnectedToInboxTarget;
    BOOLEAN                                 IsConnectedToUsbTarget;
    NTSTATUS                                ntStatus;

    PAGED_CODE();

    ntStatus = STATUS_SUCCESS;
    IsConnectedToInboxTarget = LJB_DXGK_IsSourceConnectedToInboxTarget(
        Adapter,
        VidPnSourceId
        );
    IsConnectedToUsbTarget = LJB_DXGK_IsSourceConnectedToUsbTarget(
        Adapter,
        VidPnSourceId
        );

    if (IsConnectedToInboxTarget)
    {
        ntStatus = (*DriverInitData->DxgkDdiSetPointerShape)(
            hAdapter,
            pSetPointerShape
            );
        if (!NT_SUCCESS(ntStatus))
        {
            DBG_PRINT(Adapter, DBGLVL_ERROR,
                ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        }
    }

    if (IsConnectedToUsbTarget)
    {
        LJB_MONITOR_NODE *  MonitorNode;
        UINT                i;

        /*
         * for every USB target, send LJB_GENERIC_IOCTL_SET_POINTER_SHAPE
         */
        for (i = 0; i < Adapter->NumPathsCommitted; i++)
        {
            D3DKMDT_VIDPN_PRESENT_PATH * Path = Adapter->PathsCommitted + i;

            if (Path->VidPnSourceId != VidPnSourceId)
                continue;

            MonitorNode = LJB_GetMonitorNodeFromChildUid(
                Adapter, Path->VidPnTargetId
                );
            if (MonitorNode == NULL)
            {
                DBG_PRINT(Adapter, DBGLVL_ERROR,
                    ("?" __FUNCTION__
                    ": no MonitorNode for VidPnSourceId(0x%x)/VidPnTargetId(0x%x)\n",
                    VidPnSourceId,
                    Path->VidPnTargetId
                    ));
                break;
            }

            if (MonitorNode->MonitorInterface.pfnGenericIoctl != NULL &&
                MonitorNode->MonitorInterface.Context != NULL)
            {
                LJB_MONITOR_INTERFACE* CONST MonitorInterface = &MonitorNode->MonitorInterface;
                NTSTATUS myStatus;

                myStatus = (*MonitorInterface->pfnGenericIoctl)(
                    MonitorInterface->Context,
                    LJB_GENERIC_IOCTL_SET_POINTER_SHAPE,
                    (PVOID) pSetPointerShape,
                    sizeof(DXGKARG_SETPOINTERSHAPE),
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
    }

    return ntStatus;
}