/*
 * ljb_dxgk_reset_device.c
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
 * Function: LJB_DXGK_ResetDevice
 *
 * Description:
 * The DxgkDdiResetDevice function sets a display adapter to VGA character mode (80 x 50).
 *
 * Return value
 * None
 *
 * Remarks
 * The HAL calls this function so it can display information on the screen during
 * hibernation, bug checks, and the like.
 *
 * DxgkDdiResetDevice can be called at any IRQL, so it must be in nonpageable
 * memory. DxgkDdiResetDevice must not call any code that is in pageable memory
 * and must not manipulate any data that is in pageable memory.
 */
VOID
LJB_DXGK_ResetDevice(
    _In_  const PVOID           MiniportDeviceContext
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(MiniportDeviceContext);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;

    /*
     * pass the call to inbox driver
     */
    (*DriverInitData->DxgkDdiResetDevice)(MiniportDeviceContext);
}
