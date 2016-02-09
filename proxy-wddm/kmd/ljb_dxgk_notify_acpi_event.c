/*
 * ljb_dxgk_notify_acpi_event.c
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
