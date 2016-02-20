/*
 * ljb_dxgk_system_display_enable.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

/*
 * Function: LJB_DXGK_SystemDisplayEnable
 *
 * Description:
 * Called by the operating system to request the display miniport driver to reset
 * the current display device to a specified state.
 *
 * Starting with Windows 8, the operating system calls this function during a
 * bugcheck operation following a system stop error.
 *
 * Return value
 * DxgkDdiSystemDisplayEnable returns STATUS_SUCCESS if it succeeds. If the target
 * specified by the TargetId parameter is not connected to a display device, the
 * function returns STATUS_NOT_SUPPORTED. Otherwise, it returns one of the error
 * codes defined in Ntstatus.h.
 *
 * Remarks
 * Required steps by display miniport driver
 * The display miniport driver must follow these steps when its DxgkDdiSystemDisplayEnable
 * function is called:
 *
 * 1. The driver must cancel all graphics processing unit (GPU) operations or reset
 * the GPU to the idle state.
 * 2. The operating system indicates the video present target through the TargetId
 *  parameter. The driver must keep the display associated with this target powered
 * on and visible. If the driver cannot power on the display, it must fail the call
 * to this function. In such a failure case, the operating system might call the
 * DxgkDdiResetDevice function and cause a system bugcheck to occur.
 * 3. The driver must check the connectivity of the display associated with this
 * target. If the target does not have a display connected, the driver must complete
 * the call to this function and return the STATUS_NOT_SUPPORTED error code.
 * 4. The driver must disable the signal to all other displays that are connected
 * to the display adapter. If this is not possible, the driver should attempt to
 * place a blank image on all other displays. If this is not possible, the driver
 * must leave the last image on the screen unchanged.
 * 5. The driver must keep the current display mode on the indicated target and
 * provide this mode back to the operating system as part of this function call.
 * 6. If the driver cannot maintain the current display mode, or if the target is
 * not part of the active topology, the driver should attempt to set a frame buffer
 * on another target that is capable of a display resolution of at least 640 x 480
 * pixels in a format of 24 bits per pixel. If that is not possible, the driver
 * can fail this function call, which will result in a system bugcheck and the display
 * of a black screen.
 *
 * It is not required that the driver use a linear frame buffer mode. However, the
 * driver should support write operations to this frame buffer from sources that
 * have the D3DDDIFMT_A8R8G8B8 format of the D3DDDIFORMAT enumeration.
 *
 * Source image restrictions
 * After the driver gives the operating system control over display functionality,
 * the operating system can call the DxgkDdiSystemDisplayWrite function to update
 * the screen image and to write a block of images from specified sources to the
 * screen that was reset by the DxgkDdiSystemDisplayEnable function.
 *
 * DxgkDdiSystemDisplayWrite provides the driver with the starting address of the
 * source image as well as the stride, width, and height. The color format of the
 * source image is always D3DDDIFMT_X8R8G8B8. The operating system guarantees that
 * the source image is in non-paged memory.
 *
 * The driver must write this source image to the current screen starting at the
 * positions specified by the PositionX and PositionY parameters of the DxgkDdiSystemDisplayWrite
 * function.
 *
 * It is recommended that the driver use the CPU to write the image from the source
 * to the frame buffer because a system bugcheck might be caused by repeated Timeout
 * Detection and Recovery (TDR) instances that result in the GPU being in an unknown
 * condition.
 *
 * Use non-paged memory
 * Windows kernel-mode functions might not be available while this function is being
 * called.
 *
 * DxgkDdiSystemDisplayEnable can be called at any IRQL, so it must be in nonpageable
 * memory. DxgkDdiSystemDisplayEnable must not call any code that is in pageable
 * memory and must not manipulate any data that is in pageable memory.
 */
NTSTATUS
LJB_DXGK_SystemDisplayEnable(
    _In_  const PVOID                           MiniportDeviceContext,
    _In_  D3DDDI_VIDEO_PRESENT_TARGET_ID        TargetId,
    _In_  PDXGKARG_SYSTEM_DISPLAY_ENABLE_FLAGS  Flags,
    _Out_ UINT                                  *Width,
    _Out_ UINT                                  *Height,
    _Out_ D3DDDIFORMAT                          *ColorFormat
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(MiniportDeviceContext);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    /*
     * pass the call to inbox driver
     */
    ntStatus = (*DriverInitData->DxgkDdiSystemDisplayEnable)(
        MiniportDeviceContext,
        TargetId,
        Flags,
        Width,
        Height,
        ColorFormat
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        return ntStatus;
    }
    return ntStatus;
}