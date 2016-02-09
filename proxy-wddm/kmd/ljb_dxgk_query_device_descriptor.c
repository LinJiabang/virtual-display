/*
 * ljb_dxgk_query_device_descriptor.c
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
#pragma alloc_text (PAGE, LJB_DXGK_QueryDeviceDescriptor)
#endif

static
NTSTATUS
LJB_DXGK_QueryDeviceDescriptorForUsbMonitor(
    _In_    LJB_ADAPTER *           Adapter,
    _In_    ULONG                   ChildUid,
    _Inout_ PDXGK_DEVICE_DESCRIPTOR DeviceDescriptor
    );

/*
 * Function: LJB_DXGK_QueryDeviceDescriptor
 *
 * Description:
 * The DxgkDdiQueryDeviceDescriptor function returns a descriptor for a child
 * device of a display adapter or for an external device (typically a monitor)
 * connected to a child device of a display adapter.
 *
 * Return value
 * DxgkDdiQueryDeviceDescriptor returns one of the following values:
 *
 * STATUS_SUCCESS:
 *  The function successfully returned the device descriptor.
 *
 * STATUS_GRAPHICS_CHILD_DESCRIPTOR_NOT_SUPPORTED:
 *  The (onboard) child device identified by ChildUid does not support a descriptor.
 *
 * STATUS_MONITOR_NO_DESCRIPTOR:
 *  The child device identified by ChildUid is connected to a monitor that does
 *  not support an EDID descriptor.
 *
 * STATUS_MONITOR_NO_MORE_DESCRIPTOR_DATA:
 *  The child device identified by ChildUid is connected to a monitor that does
 *  support an EDID descriptor, but the descriptor does not have the EDID extension
 *  block specified by the DescriptorOffset and DescriptorLength members of
 *  DeviceDescriptor.
 *
 * Remarks
 * DxgkDdiQueryDeviceDescriptor must never write more than the number of bytes
 * specified by DeviceDescriptor->DescriptorLength.
 *
 * If the child device identified by ChildUid has a type of TypeVideoOutput,
 * DxgkDdiQueryDeviceDescriptor returns a portion of the Extended Display
 * Identification Data (EDID) for the monitor connected to the output.
 * DeviceDescriptor->DescriptorOffset specifies the byte-offset into the EDID of
 * the start of the data to be returned.
 *
 * If the child device identified by ChildUid is not a video output,
 * DxgkDdiQueryDeviceDescriptor returns a generic device descriptor; that is, it
 * fills in the members of a DXGK_GENERIC_DESCRIPTOR structure.
 *
 * The DxgkDdiQueryDeviceDescriptor function can be called several times for one
 * child device. For a child device that has a connected monitor, the display port
 * driver calls DxgkDdiQueryDeviceDescriptor during initialization to obtain the
 * first 128-byte block of a monitor's EDID. Later the monitor class function
 * driver (Monitor.sys) calls DxgkDdiQueryDeviceDescriptor to obtain selected
 * portions (including the first 128-byte block) of that same monitor's EDID.
 *
 * DxgkDdiQueryDeviceDescriptor should be made pageable.
 */
NTSTATUS
LJB_DXGK_QueryDeviceDescriptor(
    _In_    const PVOID                   MiniportDeviceContext,
    _In_          ULONG                   ChildUid,
    _Inout_       PDXGK_DEVICE_DESCRIPTOR DeviceDescriptor
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(MiniportDeviceContext);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    /*
     * if the runtime is query our child status, don't pass it to inbox driver
     */
    if (ChildUid >= Adapter->UsbTargetIdBase)
    {
        return LJB_DXGK_QueryDeviceDescriptorForUsbMonitor(
            Adapter,
            ChildUid,
            DeviceDescriptor
            );
    }

    /*
     * pass the call to inbox driver
     */
    ntStatus = (*DriverInitData->DxgkDdiQueryDeviceDescriptor)(
        MiniportDeviceContext,
        ChildUid,
        DeviceDescriptor
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        return ntStatus;
    }

    return ntStatus;
}

static
NTSTATUS
LJB_DXGK_QueryDeviceDescriptorForUsbMonitor(
    _In_    LJB_ADAPTER *           Adapter,
    _In_    ULONG                   ChildUid,
    _Inout_ PDXGK_DEVICE_DESCRIPTOR DeviceDescriptor
    )
{
    /*
     * NOT YET IMPLEMENTED
     */
    UNREFERENCED_PARAMETER(Adapter);
    UNREFERENCED_PARAMETER(ChildUid);
    UNREFERENCED_PARAMETER(DeviceDescriptor);

    return STATUS_SUCCESS;
}