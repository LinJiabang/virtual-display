/*
 * ljb_dxgk_stop_device_and_release_post_display_ownership.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_StopDeviceAndReleasePostDisplayOwnership)
#endif

/*
 * Function: LJB_DXGK_StopDeviceAndReleasePostDisplayOwnership
 *
 * Description:
 * Called by the operating system to request the display miniport driver to reset
 * the display device and to release ownership of the current power-on self-test
 * (POST) device.
 *
 * Starting with Windows 8, the operating system calls this function during a Plug
 * and Play (PnP) stop operation.
 *
 * To indicate to the operating system that this function is supported, the driver
 * must set the NonVGASupport member of the DXGK_DRIVERCAPS structure when the
 * DxgkDdiQueryAdapterInfo function is called.
 *
 * Return value
 * Returns STATUS_SUCCESS if it succeeds. Otherwise, it returns one of the error
 * codes defined in Ntstatus.h. For more information, see the following Remarks
 * section.
 *
 * Remarks
 * Allowed color formats
 * The display miniport driver should report only a 32-bit color format. Therefore
 * the DisplayInfo->ColorFormat member must include only one of the following two
 * formats:
 * D3DDDIFMT_X8R8G8B8
 * D3DDDIFMT_A8R8G8B8
 *
 * Video present target initialization
 * The display miniport driver must set the DisplayInfo->TargetId member to the
 * target identifier of the display that remains active. Typically, this identifier
 * will be the value of the TargetId parameter that the operating system passed
 * to the driver.
 *
 * Similarly, the display miniport driver must set the DisplayInfo->AcpiId member
 * to the ACPI identifier of the display that remains active.
 *
 * Required steps by display miniport driver
 * The display miniport driver must follow these steps when its DxgkDdiStopDeviceAndReleasePostDisplayOwnership
 * function is called:
 *
 * 1. The driver must stop the display device associated with the video present
 * target indicated by the TargetId parameter but must keep the display associated
 * with this target powered on and visible.
 *
 * 2. The driver must check the connectivity of the display associated with this
 * target. If the target does not have a display connected, the driver must complete
 * the call to this function and return the STATUS_NOT_SUPPORTED error code.
 *
 * 3. The driver must disable the signal to all other displays that are connected
 * to the display adapter. If this is not possible, the driver should attempt to
 * place a blank image on all other displays. If this is not possible, the driver
 * must leave the last image on the screen unchanged.
 *
 * 4. The driver must keep the current display mode on the indicated target and
 * provide this mode back to the operating system as part of this function call.
 *
 * 5. If the driver cannot maintain the current display mode, or if the target is
 * not part of the active topology, the driver should select an alternate active
 * target and attempt to maintain the current resolution of that target. If that
 * is not possible, the driver should attempt to set the display to its native resolution
 * or to a high-resolution mode. In this case, the display resolution must be set
 * to at least 800 x 600 pixels in either D3DDDIFMT_R8G8B8 (24 bits per pixel) or
 *  D3DDDIFMT_X8R8G8B8 (32 bpp) color formats of the D3DDDIFORMAT enumeration.
 *
 * 6. If no target is active, the driver should attempt to enable a target, preferably
 * the internal panel, if it is available.
 *
 * 7. If possible, the driver must	clear the current frame buffer and disable the
 * hardware cursor and all display overlays.
 *
 * 8. If possible, the driver must set the gamma ramp of the device to its default
 * values.
 *
 * 9. The driver must set the current frame buffer to be in a linear mode. The driver
 * does this either by using the default swizzle range or by disabling swizzle mode.
 *
 * 10. The driver must make the current frame buffer accessible to the CPU by mapping
 * the current frame buffer linearly into the CPU address space.
 *
 * 11. The driver must ensure that the visibility of the indicated target is set
 * to "enabled."
 *
 * After the display miniport driver performs these steps, it must return the current
 * display settings for the device. The driver returns this information by setting
 * the members of the DXGK_DISPLAY_INFORMATION structure that is referenced by the
 * DisplayInfo parameter.
 *
 * Note  After the device has been stopped, this display information might be used
 * by the Windows generic display driver to manage the display device.
 *
 * Other requirements
 * On systems that support the Unified Extensible Firmware Interface (UEFI), a VGA
 * basic input/output system (BIOS) does not exist. To support PnP stop operations
 * on these systems, Windows Display Driver Model (WDDM) 1.2 and later provides
 * support for the operating system to reset the POST device and to get its display
 * information during a PnP stop operation. The operating system does this by calling
 * the display miniport driver's DxgkDdiStopDeviceAndReleasePostDisplayOwnership
 * function.
 * A PnP stop operation can occur in response to requests by processes such as the
 * Device Manager, or during a driver upgrade process.
 *
 * Starting with Windows 8, the operating system calls the driver's DxgkDdiStopDeviceAndReleasePostDisplayOwnership
 * function only on the POST device during a PnP stop operation.
 * Note  	It is optional for the display miniport driver to call DxgkCbAcquirePostDisplayOwnership.
 * However, the operating system might still call the DxgkDdiStopDeviceAndReleasePostDisplayOwnership
 * function of the device driver if the driver did not previously call DxgkCbAcquirePostDisplayOwnership.
 *
 * If the driver successfully completes a call to this function, the operating system
 * will not call the DxgkDdiStopDevice function. If the driver fails to complete
 * a call to this function, the operating system will call the DxgkDdiStopDevice
 * function, and device behavior will be the same as in Windows 7.
 * On UEFI-only systems, if the display miniport driver fails a call to this function,
 * a black screen is displayed and the IHV driver is not installed. The workaround
 * to this scenario is for the user to reboot the computer.
 * For more information on how this function is used in PnP scenarios, see Plug
 * and Play (PnP) in WDDM 1.2 and later.
 */
NTSTATUS
LJB_DXGK_StopDeviceAndReleasePostDisplayOwnership(
    _In_  const PVOID                       MiniportDeviceContext,
    _In_  D3DDDI_VIDEO_PRESENT_TARGET_ID    TargetId,
    _Out_ PDXGK_DISPLAY_INFORMATION         DisplayInfo
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(MiniportDeviceContext);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    /*
     * pass the call to inbox driver
     */
    ntStatus = (*DriverInitData->DxgkDdiStopDeviceAndReleasePostDisplayOwnership)(
        MiniportDeviceContext,
        TargetId,
        DisplayInfo);
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        return ntStatus;
    }

    DBG_PRINT(Adapter, DBGLVL_FLOW,
        (__FUNCTION__
        ": MiniportDeviceContext(%p)\n",
        MiniportDeviceContext
        ));
    return ntStatus;
}