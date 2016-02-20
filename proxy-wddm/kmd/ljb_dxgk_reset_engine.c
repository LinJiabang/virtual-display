/*
 * ljb_dxgk_reset_engine.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_ResetEngine)
#endif

/*
 * Function: LJB_DXGK_ResetEngine
 *
 * Description:
 * The display port driver's GPU scheduler calls this function to reset an active
 * node on a physical display adapter when the scheduler detects a timeout condition
 * on the adapter.
 *
 * Return Value:
 * Returns STATUS_SUCCESS if the function succeeds. Otherwise, this function returns
 * one of the error codes defined in Ntstatus.h.
 *
 * Remarks:
 * The display miniport driver should return from a call to this function only when
 * the reset operation is completed, nothing remains in the physical adapter's
 * hardware queue, and the specified nodes are ready to accept new packets.
 *
 * This function should be made pageable.
 *
 * The operating system guarantees that this function follows the first level
 * synchronization mode as defined in Threading and Synchronization First Level.
 *
 * For more information, see TDR changes in Windows 8.
 */
NTSTATUS
LJB_DXGK_ResetEngine(
    _In_    const HANDLE                hAdapter,
    _Inout_       DXGKARG_RESETENGINE * pResetEngine
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    UINT CONST                          NodeOrdinal = pResetEngine->NodeOrdinal;
    UINT CONST                          EngineOrdinal = pResetEngine->EngineOrdinal;
    LJB_ENGINE_INFO *                   EngineInfo = &Adapter->EngineInfo[NodeOrdinal][EngineOrdinal];
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiResetEngine)(
        hAdapter,
        pResetEngine
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        return ntStatus;
    }

    /*
     * update EngineInfo->LastAbortedFenceId
     */
    EngineInfo->LastAbortedFenceId = pResetEngine->LastAbortedFenceId;
    DBG_PRINT(Adapter, DBGLVL_ERROR,
        ("?" __FUNCTION__ ": Node(0x%x),Engine(0x%x), LastAbortedFenceId(0x%x)\n",
        NodeOrdinal,
        EngineOrdinal,
        EngineInfo->LastAbortedFenceId));
    return ntStatus;
}
