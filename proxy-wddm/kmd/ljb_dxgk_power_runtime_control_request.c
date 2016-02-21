/*
 * ljb_dxgk_power_runtime_control_request.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

/*
 * Function: LJB_DXGK_PowerRuntimeControlRequest
 *
 * Description:
 * Called by the Power Engine Plug-in (PEP) to exchange information with the
 * display miniport driver. Also called by the Microsoft DirectX graphics kernel
 * subsystem to notify the display miniport driver about certain events.
 *
 * Return Value:
 * Returns STATUS_SUCCESS if it succeeds. Otherwise, it returns one of the error
 * codes defined in Ntstatus.h.
 *
 * Remarks:
 * The operating system calls DxgkDdiPowerRuntimeControlRequest only if the display
 * miniport driver indicates support by setting DXGK_DRIVERCAPS.SupportRuntimePowerManagement
 * to TRUE.
 *
 * GUIDs used by the Power Engine Plugin (PEP)
 * The PEP uses the following GUIDs that are defined in D3dkmddi.h to exchange
 * information with the display miniport driver. The display port driver uses
 * these GUIDs to issue Event Tracing for Windows (ETW) events, which are useful
 * to profile driver performance issues.
 *
 *  GUID_DXGKDDI_POWER_VOLTAGE_UP
 *  Increase the voltage.
 *  GUID_DXGKDDI_POWER_VOLTAGE_DOWN
 *  Decrease the voltage.
 *  GUID_DXGKDDI_POWER_VOLTAGE
 *  Change the voltage, but the driver doesn't know if the change is an increase
 *  or decrease.
 *  GUID_DXGKDDI_POWER_CLOCK_UP
 *  Increase the clock setting.
 *  GUID_DXGKDDI_POWER_CLOCK_DOWN
 *  Decrease the clock setting.
 *  GUID_DXGKDDI_POWER_CLOCK
 *  Change the clock setting, but the driver doesn't know if the change is an
 *  increase or decrease.
 *  GUID_DXGKDDI_POWER_BANDWIDTH_UP
 *  Increase the bandwidth.
 *  GUID_DXGKDDI_POWER_BANDWIDTH_DOWN
 *  Decrease the bandwidth.
 *  GUID_DXGKDDI_POWER_BANDWIDTH
 *  Change the bandwidth, but the driver doesn't know if the change is an increase
 *  or decrease.
 *
 * GUIDs used by the DirectX graphics kernel subsystem
 * The DirectX graphics kernel subsystem uses the following GUIDs that are defined
 * in D3dkmddi.h to notify the display miniport driver about certain events.
 *
 *  GUID_DXGKDDI_POWER_MANAGEMENT_PREPARE_TO_START
 *  Used after the DirectX graphics kernel subsystem registers the device for runtime
 *  power management, but before the device is started. After this function has
 *  been called with this GUID, the display miniport driver can call these functions:
 *
 *  DxgkCbSetPowerComponentActive
 *  DxgkCbSetPowerComponentLatency
 *  DxgkCbSetPowerComponentResidency
 *
 *  GUID_DXGKDDI_POWER_MANAGEMENT_STARTED
 *  Used after the DirectX graphics kernel subsystem starts runtime power management.
 *  After this function has been called with this GUID, the display miniport driver
 *  can call any power runtime functions.
 *
 *  GUID_DXGKDDI_POWER_MANAGEMENT_STOPPED
 *  Used immediately before the DirectX graphics kernel subsystem unregisters the
 *  device for runtime power management. After this function has been called with
 *  this GUID, the display miniport driver should not call any power runtime
 *  functions.
 *
 * Synchronization
 * This function can be called simultaneously from multiple execution threads.
 *
 * The operating system guarantees that this function follows the zero level
 * synchronization mode as defined in Threading and Synchronization Zero Level.
 */
NTSTATUS
LJB_DXGK_PowerRuntimeControlRequest(
    _In_      const HANDLE  DriverContext,
    _In_            LPCGUID PowerControlCode,
    _In_opt_        PVOID   InBuffer,
    _In_            SIZE_T  InBufferSize,
    _Out_opt_       PVOID   OutBuffer,
    _In_            SIZE_T  OutBufferSize,
    _Out_opt_       PSIZE_T BytesReturned
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(DriverContext);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    if (IsEqualGUID(PowerControlCode, &GUID_DXGKDDI_POWER_VOLTAGE_UP))
    {
        DBG_PRINT(Adapter, DBGLVL_POWER,
            (__FUNCTION__":GUID_DXGKDDI_POWER_VOLTAGE_UP\n"));
    }
    else if (IsEqualGUID(PowerControlCode, &GUID_DXGKDDI_POWER_VOLTAGE_DOWN))
    {
        DBG_PRINT(Adapter, DBGLVL_POWER,
            (__FUNCTION__":GUID_DXGKDDI_POWER_VOLTAGE_DOWN\n"));
    }
    else if (IsEqualGUID(PowerControlCode, &GUID_DXGKDDI_POWER_VOLTAGE))
    {
        DBG_PRINT(Adapter, DBGLVL_POWER,
            (__FUNCTION__":GUID_DXGKDDI_POWER_VOLTAGE\n"));
    }
    else if (IsEqualGUID(PowerControlCode, &GUID_DXGKDDI_POWER_CLOCK_UP))
    {
        DBG_PRINT(Adapter, DBGLVL_POWER,
            (__FUNCTION__":GUID_DXGKDDI_POWER_CLOCK_UP\n"));
    }
    else if (IsEqualGUID(PowerControlCode, &GUID_DXGKDDI_POWER_CLOCK_DOWN))
    {
        DBG_PRINT(Adapter, DBGLVL_POWER,
            (__FUNCTION__":GUID_DXGKDDI_POWER_CLOCK_DOWN\n"));
    }
    else if (IsEqualGUID(PowerControlCode, &GUID_DXGKDDI_POWER_CLOCK))
    {
        DBG_PRINT(Adapter, DBGLVL_POWER,
            (__FUNCTION__":GUID_DXGKDDI_POWER_CLOCK\n"));
    }
    else if (IsEqualGUID(PowerControlCode, &GUID_DXGKDDI_POWER_BANDWIDTH_UP))
    {
        DBG_PRINT(Adapter, DBGLVL_POWER,
            (__FUNCTION__":GUID_DXGKDDI_POWER_BANDWIDTH_UP\n"));
    }
    else if (IsEqualGUID(PowerControlCode, &GUID_DXGKDDI_POWER_BANDWIDTH_DOWN))
    {
        DBG_PRINT(Adapter, DBGLVL_POWER,
            (__FUNCTION__":GUID_DXGKDDI_POWER_BANDWIDTH_UP\n"));
    }
    else if (IsEqualGUID(PowerControlCode, &GUID_DXGKDDI_POWER_BANDWIDTH))
    {
        DBG_PRINT(Adapter, DBGLVL_POWER,
            (__FUNCTION__":GUID_DXGKDDI_POWER_BANDWIDTH\n"));
    }
    else if (IsEqualGUID(PowerControlCode, &GUID_DXGKDDI_POWER_MANAGEMENT_PREPARE_TO_START))
    {
        DBG_PRINT(Adapter, DBGLVL_POWER,
            (__FUNCTION__":GUID_DXGKDDI_POWER_MANAGEMENT_PREPARE_TO_START\n"));
    }
    else if (IsEqualGUID(PowerControlCode, &GUID_DXGKDDI_POWER_MANAGEMENT_STARTED))
    {
        DBG_PRINT(Adapter, DBGLVL_POWER,
            (__FUNCTION__":GUID_DXGKDDI_POWER_MANAGEMENT_STARTED\n"));
    }
    else if (IsEqualGUID(PowerControlCode, &GUID_DXGKDDI_POWER_MANAGEMENT_STOPPED))
    {
        DBG_PRINT(Adapter, DBGLVL_POWER,
            (__FUNCTION__":GUID_DXGKDDI_POWER_MANAGEMENT_STOPPED\n"));
    }

    ntStatus = (*DriverInitData->DxgkDdiPowerRuntimeControlRequest)(
        DriverContext,
        PowerControlCode,
        InBuffer,
        InBufferSize,
        OutBuffer,
        OutBufferSize,
        BytesReturned
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}