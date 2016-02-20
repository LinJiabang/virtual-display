/*
 * ljb_dxgk_present.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_Present)
#endif

/*
 * Function: LJB_DXGK_Present
 *
 * Description:
 * The DxgkDdiPresent function copies content from source allocations to a primary
 * surface (and sometimes to off-screen system memory allocations).
 *
 * Return Value:
 * DxgkDdiPresent returns one of the following values:
 *
 *  STATUS_SUCCESS: DxgkDdiPresent successfully copied the content.
 *
 *  STATUS_NO_MEMORY or STATUS_INSUFFICIENT_RESOURCES: DxgkDdiPresent could not
 *  allocate memory that was required for it to complete.
 *
 *  STATUS_GRAPHICS_INSUFFICIENT_DMA_BUFFER: The current DMA buffer is depleted.
 *
 *  STATUS_GRAPHICS_CANNOTCOLORCONVERT: The display miniport driver detected a
 *  bit-block transfer (bitblt) for color conversion that the device could not
 *  perform. The Microsoft Direct3D runtime prevents the application from continuing,
 *  and the application receives a failure to copy content.
 *
 *  STATUS_PRIVILEGED_INSTRUCTION: DxgkDdiPresent detected nonprivileged instructions
 *  (that is, instructions that access memory beyond the privilege of the current
 *  central processing unit [CPU] process).
 *
 *  STATUS_ILLEGAL_INSTRUCTION: DxgkDdiPresent detected instructions that graphics
 *  hardware could not support.
 *
 *  STATUS_INVALID_PARAMETER: DxgkDdiRender detected instruction parameters that
 *  graphics hardware could not support; however, the graphics hardware can support
 *  the instructions themselves. The driver is not required to return this error
 *  code. Instead, it can return STATUS_ILLEGAL_INSTRUCTION when it detects
 *  unsupported instruction parameters.
 *
 *  STATUS_INVALID_HANDLE: DxgkDdiPresent detected an invalid handle in the
 *  command buffer.
 *
 *  STATUS_GRAPHICS_GPU_EXCEPTION_ON_DEVICE: The display miniport driver detected
 *  an error in the DMA stream. The graphics context device is placed in a lost
 *  state if the driver returns this error code.
 *
 * Remarks:
 * The DirectX graphics kernel subsystem calls the display miniport driver's
 * DxgkDdiPresent function to copy content from source allocations typically to
 * the primary surface. (This function can also copy content to an off-screen
 * system memory allocation.) Because a primary surface is loosely defined,
 * DxgkDdiPresent can be implemented for the following scenarios:
 *
 * Depending on the position of the window, the DxgkDdiPresent function must be
 * performed across different primaries that can be on the same adapter or across
 * different adapters.
 *
 * The primary is on a remote monitor and is accessed through a terminal services
 * client or Microsoft NetMeeting.
 *
 * A mode switch recently occurred and the primary format is different from the
 * source format, so a color conversion is needed. In addition, the DxgkDdiPresent
 * operation can be clipped because of window clipping and ordering.
 *
 * Because the preceding scenarios can change asynchronously, the user-mode display
 * driver cannot compile hardware instructions for the display miniport driver's
 * DxgkDdiPresent function in advance. The display miniport driver must create
 * hardware commands for the actual DxgkDdiPresent operation, and they must be placed
 * in an output DMA buffer. After the display miniport driver's DxgkDdiPresent
 * function is called to generate the DMA buffer, the operating system guarantees
 * that a scenario change will not occur before that buffer is rendered.
 *
 * The display miniport driver is not required to be aware of the specifics of
 * the preceding scenarios as long as the driver supports the following abstractions:
 *
 * In a copy operation from a video memory source to a primary video or system
 * memory destination, a copy from an off-screen system memory source to the
 * primary destination, a copy from and to the primary, or a copy from the primary
 * source to an off-screen system memory destination, the source is specified by
 * the hDeviceSpecificAllocation member of the pAllocationList[DXGK_PRESENT_SOURCE_INDEX]
 * array element of the DXGKARG_PRESENT structure that the pPresent parameter of
 * DxgkDdiPresent points to. The destination, which is either the current primary
 * of the device or an off-screen system memory allocation, is specified by the
 * hDeviceSpecificAllocation member of the pAllocationList[DXGK_PRESENT_DESTINATION_INDEX]
 * array element of DXGKARG_PRESENT. If the destination equals the source (that
 * is, destination == source), the copy operation is a screen-to-screen bit-block
 * transfer (bitblt). Therefore, the graphics subsystem sets the source and destination
 * to the following values:
 *
 *  destination != NULL (that is, destination == nonNULL)
 *
 *  source != NULL (that is, source == nonNULL)
 *
 *  In a video memory flip from the current allocation to another allocation, the
 *  source can be specified by the operating system and set in the hDeviceSpecificAllocation
 *  member of the pAllocationList[DXGK_PRESENT_SOURCE_INDEX] array element of
 *  DXGKARG_PRESENT. The graphics subsystem sets the source and destination to
 *  the following values:
 *  destination == NULL
 *  source != NULL (that is, source == nonNULL)
 *  Note   A no-op flip can be performed from the same source allocation as the
 *  currently scanned-out allocation. A no-op flip is used to insert a queued
 *  wait for a vertical blank in the rendering stream. The display miniport driver
 *  should insert a hardware flip command as if it were flipping to another allocation.
 *
 * In a color-fill operation to the primary surface, no source allocation is
 * required and the destination is a primary allocation handle that is specified
 * by the hDeviceSpecificAllocation member of the pAllocationList[DXGK_PRESENT_DESTINATION_INDEX]
 * array element of DXGKARG_PRESENT. The Color member of DXGKARG_PRESENT is typically
 * in the D3DDDIFMT_A8R8G8B8 format from the D3DDDIFORMAT enumeration type. However,
 * when the primary format is palettized RGB, Color contains the palette index.
 * Therefore, the graphics subsystem sets the source and destination to the following
 * values:
 * destination != NULL (that is, destination == nonNULL)
 * source == NULL
 *
 * For all of the DxgkDdiPresent scenarios to operate correctly, the display miniport
 * driver's DxgkDdiCreateDevice function should set the DmaBufferSize member of
 * the DXGK_DEVICEINFO structure to be large enough to hold the hardware commands
 * that are needed to present at least one RECT rectangle to the display or off-
 * screen target. However, the driver's DxgkDdiPresent function can return
 * STATUS_GRAPHICS_INSUFFICIENT_DMA_BUFFER if the number of subrectangles in the
 * DxgkDdiPresent scenario depletes the current DMA buffer and the driver requires
 * another DMA buffer to continue.
 *
 * The graphics subsystem then acquires a new DMA buffer and calls the driver's
 * DxgkDdiPresent function again with the same list of RECT structures as the
 * previous DxgkDdiPresent call. The driver must use the MultipassOffset member
 * of the DXGKARG_PRESENT structure that is pointed to by pPresent to record the
 * amount of progress made in completing the RECT list in the previous call to
 * DxgkDdiPresent so that the driver can continue from where it stopped with the
 * new DMA buffer. When the driver's DxgkDdiPresent function completes the list
 * of RECT structures, it returns STATUS_SUCCESS.
 *
 * In addition to generating a DMA buffer, the display miniport driver must generate
 * a patch-location list that indicates the various offsets within the DMA buffer
 * that must be patched later when physical addresses for allocations are known.
 * At times, the video memory manager provides the driver with pre-patched information
 * (that is, the last know physical addresses for the source and destination) in
 * the allocation list.
 *
 * When the video memory manager provides this information, the driver must
 * generate the DMA buffer by determining that these physical addresses are the
 * final addresses that the DirectX graphics kernel subsystem will provide. The
 * graphics subsystem might not call the DxgkDdiPatch function on the DMA buffer
 * to patch it again later. Therefore, the driver must use the pre-patch information
 * to generate the DMA buffer properly. Pre-patched information is provided for
 * element N when the SegmentId member of the Nth element of the pAllocationList
 * array of DXGKARG_PRESENT is nonzero.
 *
 * Note   Even though the driver's DxgkDdiPresent function pre-patches the DMA
 * buffer, the driver must still insert all of the references to allocations
 * into the output patch-location list that the pPatchLocationListOut member of
 * DXGKARG_PRESENT specifies. The driver must insert these references because the
 * addresses of the allocations might change before the DMA buffer is submitted
 * to the GPU; therefore, the DirectX graphics kernel subsystem will call the
 * DxgkDdiPatch function to repatch the DMA buffer.
 *
 * If the driver supports rotation (that is, reports support for rotated modes
 * in the RotationSupport member of the D3DKMDT_VIDPN_PRESENT_PATH_TRANSFORMATION
 * structure in a call to its DxgkDdiEnumVidPnCofuncModality function), the driver
 * must be able to perform rotated bit-block transfers (bitblt) from source to
 * destination. When the Rotate bit-field flag is specified in the DXGK_PRESENTFLAGS
 * structure for the Flags member of DXGKARG_PRESENT, the driver should apply the
 * rotation as if going from a non-rotated surface to the final orientation of the
 * current source.
 *
 * The primary allocation of a source is specified in the DxgkDdiCommitVidPn
 * function. If multiple paths originate from the given source (clone mode), the
 * display miniport driver must ensure that the outputs are correctly rotated given
 * the path rotation mode for the different targets. All of the parameters that
 * are supplied to DxgkDdiPresent are rotation agnostic. The source and target
 * rectangles could both be the entire screen as clients perceive it (for example,
 * 768 x 1024).
 *
 * Note   This situation does not address full-screen Direct3D applications in
 * rotated mode.
 *
 * If the display miniport driver previously indicated, in a call to its
 * DxgkDdiQueryAdapterInfo function, that it supports a memory mapped I/O (MMIO)-
 * based flip (by setting the FlipOnVSyncMmIo bit-field flag in the FlipCaps member
 * of the DXGK_DRIVERCAPS structure to TRUE), the driver's DxgkDdiPresent function
 * is subsequently called with the pDmaBuffer member of DXGKARG_PRESENT set to NULL
 * because a MMIO-based flip does not require a DMA buffer to run on the GPU. Instead,
 * the driver's DxgkDdiPresent function must validate the source surface and program
 * flip hardware as required. The DirectX graphics kernel subsystem calls the
 * driver's DxgkDdiSetVidPnSourceAddress function to run this type of flip.
 *
 * DxgkDdiPresent should be made pageable.
 */
NTSTATUS
LJB_DXGK_Present(
     _In_    const HANDLE          hContext,
    _Inout_       DXGKARG_PRESENT *pPresent
    )
{
    LJB_CONTEXT * CONST                 MyContext = LJB_DXGK_FindContext(hContext);
    LJB_ADAPTER * CONST                 Adapter = MyContext->Adapter;
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiPresent)(hContext, pPresent);
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}
