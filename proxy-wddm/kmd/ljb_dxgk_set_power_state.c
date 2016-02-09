/*
 * ljb_dxgk_set_power_state.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_SetPowerState)
#endif

CONST CHAR * CONST DevicePowerStateString[] =
{
    "PowerDeviceUnspecified",
    "PowerDeviceD0",
    "PowerDeviceD1",
    "PowerDeviceD2",
    "PowerDeviceD3",
    "PowerDeviceMaximum"
};

CONST CHAR * CONST PowerActionString[] =
{
    "PowerActionNone",
    "PowerActionReserved",
    "PowerActionSleep",
    "PowerActionHibernate",
    "PowerActionShutdown",
    "PowerActionShutdownReset",
    "PowerActionShutdownOff",
    "PowerActionWarmEject"
};
/*
 * Function: LJB_DXGK_SetPowerState
 *
 * Description:
 * The DxgkDdiSetPowerState function sets the power state of a display adapter
 * or a child device of a display adapter.
 *
 * Return value
 * DxgkDdiSetPowerState returns STATUS_SUCCESS if it succeeds. DxgkDdiSetPowerState
 * should never fail; however, it can return any NTSTATUS-typed value that is
 * defined in Ntstatus.h and that passes the NT_SUCCESS(Status) macro.
 *
 * Remarks
 * If the requested state is equal to PowerDeviceD1, PowerDeviceD2, or PowerDeviceD3,
 * DxgkDdiSetPowerState saves any context that will later be required to bring
 * the device back to PowerDeviceD0 and then places the device in the requested
 * state. If the requested state is PowerDeviceD0 (fully on), DxgkDdiSetPowerState
 * restores the device context and places the device in PowerDeviceD0.
 *
 * If DxgkDdiSetPowerState is called with a request to put the VGA-enabled display
 * adapter into hibernation, it should not power down the display adapter. Instead,
 * it should save the context and let the bus driver power down the display adapter.
 * That way, the power manager can display hibernation progress after the display
 * miniport driver has been notified about the power state change.
 *
 * The operating system might call DxgkDdiSetPowerState on a child device of the
 * display adapter that is no longer connected (for example, a monitor that was
 * recently unplugged). This anomaly occurs because an inherent latency exists
 * between the time that the operating system calls the driver's DxgkDdiSetPowerState
 * and the time that the operating system processes the disconnection. The driver
 * must handle such situations without failing.
 *
 * If DevicePowerState is equal to PowerDeviceD0, do not rely on the value of
 * ActionType.
 *
 * Starting with Windows Display Driver Model (WDDM) 1.2, if the DevicePowerState
 * parameter is set to PowerDeviceD0, the display miniport driver should call
 * DxgkCbAcquirePostDisplayOwnership to query the information about the display
 * mode. This display mode might have been been previously set by the firmware
 * and system loader. If DxgkCbAcquirePostDisplayOwnership returns with
 * STATUS_SUCCESS, the driver should determine whether it has to reinitialize the
 * display based on the display mode information that was returned through the
 * DisplayInfo parameter. Otherwise, the driver should not assume that any specific
 * display mode is currently enabled on the device, and it should initialize the
 * display.
 *
 * The DxgkDdiSetPowerState function should be made pageable.
 */
NTSTATUS
LJB_DXGK_SetPowerState(
    _In_ const PVOID              MiniportDeviceContext,
    _In_       ULONG              DeviceUid,
    _In_       DEVICE_POWER_STATE DevicePowerState,
    _In_       POWER_ACTION       ActionType
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(MiniportDeviceContext);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    /*
     * if the DeviceUid is targeted to USB monitor, don't pass it to inbox driver
     */
    if (DeviceUid != DISPLAY_ADAPTER_HW_ID && DeviceUid >= Adapter->UsbTargetIdBase)
    {
        DBG_PRINT(Adapter, DBGLVL_POWER,
            (__FUNCTION__
            ": DeviceUid(0x%x), DevicePowerState(0x%x:%s), ActionType(0x%x:%s)\n",
            DeviceUid,
            DevicePowerState,
            DevicePowerStateString[DevicePowerState],
            ActionType,
            PowerActionString[ActionType]
            ));

        return STATUS_SUCCESS;
    }

    /*
     * pass the call to inbox driver
     */
    ntStatus = (*DriverInitData->DxgkDdiSetPowerState)(
        MiniportDeviceContext,
        DeviceUid,
        DevicePowerState,
        ActionType
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        return ntStatus;
    }

    return ntStatus;
}
