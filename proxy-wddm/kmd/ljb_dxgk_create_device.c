/*
 * ljb_dxgk_create_device.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_CreateDevice)
#endif

/*
 * forward declartion
 */
static VOID
LJB_DXGK_CreateDevicePostProcessing(
    __in LJB_ADAPTER *          Adapter,
    __in LJB_DEVICE *           MyDevice,
    __in DXGKARG_CREATEDEVICE * pCreateDevice
    );

/*
 * Function: LJB_DXGK_CreateDevice
 *
 * Description:
 * The DxgkDdiCreateDevice function creates a graphics context device that is
 * subsequently used in calls to the display miniport driver's device-specific
 * functions.
 *
 * Return value
 * DxgkDdiCreateDevice returns one of the following values:
 *
 * STATUS_SUCCESS
 *  DxgkDdiCreateDevice successfully created the graphics context device.
 *
 * STATUS_NO_MEMORY
 *  DxgkDdiCreateDevice could not allocate memory that was required for it
 *  to complete.
 *
 * Remarks
 * The DirectX graphics kernel subsystem calls the display miniport driver's
 * DxgkDdiCreateDevice function to create a graphics context device that the
 * graphics subsystem subsequently passes in calls to the display miniport driver.
 * The driver uses a device to hold a collection of rendering state. The graphics
 * subsystem can create multiple devices in the same process on a given graphics
 * processing unit (GPU) adapter.
 *
 *   Note  The number of devices that can simultaneously exist is limited only
 *   by available system memory. That is, a driver cannot have a hard-coded maximum
 *   device limit.
 *
 * Generally, devices are independent of each other; in other words, resources
 * that are created in one device cannot be referenced or accessed by resources
 * that are created in another device. However, cross-process resources are an
 * exception to this rule.
 *
 * DxgkDdiCreateDevice should be made pageable.
 */
NTSTATUS
LJB_DXGK_CreateDevice(
    _In_    const HANDLE               hAdapter,
    _Inout_       DXGKARG_CREATEDEVICE *pCreateDevice
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    LJB_DEVICE *                        MyDevice;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    /*
     * create MyDevice
     */
    MyDevice = LJB_GetPoolZero(sizeof(LJB_DEVICE));
    if (MyDevice == NULL)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": unable to allocate MyDevice\n"));
        return STATUS_NO_MEMORY;
    }

    InitializeListHead(&MyDevice->ListEntry);
    MyDevice->hRTDevice = pCreateDevice->hDevice;
    MyDevice->Adapter = Adapter;

    MyDevice->CreateDevice.hDevice = pCreateDevice->hDevice;
    MyDevice->CreateDevice.Flags = pCreateDevice->Flags;

    if (DriverInitData->Version >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    {
        MyDevice->CreateDevice.Pasid = pCreateDevice->Pasid;
        MyDevice->CreateDevice.hKmdProcess = pCreateDevice->hKmdProcess;
    }

    /*
     * pass the call to inbox driver
     */
    ntStatus = (*DriverInitData->DxgkDdiCreateDevice)(hAdapter, pCreateDevice);
    if (!NT_SUCCESS(ntStatus))
    {
        LJB_FreePool(MyDevice);
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        return ntStatus;
    }

    LJB_DXGK_CreateDevicePostProcessing(Adapter, MyDevice, pCreateDevice);
    return ntStatus;
}

static VOID
LJB_DXGK_CreateDevicePostProcessing(
    __in LJB_ADAPTER *          Adapter,
    __in LJB_DEVICE *           MyDevice,
    __in DXGKARG_CREATEDEVICE * pCreateDevice
    )
{
    KIRQL                               oldIrql;

    UNREFERENCED_PARAMETER(Adapter);

    /*
     * track what the driver returns.
     */
    MyDevice->hDevice = pCreateDevice->hDevice;
    if (pCreateDevice->pInfo != MyDevice->CreateDevice.pInfo)
        MyDevice->DeviceInfo = *pCreateDevice->pInfo;

    KeAcquireSpinLock(&GlobalDriverData.ClientDeviceListLock, &oldIrql);
    InsertTailList(&GlobalDriverData.ClientDeviceListHead, &MyDevice->ListEntry);
    KeReleaseSpinLock(&GlobalDriverData.ClientDeviceListLock, oldIrql);

    //DBG_PRINT(Adapter, DBGLVL_FLOW,
    //    (__FUNCTION__ ": hDevice(%p)/hRTDevice(%p)/Flags(0x%x) tracked\n",
    //    MyDevice->hDevice,
    //    MyDevice->hRTDevice,
    //    MyDevice->CreateDevice.Flags.Value
    //    ));
}

LJB_DEVICE *
LJB_DXGK_FindDevice(
    __in HANDLE     hDevice
    )
{
    LIST_ENTRY * CONST listHead = &GlobalDriverData.ClientDeviceListHead;
    LIST_ENTRY * listEntry;
    LJB_DEVICE * MyDevice;
    KIRQL        oldIrql;

    MyDevice = NULL;
    KeAcquireSpinLock(&GlobalDriverData.ClientDeviceListLock, &oldIrql);
    for (listEntry = listHead->Flink;
         listEntry != listHead;
         listEntry = listEntry->Flink)
    {
        LJB_DEVICE * thisDevice;

        thisDevice = CONTAINING_RECORD(listEntry, LJB_DEVICE, ListEntry);
        if (thisDevice->hDevice == hDevice)
        {
            MyDevice = thisDevice;
            break;
        }
    }
    KeReleaseSpinLock(&GlobalDriverData.ClientDeviceListLock, oldIrql);

    return MyDevice;
}