#include "ljb_vmon_private.h"
#include "ljb_vmon_ioctl.h"

/*
 * Function:
 *    LJB_VMON_EvtIoStop
 *
 * Definition:
 *    EVT_WDF_IO_QUEUE_IO_STOP  LJB_VMON_EvtIoStop;
 *
 * Description:
 *    A driver's EvtIoStop event callback function completes, requeues, or
 *    suspends processing of a specified request because the request's I/O queue
 *    is being stopped.
 *
 *    A driver registers an EvtIoStop callback function when it calls
 *    WdfIoQueueCreate. For more information about calling WdfIoQueueCreate, see
 *    Creating I/O Queues.
 *
 *    If a driver registers an EvtIoStop callback function for an I/O queue, the
 *    framework calls it when the queue's underlying device is leaving its working
 *    (D0) state. The framework calls the EvtIoStop callback function for every
 *    I/O request that the driver has not completed, including requests that the
 *    driver owns and those that it has forwarded to an I/O target.
 *
 *    The EvtIoStop callback function must either complete each request (by
 *    calling WdfRequestComplete or WdfRequestCancelSentRequest) or postpone
 *    further processing of the request and call WdfRequestStopAcknowledge.
 *
 *    If the WdfRequestStopRequestCancelable flag is set in the ActionFlags
 *    parameter, the driver must call WdfRequestUnmarkCancelable before calling
 *    WdfRequestComplete to complete (or cancel) the request or calling
 *    WdfRequestStopAcknowledge to requeue the request.
 *
 *    For more information about the EvtIoStop callback function, see Using Power-
 *    Managed I/O Queues.
 *
 *    This callback function can be called at IRQL <= DISPATCH_LEVEL, unless the
 *    ExecutionLevel member of the device or driver's WDF_OBJECT_ATTRIBUTES
 *    structure is set to WdfExecutionLevelPassive.
 *
 * Return Value:
 *    None.
 *
 */
VOID
LJB_VMON_EvtIoStop(
    __in WDFQUEUE       Queue,
    __in WDFREQUEST     Request,
    __in ULONG          ActionFlags
    )
{
    WDFDEVICE CONST         WdfDevice = WdfIoQueueGetDevice(Queue);
    LJB_VMON_CTX * CONST    dev_ctx = LJB_VMON_GetVMonCtx(WdfDevice);
    WDF_REQUEST_PARAMETERS  params;

    UNREFERENCED_PARAMETER(Queue);
    UNREFERENCED_PARAMETER(ActionFlags);
    DBG_UNREFERENCED_LOCAL_VARIABLE(dev_ctx);

    LJB_VMON_Printf(dev_ctx, DBGLVL_FLOW,
        (__FUNCTION__ ": Request(%p)/Flags(0x%x)\n",
        Request,
        ActionFlags
        ));
    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(Request, &params);

    /*
     * any request type other than DeviceControl is not of our interest.
     */
    if (params.Type != WdfRequestTypeDeviceControl)
    {
        LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
            ("?" __FUNCTION__
            ": Reqeust(%p) is not WdfRequestTypeDeviceControl?\n",
            Request
            ));
        return;
    }

    switch (params.Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_LJB_VMON_WAIT_FOR_MONITOR_EVENT:
        /*
         * The WaitForMonitorEvent request is not accessing hardware. We still keep
         * the pending reqeust at driver side instead of framedwork side
         * by calling WdfRequestStopAcknowledge with Requeue set to FALSE
         */
        LJB_VMON_Printf(dev_ctx, DBGLVL_FLOW,
            (__FUNCTION__
            ": WAIT_FOR_UPDATE request (%p) is acknowledged\n",
            Request
            ));
        WdfRequestStopAcknowledge(
            Request,
            FALSE
            );
        break;

    default:
        LJB_VMON_Printf(dev_ctx, DBGLVL_FLOW,
            (__FUNCTION__
            ": IOCTL(0x)%x Request(%p) is not process. "
            "We expect framework to wait for all requests completed.\n",
            params.Parameters.DeviceIoControl.IoControlCode,
            Request
            ));
        break;
    }
}
