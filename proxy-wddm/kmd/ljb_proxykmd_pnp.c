/*
 * ljb_proxykmd_pnp.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"
#include "ljb_proxykmd_guid.h"
#include <wdmguid.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_PROXYKMD_PnpStart)
#pragma alloc_text (PAGE, LJB_PROXYKMD_PnpStop)
#pragma alloc_text (PAGE, LJB_PROXYKMD_OpenTargetDevice)
#pragma alloc_text (PAGE, LJB_PROXYKMD_GetTargetDevicePdo)
#pragma alloc_text (PAGE, LJB_PROXYKMD_CloseTargetDevice)
#endif

NTSTATUS
LJB_PROXYKMD_PnpStart(
    __in LJB_ADAPTER *      Adapter
    )
{
    LJB_CLIENT_DRIVER_DATA * CONST  ClientDriverData = Adapter->ClientDriverData;
    DRIVER_OBJECT * CONST           DriverObject = ClientDriverData->DriverObject;
    NTSTATUS                        ntStatus;

    PAGED_CODE();

    /*
     * Register for LJB_MONITOR_INTERFACE_GUID interface changes.
     *
     * We will get an ARRIVAL callback for every LJB MONITOR device that is
     * started and a REMOVAL callback for every LJB MONITOR that is removed.
     */
    ntStatus = IoRegisterPlugPlayNotification(
        EventCategoryDeviceInterfaceChange,
        PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
        (PVOID)&LJB_MONITOR_INTERFACE_GUID,
        DriverObject,
        &LBJ_PROXYKMD_PnpNotifyInterfaceChange,
        Adapter,
        &Adapter->NotificationHandle
        );

    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": IoRegisterPlugPlayNotification failed with ntStatus(0x%08x)?\n",
            ntStatus
            ));
    }
    return ntStatus;
};

VOID
LJB_PROXYKMD_PnpStop(
    __in LJB_ADAPTER *      Adapter
    )
{
    if (Adapter->NotificationHandle != NULL)
    {
        IoUnregisterPlugPlayNotification(Adapter->NotificationHandle);
        Adapter->NotificationHandle = NULL;
        DBG_PRINT(Adapter, DBGLVL_PNP,
            (__FUNCTION__": Adapter(%p)\n", Adapter));
    }
}

LJB_MONITOR_NODE *
LJB_PROXYKMD_FindMonitorNode(
    __in LJB_ADAPTER *      Adapter,
    __in PUNICODE_STRING    SymbolicLinkName
    )
{
    LJB_MONITOR_NODE *  MonitorNode;
    LIST_ENTRY * CONST  ListHead = &Adapter->MonitorNodeListHead;
    LIST_ENTRY *        listEntry;
    KIRQL               oldIrql;

    MonitorNode = NULL;
    KeAcquireSpinLock(&Adapter->MonitorNodeListLock, &oldIrql);

    for (listEntry = ListHead->Flink;
         listEntry != ListHead;
         listEntry = listEntry->Flink)
    {
        LJB_MONITOR_NODE * thisNode;

        thisNode = CONTAINING_RECORD(listEntry, LJB_MONITOR_NODE, ListEntry);
        if (RtlCompareUnicodeString(&thisNode->SymbolicLink, SymbolicLinkName, FALSE) == 0)
        {
            MonitorNode = thisNode;
            break;
        }
    }
    KeReleaseSpinLock(&Adapter->MonitorNodeListLock, oldIrql);

    return MonitorNode;
}

/*
 *    This routine is the PnP "interface change notification" callback routine.
 *
 *    This gets called on a LJB_MONITOR triggered device interface arrival or
 *    removal.
 *      - Interface arrival corresponds to a LJB_MONITOR device being STARTED
 *      - Interface removal corresponds to a LJB_MONITOR device being REMOVED
 *
 *    On arrival:
 *      - Get the target deviceobject pointer by using the symboliclink.
 *      - Get the PDO of the target device, in case you need to set the
 *        device parameters of the target device.
 *      - Regiter for EventCategoryTargetDeviceChange notification on the fileobject
 *        so that you can cleanup whenever associated device is removed.
 *
 *    On removal:
 *      - This callback is a NO-OP for interface removal because we
 *      for PnP EventCategoryTargetDeviceChange callbacks and
 *      use that callback to clean up when their associated toaster device goes
 *      away.
 *
 * Return Value:
 *    STATUS_SUCCESS - always, even if something goes wrong
 *
 */
NTSTATUS
LBJ_PROXYKMD_PnpNotifyInterfaceChange(
    __in PDEVICE_INTERFACE_CHANGE_NOTIFICATION  NotificationStruct,
    __in PVOID                                  Context
    )
{
    LJB_ADAPTER * CONST     Adapter = Context;
    PUNICODE_STRING CONST   SymbolicLinkName = NotificationStruct->SymbolicLinkName;
    LJB_MONITOR_NODE *      MonitorNode;
    PIO_WORKITEM            IoWorkItem;
    KIRQL                   oldIrql;

    if (!IsEqualGUID( (LPGUID)&(NotificationStruct->InterfaceClassGuid),
                      (LPGUID)&LJB_MONITOR_INTERFACE_GUID) )
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR, (__FUNCTION__": Bad interfaceClassGuid\n"));
        return STATUS_SUCCESS;
    }

    if (IsEqualGUID((LPGUID)&(NotificationStruct->Event),
                    (LPGUID)&GUID_DEVICE_INTERFACE_ARRIVAL))
    {
        /*
         * Warning  If the caller specifies PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
         * the operating system might call the PnP notification callback routine
         * twice for a single EventCategoryDeviceInterfaceChange event for an existing
         * interface. You can safely ignore the second call to the callback. The
         * operating system will not call the callback more than twice for a single
         * event.
         */
        MonitorNode = LJB_PROXYKMD_FindMonitorNode(Adapter, SymbolicLinkName);
        if (MonitorNode != NULL)
        {
        DBG_PRINT(Adapter, DBGLVL_PNP,
            (__FUNCTION__ ": duplicated arrival (%ws)\n",
            SymbolicLinkName->Buffer));
            return STATUS_SUCCESS;
        }

        DBG_PRINT(Adapter, DBGLVL_PNP,
            (__FUNCTION__ ": first arrival (%ws)\n",
            SymbolicLinkName->Buffer));

        //
        // Allocate memory for the MonitorNode
        //
        MonitorNode = LJB_PROXYKMD_GetPoolZero(sizeof(LJB_MONITOR_NODE));
        if (MonitorNode == NULL)
        {
            DBG_PRINT(Adapter, DBGLVL_ERROR,
                ("?" __FUNCTION__ ": unable to allocate LJB_MONITOR_NODE\n"));
            return STATUS_SUCCESS;
        }

        //
        // Copy the symbolic link
        //
        MonitorNode->SymbolicLink.MaximumLength = SymbolicLinkName->Length +
                                          sizeof(UNICODE_NULL);
        MonitorNode->SymbolicLink.Length = SymbolicLinkName->Length;
        MonitorNode->SymbolicLink.Buffer = MonitorNode->NameBuffer;
        RtlCopyUnicodeString(&MonitorNode->SymbolicLink, SymbolicLinkName);

        MonitorNode->Adapter = Adapter;
        InitializeListHead(&MonitorNode->ListEntry);
        MonitorNode->ReferenceCount = 1;

        //
        // Finally queue the Deviceinfo.
        //
        KeAcquireSpinLock(&Adapter->MonitorNodeListLock, &oldIrql);
        InsertTailList(&Adapter->MonitorNodeListHead, &MonitorNode->ListEntry);
        KeReleaseSpinLock(&Adapter->MonitorNodeListLock, oldIrql);
        InterlockedIncrement(&Adapter->MonitorNodeListCount);

        IoWorkItem = IoAllocateWorkItem(Adapter->DxgkInterface.DeviceHandle);
        if (IoWorkItem == NULL)
        {
            KeAcquireSpinLock(&Adapter->MonitorNodeListLock, &oldIrql);
            RemoveEntryList(&MonitorNode->ListEntry);
            KeReleaseSpinLock(&Adapter->MonitorNodeListLock, oldIrql);\
            InterlockedDecrement(&Adapter->MonitorNodeListCount);
            LJB_PROXYKMD_FreePool(MonitorNode);
            return STATUS_SUCCESS;
        }

        IoQueueWorkItemEx(
            IoWorkItem,
            &LJB_PROXYKMD_OpenTargetDeviceWorkItem,
            DelayedWorkQueue,
            MonitorNode
            );
    }
    else if (IsEqualGUID((LPGUID)&(NotificationStruct->Event),
                    (LPGUID)&GUID_DEVICE_INTERFACE_REMOVAL))
    {
        DBG_PRINT(Adapter, DBGLVL_PNP,
            (__FUNCTION__":Removal Interface Notification\n"));
    }

    return STATUS_SUCCESS;
}

void
LJB_PROXYKMD_OpenTargetDeviceWorkItem(
    _In_     PVOID        IoObject,
    _In_opt_ PVOID        Context,
    _In_     PIO_WORKITEM IoWorkItem
    )
{
    LJB_MONITOR_NODE *  CONST   MonitorNode = Context;
    LJB_ADAPTER * CONST         Adapter = MonitorNode->Adapter;
    NTSTATUS                    ntStatus;

    UNREFERENCED_PARAMETER(IoObject);

    ntStatus = LJB_PROXYKMD_OpenTargetDevice(MonitorNode);
    if (!NT_SUCCESS(ntStatus))
    {
        KIRQL oldIrql;

        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": LJB_PROXYKMD_OpenTargetDevice failed with (0x%08x)\n",
            ntStatus
            ));
        KeAcquireSpinLock(&Adapter->MonitorNodeListLock, &oldIrql);
        RemoveEntryList(&MonitorNode->ListEntry);
        KeReleaseSpinLock(&Adapter->MonitorNodeListLock, oldIrql);
        InterlockedDecrement(&Adapter->MonitorNodeListCount);
        LJB_PROXYKMD_CloseTargetDevice(MonitorNode);
    }

    IoFreeWorkItem(IoWorkItem);
}
/*
 * This routine is the PnP "Device Change Notification" callback routine.
 * This gets called on a when the target is query or surprise removed.
 *
 *   - Interface arrival corresponds to a toaster device being STARTed
 *   - Interface removal corresponds to a toaster device being REMOVEd
 *
 * On Query_Remove or Remove_Complete:
 *   - Find the targetdevice from the list by matching the fileobject pointers.
 *   - Dereference the FileObject (this generates a  close to the target device)
 *   - Free the resources.
 *
 * Arguments:
 *  NotificationStruct  - Structure defining the change.
 *
 *  Context -    pointer to the device extension.
 *               (supplied as the "context" when we
 *               registered for this callback)
 * Return Value:
 *  STATUS_SUCCESS - always, even if something goes wrong
 */
NTSTATUS
LJB_PROXYKMD_PnpNotifyDeviceChange(
    PTARGET_DEVICE_REMOVAL_NOTIFICATION    NotificationStruct,
    PVOID                                  Context
    )
{
    LJB_MONITOR_NODE * CONST        MonitorNode = Context;
    LJB_ADAPTER * CONST             Adapter = MonitorNode->Adapter;
    NTSTATUS                        ntStatus;
    KIRQL                           oldIrql;

    PAGED_CODE();

    //
    // if the event is query_remove
    //
    if ((IsEqualGUID((LPGUID)&(NotificationStruct->Event),
                     (LPGUID)&GUID_TARGET_DEVICE_QUERY_REMOVE)))
    {
        PTARGET_DEVICE_REMOVAL_NOTIFICATION removalNotification;

        removalNotification =
                    (PTARGET_DEVICE_REMOVAL_NOTIFICATION)NotificationStruct;

        DBG_PRINT(Adapter, DBGLVL_PNP,
            (__FUNCTION__" : QueryRemove on (%ws)\n", MonitorNode->NameBuffer
            ));

        ASSERT(MonitorNode->FileObject == removalNotification->FileObject);
        //
        // Deref the fileobject so that we don't prevent
        // the target device from being removed.
        //
        ObDereferenceObject(MonitorNode->FileObject);
        MonitorNode->FileObject = NULL;
        //
        // Deref the PDO to compensate for the reference taken
        // by the bus driver when it returned the PDO in response
        // to the query-device-relations (target-relations).
        //
        ObDereferenceObject(MonitorNode->PDO);
        MonitorNode->PDO = NULL;
        //
        // We will defer freeing other resources to remove-complete
        // notification because if query-remove is vetoed, we would reopen
        // the device in remove-cancelled notification.
        //

    }
    else if (IsEqualGUID((LPGUID)&(NotificationStruct->Event),
                         (LPGUID)&GUID_TARGET_DEVICE_REMOVE_COMPLETE))
    {
        //
        // Device is gone. Let us cleanup our resources.
        //
        DBG_PRINT(Adapter, DBGLVL_PNP,
            ("RemoveComplete on %ws\n", MonitorNode->NameBuffer
            ));

        KeAcquireSpinLock(&Adapter->MonitorNodeListLock, &oldIrql);
        RemoveEntryList(&MonitorNode->ListEntry);
        KeReleaseSpinLock(&Adapter->MonitorNodeListLock, oldIrql);
        InterlockedDecrement(&Adapter->MonitorNodeListCount);

        LJB_PROXYKMD_CloseTargetDevice(MonitorNode);

    }
    else if (IsEqualGUID((LPGUID)&(NotificationStruct->Event),
                         (LPGUID)&GUID_TARGET_DEVICE_REMOVE_CANCELLED))
    {

        DBG_PRINT(Adapter, DBGLVL_PNP,
            ("RemoveCancelled on (%ws)\n", MonitorNode->NameBuffer
            ));

        // Should be null because we cleared it in query-remove

        ASSERT(!MonitorNode->FileObject);
        ASSERT(!MonitorNode->PDO);

        //
        // Unregister the previous notification because when we reopen
        // the device we will register again on the new fileobject.
        //
        IoUnregisterPlugPlayNotification(MonitorNode->NotificationHandle);

        //
        // Reopen the device
        //
        ntStatus = LJB_PROXYKMD_OpenTargetDevice(MonitorNode);
        if (!NT_SUCCESS (ntStatus))
        {
            //
            // Couldn't reopen the device. Cleanup.
            //
            KeAcquireSpinLock (&Adapter->MonitorNodeListLock, &oldIrql);
            RemoveEntryList(&MonitorNode->ListEntry);
            KeReleaseSpinLock (&Adapter->MonitorNodeListLock, oldIrql);
            InterlockedDecrement(&Adapter->MonitorNodeListCount);

            LJB_PROXYKMD_CloseTargetDevice(MonitorNode);
        }
    }
    else
    {
        DBG_PRINT(Adapter, DBGLVL_PNP,
            (__FUNCTION__ ":Unknown Device Notification\n"));
    }

    return STATUS_SUCCESS;
}

NTSTATUS
LJB_PROXYKMD_GetTargetDevicePdo(
    __in PDEVICE_OBJECT DeviceObject,
    __out PDEVICE_OBJECT *PhysicalDeviceObject
    )
{
    KEVENT                  event;
    NTSTATUS                ntStatus;
    PIRP                    irp;
    IO_STATUS_BLOCK         ioStatusBlock;
    PIO_STACK_LOCATION      irpStack;
    PDEVICE_RELATIONS       deviceRelations;

    PAGED_CODE();

    KeInitializeEvent( &event, NotificationEvent, FALSE );
    irp = IoBuildSynchronousFsdRequest( IRP_MJ_PNP,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        &event,
                                        &ioStatusBlock );

    if (irp == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto End;
    }

    irpStack = IoGetNextIrpStackLocation( irp );
    irpStack->MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;
    irpStack->Parameters.QueryDeviceRelations.Type = TargetDeviceRelation;

    //
    // Initialize the ntStatus to error in case the bus driver decides not to
    // set it correctly.
    //
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED ;
    ntStatus = IoCallDriver( DeviceObject, irp );

    if (ntStatus == STATUS_PENDING)
    {
        KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );
        ntStatus = ioStatusBlock.Status;
    }

    if (NT_SUCCESS(ntStatus))
    {
        deviceRelations = (PDEVICE_RELATIONS)ioStatusBlock.Information;
        ASSERT(deviceRelations);
        //
        // You must dereference the PDO when it's no longer
        // required.
        //
        *PhysicalDeviceObject = deviceRelations->Objects[0];
        ExFreePool(deviceRelations);
    }

End:
    return ntStatus;
}

NTSTATUS
LJB_PROXYKM_QueryMonitorInterface(
    __in LJB_MONITOR_NODE * MonitorNode
    )
{
    LJB_ADAPTER * CONST Adapter = MonitorNode->Adapter;

    KEVENT                  event;
    NTSTATUS                ntStatus;
    PIRP                    irp;
    IO_STATUS_BLOCK         ioStatusBlock;
    PIO_STACK_LOCATION      irpStack;

    PAGED_CODE();

    KeInitializeEvent( &event, NotificationEvent, FALSE );
    irp = IoBuildSynchronousFsdRequest(
        IRP_MJ_PNP,
        MonitorNode->FDO,
        NULL,
        0,
        NULL,
        &event,
        &ioStatusBlock
        );

    if (irp == NULL) {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto End;
    }

    irpStack = IoGetNextIrpStackLocation( irp );
    irpStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    irpStack->Parameters.QueryInterface.InterfaceType = &LJB_MONITOR_INTERFACE_GUID;
    irpStack->Parameters.QueryInterface.Size = sizeof(LJB_MONITOR_INTERFACE);
    irpStack->Parameters.QueryInterface.Version = LJB_MONITOR_INTERFACE_V0;
    irpStack->Parameters.QueryInterface.Interface = (PINTERFACE)&MonitorNode->MonitorInterface;
    irpStack->Parameters.QueryInterface.InterfaceSpecificData = Adapter;

    //
    // Initialize the ntStatus to error in case the bus driver decides not to
    // set it correctly.
    //
    irp->IoStatus.Status = STATUS_NOT_SUPPORTED ;
    ntStatus = IoCallDriver( MonitorNode->FDO, irp );
    if (ntStatus == STATUS_PENDING)
    {
        KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL );
        ntStatus = ioStatusBlock.Status;
    }

    if (NT_SUCCESS(ntStatus))
    {
    }

End:
    return ntStatus;
}

static VOID
AssignChildUid(
    __in LJB_MONITOR_NODE * MonitorNode
    )
{
    LJB_ADAPTER * CONST Adapter = MonitorNode->Adapter;
    ULONG               Index;
    KIRQL               oldIrql;

    KeAcquireSpinLock(&Adapter->MonitorNodeListLock, &oldIrql);
    for (Index = 0; Index < MAX_NUM_OF_USB_MONITOR; Index++)
    {
        ULONG CONST Mask = (1 << Index);

        if ((Adapter->MonitorNodeMask & Mask) == 0)
        {
            Adapter->MonitorNodeMask |= Mask;
            MonitorNode->ChildUid = Adapter->UsbTargetIdBase + Index;
            DBG_PRINT(Adapter, DBGLVL_PNP,
                (__FUNCTION__ ": Assign ChildUid(0x%x) for MonitorNode(%p)\n",
                MonitorNode->ChildUid,
                MonitorNode
                ));
            break;
        }
    }
    KeReleaseSpinLock(&Adapter->MonitorNodeListLock, oldIrql);
}

static VOID
ReleaseChildUid(
    __in LJB_MONITOR_NODE * MonitorNode
    )
{
    LJB_ADAPTER * CONST Adapter = MonitorNode->Adapter;
    ULONG               ChildUid = MonitorNode->ChildUid;
    ULONG CONST         Index = ChildUid - Adapter->UsbTargetIdBase;
    KIRQL               oldIrql;

    if (Index < MAX_NUM_OF_USB_MONITOR)
    {
        ULONG CONST Mask = (1 << Index);

        KeAcquireSpinLock(&Adapter->MonitorNodeListLock, &oldIrql);
        Adapter->MonitorNodeMask &= ~Mask;
        KeReleaseSpinLock(&Adapter->MonitorNodeListLock, oldIrql);
    }
}

NTSTATUS
LJB_PROXYKMD_OpenTargetDevice(
    __in LJB_MONITOR_NODE * MonitorNode
    )
{
    LJB_ADAPTER * CONST             Adapter = MonitorNode->Adapter;
    LJB_CLIENT_DRIVER_DATA * CONST  ClientDriverData = Adapter->ClientDriverData;
    DRIVER_OBJECT * CONST           DriverObject = ClientDriverData->DriverObject;
    DXGKRNL_INTERFACE * CONST       DxgkInterface = &Adapter->DxgkInterface;
    DXGK_CHILD_STATUS               ChildStatus;
    NTSTATUS                        ntStatus;

    PAGED_CODE();

    /*
     * Get a pointer to and open a handle to the LBJ_MONITOR device
     */
    ntStatus = IoGetDeviceObjectPointer(
        &MonitorNode->SymbolicLink,
        STANDARD_RIGHTS_ALL,
        &MonitorNode->FileObject,
        &MonitorNode->FDO);

    if ( !NT_SUCCESS(ntStatus) )
    {
        goto Error;
    }

    //
    // Register for TargerDeviceChange notification on the fileobject.
    //
    ntStatus = IoRegisterPlugPlayNotification(
        EventCategoryTargetDeviceChange,
        0,
        MonitorNode->FileObject,
        DriverObject,
        LJB_PROXYKMD_PnpNotifyDeviceChange,
        MonitorNode,
        &MonitorNode->NotificationHandle);

    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": IoRegisterPlugPlayNotification failed (0x%08x)\n",
            ntStatus
            ));
        goto Error;
    }

    //
    // Get the PDO. This is required in case you need to set
    // the target device's parameters using IoOpenDeviceRegistryKey
    //
    ntStatus = LJB_PROXYKMD_GetTargetDevicePdo(
        MonitorNode->FDO,
        &MonitorNode->PDO
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": LJB_PROXYKMD_GetTargetDevicePdo failed (0x%08x)\n",
            ntStatus
            ));
        goto Error;
    }

    ntStatus = LJB_PROXYKM_QueryMonitorInterface(MonitorNode);
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": LJB_PROXYKM_QueryMonitorInterface failed (0x%08x)\n",
            ntStatus
            ));
        goto Error;
    }

    DBG_PRINT(Adapter, DBGLVL_PNP,
        (__FUNCTION__": FDO(%p), PDO(%p)\n",
        MonitorNode->FDO,
        MonitorNode->PDO
        ));

    /*
     * Calculate ChildUid
     */
    AssignChildUid(MonitorNode);

    /*
     * notify DXGK about Monitor arrival, after first commitVidPn occurs.
     */
    if (!Adapter->FirstVidPnArrived)
    {
        UINT    WaitMs;

        DBG_PRINT(Adapter, DBGLVL_PNP,
            (__FUNCTION__": waited for first VidPn event\n"));
        WaitMs = 0;
        while (!Adapter->FirstVidPnArrived)
        {
            LJB_PROXYKMD_DelayMs(10);
            WaitMs += 10;
        }
        DBG_PRINT(Adapter, DBGLVL_PNP,
            (__FUNCTION__": Waited %u ms\n", WaitMs));
    }

    RtlZeroMemory(&ChildStatus, sizeof(ChildStatus));
    ChildStatus.Type = StatusConnection;
    ChildStatus.ChildUid = MonitorNode->ChildUid;
    ChildStatus.HotPlug.Connected = TRUE;
    (*DxgkInterface->DxgkCbIndicateChildStatus)(
        DxgkInterface->DeviceHandle,
        &ChildStatus
        );
Error:
    return ntStatus;
}

VOID
LJB_PROXYKMD_CloseTargetDevice(
    __in __drv_freesMem(MonitorNode) LJB_MONITOR_NODE * MonitorNode
    )
{
    LJB_ADAPTER * CONST             Adapter = MonitorNode->Adapter;
    LJB_MONITOR_INTERFACE* CONST    MonitorInterface = &MonitorNode->MonitorInterface;

    PAGED_CODE();

    DBG_UNREFERENCED_LOCAL_VARIABLE(Adapter);

    DBG_PRINT(Adapter, DBGLVL_PNP,
        (__FUNCTION__ ":Closing %ws\n", MonitorNode->NameBuffer
        ));

    /*
     * Wait until ReferenceCount drop to 1
     */
    while (MonitorNode->ReferenceCount != 1)
    {
        LJB_PROXYKMD_DelayMs(10);
    }
    ReleaseChildUid(MonitorNode);
    if (MonitorInterface->Context != NULL)
    {
        (*MonitorInterface->InterfaceDereference)(
            MonitorInterface->Context);
        MonitorInterface->Context = NULL;
    }

    if (MonitorNode->FileObject)
    {
        ObDereferenceObject(MonitorNode->FileObject);
    }
    if (MonitorNode->NotificationHandle)
    {
        IoUnregisterPlugPlayNotification(MonitorNode->NotificationHandle);
    }
    if (MonitorNode->PDO)
    {
        ObDereferenceObject(MonitorNode->PDO);
    }
    LJB_PROXYKMD_FreePool(MonitorNode);
}

LJB_MONITOR_NODE *
LJB_GetMonitorNodeFromChildUid(
    __in LJB_ADAPTER *      Adapter,
    __in ULONG              ChildUid
    )
{
    LIST_ENTRY * CONST  ListHead = &Adapter->MonitorNodeListHead;
    LIST_ENTRY *        listEntry;
    LJB_MONITOR_NODE *  MonitorNode;
    KIRQL               oldIrql;

    MonitorNode = NULL;
    KeAcquireSpinLock(&Adapter->MonitorNodeListLock, &oldIrql);

    for (listEntry = ListHead->Flink;
         listEntry != ListHead;
         listEntry = listEntry->Flink)
    {
        LJB_MONITOR_NODE * thisNode;

        thisNode = CONTAINING_RECORD(listEntry, LJB_MONITOR_NODE, ListEntry);
        if (thisNode->ChildUid == ChildUid)
        {
            MonitorNode = thisNode;
            InterlockedIncrement(&MonitorNode->ReferenceCount);
            break;
        }
    }
    KeReleaseSpinLock(&Adapter->MonitorNodeListLock, oldIrql);

    return MonitorNode;
}

VOID
LJB_DereferenceMonitorNode(
    __in LJB_MONITOR_NODE * MonitorNode
    )
{
    InterlockedDecrement(&MonitorNode->ReferenceCount);
}