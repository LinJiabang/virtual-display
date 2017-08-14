/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Toaster.c

Abstract:

    This is a featured version of the toaster function driver. This version
    shows how to register for PNP and Power events, handle create & close
    file requests, handle WMI set and query events, fire WMI notification
    events. This driver is functionally equivalent to the feature2 version
    of toaster WDM driver present in the DDK
    (src\general\toaster\func\featured2).


Environment:

    Kernel mode

--*/

#include "ljb_vmon_private.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, LJB_VMON_EvtDeviceAdd)
#pragma alloc_text (PAGE, LJB_VMON_EvtDeviceFileCreate)

#pragma alloc_text (PAGE, LJB_VMON_EvtDevicePrepareHardware)

#pragma alloc_text (PAGE, LJB_VMON_EvtIoDeviceControl)

#endif

ULONG DebugLevel = 3; //unused

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry specifies the other entry
    points in the function driver, such as ToasterAddDevice and ToasterUnload.

Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
    into memory. DriverEntry must initialize members of DriverObject before it
    returns to the caller. DriverObject is allocated by the system before the
    driver is loaded, and it is released by the system after the system unloads
    the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
    The function driver can use the path to store driver related data between
    reboots. The path does not store hardware instance specific data.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
    NTSTATUS            ntStatus = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG   config;

    KdPrint(("Built %s %s\n", __DATE__, __TIME__));

    //
    // Initialize driver config to control the attributes that
    // are global to the driver. Note that framework by default
    // provides a driver unload routine. If you create any resources
    // in the DriverEntry and want to be cleaned in driver unload,
    // you can override that by manually setting the EvtDriverUnload in the
    // config structure. In general xxx_CONFIG_INIT macros are provided to
    // initialize most commonly used members.
    //
    WDF_DRIVER_CONFIG_INIT(
        &config,
        LJB_VMON_EvtDeviceAdd
        );

    //
    // Create a framework driver object to represent our driver.
    //
    ntStatus = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES, // Driver Attributes
        &config,          // Driver Config Info
        WDF_NO_HANDLE
        );

    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint( ("WdfDriverCreate failed with ntStatus 0x%x\n", ntStatus));
    }
    return ntStatus;
}

/*++
Routine Description:

    LJB_VMON_EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of toaster device.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
NTSTATUS
LJB_VMON_EvtDeviceAdd(
    IN WDFDRIVER       Driver,
    IN PWDFDEVICE_INIT DeviceInit
    )
{
    NTSTATUS                              ntStatus = STATUS_SUCCESS;
    WDF_PNPPOWER_EVENT_CALLBACKS          pnpPowerCallbacks;
    WDF_OBJECT_ATTRIBUTES                 fdoAttributes;
    WDFDEVICE                             device;
    WDF_FILEOBJECT_CONFIG                 fileConfig;
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
    WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS wakeSettings;
    WDF_POWER_POLICY_EVENT_CALLBACKS      powerPolicyCallbacks;
    WDF_IO_QUEUE_CONFIG                   queueConfig;
    LJB_VMON_CTX *                        dev_ctx;
    WDFQUEUE                              queue;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    KdPrint((__FUNCTION__ ": entered\n"));

    //
    // Initialize the pnpPowerCallbacks structure.  Callback events for PNP
    // and Power are specified here.  If you don't supply any callbacks,
    // the Framework will take appropriate default actions based on whether
    // DeviceInit is initialized to be an FDO, a PDO or a filter device
    // object.
    //
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

    //
    // Register PNP callbacks.
    //
    pnpPowerCallbacks.EvtDevicePrepareHardware = LJB_VMON_EvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = LJB_VMON_EvtDeviceReleaseHardware;
    pnpPowerCallbacks.EvtDeviceSurpriseRemoval = LJB_VMON_EvtDeviceSurpriseRemoval;
    pnpPowerCallbacks.EvtDeviceSelfManagedIoInit = LJB_VMON_EvtDeviceSelfManagedIoInit;

    //
    // Register Power callbacks.
    //
    pnpPowerCallbacks.EvtDeviceD0Entry = LJB_VMON_EvtDeviceD0Entry;
    pnpPowerCallbacks.EvtDeviceD0Exit = LJB_VMON_EvtDeviceD0Exit;


    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    //
    // Register power policy event callbacks so that we would know when to
    // arm/disarm the hardware to handle wait-wake and when the wake event
    // is triggered by the hardware.
    //
    WDF_POWER_POLICY_EVENT_CALLBACKS_INIT(&powerPolicyCallbacks);

    //
    // This group of three callbacks allows this sample driver to manage
    // arming the device for wake from the S0 or Sx state.  We don't really
    // differentiate between S0 and Sx state..
    //
    powerPolicyCallbacks.EvtDeviceArmWakeFromS0 = LJB_VMON_EvtDeviceArmWakeFromS0;
    powerPolicyCallbacks.EvtDeviceDisarmWakeFromS0 = LJB_VMON_EvtDeviceDisarmWakeFromS0;
    powerPolicyCallbacks.EvtDeviceWakeFromS0Triggered = LJB_VMON_EvtDeviceWakeFromS0Triggered;
    powerPolicyCallbacks.EvtDeviceArmWakeFromSx = LJB_VMON_EvtDeviceArmWakeFromSx;
    powerPolicyCallbacks.EvtDeviceDisarmWakeFromSx = LJB_VMON_EvtDeviceDisarmWakeFromSx;
    powerPolicyCallbacks.EvtDeviceWakeFromSxTriggered = LJB_VMON_EvtDeviceWakeFromSxTriggered;

    //
    // Register the power policy callbacks.
    //
    WdfDeviceInitSetPowerPolicyEventCallbacks(DeviceInit, &powerPolicyCallbacks);

    //
    // Initialize WDF_FILEOBJECT_CONFIG_INIT struct to tell the
    // framework whether you are interested in handling Create, Close and
    // Cleanup requests that gets genereate when an application or another
    // kernel component opens an handle to the device. If you don't register,
    // the framework default behaviour would be complete these requests
    // with STATUS_SUCCESS. A driver might be interested in registering these
    // events if it wants to do security validation and also wants to maintain
    // per handle (fileobject) context.
    //
    WDF_FILEOBJECT_CONFIG_INIT(
        &fileConfig,
        LJB_VMON_EvtDeviceFileCreate,
        LJB_VMON_EvtFileClose,
        WDF_NO_EVENT_CALLBACK // not interested in Cleanup
        );

    WdfDeviceInitSetFileObjectConfig(
        DeviceInit,
        &fileConfig,
        WDF_NO_OBJECT_ATTRIBUTES
        );

    //
    // Now specify the size of device extension where we track per device
    // context. Along with setting the context type as shown below, you should also
    // specify the WDF_DECLARE_CONTEXT_TYPE_WITH_NAME in header to specify the
    // accessor function name.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&fdoAttributes, LJB_VMON_CTX);

    //
    // Set a context cleanup routine to cleanup any resources that are not
    // parent to this device. This cleanup will be called in the context of
    // pnp remove-device when the framework deletes the device object.
    //
    fdoAttributes.EvtCleanupCallback = LJB_VMON_EvtDeviceContextCleanup;

    WdfDeviceInitSetIoInCallerContextCallback(
        DeviceInit,
        &LJB_VMON_IoInCallerContext
        );

    //
    // DeviceInit is completely initialized. So call the framework to create the
    // device and attach it to the lower stack.
    //
    ntStatus = WdfDeviceCreate(&DeviceInit, &fdoAttributes, &device);
    if (!NT_SUCCESS(ntStatus))
        {
        KdPrint( ("WdfDeviceCreate failed with Status code 0x%x\n", ntStatus));
        return ntStatus;
        }

    //
    // Get the device context by using accessor function specified in
    // the WDF_DECLARE_CONTEXT_TYPE_WITH_NAME macro for LJB_VMON_CTX.
    //
    dev_ctx = LJB_VMON_GetVMonCtx(device);
    dev_ctx->DebugLevel = 0xFFFFFFFF;

    // Initial some parameters.
    dev_ctx->LastSentFrameId  = 0;
    dev_ctx->LatestFrameId = 0;

    //
    // Tell the Framework that this device will need an interface so that
    // application can find our device and talk to it.
    //
    ntStatus = WdfDeviceCreateDeviceInterface(
        device,
        (LPGUID) &LJB_MONITOR_INTERFACE_GUID,
        NULL);
    if (!NT_SUCCESS (ntStatus))
    {
        KdPrint((__FUNCTION__
            ": WdfDeviceCreateDeviceInterface failed 0x%x\n", ntStatus));
        return ntStatus;
    }

    ntStatus = WdfDeviceCreateDeviceInterface(
        device,
        (LPGUID) &GUID_LCI_USBAV,
        NULL);
    if (!NT_SUCCESS (ntStatus))
    {
        KdPrint((__FUNCTION__
            ": WdfDeviceCreateDeviceInterface failed 0x%x\n", ntStatus));
        return ntStatus;
    }

    /*
     * we will enable device interface in IRP_MN_START_DEVICE processing.
     */
    WdfDeviceSetDeviceInterfaceState(
        device,
        (LPGUID) &LJB_MONITOR_INTERFACE_GUID,
        NULL,
        FALSE
        );
    WdfDeviceSetDeviceInterfaceState(
        device,
        (LPGUID) &GUID_LCI_USBAV,
        NULL,
        FALSE
        );

    //
    // Register I/O callbacks to tell the framework that you are interested
    // in handling IRP_MJ_READ, IRP_MJ_WRITE, and IRP_MJ_DEVICE_CONTROL requests.
    // In case a specific handler is not specified for one of these,
    // the request will be dispatched to the EvtIoDefault handler, if any.
    // If there is no EvtIoDefault handler, the request will be failed with
    // STATUS_INVALID_DEVICE_REQUEST.
    // WdfIoQueueDispatchParallel means that we are capable of handling
    // all the I/O request simultaneously and we are responsible for protecting
    // data that could be accessed by these callbacks simultaneously.
    // A default queue gets all the requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
        &queueConfig,
        WdfIoQueueDispatchParallel
        );

    queueConfig.EvtIoDeviceControl = LJB_VMON_EvtIoDeviceControl;
    queueConfig.EvtIoInternalDeviceControl = LJB_VMON_InternalDeviceIoControl;
    queueConfig.EvtIoStop = &LJB_VMON_EvtIoStop;

    ntStatus = WdfIoQueueCreate(
        device,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &queue
        );
    if (!NT_SUCCESS (ntStatus))
    {
        KdPrint((__FUNCTION__
            ": WdfIoQueueCreate failed 0x%x\n", ntStatus));
        return ntStatus;
    }

    //
    // Set the idle power policy to put the device to Dx if the device is not used
    // for the specified IdleTimeout time. Since this is a virtual device we
    // tell the framework that we cannot wake ourself if we sleep in S0. Only
    // way the device can be brought to D0 is if the device recieves an I/O from
    // the system.
    //
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleCannotWakeFromS0);
    idleSettings.IdleTimeout = 60000; // 60 secs idle timeout
    ntStatus = WdfDeviceAssignS0IdleSettings(device, &idleSettings);
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint((__FUNCTION__
            ":WdfDeviceAssignS0IdleSettings failed 0x%x\n", ntStatus));
        return ntStatus;
    }

    //
    // Set the wait-wake policy.
    //
    WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_INIT(&wakeSettings);
    ntStatus = WdfDeviceAssignSxWakeSettings(device, &wakeSettings);
    if (!NT_SUCCESS(ntStatus))
    {
        //
        // We are probably enumerated on a bus that doesn't support Sx-wake.
        // Let us not fail the device add just because we aren't able to support
        // wait-wake. I will let the user of this sample decide how important it's
        // to support wait-wake for their hardware and return appropriate ntStatus.
        //
        KdPrint( ("WdfDeviceAssignSxWakeSettings failed 0x%x\n", ntStatus));
        ntStatus = STATUS_SUCCESS;
    }

    //
    // Finally register all our WMI datablocks with WMI subsystem.
    //
    ntStatus = VMON_WmiRegistration(device);

    //
    // Please note that if this event fails or eventually device gets removed
    // the framework will automatically take care of deregistering with
    // WMI, detaching and deleting the deviceobject and cleaning up other
    // resources. Framework does most of the resource cleanup during device
    // remove and driver unload.
    //
    return ntStatus;
    }

/*++

Routine Description:

    EvtDevicePrepareHardware event callback performs operations that are
    necessary to make the driver's device operational. The framework calls the
    driver's EvtDevicePrepareHardware callback when the PnP manager sends an
    IRP_MN_START_DEVICE request to the driver stack.

    Specifically, most drivers will use this callback to map resources.  USB
    drivers may use it to get device descriptors, config descriptors and to
    select configs.

    Some drivers may choose to download firmware to a device in this callback,
    but that is usually only a good choice if the device firmware won't be
    destroyed by a D0 to D3 transition.  If firmware will be gone after D3,
    then firmware downloads should be done in EvtDeviceD0Entry, not here.

Arguments:

    Device - Handle to a framework device object.

    ResourcesRaw - Handle to a collection of framework resource objects.
                This collection identifies the raw (bus-relative) hardware
                resources that have been assigned to the device.

    ResourcesTranslated - Handle to a collection of framework resource objects.
                This collection identifies the translated (system-physical)
                hardware resources that have been assigned to the device.
                The resources appear from the CPU's point of view.
                Use this list of resources to map I/O space and
                device-accessible memory into virtual address space

Return Value:

    WDF ntStatus code

--*/
NTSTATUS
LJB_VMON_EvtDevicePrepareHardware(
    WDFDEVICE      Device,
    WDFCMRESLIST   ResourcesRaw,
    WDFCMRESLIST   ResourcesTranslated
    )
{
    LJB_VMON_CTX *  dev_ctx = LJB_VMON_GetVMonCtx(Device);
    NTSTATUS        ntStatus = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(ResourcesRaw);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    PAGED_CODE();

    KeInitializeSpinLock(&dev_ctx->surface_lock);
    InitializeListHead(&dev_ctx->surface_list);

    KeInitializeSpinLock(&dev_ctx->event_req_lock);
    InitializeListHead(&dev_ctx->event_req_list);
    KeInitializeSpinLock(&dev_ctx->ioctl_lock);

    KdPrint((__FUNCTION__": entered\n"));

    WdfDeviceSetDeviceInterfaceState(
        Device,
        (LPGUID) &LJB_MONITOR_INTERFACE_GUID,
        NULL,
        TRUE);

    //
    // Fire device arrival event.
    //
    LJB_VMON_FireArrivalEvent(Device);
    return ntStatus;
}

NTSTATUS
LJB_VMON_EvtDeviceReleaseHardware(
    IN  WDFDEVICE    Device,
    IN  WDFCMRESLIST ResourcesTranslated
    )
{
    LJB_VMON_CTX *                  dev_ctx = LJB_VMON_GetVMonCtx(Device);
    LIST_ENTRY * CONST              list_head = &dev_ctx->event_req_list;
    LJB_VMON_WAIT_FOR_EVENT_REQ *  wait_event_req;
    LIST_ENTRY *                    list_entry;
    KIRQL                           old_irql;

    UNREFERENCED_PARAMETER(ResourcesTranslated);

    KdPrint((__FUNCTION__": entered\n"));
    WdfDeviceSetDeviceInterfaceState(
        Device,
        (LPGUID) &LJB_MONITOR_INTERFACE_GUID,
        NULL,
        FALSE);

    WdfDeviceSetDeviceInterfaceState(
        Device,
        (LPGUID) &GUID_LCI_USBAV,
        NULL,
        FALSE);
    /*
     * Release any pending Wait request
     */
    KeAcquireSpinLock(&dev_ctx->event_req_lock, &old_irql);
    while (!IsListEmpty(list_head))
    {
        list_entry = RemoveHeadList(list_head);
        wait_event_req = CONTAINING_RECORD(
                list_entry,
                LJB_VMON_WAIT_FOR_EVENT_REQ,
                list_entry
                );
        WdfRequestCompleteWithInformation(
            wait_event_req->Request,
            STATUS_CANCELLED,
            0
            );
        LJB_VMON_Printf(dev_ctx, DBGLVL_FLOW,
            (" " __FUNCTION__ ": "
            "Complete wait_event_req(%p) immediately with STATUS_CANCELLED\n",
            wait_event_req
            ));
        LJB_VMON_FreePool(wait_event_req);
    }
    KeReleaseSpinLock(&dev_ctx->event_req_lock, old_irql);

    return STATUS_SUCCESS;
}

VOID
LJB_VMON_EvtDeviceSurpriseRemoval(
    IN  WDFDEVICE    Device
    )
{
    LJB_VMON_CTX *                  dev_ctx = LJB_VMON_GetVMonCtx(Device);
    LIST_ENTRY * CONST              list_head = &dev_ctx->event_req_list;
    LJB_VMON_WAIT_FOR_EVENT_REQ *  wait_event_req;
    LIST_ENTRY *                    list_entry;
    KIRQL                           old_irql;

    KdPrint((__FUNCTION__ ": entered\n"));

    /*
     * Release any pending Wait request
     */
    KeAcquireSpinLock(&dev_ctx->event_req_lock, &old_irql);
    while (!IsListEmpty(list_head))
    {
        list_entry = RemoveHeadList(list_head);
        wait_event_req = CONTAINING_RECORD(
                list_entry,
                LJB_VMON_WAIT_FOR_EVENT_REQ,
                list_entry
                );
        WdfRequestCompleteWithInformation(
            wait_event_req->Request,
            STATUS_CANCELLED,
            0
            );
        LJB_VMON_Printf(dev_ctx, DBGLVL_FLOW,
            (__FUNCTION__
            ": Complete wait_event_req(%p) with STATUS_CANCELLED\n",
            wait_event_req
            ));
        LJB_VMON_FreePool(wait_event_req);
    }
    KeReleaseSpinLock(&dev_ctx->event_req_lock, old_irql);
}

/*++

Routine Description:

    EvtDeviceSelfManagedIoInit is called it once for each device,
    after the framework has called the driver's EvtDeviceD0Entry
    callback function for the first time. The framework does not
    call the EvtDeviceSelfManagedIoInit callback function again for
    that device, unless the device is removed and reconnected, or
    the drivers are reloaded.

    The EvtDeviceSelfManagedIoInit callback function must initialize
    the self-managed I/O operations that the driver will handle
    for the device.

    This function is not marked pageable because this function is in the
    device power up path. When a function is marked pagable and the code
    section is paged out, it will generate a page fault which could impact
    the fast resume behavior because the client driver will have to wait
    until the system drivers can service this page fault.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    NTSTATUS - Failures will result in the device stack being torn down.

--*/
NTSTATUS
LJB_VMON_EvtDeviceSelfManagedIoInit(
    IN  WDFDEVICE Device
    )
{
    LJB_VMON_CTX * CONST    dev_ctx = LJB_VMON_GetVMonCtx(Device);
    NTSTATUS                ntStatus;

    KdPrint((__FUNCTION__": entered\n"));

    //
    // We will provide an example on how to get a bus-specific direct
    // call interface from a bus driver.
    //
    ntStatus = WdfFdoQueryForInterface(
        Device,
        &GUID_TOASTER_INTERFACE_STANDARD,
        (PINTERFACE) &dev_ctx->BusInterface,
        sizeof(TOASTER_INTERFACE_STANDARD),
        1,
        NULL
        );
    if(NT_SUCCESS(ntStatus))
    {
        UCHAR powerlevel;

        //
        // Call the direct callback functions to get the property or
        // configuration information of the device.
        //
        (*dev_ctx->BusInterface.GetCrispinessLevel)(
            dev_ctx->BusInterface.InterfaceHeader.Context,
            &powerlevel
            );
        (*dev_ctx->BusInterface.SetCrispinessLevel)(
            dev_ctx->BusInterface.InterfaceHeader.Context,
            8
            );
        (*dev_ctx->BusInterface.IsSafetyLockEnabled)(
            dev_ctx->BusInterface.InterfaceHeader.Context
            );

        //
        // Provider of this interface may have taken a reference on it.
        // So we must release the interface as soon as we are done using it.
        //
        (*dev_ctx->BusInterface.InterfaceHeader.InterfaceDereference)(
            (PVOID)dev_ctx->BusInterface.InterfaceHeader.Context
            );
    }
    else
    {
        //
        // In this sample, we don't want to fail start just because we weren't
        // able to get the direct-call interface. If this driver is loaded on top
        // of a bus other than toaster, ToasterGetStandardInterface will return
        // an error.
        //
        ntStatus = STATUS_SUCCESS;
    }
    return ntStatus;
}

/*++

Routine Description:

   EvtDeviceContextCleanup event callback must perform any operations that are
   necessary before the specified device is removed. The framework calls
   the driver's EvtDeviceContextCleanup callback when the device is deleted in response
   to IRP_MN_REMOVE_DEVICE request.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    WDF ntStatus code

--*/
VOID
LJB_VMON_EvtDeviceContextCleanup(
    IN WDFDEVICE Device
    )
{
    UNREFERENCED_PARAMETER(Device);
}

/*++

Routine Description:

    The framework calls a driver's EvtDeviceFileCreate callback
    when the framework receives an IRP_MJ_CREATE request.
    The system sends this request when a user application opens the
    device to perform an I/O operation, such as reading or writing to a device.
    This callback is called in the context of the thread
    that created the IRP_MJ_CREATE request.

Arguments:

    Device - Handle to a framework device object.
    FileObject - Pointer to fileobject that represents the open handle.
    CreateParams - Parameters for create

Return Value:

   NT ntStatus code

--*/
VOID
LJB_VMON_EvtDeviceFileCreate (
    IN WDFDEVICE Device,
    IN WDFREQUEST Request,
    IN WDFFILEOBJECT FileObject
    )
{
    LJB_VMON_CTX * CONST    dev_ctx = LJB_VMON_GetVMonCtx(Device);

    UNREFERENCED_PARAMETER(FileObject);

    PAGED_CODE ();

    KdPrint((__FUNCTION__": entered\n"));
    WdfRequestComplete(Request, STATUS_SUCCESS);

    return;
}


VOID
LJB_VMON_EvtFileClose (
    IN WDFFILEOBJECT    FileObject
    )
{
    WDFDEVICE CONST         Device =  WdfFileObjectGetDevice(FileObject);
    LJB_VMON_CTX * CONST    dev_ctx = LJB_VMON_GetVMonCtx(Device);

    DBG_UNREFERENCED_LOCAL_VARIABLE(dev_ctx);
    KdPrint((__FUNCTION__": entered\n"));

}
