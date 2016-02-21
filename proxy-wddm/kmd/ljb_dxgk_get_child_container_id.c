/*
 * ljb_dxgk_get_child_container_id.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

/*
 * Function: LJB_DXGK_GetChildContainerId
 *
 * Description:
 * Cleans up internal resources associated with a direct memory access (DMA) packet
 * that was in the GPU scheduler's software queue but never reached the hardware
 * queue because the device went into an error state. Such an error state is typically
 * caused by a Timeout Detection and Recovery (TDR) event.
 *
 * Return Value:
 * Returns one of the following error codes.
 *   STATUS_SUCCESS: The driver has updated the structure pointed to by the ContainerId
 *   parameter with container ID information obtained from the display hardware.
 *
 *   STATUS_MONITOR_NO_DESCRIPTOR:The driver has accepted the default container
 *   ID information provided in the structure pointed to by ContainerId.
 *   Note If the driver returns this status code, it should not modify the structure.
 *
 * Otherwise the function returns one of the status codes defined in Ntstatus.h.
 *
 * Remarks:
 * The operating system calls the display miniport driver's DxgkDdiQueryChildRelations
 * function to enumerate the child devices of the display adapter. The operating
 * system then calls the display miniport driver's DxgkDdiQueryDeviceDescriptor
 * function for each child device to obtain the Extended Display Information Data
 * (EDID) for the device. For more information on this procedure, see Enumerating
 * Child Devices of a Display Adapter.
 *
 * Based on the device's EDID data, the operating system generates a default
 * container ID for the child device. Then, the operating system calls the display
 * miniport driver's DxgkDdiGetChildContainerId function and passes a pointer to
 * a DXGK_CHILD_CONTAINER_ID structure through the ContainerId parameter. The
 * ContainerId member of this structure contains the default container ID for the
 * child display device.
 *
 * The display miniport driver can either accept the default container ID or set
 * the ContainerId member to a unique identifier for the device before it returns
 * from the call to DxgkDdiGetChildContainerId.
 *
 * For more information about Container IDs, see Container IDs.
 */
NTSTATUS
LJB_DXGK_GetChildContainerId(
    _In_    PVOID                    MiniportDeviceContext,
    _In_    ULONG                    ChildUid,
    _Inout_ PDXGK_CHILD_CONTAINER_ID ContainerId
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(MiniportDeviceContext);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    if (ChildUid >= Adapter->UsbTargetIdBase)
    {
        DBG_PRINT(Adapter, DBGLVL_FLOW,
            (__FUNCTION__ ": accepting default container id for ChildUid(0x%x)\n",
            ChildUid
            ));
        return STATUS_MONITOR_NO_DESCRIPTOR;
    }
    ntStatus = (*DriverInitData->DxgkDdiGetChildContainerId)(
        MiniportDeviceContext,
        ChildUid,
        ContainerId
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}