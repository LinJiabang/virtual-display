/*
 * ljb_dxgk_destroy_device.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_DestroyDevice)
#endif

static VOID
LJB_DXGK_DestroyDevicePostProcessing(
    __in LJB_DEVICE * MyDevice
    );

/*
 * Function: LJB_DXGK_DestroyDevice
 *
 * Description:
 * The DxgkDdiDestroyDevice function destroys a graphics context device.
 *
 * Return Value:
 * DxgkDdiDestroyDevice returns STATUS_SUCCESS, or an appropriate error result
 * if the graphics context device is not successfully destroyed.
 *
 * Remarks:
 * The DirectX graphics kernel subsystem calls the display miniport driver's
 * DxgkDdiDestroyDevice function to destroy a graphics context device that the
 * driver's DxgkDdiCreateDevice function created. DxgkDdiDestroyDevice should
 * free all of the resources that were allocated for the device and clean up any
 * internal tracking data structures.
 *
 * DxgkDdiDestroyDevice should be made pageable.
 */
NTSTATUS
LJB_DXGK_DestroyDevice(
    _In_ const HANDLE hDevice
    )
{
    LJB_DEVICE * CONST                  MyDevice = LJB_DXGK_FindDevice(hDevice);
    LJB_ADAPTER * CONST                 Adapter = MyDevice->Adapter;
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiDestroyDevice)(hDevice);
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }
    LJB_DXGK_DestroyDevicePostProcessing(MyDevice);

    return ntStatus;
}

static VOID
LJB_DXGK_DestroyDevicePostProcessing(
    __in LJB_DEVICE * MyDevice
    )
{
    KIRQL                               oldIrql;

    /*
     * free MyDevice
     */
    KeAcquireSpinLock(&GlobalDriverData.ClientDeviceListLock, &oldIrql);
    RemoveEntryList(&MyDevice->ListEntry);
    KeReleaseSpinLock(&GlobalDriverData.ClientDeviceListLock, oldIrql);
    LJB_FreePool(MyDevice);

}
