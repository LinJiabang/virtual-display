/*
 * ljb_dxgk_multiplane_overlay.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

/*
 * Function: LJB_DXGK_CheckMultiPlaneOverlaySupport
 *
 * Description:
 * Called by the Microsoft DirectX graphics kernel subsystem to check the details
 * of hardware support for multiplane overlays.
 *
 * Return Value:
 * Returns STATUS_SUCCESS if it succeeds; otherwise it returns one of the error
 * codes defined in Ntstatus.h.
 *
 * Remarks:
 */
NTSTATUS
LJB_DXGK_CheckMultiPlaneOverlaySupport(
    _In_ const HANDLE                                   hAdapter,
    _Inout_       DXGKARG_CHECKMULTIPLANEOVERLAYSUPPORT *pCheckMultiPlaneOverlaySupport
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    ntStatus = (*DriverInitData->DxgkDdiCheckMultiPlaneOverlaySupport)(
        hAdapter,
        pCheckMultiPlaneOverlaySupport
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}

/*
 * Function: LJB_DXGK_CheckMultiPlaneOverlaySupport2
 *
 * Description:
 * DxgkDdiCheckMultiPlaneOverlaySupport2 is called to determine whether a specific
 * multi-plane overlay configuration is supported. It must be implemented by Windows
 * Display Driver Model (WDDM) 2.0 or later drivers that support multi-plane overlays.
 *
 * Return Value:
 * Returns STATUS_SUCCESS if it succeeds; otherwise it returns one of the error
 * codes defined in Ntstatus.h.
 *
 * Remarks:
 * The kernel mode driver reports whether the specified configuration is supported.
 * The kernel mode driver should not raise or lower the available bandwidth in
 * anticipation to this configuration getting set.
 */
NTSTATUS
LJB_DXGK_CheckMultiPlaneOverlaySupport2(
    _In_ const HANDLE                                   hAdapter,
    _Inout_    DXGKARG_CHECKMULTIPLANEOVERLAYSUPPORT2 * pData
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    /*
     * FIXME: need to check pData->pPlanes->VidPnSourceId
     */
    ntStatus = (*DriverInitData->DxgkDdiCheckMultiPlaneOverlaySupport2)(
        hAdapter,
        pData
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}

/*
 * Function: LJB_DXGK_SetVidPnSourceAddressWithMultiPlaneOverlay
 *
 * Description:
 * Sets the addresses of multiple surfaces, including the Desktop Window Manager
 * (DWM)'s swapchain, that are associated with a particular video present source.
 * This function is used to present multiple surfaces (including the DWM¡¦s swapchain)
 * to the screen.
 *
 * Return Value:
 * Returns STATUS_SUCCESS if it succeeds; otherwise it returns one of the error
 * codes defined in Ntstatus.h.
 *
 * Remarks:
 * See requirements on calling this function in Multiplane overlay VidPN presentation.
 */
NTSTATUS
LJB_DXGK_SetVidPnSourceAddressWithMultiPlaneOverlay(
    _In_ const HANDLE                                             hAdapter,
    _In_ const DXGKARG_SETVIDPNSOURCEADDRESSWITHMULTIPLANEOVERLAY *pSetVidPnSourceAddressWithMultiPlaneOverlay
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    ntStatus = (*DriverInitData->DxgkDdiSetVidPnSourceAddressWithMultiPlaneOverlay)(
        hAdapter,
        pSetVidPnSourceAddressWithMultiPlaneOverlay
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}

/*
 * Function: LJB_DXGK_SetVidPnSourceAddressWithMultiPlaneOverlay2
 *
 * Description:
 * DxgkDdiSetVidPnSourceAddressWithMultiPlaneOverlay2 is called to change the overlay
 * configuration being displayed. It must be implemented by Windows Display Driver
 * Model (WDDM) 2.0 or later drivers that support multi-plane overlays.
 *
 * Return Value:
 * If this routine succeeds, it returns NTSTATUS_SUCCESS. The driver should always
 * return a success code.
 *
 * Remarks:
 * None.
 */
NTSTATUS
LJB_DXGK_SetVidPnSourceAddressWithMultiPlaneOverlay2(
    _In_ const HANDLE                                               hAdapter,
    _In_ const DXGKARG_SETVIDPNSOURCEADDRESSWITHMULTIPLANEOVERLAY2 *pData
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    /*
     * FIXEME: check pData->VidPnSourceId
     */
    ntStatus = (*DriverInitData->DxgkDdiSetVidPnSourceAddressWithMultiPlaneOverlay2)(
        hAdapter,
        pData
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}