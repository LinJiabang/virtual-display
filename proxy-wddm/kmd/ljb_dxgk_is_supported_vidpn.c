/*
 * ljb_dxgk_is_supported_vidpn.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_IsSupportedVidPn)
#endif

/*
 * Function: LJB_DXGK_IsSupportedVidPn
 *
 * Description:
 * The DxgkDdiIsSupportedVidPn function determines whether a specified VidPN is
 * supported on a display adapter.
 *
 * Return Value:
 *  DxgkDdiIsSupportedVidPn returns one of the following values:
 *
 *  STATUS_SUCCESS: The topology of the VidPN is valid. The IsVidPnSupported
 *  structure member is set to either TRUE or FALSE.
 *
 *  STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY: The topology of the VidPN implementation
 *  is invalid. The IsVidPnSupported structure member is set to FALSE.
 *
 *  STATUS_NO_MEMORY: The function failed because it was unable to allocate memory.
 *
 * Remarks:
 * For more information about the analysis that this function must perform, see
 * Determining Whether a VidPN is Supported on a Display Adapter.
 *
 * If pIsSupportedVidPnArg->hDesiredVidPn is zero, DxgkDdiIsSupportedVidPn must
 * set pIsSupportedVidPnArg->IsVidPnSupported to TRUE, the idea being that the
 * display adapter can always be configured to display nothing.
 *
 * DxgkDdiIsSupportedVidPn should be made pageable.
 */
NTSTATUS
LJB_DXGK_IsSupportedVidPn(
    _In_    const HANDLE                   hAdapter,
    _Inout_       DXGKARG_ISSUPPORTEDVIDPN *pIsSupportedVidPnArg
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiIsSupportedVidPn)(
        hAdapter,
        pIsSupportedVidPnArg
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}