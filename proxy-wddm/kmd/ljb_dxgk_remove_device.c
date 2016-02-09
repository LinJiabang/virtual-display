/*
 * ljb_dxgk_remove_device.c
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
#pragma alloc_text (PAGE, LJB_DXGK_RemoveDevice)
#endif

/*
 * Function: LJB_DXGK_RemoveDevice
 *
 * Description:
 * The DxgkDdiRemoveDevice function frees any resources allocated during
 * DxgkDdiAddDevice.
 *
 * Return Value:
 * DxgkDdiRemoveDevice returns STATUS_SUCCESS if it succeeds; otherwise, it returns
 * one of the error codes defined in Ntstatus.h.
 *
 * Remarks:
 * DxgkDdiRemoveDevice must free the context block represented by
 * MiniportDeviceContext.
 *
 * The DxgkDdiRemoveDevice function should be made pageable.
 */
NTSTATUS
LJB_DXGK_RemoveDevice(
    _In_  const PVOID   MiniportDeviceContext
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(MiniportDeviceContext);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;
    KIRQL                               oldIrql;
    LJB_DEVICE *                        MyDevice;
    LIST_ENTRY *                        listHead;
    LIST_ENTRY *                        listNext;
    LIST_ENTRY *                        listEntry;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiRemoveDevice)(MiniportDeviceContext);
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    /*
     * remove any LBJ_DEVICE associated with Adapter
     */
    listHead = &GlobalDriverData.ClientDeviceListHead;
    KeAcquireSpinLock(&GlobalDriverData.ClientDeviceListLock, &oldIrql);
    for (listEntry = listHead->Flink;
         listEntry != listHead;
         listEntry = listNext)
    {
        listNext = listEntry->Flink;
        MyDevice = CONTAINING_RECORD(listEntry, LJB_DEVICE, ListEntry);
        if (MyDevice->Adapter == Adapter)
        {
            RemoveEntryList(listEntry);
            LJB_PROXYKMD_FreePool(MyDevice);
        }
    }
    KeReleaseSpinLock(&GlobalDriverData.ClientDeviceListLock, oldIrql);

    KeAcquireSpinLock(&GlobalDriverData.ClientAdapterListLock, &oldIrql);
    RemoveEntryList(&Adapter->ListEntry);
    KeReleaseSpinLock(&GlobalDriverData.ClientAdapterListLock, oldIrql);
    LJB_PROXYKMD_FreePool(Adapter);
    return ntStatus;
}