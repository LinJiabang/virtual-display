/*
 * ljb_dxgk_dpc_routine.c
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

/*
 * Function: LJB_DXGK_DpcRoutine
 *
 * Description:
 * The DxgkDdiDpcRoutine function is called back at IRQL DISPATCH_LEVEL after 
 * the display miniport driver calls DxgkCbQueueDpc.
 *
 * Return Value:
 * None.
 *
 * Remarks:
 * Only one deferred procedure call (DPC) can be scheduled (at a given time) for
 * a given display adapter.
 * If the display miniport driver is supporting several display adapters, the
 * DxgkDdiDpcRoutine might be called in a reentrant fashion. That is, while
 * DxgkDdiDpcRoutine is executing on one processor on behalf of a particular
 * display adapter, it could be called again on another processor on behalf of
 * a different display adapter.
 */
VOID
LJB_DXGK_DpcRoutine(
    _In_ const PVOID MiniportDeviceContext
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(MiniportDeviceContext);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;

    (*DriverInitData->DxgkDdiDpcRoutine)(MiniportDeviceContext);
}
