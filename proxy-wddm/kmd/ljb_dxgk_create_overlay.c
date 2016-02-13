/*
 * ljb_dxgk_create_overlay.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_CreateOverlay)
#endif

/*
 * Function: LJB_DXGK_CreateOverlay
 *
 * Description:
 * The DxgkDdiCreateOverlay function enables the overlay hardware if the hardware
 * is capable.
 *
 * Return Value:
 * DxgkDdiCreateOverlay returns one of the following values:
 *
 *  STATUS_SUCCESS: DxgkDdiCreateOverlay successfully created the overlay.
 *
 *  STATUS_INVALID_PARAMETER: Parameters that were passed to DxgkDdiCreateOverlay
 *  contained errors that prevented it from completing.
 *
 *  STATUS_NO_MEMORY: DxgkDdiCreateOverlay could not allocate memory that was
 *  required for it to complete.
 *
 *  STATUS_INSUFFICIENT_RESOURCES: DxgkDdiCreateOverlay could not complete
 *  because insufficient bandwidth was available or the requested overlay hardware
 *  could not complete the task.
 *
 *  STATUS_GRAPHICS_DRIVER_MISMATCH: The display miniport driver is not compatible
 *  with the user-mode display driver that initiated the call to DxgkDdiCreateOverlay.
 *
 * Remarks:
 * DxgkDdiCreateOverlay should be made pageable..
 */
NTSTATUS
LJB_DXGK_CreateOverlay(
    _In_    const HANDLE                hAdapter,
    _Inout_       DXGKARG_CREATEOVERLAY *pCreateOverlay
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiCreateOverlay)(
        hAdapter,
        pCreateOverlay
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}
