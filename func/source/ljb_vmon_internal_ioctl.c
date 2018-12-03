#include "ljb_vmon_private.h"

/*
 * forward declaration
 */
LCI_RELEASE_INTERFACE   LJB_VMON_ReleaseInterface;


/*
 * Function:
 *    LJB_VMON_InternalDeviceIoControl
 *
 * Definition:
 *    EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL  LJB_VMON_InternalDeviceIoControl;
 *
 * Description:
 *    A driver's EvtIoInternalDeviceControl event callback function processes an
 *    I/O request that contains an internal device I/O control code (IOCTL).
 *
 *    A driver registers an EvtIoInternalDeviceControl callback function when it
 *    calls the WdfIoQueueCreate method. For more information about calling
 *    WdfIoQueueCreate, see Creating I/O Queues.
 *
 *    If a driver has registered an EvtIoInternalDeviceControl callback function
 *    for a device's I/O queue, the callback function receives every internal I/O
 *    control request (IRP_MJ_INTERNAL_DEVICE_CONTROL) from the queue. For more
 *    information, see Request Handlers.
 *
 *    The EvtIoInternalDeviceControl callback function must process each received
 *    I/O request in some manner. For more information, see Processing I/O
 *    Requests.
 *
 *    Drivers receive internal I/O control requests when another driver creates a
 *    request by calling either WdfIoTargetSendInternalIoctlSynchronously or
 *    WdfIoTargetFormatRequestForInternalIoctl.
 *
 *    The type of operation to be performed depends on the value of the
 *    IoControlCode parameter. You must determine the set of IoControlCode values
 *    that applications and other drivers can send to your driver. For more
 *    information about IOCTLs, see Using I/O Control Codes.
 *
 *    Most internal I/O control operations require an input buffer, an output
 *    buffer, or both. For information about how the driver can access a request's
 *    buffers, see Accessing Data Buffers in Framework-Based Drivers.
 *
 *    The techniques that your driver can use to access the request's input and o
 *    utput buffers (if they exist) depend on the TransferType field of the IOCTL.
 *    The value of the IOCTL's TransferType field can be METHOD_BUFFERED,
 *    METHOD_DIRECT_IN, METHOD_DIRECT_OUT, or METHOD_NEITHER. For more information
 *    about the TransferType field, see Defining I/O Control Codes.
 *
 *    The EvtIoInternalDeviceControl callback function can be called at IRQL <=
 *    DISPATCH_LEVEL, unless the ExecutionLevel member of the device or driver's
 *    WDF_OBJECT_ATTRIBUTES structure is set to WdfExecutionLevelPassive. (If your
 *    driver is at the top of its driver stack, the callback function is called at
 *    IRQL = PASSIVE_LEVEL.)
 *
 *    If the IRQL is PASSIVE_LEVEL, the framework calls the callback function
 *    within a critical region.
 *
 *    For more information about IRQL levels for request handlers, see Using
 *    Automatic Synchronization.
 *
 * Return Value:
 *    None
 *
 */
VOID
LJB_VMON_InternalDeviceIoControl(
    __in WDFQUEUE       Queue,
    __in WDFREQUEST     Request,
    __in size_t         OutputBufferLength,
    __in size_t         InputBufferLength,
    __in ULONG          IoControlCode
    )
{
    WDFDEVICE CONST             WdfDevice = WdfIoQueueGetDevice(Queue);
    LJB_VMON_CTX * CONST        dev_ctx = LJB_VMON_GetVMonCtx(WdfDevice);
    LCI_GENERIC_INTERFACE       MyGenericInterface;
    PVOID                       pIoBuffer;
    SIZE_T                      BytesReturned;
    NTSTATUS                    ntStatus;
    LONG                        InterfaceReferenceCount;

    BytesReturned = 0;
    LJB_VMON_Printf(dev_ctx, DBGLVL_FLOW,
        (" " __FUNCTION__
        ": IoctlCode(0x%x), OutputBufferLength(%u), InputBufferLength(%u)",
        IoControlCode,
        OutputBufferLength,
        InputBufferLength
        ));
    ntStatus = STATUS_NOT_SUPPORTED;
    switch(IoControlCode)
    {
    case INTERNAL_IOCTL_QUERY_USB_MONITOR_INTERFACE:
        if (InputBufferLength < sizeof(LCI_GENERIC_INTERFACE))
        {
            LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
                ("?" __FUNCTION__
                ": InputBufferLength(0x%x) too small, required %u bytes)\n",
                InputBufferLength,
                sizeof(LCI_GENERIC_INTERFACE)
                ));
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        if (OutputBufferLength < sizeof(LCI_GENERIC_INTERFACE))
        {
            LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
                ("?" __FUNCTION__
                ": OutputBufferLength(0x%x) too small, required %u bytes)\n",
                OutputBufferLength,
                sizeof(LCI_GENERIC_INTERFACE)
                ));
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        ntStatus =  WdfRequestRetrieveInputBuffer(
            Request,
            sizeof(LCI_GENERIC_INTERFACE),
            &pIoBuffer,
            NULL
            );
        if (!NT_SUCCESS(ntStatus))
        {
            LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
                ("?" __FUNCTION__
                ": unable to get InputBuffer, "
                "return ntStatus(0x%08x)\n",
                ntStatus
                ));
            break;
        }

        InterfaceReferenceCount = InterlockedIncrement(
            &dev_ctx->InterfaceReferenceCount
            );
        if (InterfaceReferenceCount != 1)
        {
            LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
                ("?" __FUNCTION__
                ": InterfaceReferenceCount(%u) too large!\n",
                InterfaceReferenceCount
                ));
            InterlockedDecrement(&dev_ctx->InterfaceReferenceCount);
            ntStatus = STATUS_UNSUCCESSFUL;
            break;
        }

        RtlCopyMemory(
            &dev_ctx->TargetGenericInterface,
            pIoBuffer,
            sizeof(MyGenericInterface)
            );
        RtlZeroMemory(&MyGenericInterface, sizeof(MyGenericInterface));
        MyGenericInterface.Version              = LCI_GENERIC_INTERFACE_V1;
        MyGenericInterface.Size                 = sizeof(MyGenericInterface);
        MyGenericInterface.ProviderContext      = dev_ctx;
        MyGenericInterface.pfnGenericIoctl      = &LJB_VMON_GenericIoctl;
        MyGenericInterface.pfnReleaseInterface  = &LJB_VMON_ReleaseInterface
        ntStatus = WdfRequestRetrieveOutputBuffer(
            Request,
            OutputBufferLength,
            &pIoBuffer,
            NULL
            );
        if (!NT_SUCCESS(ntStatus))
        {
            LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
                ("?" __FUNCTION__
                ": unable to get OutputBuffer, "
                "return ntStatus(0x%08x)\n",
                ntStatus
                ));
            break;
        }
        RtlCopyMemory(
            pIoBuffer,
            &MyGenericInterface,
            sizeof(MyGenericInterface)
            );
        BytesReturned = sizeof(MyGenericInterface);
        break;

    default:
        break;
    }

    /*
     * complete the user request now.
     */
    LJB_VMON_Printf(dev_ctx, DBGLVL_FLOW,
        (" " __FUNCTION__
        ": complete IoctlCode(0x%x) with ntStatus(0x%x), "
        "BytesReturned(%u)\n",
        IoControlCode,
        ntStatus,
        BytesReturned
        ));
    WdfRequestCompleteWithInformation(Request, ntStatus, BytesReturned);
}

VOID
LJB_VMON_ReleaseInterface(
    __in PVOID          ProviderContext
    )
{
    LJB_VMON_CTX * CONST    dev_ctx = ProviderContext;

    InterlockedDecrement(
       &dev_ctx->InterfaceReferenceCount
       );
    LJB_VMON_Printf(dev_ctx, DBGLVL_FLOW,
        (__FUNCTION__
        "InterfaceReferenceCount is now (0x%x)\n",
        dev_ctx->InterfaceReferenceCount
        ));
}