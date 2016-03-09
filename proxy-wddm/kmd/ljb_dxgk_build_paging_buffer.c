/*
 * ljb_dxgk_build_paging_buffer.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_BuildPagingBuffer)
#endif

/*
 * Function: LJB_DXGK_BuildPagingBuffer
 *
 * Description:
 * The DxgkDdiBuildPagingBuffer function builds paging buffers for memory operations.
 *
 * Return Value:
 * DxgkDdiBuildPagingBuffer returns one of the following values:
 *
 *  STATUS_SUCCESS: DxgkDdiBuildPagingBuffer successfully built a paging buffer.
 *
 *  STATUS_GRAPHICS_ALLOCATION_BUSY: The GPU is currently using the allocation for
 *  the paging buffer.
 *
 *  STATUS_GRAPHICS_INSUFFICIENT_DMA_BUFFER: More space is required in the paging
 *  buffer (that is, in the pDmaBuffer member of the DXGKARG_BUILDPAGINGBUFFER
 *  structure that the pBuildPagingBuffer parameter points to).
 *
 * Remarks:
 * The DxgkDdiBuildPagingBuffer function is called to build special purpose direct
 * memory access (DMA) buffers that are known as paging buffers. A paging buffer
 * contains an operation that moves the content of portions of allocations:
 * Within a segment of an allocation.
 * Between segments of allocations.
 * From a segment of an allocation to system memory.
 * From system memory to a segment of an allocation.
 * The display miniport driver must write the appropriate graphics processing
 * unit (GPU) instruction in the provided paging buffer (in the pDmaBuffer member
 * of DXGKARG_BUILDPAGINGBUFFER) according to the requested paging operation; and
 * then the driver must return the paging buffer back to the video memory manager
 * (which is part of Dxgkrnl.sys). The GPU scheduler (which is also part of Dxgkrnl.sys)
 * subsequently calls the driver's DxgkDdiSubmitCommand function to request that
 * the driver submit the paging buffer as a regular DMA buffer to the GPU.
 * Note  Before the video memory manager submits the paging buffer, it calls the
 * driver's DxgkDdiPatch function to assign (that is, patch) physical addresses
 * to the paging buffer; however, in the call to DxgkDdiPatch, the video memory
 * manager does not provide patch-location lists. The driver's DxgkDdiPatch
 * function can perform last-minute updates to the paging buffer; however, the
 * driver's DxgkDdiPatch function cannot change the size of the paging buffer.
 *
 * When the driver successfully builds the paging buffer, the driver's
 * DxgkDdiBuildPagingBuffer should update pDmaBuffer to point past the last byte
 * that is written to the paging buffer and then return STATUS_SUCCESS. Because
 * DxgkDdiBuildPagingBuffer can fail only if it runs out of space in the paging
 * buffer, the driver should always verify that the paging buffer has enough space
 * remaining before it writes to the buffer. If not enough space remains in the
 * paging buffer, the driver should return STATUS_GRAPHICS_INSUFFICIENT_DMA_BUFFER.
 * The video memory manager would then acquire a new paging buffer and call the
 * driver's DxgkDdiBuildPagingBuffer function again to fill the new paging buffer
 * according to the requested paging operation. Note that for a given requested
 * paging operation that fills multiple paging buffers, the scheduler calls the
 * driver's DxgkDdiSubmitCommand function multiple times for each partial paging
 * buffer to submit each buffer independently.
 *
 * If DxgkDdiBuildPagingBuffer determines that a paging operation requires more
 * than one paging buffer, the driver can specify information in the MultipassOffset
 * member of DXGKARG_BUILDPAGINGBUFFER and can use this information across multiple
 * iterations of the paging operation. The video memory manager initializes the
 * information in MultipassOffset to zero before the first paging operation request
 * and does not modify the information in MultipassOffset between iterations.
 * Therefore, the driver can use MultipassOffset to save the progress between
 * iterations. For example, the driver can store the page number that was last
 * transferred for a paged-based transfer.
 * A paging buffer is currently built for the following types of operations:
 *
 * Transfer
 * The transfer operation moves the content of an allocation from one location
 * to another. This operation is the most common type of memory operation.
 *
 * An allocation is always entirely transferred from one location to another.
 * However, because of memory constraints, the transfer of an allocation can be
 * divided into multiple sub-transfers (that is, a portion of the allocation is
 * moved from location A to B, and then the following portion is moved, and so
 * on, until the entire allocation is transferred). The first sub-transfer of an
 * allocation is marked with the TransferStart bit-field flag in the Flags member
 * of the Transfer member of DXGKARG_BUILDPAGINGBUFFER; the last sub-transfer of
 * an allocation is marked with the TransferEnd bit-field flag. The driver is
 * guaranteed to receive the end of a pending transfer (that is, the last sub-
 * transfer) before the driver receives the start of a new transfer.
 * Each sub-transfer might require multiple calls to DxgkDdiBuildPagingBuffer to
 * complete (for example, the driver might run out of DMA buffer space). Therefore,
 * the driver might receive the TransferStart flag in multiple calls to
 * DxgkDdiBuildPagingBuffer until the driver receives the TransferEnd flag in a
 * call to DxgkDdiBuildPagingBuffer. Receiving the TransferStart flag multiple
 * times does not indicate the start of multiple new transfers; it indicates that
 * the sub-transfers for the allocation require multiple iterations (for example,
 * if the driver ran out of DMA buffer space). The driver can use the MultipassOffset
 * member of DXGKARG_BUILDPAGINGBUFFER to keep track of the progress for a particular
 * sub-transfer across multiple iterations of DxgkDdiBuildPagingBuffer.
 * Typically, a transfer occurs in a single operation. In this situation, both
 * the TransferStart and TransferEnd bit-field flags are set.
 * In some scenarios, the driver might be required to set up hardware resources
 * when certain allocations are paged in or out of memory. By default, the GPU
 * might be using the allocation that is referenced during the call to
 * DxgkDdiBuildPagingBuffer. In these scenarios, the driver might require the
 * allocation to be idle before the driver programs the required hardware resources
 * (that is, programming the hardware resources cannot be queued in the provided
 * DMA buffer). For such scenarios, the driver can fail the call to
 * DxgkDdiBuildPagingBuffer with STATUS_GRAPHICS_ALLOCATION_BUSY.
 *
 * If the driver returns STATUS_GRAPHICS_ALLOCATION_BUSY, the video memory manager
 * waits until the GPU is done with any reference to the current allocation and
 * then calls the driver's DxgkDdiBuildPagingBuffer function again. In the second
 * call to DxgkDdiBuildPagingBuffer, the video memory manager sets the
 * AllocationIsIdle bit-field flag in the Flags member of the Transfer member of
 * DXGKARG_BUILDPAGINGBUFFER to indicate that the allocation that is being
 * referenced is idle. If the idle flag is not set, the driver should always
 * determine that the allocation is either currently busy or might soon become
 * busy. If the idle flag is set, the video memory manager guarantees that the
 * allocation that is being referenced remains idle for the duration of the call
 * to DxgkDdiBuildPagingBuffer.
 *
 * If the hAllocation member of DXGKARG_BUILDPAGINGBUFFER is NULL, the driver
 * should copy the data in the source to the destination without performing any
 * swizzling or tiling.
 *
 * Fill
 * The fill operation fills an allocation with a specified pattern. The fill
 * operation is used to set up the initial content of an allocation. When the
 * content of the allocation is filled, the allocation is guaranteed to be idle
 * (that is, not in use by the GPU). The fill operation can be performed only on
 * a memory segment. The video memory manager never requests that the display
 * miniport driver fill an aperture segment.
 *
 * Discard content
 * The discard-content operation notifies the driver that an allocation is
 * discarded from the allocation's current location in a memory segment. That is,
 * the allocation is evicted and not copied back to system memory.
 *
 * In some scenarios, the driver might be required to set up hardware resources
 * when certain allocations are paged in or out of memory. By default, the GPU
 * might use the allocation that is referenced during the call to
 * DxgkDdiBuildPagingBuffer. In these scenarios, the driver might require the
 * allocation to be idle before the driver programs the required hardware resources
 * (that is, programming the hardware resources cannot be queued in the provided
 * DMA buffer). For such scenarios, the driver can fail the call to
 * DxgkDdiBuildPagingBuffer with STATUS_GRAPHICS_ALLOCATION_BUSY.
 *
 * If the driver returns STATUS_GRAPHICS_ALLOCATION_BUSY, the video memory manager
 * waits until the GPU is done with any reference to the current allocation and
 * then calls the driver's DxgkDdiBuildPagingBuffer function again. In the second
 * call to DxgkDdiBuildPagingBuffer, the video memory manager sets the
 * AllocationIsIdle bit-field flag in the Flags member of the DiscardContent member
 * of the DXGKARG_BUILDPAGINGBUFFER structure to indicate that the allocation that
 * is being referenced is idle. If the idle flag is not set, the driver should
 * always determine that the allocation is either currently busy or might soon
 * become busy. If the idle flag is set, the video memory manager guarantees that
 * the allocation that is being referenced remains idle for the duration of the
 * call to DxgkDdiBuildPagingBuffer.
 *
 * Read physical
 * The read-physical operation reads from a specified physical memory address.
 * The driver is requested to program the GPU for the operation. The size of the
 * physical memory to access for the read can be from 1 byte through 8 bytes.
 * Because the data that is read is irrelevant, DxgkDdiBuildPagingBuffer is not
 * required to return the data. However, in scenarios where the CPU attempts to
 * read from AGP memory after the GPU writes to that AGP memory, the read-physical
 * operation is critical to ensure memory coherency.
 *
 * Write physical
 * The write-physical operation writes to a specified physical address. The driver
 * is requested to program the GPU for the operation. The size of the physical
 * memory to access for the write operation can be from 1 byte through 8 bytes.
 * Because the data that is written is irrelevant, DxgkDdiBuildPagingBuffer can
 * write any data to the memory. However, in scenarios where the CPU attempts to
 * read from AGP memory after the GPU writes to that AGP memory, the write-physical
 * operation is critical to ensure memory coherency.
 *
 * Map aperture segment
 * The map-aperture-segment operation maps a specified memory descriptor list
 * (MDL) into a specified aperture segment at a specified segment offset for a
 * specified number of pages. If the CacheCoherent bit-field flag is set in the
 * Flags member of the MapApertureSegment member of the DXGKARG_BUILDPAGINGBUFFER
 * structure, the driver must ensure that cache coherency is enforced on the pages
 * that are mapped; otherwise, cache coherency is not required for the pages that
 * are mapped.
 *
 * Note  The CacheCoherent bit-field flag is set only when cacheable memory is
 * being mapped into a cache-coherent aperture segment and is never set on a
 * non-cache-coherent aperture segment or on a write-combined allocation that is
 * mapped into a cache-coherent segment.
 *
 * The driver can optionally use memory-mapped I/O (MMIO) to configure an aperture
 * segment. The GPU will not be accessing the aperture range at configuration time.
 * However, this aperture configuration must not interfere with the execution of
 * the GPU. The GPU will not be idle when DxgkDdiBuildPagingBuffer is called with
 * the DXGK_OPERATION_MAP_APERTURE_SEGMENT operation type set, and the GPU might
 * be busy accessing other portions of the aperture segment that is being
 * reconfigured.
 *
 * Unmap aperture segment
 * The unmap-aperture-segment operation unmaps a previously mapped range of a
 * specified aperture segment. The driver must map the range that is unmapped to
 * the dummy page that the DummyPage member of the UnmapApertureSegment member of
 * the DXGKARG_BUILDPAGINGBUFFER structure specifies.
 * Note  When the driver unmaps to the dummy page, the driver must enable GPU
 * accesses through the specified aperture range so the DirectX graphics kernel
 * subsystem can detect corruption issues. Conformance tests exist to check this
 * situation.
 *
 * The video memory manager uses the dummy page that is in the unmapped portion
 * of the aperture to determine difficulties the memory manager has accessing the
 * aperture segment.
 * The driver can optionally use MMIO to configure an aperture segment. The GPU
 * will not be accessing the aperture range at configuration time. However, this
 * aperture configuration must not interfere with the execution of the GPU. The
 * GPU will not be idle when DxgkDdiBuildPagingBuffer is called with the
 * DXGK_OPERATION_UNMAP_APERTURE_SEGMENT operation type set, and the GPU might
 * be busy accessing other portions of the aperture segment that is being
 * reconfigured.
 *
 * Special-lock transfer
 * The special-lock-transfer operation is similar to the regular transfer operation.
 * However, instead of transferring the content of the allocation from or to the
 * allocation's regular backing store, the special-lock-transfer operation transfers
 * the content of the allocation from or to the alternate virtual address that was
 * set up for the allocation when the pfnLockCb function was called with the
 * UseAlternateVA bit-field flag set.
 * The special-lock-transfer operation occurs only in one of the following scenarios:
 * The allocation is currently CPU-accessible with an alternate virtual address
 * and is being evicted.
 * An allocation that was previously evicted, such as the situation that is
 * described in the preceding bullet, is being paged back in.
 * Drivers that do not support the use of the UseAlternateVA bit-field flag will
 * not be called to perform a special-lock-transfer operation.
 *
 * In some scenarios, the driver might be required to set up hardware resources
 * when certain allocations are paged in or out of memory. By default, the GPU
 * might be using the allocation that is referenced during the call to
 * DxgkDdiBuildPagingBuffer. In these scenarios, the driver might require the
 * allocation to be idle before the driver programs the required hardware resources
 * (that is, programming the hardware resources cannot be queued in the provided
 * DMA buffer). For such scenarios, the driver can fail the call to
 * DxgkDdiBuildPagingBuffer with STATUS_GRAPHICS_ALLOCATION_BUSY.
 *
 * If the driver returns STATUS_GRAPHICS_ALLOCATION_BUSY, the video memory manager
 * waits until the GPU is done with any reference to the current allocation and
 * then calls the driver's DxgkDdiBuildPagingBuffer function again. In the second
 * call to DxgkDdiBuildPagingBuffer, the video memory manager sets the AllocationIsIdle
 * bit-field flag in the Flags member of the SpecialLockTransfer member of the
 * DXGKARG_BUILDPAGINGBUFFER structure to indicate that the allocation that is
 * being referenced is idle. If the idle flag is not set, the driver should always
 * determine that the allocation is either currently busy or might soon become
 * busy. If the idle flag is set, the video memory manager guarantees that the
 * allocation that is being referenced remains idle for the duration of the call
 * to DxgkDdiBuildPagingBuffer.
 *
 * Note that if the driver must use a hardware aperture to linearize a swizzled
 * allocation that an application can directly access, the driver must unswizzle
 * that allocation while the driver transfers the allocation to system memory to
 * maintain the coherency of the allocation's virtual address. The driver must
 * unswizzle the allocation because an eviction might occur while the application
 * is accessing the allocation.
 *
 * The system's memory manager ensures that the transfer is invisible to the
 * application. However, because the allocation is in system memory and the
 * allocation's virtual address can no longer go through the hardware aperture,
 * the driver must ensure the byte ordering into system memory matches what was
 * visible through the aperture.
 *
 * DxgkDdiBuildPagingBuffer should be made pageable.
 */
NTSTATUS
LJB_DXGK_BuildPagingBuffer(
    _In_ const HANDLE                    hAdapter,
    _In_       DXGKARG_BUILDPAGINGBUFFER *pBuildPagingBuffer
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiBuildPagingBuffer)(
        hAdapter,
        pBuildPagingBuffer
        );
    if (!NT_SUCCESS(ntStatus) && ntStatus != STATUS_GRAPHICS_INSUFFICIENT_DMA_BUFFER)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}