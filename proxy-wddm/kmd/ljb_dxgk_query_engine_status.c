/*
 * ljb_dxgk_query_engine_status.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_QueryEngineStatus)
#endif

/*
 * Function: LJB_DXGK_QueryEngineStatus
 *
 * Description:
 * The display port driver's GPU scheduler calls this function to determine the
 * progress of a node within an active physical display adapter (engine).
 *
 * Return Value:
 * Returns STATUS_SUCCESS if it succeeds. Otherwise, it returns one of the error
 * codes defined in Ntstatus.h.
 *
 * Remarks:
 * This function should be made pageable, and it should always succeed.
 *
 * The operating system guarantees that this function follows the first level synchronization
 * mode as defined in Threading and Synchronization First Level.
 *
 * For more information, see TDR changes in Windows 8.
 */
NTSTATUS
LJB_DXGK_QueryEngineStatus(
    _In_    const HANDLE                    hAdapter,
    _Inout_       DXGKARG_QUERYENGINESTATUS *pQueryEngineStatus
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    UINT CONST                          NodeOrdinal = pQueryEngineStatus->NodeOrdinal;
    UINT CONST                          EngineOrdinal = pQueryEngineStatus->EngineOrdinal;
    LJB_ENGINE_INFO *                   EngineInfo = &Adapter->EngineInfo[NodeOrdinal][EngineOrdinal];
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiQueryEngineStatus)(
        hAdapter,
        pQueryEngineStatus
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        return ntStatus;
    }

    /*
     * update EngineInfo->EngineStatus
     */
    EngineInfo->EngineStatus = pQueryEngineStatus->EngineStatus;
    DBG_PRINT(Adapter, DBGLVL_FLOW,
        (__FUNCTION__ ": Node(0x%x),Engine(0x%x), EngineStatus(0x%x)\n",
        NodeOrdinal,
        EngineOrdinal,
        EngineInfo->EngineStatus.Value));
    return ntStatus;
}
