/*
 * ljb_proxykmd_dispatch_create.c, adapted from WINDDK toaster code.
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

#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_PROXYKMD_DispatchCreate)
#endif

/*
 * if we are filter driver, simply pass down the request. Otherwise complete
 * the request with STATUS_SUCCESS.
 */
NTSTATUS
LJB_PROXYKMD_DispatchCreate(
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP Irp
    )
{
    LJB_DEVICE_EXTENSION * CONST DeviceExtension = DeviceObject->DeviceExtension;

    PAGED_CODE();

    if (DeviceExtension->DeviceType == DEVICE_TYPE_FILTER)
    {
        return LJB_PROXYKMD_PassDown(DeviceObject, Irp);
    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}
