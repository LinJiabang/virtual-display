/*
 * ljb_dxgk_acquire_swizzling_range.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_AcquireSwizzlingRange)
#endif

/*
 * Function: LJB_DXGK_AcquireSwizzlingRange
 *
 * Description:
 * The DxgkDdiAcquireSwizzlingRange function makes an allocation accessible
 * through the central processing unit (CPU) aperture for the given segment.
 *
 * Return Value:
 * DxgkDdiAcquireSwizzlingRange returns one of the following values:
 *
 *  STATUS_SUCCESS: DxgkDdiAcquireSwizzlingRange successfully made the allocation
 *  accessible.
 *
 *  STATUS_GRAPHICS_UNSWIZZLING_APERTURE_UNSUPPORTED: DxgkDdiAcquireSwizzlingRange
 *  could not program the swizzling range for the allocation. The video memory
 *  manager fails to acquire the swizzling range without making futher attempts.
 *
 *  STATUS_GRAPHICS_UNSWIZZLING_APERTURE_UNAVAILABLE: DxgkDdiAcquireSwizzlingRange
 *  could not program the swizzling range for the allocation because another
 *  swizzling range is currently using the graphics processing unit (GPU) resources
 *  that are required. The video memory manager attempts to release a range that
 *  is currently in use and then attempts to set up the swizzling range again.
 *
 * Remarks:
 * The DxgkDdiAcquireSwizzlingRange function is called after the user-mode display
 * driver requests a virtual address that references the bits of an allocation
 * (that is, after the user-mode display driver calls the pfnLockCb function with
 * the AcquireAperture bit-field flag set in the Flags member of the D3DDDICB_LOCK
 * structure and while the allocation is currently located in a CPU-accessible memory
 * segment). If the AcquireAperture bit-field flag is not set in the call to
 * pfnLockCb, DxgkDdiAcquireSwizzlingRange is not called, and the allocation must
 * be in a format that the user-mode display driver or an application can process.
 *
 * When DxgkDdiAcquireSwizzlingRange is called, the display miniport driver must
 * make the specified allocation (that is, the hAllocation member of the
 * DXGKARG_ACQUIRESWIZZLINGRANGE structure that the pAcquireSwizzlingRange parameter
 * points to) accessible through the CPU aperture for the specified segment (that
 * is, the SegmentId member of DXGKARG_ACQUIRESWIZZLINGRANGE). The allocation must
 * appear exactly as it appears in computer memory after an unswizzling eviction.
 *
 * Not all of the requests by the user-mode display driver are accessible to the
 * display miniport driver. Swizzling ranges that the video memory manager acquired
 * are cached based on the allocation that they are associated with and the
 * specified private data. On the first request by the user-mode display driver
 * to access an allocation, the display miniport driver's DxgkDdiAcquireSwizzlingRange
 * function is called to make the allocation accessible. On subsequent requests
 * with matching private data, the previously setup mapping is used to access the
 * allocation.
 *
 * The video memory manager calls the display miniport driver's DxgkDdiReleaseSwizzlingRange
 * function to release a swizzling range when an allocation is evicted or destroyed,
 * or when another allocation requires an aperture. An allocation can be associated
 * with any number of swizzling ranges (for example, one aperture per MIP level).
 *
 * The number of swizzling ranges that an adapter supports is exposed by the
 * driver in the NumberOfSwizzlingRanges member of the DXGK_DRIVERCAPS structure
 * when the driver's DxgkDdiQueryAdapterInfo function is called. All ranges are
 * equal (that is, any range can unswizzle or untile any type of swizzling or
 * tiling). The video memory manager arbitrates the available swizzling ranges
 * among all of the applications that require them.
 *
 * The driver must use memory-mapped I/O (MMIO) to set up a swizzling range. These
 * swizzling-range accesses must not interfere with the execution of the GPU (that
 * is, the GPU must not be idle when DxgkDdiAcquireSwizzlingRange is called).
 *
 * All calls to DxgkDdiAcquireSwizzlingRange are serialized among themselves but
 * not with any other DDI function.
 *
 * If the GPU supports swizzling ranges that redirect CPU accesses to non-CPU-
 * accessible memory segments or system memory, the user-mode display driver can
 * set the combination of AcquireAperture and UseAlternateVA bit-field flags in
 * the Flags member of the D3DDDICB_LOCK structure in a call to the pfnLockCb
 * function to lock the allocation. In this situation, the video memory manager
 * calls the display miniport driver's DxgkDdiAcquireSwizzlingRange function to
 * acquire a swizzling range for the allocation even though the allocation is
 * located in a non-CPU-accessible memory segment or an aperture segment.
 * Swizzling ranges are associated with some GPU resources (for example,
 * PCI aperture ranges) that the driver manages and are not accessible to or
 * accounted for by the video memory manager.
 *
 * A call to DxgkDdiAcquireSwizzlingRange to acquire a swizzling range might fail
 * because a driver-managed resource ran out. Although the swizzling range itself
 * is free, it might not be usable because of the missing resource. The driver can
 * indicate that a swizzling range is unavailable to the video memory manager by
 * returning STATUS_GRAPHICS_UNSWIZZLING_APERTURE_UNAVAILABLE from
 * DxgkDdiAcquireSwizzlingRange. The video memory manager next attempts to release
 * a swizzling range that is currently in use and then calls the driver's
 * DxgkDdiAcquireSwizzlingRange function again to set up the new swizzling range.
 * If all of the swizzling ranges are released and the driver still fails with
 * STATUS_GRAPHICS_UNSWIZZLING_APERTURE_UNAVAILABLE, the video memory manager
 * fails to acquire a swizzling range for the allocation.
 *
 * DxgkDdiAcquireSwizzlingRange should be made pageable.
 */
NTSTATUS
LJB_DXGK_AcquireSwizzlingRange(
    _In_    const HANDLE                        hAdapter,
    _Inout_       DXGKARG_ACQUIRESWIZZLINGRANGE *pAcquireSwizzlingRange
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiAcquireSwizzlingRange)(
        hAdapter,
        pAcquireSwizzlingRange
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}