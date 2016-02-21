/*
 * ljb_dxgk_get_node_meta_data.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

/*
 * Function: LJB_DXGK_GetNodeMetadata
 *
 * Description:
 * From a provided adapter handle, returns the engine type and friendly name of
 * an engine on a specified GPU node. Must be implemented by Windows Display
 * Driver Model (WDDM) 1.3 and later display miniport drivers.
 *
 * Return Value:
 * Returns one of the following values:
 *   STATUS_SUCCESS: DxgkDdiGetNodeMetadata successfully retrieved the engine information.
 *   STATUS_INVALID_PARAMETER:The caller-provided hAdapter or pGetNodeMetadata
 *   parameters are invalid, or the caller-provided value of NodeOrdinal is greater
 *   than or equal to the number of nodes on the adapter.
 *
 * If the hAdapter and pGetNodeMetadata parameters are valid, and NodeOrdinal has
 * a value in the range of 0 to (number of nodes - 1), all calls to this function
 * must be successful.
 *
 * Remarks:
 * For more information on how to implement this function, see Enumerating GPU
 * engine capabilities.
 */
NTSTATUS
LJB_DXGK_GetNodeMetadata(
    _In_ const HANDLE                   hAdapter,
    _In_       UINT                     NodeOrdinal,
    _Out_      DXGKARG_GETNODEMETADATA *pGetNodeMetadata
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    ntStatus = (*DriverInitData->DxgkDdiGetNodeMetadata)(
        hAdapter,
        NodeOrdinal,
        pGetNodeMetadata
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    Adapter->NodeMetaData[NodeOrdinal] = *pGetNodeMetadata;
    DBG_PRINT(Adapter, DBGLVL_FLOW,
        (__FUNCTION__
        ": NodeOrdinal(%u), EngineType(%u), FriendlyName(%s), GpuMmuSupported(%u), IoMmuSupported(%u)\n",
        NodeOrdinal,
        pGetNodeMetadata->EngineType,
        pGetNodeMetadata->FriendlyName,
        pGetNodeMetadata->GpuMmuSupported,
        pGetNodeMetadata->IoMmuSupported
        ));

    return ntStatus;
}
