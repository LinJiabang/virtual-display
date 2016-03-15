/*
 * ljb_dxgk_commit_vidpn.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_CommitVidPn)
#pragma alloc_text (PAGE, LJB_DXGK_UpdateActiveVidPnPresentPath)
#endif

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
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    if (!Adapter->FirstVidPnArrived)
    {
        Adapter->FirstVidPnArrived = TRUE;
    }
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
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiUpdateActiveVidPnPresentPath)(
        hAdapter,
        pUpdateActiveVidPnPresentPath
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}