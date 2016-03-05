/*
 * ljb_dxgk_notify_acpi_event.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_NotifyAcpiEvent)
#endif

/*
 * Function: LJB_DXGK_NotifyAcpiEvent
 *
 * Description:
 * Notifies the display miniport driver about certain ACPI events.
 *
 * Return value
 * DxgkDdiNotifyAcpiEvent returns STATUS_SUCCESS if it succeeds; otherwise, it
 * returns one of the error codes defined in Ntstatus.h.
 *
 * Remarks
 * DxgkDdiNotifyAcpiEvent is an optional display miniport driver function.
 *
 * DxgkDdiNotifyAcpiEvent should be made pageable.
 */
NTSTATUS
LJB_DXGK_NotifyAcpiEvent(
    _In_  const PVOID           MiniportDeviceContext,
    _In_        DXGK_EVENT_TYPE EventType,
    _In_        ULONG           Event,
    _In_        PVOID           Argument,
    _Out_       PULONG          AcpiFlags
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
    ntStatus = (*DriverInitData->DxgkDdiNotifyAcpiEvent)(
        MiniportDeviceContext,
        EventType,
        Event,
        Argument,
        AcpiFlags
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        return ntStatus;
    }

    return ntStatus;
}
