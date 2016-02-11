/*
 * ljb_dxgk_get_std_allocation_drv_data.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_GetStdAllocationDrvData)
#endif

/*
 * Function: LJB_DXGK_GetStdAllocationDrvData
 *
 * Description:
 * The DxgkDdiGetStandardAllocationDriverData function returns a description of
 * a standard allocation type.
 *
 * Return Value:
 * DxgkDdiGetStandardAllocationDriverData returns one of the following values:
 *
 *  STATUS_SUCCESS: DxgkDdiGetStandardAllocationDriverData successfully returned
 *  a description of the standard allocation type.
 *
 *  STATUS_NO_MEMORY: DxgkDdiGetStandardAllocationDriverData could not allocate
 *  memory that was required for it to complete.
 *
 * Remarks:
 * Standard allocation types are allocations that must be created in kernel mode
 * without communication from the user-mode display driver. The DirectX graphics
 * kernel subsystem calls the DxgkDdiGetStandardAllocationDriverData function to
 * generate a description of the standard allocation type that the
 * pGetStandardAllocationDriverData parameter specifies. The display miniport
 * driver returns the description of the allocation type in the pAllocationPrivateDriverData
 * and pResourcePrivateDriverData members of the DXGKARG_GETSTANDARDALLOCATIONDRIVERDATA
 * structure that the pGetStandardAllocationDriverData parameter points to. The
 * DirectX graphics kernel subsystem subsequently passes the description to the
 * DxgkDdiCreateAllocation function to actually create the allocation.
 *
 * Beginning with Windows 7, if a display miniport driver processes a call to the
 * DxgkDdiGetStandardAllocationDriverData function to create allocations for GDI
 * hardware acceleration, the driver should set the pitch of the allocation for
 * CPU visible allocations, pGetStandardAllocationDriverData->pCreateGdiSurfaceData->Pitch.
 *
 * DxgkDdiGetStandardAllocationDriverData should be made pageable.
 */
NTSTATUS
LJB_DXGK_GetStdAllocationDrvData(
    _In_    const HANDLE                                  hAdapter,
    _Inout_       DXGKARG_GETSTANDARDALLOCATIONDRIVERDATA *pGetStandardAllocationDriverData
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiGetStandardAllocationDriverData)(
        hAdapter,
        pGetStandardAllocationDriverData
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}