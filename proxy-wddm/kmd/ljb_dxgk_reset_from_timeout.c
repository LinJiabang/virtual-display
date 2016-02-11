/*
 * ljb_dxgk_reset_from_timeout.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_ResetFromTimeout)
#pragma alloc_text (PAGE, LJB_DXGK_RestartFromTimeout)
#endif

/*
 * Function: LJB_DXGK_ResetFromTimeout
 *
 * Description:
 * The DxgkDdiResetFromTimeout function resets the graphics processing unit (GPU)
 * after a hardware timeout occurs and guarantees that the GPU is not writing or
 * reading any memory by the time that DxgkDdiResetFromTimeout returns.
 *
 * Return Value:
 * DxgkDdiResetFromTimeout returns STATUS_SUCCESS to indicate that the driver
 * handled the call successfully; otherwise, the operating system bug checks and
 * causes a restart.
 *
 * Remarks:
 * The GPU scheduler calls DxgkDdiResetFromTimeout when it detects that a hardware
 * time-out occurred. The time-out is typically a delayed response to a preempt
 * request. DxgkDdiResetFromTimeout should reset the GPU.
 *
 * For more information about time-outs in this situation, see Thread Synchronization
 * and TDR.
 *
 * DxgkDdiResetFromTimeout should be made pageable.
 */
NTSTATUS
LJB_DXGK_ResetFromTimeout(
    _In_ const HANDLE                  hAdapter
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    DBG_PRINT(Adapter, DBGLVL_ERROR, ("?" __FUNCTION__ "\n"));
    ntStatus = (*DriverInitData->DxgkDdiResetFromTimeout)(hAdapter);
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}

NTSTATUS
LJB_DXGK_RestartFromTimeout(
    _In_ const HANDLE                  hAdapter
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    DBG_PRINT(Adapter, DBGLVL_ERROR, ("?" __FUNCTION__ "\n"));
    ntStatus = (*DriverInitData->DxgkDdiRestartFromTimeout)(hAdapter);
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}