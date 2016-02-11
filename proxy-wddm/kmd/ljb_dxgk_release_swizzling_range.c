/*
 * ljb_dxgk_release_swizzling_range.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_ReleaseSwizzlingRange)
#endif

/*
 * Function: LJB_DXGK_ReleaseSwizzlingRange
 *
 * Description:
 * The DxgkDdiReleaseSwizzlingRange function releases a swizzling range that the
 * DxgkDdiAcquireSwizzlingRange function previously set up.
 *
 * Return Value:
 * DxgkDdiReleaseSwizzlingRange returns STATUS_SUCCESS, or an appropriate error
 * result if the swizzling range is not successfully released.
 *
 * Remarks:
 * The DxgkDdiReleaseSwizzlingRange function is typically called when the
 * specified allocation (that is, the hAllocation member of the
 * DXGKARG_RELEASESWIZZLINGRANGE structure that the pReleaseSwizzlingRange parameter
 * points to) is evicted or destroyed, or when another allocation requires the
 * swizzling range that the RangeId member of DXGKARG_RELEASESWIZZLINGRANGE
 * specifies.
 * If the specified allocation is currently associated with multiple swizzling ranges
 * (through calls to the DxgkDdiAcquireSwizzlingRange function), the display miniport
 * driver should release only the swizzling range that the RangeId member of
 * DXGKARG_RELEASESWIZZLINGRANGE specifies. If the display miniport driver
 * releases all of the swizzling ranges that are associated with the allocation,
 * random corruption in the allocation might result because an application might
 * currently be using one or more of the swizzling ranges.
 * The driver must use memory-mapped I/O (MMIO) to set up a swizzling range.
 * These swizzling-range accesses must not interfere with the execution of the
 * GPU (that is, the GPU must not be idle when DxgkDdiReleaseSwizzlingRange is
 * called).
 * All calls to DxgkDdiReleaseSwizzlingRange are serialized among themselves but
 * not with any other device driver interface (DDI) function.
 *
 * DxgkDdiReleaseSwizzlingRange should be made pageable.
 */
NTSTATUS
LJB_DXGK_ReleaseSwizzlingRange(
    _In_    const HANDLE                     hAdapter,
    _In_ const DXGKARG_RELEASESWIZZLINGRANGE *pReleaseSwizzlingRange
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiReleaseSwizzlingRange)(
        hAdapter,
        pReleaseSwizzlingRange
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}