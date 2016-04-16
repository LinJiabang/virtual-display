/*
 * ljb_proxykmd_dispatch_ioctl.c, adapted from WINDDK toaster code.
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
#include "ljb_proxykmd_ioctl.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_PROXYKMD_DispatchIoctl)
#endif

NTSTATUS
LJB_PROXYKMD_DispatchIoctl (
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP Irp
    )
{
    LJB_DEVICE_EXTENSION *  CONST   DeviceExtension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION CONST        IoStackLocation = IoGetCurrentIrpStackLocation(Irp);
    ULONG                           IoctlCode = IoStackLocation->Parameters.DeviceIoControl.IoControlCode;
    LIST_ENTRY * CONST              ListHead = &GlobalDriverData.ClientAdapterListHead;
    LIST_ENTRY *                    ListEntry;
    LJB_ADAPTER *                   Adapter;
    DXGKRNL_INTERFACE *             DxgkInterface;
    DXGK_CHILD_STATUS               DxgkChildStatus;
    NTSTATUS                        ntStatus;

    PAGED_CODE();

    if (DeviceExtension->DeviceType == DEVICE_TYPE_FILTER)
    {
        return LJB_PROXYKMD_PassDown(DeviceObject, Irp);
    }

    ntStatus = STATUS_SUCCESS;

    switch (IoctlCode)
    {
    case IOCTL_PROXYKMD_PLUGIN_FAKE_MONITOR:
        /*
         * Locate the first adapter
         */
        if (IsListEmpty(ListHead))
            {
            KdPrint(( __FUNCTION__ ": No adapter intercepted\n"));
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
            }

        ListEntry = ListHead->Flink;
        Adapter = CONTAINING_RECORD(ListEntry, LJB_ADAPTER, ListEntry);
        DxgkInterface = &Adapter->DxgkInterface;
        DxgkChildStatus.Type = StatusConnection;
        DxgkChildStatus.ChildUid = Adapter->UsbTargetIdBase;

        KdPrint((__FUNCTION__ ": sending attach event\n"));
        Adapter->FakeMonitorEnabled         = TRUE;
        DxgkChildStatus.HotPlug.Connected   = TRUE;
        ntStatus = (*DxgkInterface->DxgkCbIndicateChildStatus)(
            DxgkInterface->DeviceHandle,
            &DxgkChildStatus
            );
        break;

    case IOCTL_PROXYKMD_UNPLUG_FAKE_MONITOR:
        /*
         * Locate the first adapter
         */
        if (IsListEmpty(ListHead))
            {
            KdPrint(( __FUNCTION__ ": No adapter intercepted\n"));
            Irp->IoStatus.Information = 0;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return STATUS_SUCCESS;
            }

        ListEntry = ListHead->Flink;
        Adapter = CONTAINING_RECORD(ListEntry, LJB_ADAPTER, ListEntry);
        DxgkInterface = &Adapter->DxgkInterface;
        DxgkChildStatus.Type = StatusConnection;
        DxgkChildStatus.ChildUid = Adapter->UsbTargetIdBase;

        KdPrint((__FUNCTION__ ": sending detach event\n"));
        Adapter->FakeMonitorEnabled         = FALSE;
        DxgkChildStatus.HotPlug.Connected   = FALSE;
        ntStatus = (*DxgkInterface->DxgkCbIndicateChildStatus)(
            DxgkInterface->DeviceHandle,
            &DxgkChildStatus
            );
        break;

    default:
        ntStatus = STATUS_NOT_SUPPORTED;
        break;
    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return ntStatus;
}

