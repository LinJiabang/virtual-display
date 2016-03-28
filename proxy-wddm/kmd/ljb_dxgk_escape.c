/*
 * ljb_dxgk_escape.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_Escape)
#endif

/*
 * Function: LJB_DXGK_Escape
 *
 * Description:
 * The DxgkDdiEscape function shares information with the user-mode display driver.
 *
 * Return Value:
 * DxgkDdiEscape returns one of the following values:
 *
 *  STATUS_SUCCESS: DxgkDdiEscape successfully shared information.
 *
 *  STATUS_INVALID_PARAMETER: Parameters that were passed to DxgkDdiEscape contained
 *  errors that prevented it from completing.
 *
 *  STATUS_NO_MEMORY: DxgkDdiEscape could not allocate memory that was required
 *  for it to complete.
 *
 *  STATUS_PRIVILEGED_INSTRUCTION: DxgkDdiEscape detected nonprivileged instructions
 *  (that is, instructions that access memory beyond the privilege of the current
 *  central processing unit [CPU] process).
 *
 *  STATUS_ILLEGAL_INSTRUCTION: DxgkDdiEscape detected instructions that graphics
 *  hardware could not support.
 *
 *  STATUS_GRAPHICS_DRIVER_MISMATCH: The display miniport driver is not compatible
 *  with the user-mode display driver that initiated the call to DxgkDdiEscape.
 *
 * Remarks:
 * The DirectX graphics kernel subsystem calls the display miniport driver's
 * DxgkDdiEscape function whenever the user-mode display driver must share
 * information with the display miniport driver in a way that is not supported
 * through other driver communications.
 *
 * DxgkDdiEscape should be made pageable.
 */
NTSTATUS
LJB_DXGK_Escape(
    _In_ const HANDLE         hAdapter,
    _In_ const DXGKARG_ESCAPE *pEscape
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiEscape)(
        hAdapter,
        pEscape
        );
    if (!NT_SUCCESS(ntStatus))
    {
        // Sometimes the ATI app might issue Escape to Intel KMD. Don't try to
        // catch this error.
        //DBG_PRINT(Adapter, DBGLVL_ERROR,
        //    ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}