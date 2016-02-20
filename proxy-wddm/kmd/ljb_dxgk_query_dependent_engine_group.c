/*
 * ljb_dxgk_query_dependent_engine_group.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_QueryDependentEngineGroup)
#endif

/*
 * Function: LJB_DXGK_QueryDependentEngineGroup
 *
 * Description:
 * Called by the display port driver's GPU scheduler to query dependencies of nodes
 * in a physical display adapter.
 *
 * Return Value:
 * Returns STATUS_SUCCESS if it succeeds. Otherwise, it returns one of the error
 * codes defined in Ntstatus.h.
 *
 * Remarks:
 * This function is used to describe all physical adapters (engines) that are affected
 * by an engine reset request. It helps improve user experience on hardware architectures
 * that have dependencies among multiple engines that can affect the reset process.
 * Note that all affected nodes must have the same engine affinity value. (See engine
 * affinity discussion in TDR changes in Windows 8.)
 *
 * The display port driver's GPU scheduler calls DxgkDdiQueryDependentEngineGroup
 * every time it calls the DxgkDdiResetEngine function. The GPU scheduler waits
 * 500 milliseconds for the display miniport driver to complete preemption of all
 * dependent engines. For any engines for which the driver cannot complete a preemption,
 * the GPU scheduler calls the DxgkDdiResetEngine function sequentially based upon
 * the engine ordinal value.
 *
 * Here is an example of how to compute the bitmask in the DXGKARG_QUERYDEPENDENTENGINEGROUP.
 * DependentNodeOrdinalMask member. If the original values of the DXGKARG_QUERYDEPENDENTENGINEGROUP
 * structure's NodeOrdinal and EngineOrdinal members are 1 and 0, respectively,
 * and additional nodes with identifiers 2 and 4 will also be reset when node 1
 * is reset, the driver should set the binary value of DependentNodeOrdinalMask
 * to 10110, or 0x16 in hexadecimal notation. The index value EngineOrdinal is assumed
 * to be identical for all dependent nodes. The node being reset is included in
 * the DependentNodeOrdinalMask bit mask.
 *
 * This function should be made pageable, and it should always succeed.
 *
 * The operating system guarantees that this function follows the first level synchronization
 * mode as defined in Threading and Synchronization First Level.
 *
 * For more information, see TDR changes in Windows 8.
 */
NTSTATUS
LJB_DXGK_QueryDependentEngineGroup(
    _In_    const HANDLE                        hAdapter,
    _Inout_ DXGKARG_QUERYDEPENDENTENGINEGROUP * pQueryDependentEngineGroup
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    UINT CONST                          NodeOrdinal = pQueryDependentEngineGroup->NodeOrdinal;
    UINT CONST                          EngineOrdinal = pQueryDependentEngineGroup->EngineOrdinal;
    LJB_ENGINE_INFO *                   EngineInfo = &Adapter->EngineInfo[NodeOrdinal][EngineOrdinal];
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiQueryDependentEngineGroup)(
        hAdapter,
        pQueryDependentEngineGroup
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        return ntStatus;
    }

    /*
     * update EngineInfo->DependentNodeOrdinalMask
     */
    EngineInfo->DependentNodeOrdinalMask = pQueryDependentEngineGroup->DependentNodeOrdinalMask;
    DBG_PRINT(Adapter, DBGLVL_ERROR,
        ("?" __FUNCTION__ ": Node(0x%x),Engine(0x%x), DependentNodeOrdinalMask(0x%llx)\n",
        NodeOrdinal,
        EngineOrdinal,
        EngineInfo->DependentNodeOrdinalMask));
    return ntStatus;
}
