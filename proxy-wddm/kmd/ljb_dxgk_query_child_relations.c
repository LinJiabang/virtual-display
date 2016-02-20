/*
 * ljb_dxgk_query_child_relations.c
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
#pragma alloc_text (PAGE, LJB_DXGK_QueryChildRelations)
#endif

static CONST CHAR * CONST ChildDeviceTypeString[] = {
    "TypeUninitialized"
    "TypeVideoOutput",
    "TypeOther"
};
static CONST CHAR * CONST HpdAwarenessString[] = {
    "HpdAwarenessUninitialized",
    "HpdAwarenessAlwaysConnected",
    "HpdAwarenessNone",
    "HpdAwarenessPolled",
    "HpdAwarenessInterruptible"
};


/*
 * Function: LJB_DXGK_QueryChildRelations
 *
 * Description:
 * The DxgkDdiQueryChildRelations function enumerates the child devices of a
 * display adapter.
 *
 * Return value
 * DxgkDdiQueryChildRelations returns STATUS_SUCCESS if it succeeds; otherwise,
 * it returns one of the error codes defined in Ntstatus.h.
 *
 * Remarks
 * All child devices of the display adapter are onboard; monitors and other
 * external devices that connect to the display adapter are not considered child
 * devices.
 *
 * The display miniport driver must fill in an array of DXGK_CHILD_DESCRIPTOR
 * structures, one for each of the display adapter's children. The array must
 * contain DXGK_CHILD_DESCRIPTOR structures for all current child devices and
 * all potential child devices. For example, if docking a portable computer will
 * result in new video outputs becoming available, those video outputs must have
 * descriptors in the array, even if they are not currently available.
 *
 * The DxgkDdiQueryChildRelations function should be made pageable.
 */
NTSTATUS
LJB_DXGK_QueryChildRelations(
    _In_    const PVOID                  MiniportDeviceContext,
    _Inout_       PDXGK_CHILD_DESCRIPTOR ChildRelations,
    _In_          ULONG                  ChildRelationsSize
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(MiniportDeviceContext);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    PDXGK_CHILD_DESCRIPTOR              ChildDescriptor;
    ULONG                               TargetChildRelationsSize;
    ULONG                               LargestChildId;
    NTSTATUS                            ntStatus;
    UINT                                i;

    PAGED_CODE();
    UNREFERENCED_PARAMETER(ChildRelationsSize);

    /*
     * pass the call to inbox driver
     */
    TargetChildRelationsSize = (Adapter->NumberOfChildren + 1) * sizeof(DXGK_CHILD_DESCRIPTOR);
    ntStatus = (*DriverInitData->DxgkDdiQueryChildRelations)(
        MiniportDeviceContext,
        ChildRelations,
        TargetChildRelationsSize
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        return ntStatus;
    }

    /*
     * count the actual number of ChildDescriptor enumerated by target driver
     */
    LargestChildId = 0;
    for( i = 0; i < Adapter->NumberOfChildren; i++)
    {
        ChildDescriptor = ChildRelations + i;
        if (ChildDescriptor->ChildDeviceType == 0 &&
            ChildDescriptor->ChildCapabilities.HpdAwareness == HpdAwarenessUninitialized &&
            ChildDescriptor->ChildCapabilities.Type.VideoOutput.MonitorOrientationAwareness == D3DKMDT_MOA_UNINITIALIZED &&
            ChildDescriptor->AcpiUid == 0 &&
            ChildDescriptor->ChildUid == 0)
            break;
        if (ChildDescriptor->ChildUid > LargestChildId)
            LargestChildId = ChildDescriptor->ChildUid;

        DBG_PRINT(Adapter, DBGLVL_INFO,
            (__FUNCTION__
            "Child[%] = ChildDeviceType(%u:%s), HpdAwareness(%u, %s), AcpiUid(0x%x), ChildUid(0x%x), InterfaceTechnology(0x%x)\n",
            i,
            ChildDescriptor->ChildDeviceType,
            ChildDeviceTypeString[ChildDescriptor->ChildDeviceType],
            ChildDescriptor->ChildCapabilities.HpdAwareness,
            HpdAwarenessString[ChildDescriptor->ChildCapabilities.HpdAwareness],
            ChildDescriptor->AcpiUid,
            ChildDescriptor->ChildUid,
            ChildDescriptor->ChildCapabilities.Type.VideoOutput.InterfaceTechnology
            ));

    }

    Adapter->ActualNumberOfChildren = i;
    Adapter->UsbTargetIdBase = LargestChildId + 0x100;

    DBG_PRINT(Adapter, DBGLVL_INFO,
        (__FUNCTION__": ActualNumberOfChildren(%u), UsbTargetIdBase(0x%x)\n",
        Adapter->ActualNumberOfChildren,
        Adapter->UsbTargetIdBase
        ));

    /*
     * enumerate USB monitors
     */
    for (i = Adapter->ActualNumberOfChildren;
         i < Adapter->ActualNumberOfChildren + MAX_NUM_OF_USB_MONITOR;
         i++)
    {
        ChildDescriptor = ChildRelations + i;
        ChildDescriptor->ChildDeviceType = TypeVideoOutput;
        ChildDescriptor->AcpiUid = 0;
        ChildDescriptor->ChildUid = Adapter->UsbTargetIdBase + i - Adapter->ActualNumberOfChildren;
        ChildDescriptor->ChildCapabilities.HpdAwareness = HpdAwarenessInterruptible;
        ChildDescriptor->ChildCapabilities.Type.VideoOutput.InterfaceTechnology = D3DKMDT_VOT_HD15;
        ChildDescriptor->ChildCapabilities.Type.VideoOutput.MonitorOrientationAwareness = D3DKMDT_MOA_NONE;
        ChildDescriptor->ChildCapabilities.Type.VideoOutput.SupportsSdtvModes = FALSE;
    }

    return ntStatus;
}