/*
 * ljb_dxgk_open_allocation.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_OpenAllocation)
#endif

/*
 * Function: LJB_DXGK_OpenAllocation
 *
 * Description:
 * The DxgkDdiOpenAllocation function binds non-device-specific allocations that
 * the DxgkDdiCreateAllocation function created to allocations that are specific
 * to the specified graphics context device.
 *
 * Return Value:
 * DxgkDdiOpenAllocation returns one of the following values:
 *
 *  STATUS_SUCCESS: DxgkDdiOpenAllocation successfully bound allocations to the
 *  graphics context device that the hDevice parameter specified.
 *
 *  STATUS_INVALID_PARAMETER: Parameters that were passed to DxgkDdiOpenAllocation
 *  contained errors that prevented it from completing.
 *
 *  STATUS_NO_MEMORY: DxgkDdiOpenAllocation could not allocate memory that was
 *  required for it to complete.
 *
 *  STATUS_GRAPHICS_DRIVER_MISMATCH: The display miniport driver is not compatible
 *  with the user-mode display driver that initiated the call to DxgkDdiOpenAllocation
 *  (that is, supplied private data to the display miniport driver).
 *
 * Remarks:
 * The DirectX graphics kernel subsystem calls the display miniport driver's
 * DxgkDdiOpenAllocation function to bind nondevice-specific allocations that the
 * DxgkDdiCreateAllocation function created to allocations that are specific to
 * the graphics context device that the hDevice parameter specifies. The display
 * miniport driver binds allocations to a device so the driver can keep track of
 * allocation data that is specific to a device.
 *
 * The display miniport driver can bind an allocation to any device that any process
 * (on the same graphics adapter) created and not just to a device in the creating
 * process.
 *
 * When DxgkDdiOpenAllocation returns STATUS_SUCCESS, the driver sets the
 * hDeviceSpecificAllocation member of the DXGK_OPENALLOCATIONINFO structure for
 * each allocation to a non-NULL value. The DXGK_OPENALLOCATIONINFO structure for
 * each allocation is an element of the array that the pOpenAllocation member of
 * the DXGKARG_OPENALLOCATION structure specifies.
 *
 * The driver can modify the allocation private driver data that is passed in the
 * pPrivateDriverData member of the DXGK_OPENALLOCATIONINFO structure only when
 * the allocation is created (which is indicated when the Create bit-field flag
 * in the Flags member of the DXGKARG_OPENALLOCATION structure is set). The driver
 * should determine that it can only read the allocation private driver data when
 * the allocation is opened (that is, when the Create bit-field flag is not set).
 *
 * DxgkDdiOpenAllocation should be made pageable.
 */
NTSTATUS
LJB_DXGK_OpenAllocation(
    _In_ const HANDLE                 hDevice,
    _In_ const DXGKARG_OPENALLOCATION *pOpenAllocation
    )
{
    LJB_DEVICE * CONST                  MyDevice = LJB_DXGK_FindDevice(hDevice);
    LJB_ADAPTER * CONST                 Adapter = MyDevice->Adapter;
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiOpenAllocation)(hDevice, pOpenAllocation);
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}
