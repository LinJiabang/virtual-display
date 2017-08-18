#include "ljb_vmon_private.h"
#include "ljb_vmon_ioctl.h"

/*
 * Name:  LJB_VMON_IoInCallerContext
 *
 * Definition:
 *    EVT_WDF_IO_IN_CALLER_CONTEXT    LJB_VMON_IoInCallerContext;
 *
 *    VOID
 *    LJB_VMON_IoInCallerContext(
 *        __in WDFDEVICE      WdfDevice,
 *        __in WDFREQUEST     WdfRequest
 *        );
 *
 * Description:
 *    This routine is to pre-process the IOCTL and retrieve the IOCTL specific
 *    user buffer. If the IOCTL needs to be processed, this routine allocates
 *    a request context and saves the retrieved user memory into request context.
 *
 *  Return Value:
 *    None.
 *
 */
VOID
LJB_VMON_IoInCallerContext(
    __in WDFDEVICE      WdfDevice,
    __in WDFREQUEST     WdfRequest
    )
{
    NTSTATUS                ntStatus;
    
    /*
     * We don't process any particular request in IoInCallerContext,
     * simply re-queue the request
     */
    ntStatus = WdfDeviceEnqueueRequest(WdfDevice, WdfRequest);
    if (!NT_SUCCESS(ntStatus))
    {
        WdfRequestComplete(WdfRequest, ntStatus);
    }
}
