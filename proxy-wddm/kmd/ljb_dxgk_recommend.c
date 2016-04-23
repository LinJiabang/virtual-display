/*
 * ljb_dxgk_recommend.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"
#include "ljb_dxgk_vidpn_interface.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_RecommendMonitorModes)
#pragma alloc_text (PAGE, LJB_DXGK_RecommendVidPnTopology)
#endif

/*
 * Function: LJB_DXGK_RecommendMonitorModes
 *
 * Description:
 * The DxgkDdiRecommendMonitorModes function inspects the monitor source mode set
 * that is associated with a particular video present target and possibly adds modes
 * to the set.
 *
 * Return Value:
 * DxgkDdiRecommendMonitorModes returns one of the following values:
 *
 *  STATUS_SUCCESS: The function succeeded.
 *
 *  STATUS_NO_MEMORY: The function failed because it was unable to allocate
 *  enough memory.
 *
 *  The miniport driver should pass through any error code that it gets from the
 *  operating system for which it does not have a fallback code path.
 *
 * Remarks:
 * DxgkDdiRecommendMonitorModes should be made pageable.
 */
NTSTATUS
LJB_DXGK_RecommendMonitorModes(
    IN_CONST_HANDLE                                 hAdapter,
    IN_CONST_PDXGKARG_RECOMMENDMONITORMODES_CONST   pRecommendMonitorModes
    )
{
    LJB_ADAPTER * CONST                     Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST          ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST      DriverInitData = &ClientDriverData->DriverInitData;
    D3DDDI_VIDEO_PRESENT_TARGET_ID CONST    VidPnTargetId = pRecommendMonitorModes->VideoPresentTargetId;
    NTSTATUS                                ntStatus;

    PAGED_CODE();

    /*
     * if the monitor is ours, just return STATUS_SUCCESS;
     */
    if (VidPnTargetId >= Adapter->UsbTargetIdBase)
        return STATUS_SUCCESS;

    ntStatus = (*DriverInitData->DxgkDdiRecommendMonitorModes)(
        hAdapter,
        pRecommendMonitorModes
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}

/*
 * Function: LJB_DXGK_RecommendVidPnTopology
 *
 * Description:
 * The DxgkDdiRecommendVidPnTopology function creates the topology of a specified
 * VidPN or augments the topology with a new path to video present targets.
 *
 * Return Value:
 * DxgkDdiRecommendVidPnTopology returns one of the following values:
 *
 *  STATUS_SUCCESS: The function successfully created or augmented the topology.
 *
 *  STATUS_GRAPHICS_NO_RECOMMENDED_VIDPN_TOPOLOGY:The function has no
 *  recommendation for the augmentation of the specified VidPN topology.
 *
 *  STATUS_GRAPHICS_CANCEL_VIDPN_TOPOLOGY_AUGMENTATION: The function recommends
 *  to cancel the augmentation of the specified VidPN's topology on the specified
 *  source. This return code is allowed only in the case of VidPN topology
 *  augmentation.
 *
 *  STATUS_NO_MEMORY: The function failed because it was unable to allocate
 *  enough memory.
 *
 *  The miniport driver should pass through any error code that it gets from the
 *  operating system for which it does not have a fallback code path.
 *
 * Remarks:
 * DxgkDdiRecommendVidPnTopology should be made pageable.
 */
NTSTATUS
LJB_DXGK_RecommendVidPnTopology(
    IN_CONST_HANDLE                                 hAdapter,
    IN_CONST_PDXGKARG_RECOMMENDVIDPNTOPOLOGY_CONST  pRecommendVidPnTopology
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    LJB_VIDPN *                         MyVidPn;
    UINT                                NumberOfUsbTarget;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    MyVidPn = LJB_VIDPN_CreateVidPn(Adapter, pRecommendVidPnTopology->hVidPn);
    if (MyVidPn == NULL)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?"__FUNCTION__": no MyVidPn allocated.\n"));
        return STATUS_NO_MEMORY;
    }

    NumberOfUsbTarget = LJB_VIDPN_GetNumberOfUsbTarget(MyVidPn);
    if (NumberOfUsbTarget == 0)
    {
        ntStatus = (*DriverInitData->DxgkDdiRecommendVidPnTopology)(
            hAdapter,
            pRecommendVidPnTopology
            );
    }
    else
    {
        BOOLEAN IsConnectedToUsbTarget;
        BOOLEAN IsConnectedToInboxTarget;
        UINT    i;

        /*
         * check if pRecommendVidPnTopology->VidPnSourceId is connected to
         * USB target
         */
        IsConnectedToUsbTarget = FALSE;
        IsConnectedToInboxTarget = FALSE;
        for (i = 0; i < MyVidPn->NumPaths; i++)
        {
            D3DKMDT_VIDPN_PRESENT_PATH * CONST Path = MyVidPn->Paths + i;

            if (Path->VidPnSourceId != pRecommendVidPnTopology->VidPnSourceId)
                continue;
            if (Path->VidPnTargetId >= Adapter->UsbTargetIdBase)
                IsConnectedToUsbTarget = TRUE;
            else
                IsConnectedToInboxTarget = TRUE;
        }

        /*
         * if the VidPnSourceId is connected to USB monitor only, don't
         * recommend anything, otherwise pass the call to inbox driver.
         */
        if (IsConnectedToUsbTarget && !IsConnectedToInboxTarget)
        {
            ntStatus = STATUS_GRAPHICS_NO_RECOMMENDED_VIDPN_TOPOLOGY;
        }
        else
        {
            DXGKARG_RECOMMENDVIDPNTOPOLOGY MyRecommendVidPnTopology;

            MyRecommendVidPnTopology = *pRecommendVidPnTopology;
            MyRecommendVidPnTopology.hVidPn = (D3DKMDT_HVIDPN) MyVidPn;
            ntStatus = (*DriverInitData->DxgkDdiRecommendVidPnTopology)(
                hAdapter,
                &MyRecommendVidPnTopology
                );
        }
    }

    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    LJB_VIDPN_DestroyVidPn(MyVidPn);
    return ntStatus;
}