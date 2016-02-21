/*
 * ljb_dxgk_cancel_command.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

/*
 * Function: LJB_DXGK_CancelCommand
 *
 * Description:
 * Cleans up internal resources associated with a direct memory access (DMA) packet
 * that was in the GPU scheduler's software queue but never reached the hardware
 * queue because the device went into an error state. Such an error state is typically
 * caused by a Timeout Detection and Recovery (TDR) event.
 *
 * Return Value:
 * Returns STATUS_SUCCESS upon successful completion. If the driver instead
 * returns an error code, the operating system causes a system bugcheck to occur.
 * For more information, see the following Remarks section.
 *
 * Remarks:
 * Note  The DirectX graphics kernel subsystem calls this function only if the
 * DXGK_VIDSCHCAPS.CancelCommandAware member is set.
 *
 * If the driver returns an error code, the DirectX graphics kernel subsystem causes
 * a system bugcheck to occur. In a crash dump file, the error is noted by the message
 * BugCheck 0x119, which has the following four parameters.
 * 1. 0x9
 * 2. The NTSTATUS error code returned from the failed driver call
 * 3. A pointer to the DXGKARG_CANCELCOMMAND structure
 * 4. A pointer to an internal scheduler data structure
 */
NTSTATUS
LJB_DXGK_CancelCommand(
    _In_ const HANDLE                   hAdapter,
    _In_ const DXGKARG_CANCELCOMMAND *  pCancelCommand
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    DBG_PRINT(Adapter, DBGLVL_ERROR, ("?" __FUNCTION__ "\n"));
    ntStatus = (*DriverInitData->DxgkDdiCancelCommand)(
        hAdapter,
        pCancelCommand
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}