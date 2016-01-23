/*
 * ljb_proxykmd_passdown.c, adapted from WINDDK toaster code.
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
 * Name: LJB_PROXYKMD_PassDown
 *
 * NTSTATUS
 * LJB_PROXYKMD_PassDown(
 *        __in PDEVICE_OBJECT DeviceObject,
 *        __in PIRP Irp
 *        );
 *
 * Description:
 *    The default dispatch routine.  If this driver does not recognize the
 *    IRP, then it should send it down, unmodified.
 *    If the device holds iris, this IRP must be queued in the device extension
 *    No completion routine is required.
 *
 *    For demonstrative purposes only, we will pass all the (non-PnP) Irps down
 *    on the stack (as we are a filter driver). A real driver might choose to
 *    service some of these Irps.
 *
 *    As we have NO idea which function we are happily passing on, we can make
 *    NO assumptions about whether or not it will be called at raised IRQL.
 *    For this reason, this function must be in put into non-paged pool
 *    (aka the default location).
 *
 * Return Value:
 *      NT status code
 *
 */
NTSTATUS
LJB_PROXYKMD_PassDown (
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP Irp
    )
{
    LJB_DEVICE_EXTENSION *  CONST   DeviceExtension = DeviceObject->DeviceExtension;
    NTSTATUS                        ntStatus;

    ntStatus = IoAcquireRemoveLock (&DeviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS (ntStatus))
    {
        Irp->IoStatus.Status = ntStatus;
        IoCompleteRequest (Irp, IO_NO_INCREMENT);
        return ntStatus;
    }

    IoSkipCurrentIrpStackLocation (Irp);
    ntStatus = IoCallDriver (DeviceExtension->StackDeviceObject, Irp);
    IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);
    return ntStatus;
}