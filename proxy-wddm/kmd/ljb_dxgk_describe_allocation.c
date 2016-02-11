/*
 * ljb_dxgk_describe_allocation.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_DescribeAllocation)
#endif

/*
 * Function: LJB_DXGK_DescribeAllocation
 *
 * Description:
 * The DxgkDdiDescribeAllocation function retrieves information about an existing
 * allocation that is not otherwise available to the Microsoft DirectX graphics
 * kernel subsystem.
 *
 * Return Value:
 * DxgkDdiDescribeAllocation returns STATUS_SUCCESS if it succeeds; otherwise,
 * it returns one of the error codes defined in Ntstatus.h.
 *
 * Remarks:
 * Because the DirectX graphics kernel subsystem does not necessarily maintain
 * records of allocations, the graphics kernel subsystem calls DxgkDdiDescribeAllocation
 * to request that the display miniport driver return information about existing
 * allocations. Currently, the display miniport driver must be able to return this
 * information for the following allocations:
 *
 *   Allocations that are also primaries (that is, allocations that the user-mode
 *   display driver created by setting the Primary bit-field flag in the Flags
 *   member of the D3DDDI_ALLOCATIONINFO structure in a call to the pfnAllocateCb
 *   function).
 *
 *   Allocations that might be the source of a presentation (that is, allocations
 *   that are represented by the hSource member of the DXGKARG_PRESENT structure
 *   in a call to the display miniport driver's DxgkDdiPresent function).
 *
 * DxgkDdiDescribeAllocation should be made pageable.
 */
NTSTATUS
LJB_DXGK_DescribeAllocation(
     _In_    const HANDLE                     hAdapter,
    _Inout_       DXGKARG_DESCRIBEALLOCATION *pDescribeAllocation
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiDescribeAllocation)(
        hAdapter,
        pDescribeAllocation
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}