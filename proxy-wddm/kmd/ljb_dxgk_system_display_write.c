/*
 * ljb_dxgk_system_display_write.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

/*
 * Function: LJB_DXGK_SystemDisplayWrite
 *
 * Description:
 * Called by the operating system to request the display miniport driver to write
 * an image block to the display device.
 *
 * Starting with Windows 8, the operating system calls this function during a bugcheck
 * operation following a system stop error. The operating system calls this function
 * only if the display device was previously reset through a call to DxgkDdiSystemDisplayEnable.
 *
 * Return value
 * This routine does not return a value.
 *
 * Remarks
 * Source image restrictions
 * The display miniport driver must follow these guidelines when its DxgkDdiSystemDisplayWrite
 * function is called:
 *   The color format of the source image is always in the D3DDDIFMT_R8G8B8 (24
 *   bits per pixel) or D3DDDIFMT_A8R8G8B8 (32 bpp) formats of the D3DDDIFORMAT
 *   enumeration. The display miniport driver had previously set the display mode
 *   to enable write operations in this format when its DxgkDdiSystemDisplayEnable
 *   was called.
 *   The source image is in non-paged memory. The display miniport driver should
 *   write this source image to the current frame buffer starting at the positions
 *   specified by the PostionX and PositionY parameters.
 *   The display miniport driver should use the CPU to write the image block to
 *   the frame buffer. When the system encounters a stop error, it may have been
 *   caused by a continuous timeout detection and recovery (TDR) on the display
 *   device. In that case, the graphics processing unit (GPU) might be in an unknown
 *   state.
 *
 * For more information about TDR, see Timeout Detection and Recovery (TDR).
 *
 * Use non-paged memory
 * Windows kernel-mode functions might not be available while this function is
 * being called. DxgkDdiSystemDisplayWrite can be called at any IRQL, so it must
 * be in nonpageable memory. DxgkDdiSystemDisplayWrite must not call any code that
 * is in pageable memory and must not manipulate any data that is in pageable memory.
 */
VOID
LJB_DXGK_SystemDisplayWrite(
    _In_ PVOID MiniportDeviceContext,
    _In_ PVOID Source,
    _In_ UINT  SourceWidth,
    _In_ UINT  SourceHeight,
    _In_ UINT  SourceStride,
    _In_ UINT  PositionX,
    _In_ UINT  PositionY
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(MiniportDeviceContext);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;

    /*
     * pass the call to inbox driver
     */
    (*DriverInitData->DxgkDdiSystemDisplayWrite)(
        MiniportDeviceContext,
        Source,
        SourceWidth,
        SourceHeight,
        SourceStride,
        PositionX,
        PositionY
        );
}