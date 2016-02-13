/*
 * ljb_dxgk_get_scan_line.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_GetScanLine)
#endif

/*
 * Function: LJB_DXGK_GetScanLine
 *
 * Description:
 * The DxgkDdiGetScanLine function determines whether the specified video present
 * target of a video present network (VidPN) is in vertical blanking mode and
 * retrieves the current scan line.
 *
 * Return Value:
 * DxgkDdiGetScanLine returns STATUS_SUCCESS if it succeeds; otherwise, it
 * returns one of the error codes defined in Ntstatus.h.
 *
 * Remarks:
 * DxgkDdiGetScanLine should be made pageable.
 */
NTSTATUS
LJB_DXGK_GetScanLine(
    _In_    const HANDLE              hAdapter,
    _Inout_       DXGKARG_GETSCANLINE *pGetScanLine
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    /*
     * check pGetScanLine->VidPnTargetId
     */
    if (pGetScanLine->VidPnTargetId >= Adapter->UsbTargetIdBase)
    {
        DBG_PRINT(Adapter, DBGLVL_VSYNC, (__FUNCTION__ "\n"));
        return STATUS_SUCCESS;
    }

    ntStatus = (*DriverInitData->DxgkDdiGetScanLine)(
        hAdapter,
        pGetScanLine
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}
