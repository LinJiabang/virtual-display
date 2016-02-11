/*
 * ljb_dxgk_set_pointer_position.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_SetPointerPosition)
#endif

/*
 * Function: LJB_DXGK_SetPointerPosition
 *
 * Description:
 * The DxgkDdiSetPointerPosition function sets the location and visibility state
 * of the mouse pointer.
 *
 * Return Value:
 * DxgkDdiSetPointerPosition returns STATUS_SUCCESS if it succeeds; otherwise,
 * it returns one of the error codes defined in Ntstatus.h.
 *
 * Remarks:
 * The DirectX graphics kernel subsystem calls the display miniport driver's
 * DxgkDdiSetPointerPosition function to set the location of the mouse pointer.
 * The DxgkDdiSetPointerPosition function is called independently of all of the
 * other display miniport driver functions. Therefore, a DxgkDdiSetPointerPosition
 * thread can run simultaneously with another display miniport driver thread.
 * However, the system ensures that DxgkDdiSetPointerPosition and
 * DxgkDdiSetPointerShape threads cannot run simultaneously.
 * If you run a DxgkDdiSetPointerPosition thread simultaneously with another
 * display miniport driver thread, the display miniport driver should be able to
 * program the mouse pointer hardware independently of other activities, such as
 * operations that send a command buffer through direct memory access (DMA) to
 * the graphics hardware, operations that program the graphics hardware by using
 * memory-mapped I/O (MMIO), and so on.
 * DxgkDdiSetPointerPosition can be called even if the video present network
 * (VidPN) topology that is associated with the VidPnSourceId member of the
 * DXGKARG_SETPOINTERPOSITION structure that the pSetPointerPosition parameter
 * points to is disabled. In this case, the driver should return STATUS_SUCCESS
 * but should make no changes to the state of the driver or hardware.
 *
 * DxgkDdiSetPointerPosition should be made pageable.
 */
NTSTATUS
LJB_DXGK_SetPointerPosition(
    _In_ const HANDLE                     hAdapter,
    _In_ const DXGKARG_SETPOINTERPOSITION *pSetPointerPosition
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiSetPointerPosition)(
        hAdapter,
        pSetPointerPosition
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}