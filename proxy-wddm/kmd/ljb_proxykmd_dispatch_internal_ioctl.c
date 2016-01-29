/*
 * ljb_proxykmd_dispatch_internal_ioctl.c.
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
#pragma alloc_text (PAGE, LJB_PROXYKMD_DispatchInternalIoctl)
#endif

static
NTSTATUS
LJB_PROXYKMD_InternalIoctlCompletion(
    __in PDEVICE_OBJECT     DeviceObject,
    __in PIRP               Irp,
    __in PVOID              Context
    )
{
    PKEVENT             event = Context;

    UNREFERENCED_PARAMETER (DeviceObject);

    if (Irp->PendingReturned == TRUE)
    {
        KeSetEvent (event, IO_NO_INCREMENT, FALSE);
    }
    return STATUS_MORE_PROCESSING_REQUIRED;
}

/*
 * we are interested in IOCTL_GET_DXGK_INITIALIZE_WIN7 &  IOCTL_GET_DXGK_INITIALIZE_WIN8
 * called from client driver. If we are not filter driver, don't process it, and
 * simply pass it down.
 */
NTSTATUS
LJB_PROXYKMD_DispatchInternalIoctl (
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP Irp
    )
    {
    LJB_DEVICE_EXTENSION *  CONST   DeviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION CONST        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    ULONG                           IoctlCode = IoStackLocation->Parameters.DeviceIoControl.IoControlCode;
    KEVENT                          event;
    PVOID                           UserBuffer;
    NTSTATUS                        ntStatus;

    if (DeviceExtension->DeviceType != DEVICE_TYPE_FILTER)
    {
        return LJB_PROXYKMD_PassDown(DeviceObject, Irp);
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    switch (IoctlCode)
    {
    case IOCTL_GET_DXGK_INITIALIZE_WIN7:
        KdPrint((__FUNCTION__ ":IOCTL_GET_DXGK_INITIALIZE_WIN7\n"));

        UserBuffer = Irp->UserBuffer;
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(
            Irp,
            &LJB_PROXYKMD_InternalIoctlCompletion,
            &event,
            TRUE,
            TRUE,
            TRUE
            );
        ntStatus = IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
        if (ntStatus == STATUS_PENDING)
        {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            ntStatus = Irp->IoStatus.Status;
        }
        if (NT_SUCCESS(ntStatus))
        {
            /*
             * METHOD_FROM_CTL_CODE(IOCTL_GET_DXGK_INITIALIZE_WIN7) ==
             * METHOD_NEITHER, which means that, according to
             * http://msdn.microsoft.com/en-us/library/windows/hardware/ff540663(v=vs.85).aspx,
             * the output buffer's address is specified by Irp->UserBuffer.
             */
            GlobalDriverData.DxgkInitializeWin7 = *((PFN_DXGK_INITIALIZE *) UserBuffer);
            KdPrint((__FUNCTION__ ": successfully get pfnDxgkInializeWin7(%p)\n",
                GlobalDriverData.DxgkInitializeWin8
                ));
            //*((PFN_DXGK_INITIALIZE *) UserBuffer) = &DxgkInitializeWin7;
        }
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        break;

    case IOCTL_GET_DXGK_INITIALIZE_WIN8:
        KdPrint((__FUNCTION__ ":IOCTL_GET_DXGK_INITIALIZE_WIN8\n" ));
        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(
            Irp,
            &LJB_PROXYKMD_InternalIoctlCompletion,
            &event,
            TRUE,
            TRUE,
            TRUE
            );
        ntStatus = IoCallDriver(DeviceExtension->NextLowerDriver, Irp);
        if (ntStatus == STATUS_PENDING)
        {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            ntStatus = Irp->IoStatus.Status;
        }
        if (NT_SUCCESS(ntStatus))
        {
            /*
             * METHOD_FROM_CTL_CODE(IOCTL_GET_DXGK_INITIALIZE_WIN8) ==
             * METHOD_NEITHER, which means that, according to
             * http://msdn.microsoft.com/en-us/library/windows/hardware/ff540663(v=vs.85).aspx,
             * the output buffer's address is specified by Irp->UserBuffer.
             */
            GlobalDriverData.DxgkInitializeWin8 = *((PFN_DXGK_INITIALIZE *) Irp->UserBuffer);
            KdPrint((__FUNCTION__ ": successfully get pfnDxgkInializeWin8(%p)\n",
                GlobalDriverData.DxgkInitializeWin8
                ));
            //*((PFN_DXGK_INITIALIZE *) Irp->UserBuffer) = &DxgkInitializeWin8;
        }
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        break;

    default:
        ntStatus = LJB_PROXYKMD_PassDown(DeviceObject, Irp);
        break;
    }
    return ntStatus;
}

