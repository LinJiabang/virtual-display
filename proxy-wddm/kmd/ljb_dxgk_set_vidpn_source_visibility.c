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
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiSetVidPnSourceVisibility)(
        hAdapter,
        pSetVidPnSourceVisibility
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}