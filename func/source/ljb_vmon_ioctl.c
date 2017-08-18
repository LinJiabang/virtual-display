#include "ljb_vmon_private.h"

/*
 * Routine Description:
 *
 *     This event is called when the framework receives IRP_MJ_DEVICE_CONTROL
 *     requests from the system.
 *
 * Arguments:
 *
 *     Queue - Handle to the framework queue object that is associated
 *             with the I/O request.
 *     Request - Handle to a framework request object.
 *
 *     output_buffer_length - length of the request's output buffer,
 *                         if an output buffer is available.
 *     input_buffer_length - length of the request's input buffer,
 *                         if an input buffer is available.
 *
 *     IoControlCode - the driver-defined or system-defined I/O control code
 *                     (IOCTL) that is associated with the request.
 *
 * Return Value:
 *
 *    VOID
 *
 */
_Use_decl_annotations_
VOID
LJB_VMON_EvtIoDeviceControl(
    IN WDFQUEUE     Queue,
    IN WDFREQUEST   Request,
    IN size_t       output_buffer_length,
    IN size_t       input_buffer_length,
    IN ULONG        IoControlCode
    )
{
    WDFDEVICE               Device = WdfIoQueueGetDevice(Queue);
    LJB_VMON_CTX * CONST    dev_ctx = LJB_VMON_GetVMonCtx(Device);
    UCHAR *                 edid_block;
    NTSTATUS                ntStatus= STATUS_SUCCESS;
    ULONG                   bytes_written = 0;

    PAGED_CODE();

    switch (IoControlCode)
    {
    case IOCTL_LJB_VMON_PLUGIN_MONITOR:
        if (input_buffer_length < 128)
        {
            LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
                (__FUNCTION__
                ": input_buffer_length(%u) too small?\n",
                input_buffer_length
                ));
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        ntStatus = WdfRequestRetrieveInputBuffer(
            Request,
            128,
            &edid_block,
            NULL);

        if (!NT_SUCCESS(ntStatus))
        {
            LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
                (__FUNCTION__
                ": output_buffer_length(%u) too small?\n",
                output_buffer_length
                ));
            break;
        }

        RtlCopyMemory(dev_ctx->EdidBlock, edid_block, 128);
        IoSetDeviceInterfaceState(
            &dev_ctx->lci_interface_path,
            TRUE);
        ntStatus = STATUS_SUCCESS;
        break;

    case IOCTL_LJB_VMON_UNPLUG_MONITOR:
        RtlZeroMemory(dev_ctx->EdidBlock, 128);
        IoSetDeviceInterfaceState(
            &dev_ctx->lci_interface_path,
            FALSE);
        ntStatus = STATUS_SUCCESS;
        break;

    case IOCTL_LJB_VMON_WAIT_FOR_MONITOR_EVENT:
        LJB_VMON_WaitForMonitorEvent(
            dev_ctx,
            Request,
            input_buffer_length,
            output_buffer_length);
        return;

    case IOCTL_LJB_VMON_GET_POINTER_SHAPE:
        LJB_VMON_GetPointerShape(
            dev_ctx,
            Request,
            input_buffer_length,
            output_buffer_length);
        return;

    case IOCTL_LJB_VMON_BLT_BITMAP:
        LJB_VMON_BltBitmap(
            dev_ctx,
            Request,
            input_buffer_length,
            output_buffer_length);
        return;

    case IOCTL_LJB_VMON_LOCK_BUFFER:
        LJB_VMON_LockBuffer(
            dev_ctx,
            Request,
            input_buffer_length,
            output_buffer_length);
        return;

    case IOCTL_LJB_VMON_UNLOCK_BUFFER:
        LJB_VMON_UnlockBuffer(
            dev_ctx,
            Request,
            input_buffer_length,
            output_buffer_length);
        return;

    default:
        ntStatus = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    //
    // Complete the Request.
    //
    WdfRequestCompleteWithInformation(Request, ntStatus, (ULONG_PTR) bytes_written);
}

VOID
LJB_VMON_WaitForMonitorEvent(
    __in LJB_VMON_CTX *     dev_ctx,
    __in WDFREQUEST         wdf_request,
    __in size_t             input_buffer_length,
    __in size_t             output_buffer_length
    )
{
    LJB_VMON_MONITOR_EVENT *    input_data;
    LJB_VMON_MONITOR_EVENT *    output_data;
    LJB_VMON_MONITOR_EVENT      output_event;
    LJB_VMON_WAIT_FLAGS         input_flags;
    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    ULONG                       bytes_returned = 0;
    KIRQL                       old_irql_ioctl;

    if (input_buffer_length < sizeof(LJB_VMON_MONITOR_EVENT))
    {
        LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
            (__FUNCTION__
            ": input_buffer_length(%u) too small?\n",
            output_buffer_length
            ));
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        goto exit;
    }

    if (output_buffer_length < sizeof(LJB_VMON_MONITOR_EVENT))
    {
        LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
            (__FUNCTION__
            ": output_buffer_length(%u) too small?\n",
            output_buffer_length
            ));
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        goto exit;
    }

    ntStatus = WdfRequestRetrieveInputBuffer(
            wdf_request,
            sizeof(LJB_VMON_MONITOR_EVENT),
            &input_data,
            NULL);

    if (!NT_SUCCESS(ntStatus))
    {
        LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
            (__FUNCTION__
            ": WdfRequestRetrieveInputBuffer failed with 0x%08x?\n",
            ntStatus
            ));
        goto exit;
    }

    ntStatus = WdfRequestRetrieveOutputBuffer(
            wdf_request,
            sizeof(LJB_VMON_MONITOR_EVENT),
            &output_data,
            NULL);

    if (!NT_SUCCESS(ntStatus))
    {
        LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
            (__FUNCTION__
            ": WdfRequestRetrieveOutputBuffer failed with 0x%08x?\n",
            ntStatus
            ));
        goto exit;
    }

    /*
     * check each input flag
     */
    RtlZeroMemory(&output_event, sizeof(LJB_VMON_MONITOR_EVENT));
    input_flags = input_data->Flags;

    KeAcquireSpinLock(&dev_ctx->ioctl_lock, &old_irql_ioctl);
    if (input_flags.ModeChange)
    {
        if (input_data->TargetModeData.Width != dev_ctx->Width ||
            input_data->TargetModeData.Height != dev_ctx->Height ||
            input_data->TargetModeData.Rotation != dev_ctx->ContentTransformation.Rotation)
        {
            output_event.Flags.ModeChange = TRUE;
            output_event.TargetModeData.Width = dev_ctx->Width;
            output_event.TargetModeData.Height = dev_ctx->Height;
            output_event.TargetModeData.Rotation = dev_ctx->ContentTransformation.Rotation;
            output_event.TargetModeData.Enabled = (dev_ctx->Width != 0);
        }
    }

    if (input_flags.VidPnSourceVisibilityChange)
    {
        if (input_data->VidPnSourceVisibilityData.Visible != dev_ctx->VidPnVisible)
        {
            output_event.Flags.VidPnSourceVisibilityChange = TRUE;
            output_event.VidPnSourceVisibilityData.Visible = dev_ctx->VidPnVisible;
        }
    }

    if (input_flags.VidPnSourceBitmapChange)
    {
        if (input_data->FrameId != dev_ctx->LatestFrameId)
        {
            output_event.Flags.VidPnSourceBitmapChange = TRUE;
            output_event.FrameId = dev_ctx->LatestFrameId;
        }
    }

    if (input_flags.VidPnSourceBitmapChange)
    {
        if (input_data->FrameId != dev_ctx->LatestFrameId)
        {
            output_event.Flags.VidPnSourceBitmapChange = TRUE;
            output_event.FrameId = dev_ctx->LatestFrameId;
        }
    }

    if (input_flags.PointerPositionChange)
    {
        if (input_data->PointerPositionData.X != dev_ctx->PointerInfo.X ||
            input_data->PointerPositionData.Y != dev_ctx->PointerInfo.Y ||
            input_data->PointerPositionData.Visible != dev_ctx->PointerInfo.Visible)
        {
            output_event.Flags.PointerPositionChange = TRUE;
            output_event.PointerPositionData.X = dev_ctx->PointerInfo.X;
            output_event.PointerPositionData.Y = dev_ctx->PointerInfo.Y;
            output_event.PointerPositionData.Visible = dev_ctx->PointerInfo.Visible;
        }
    }

    if (input_flags.PointerShapeChange)
    {
        if (dev_ctx->PointerShapeChanged)
        {
            dev_ctx->PointerShapeChanged = FALSE;
            output_event.Flags.PointerShapeChange = TRUE;
        }
    }
    KeReleaseSpinLock(&dev_ctx->ioctl_lock, old_irql_ioctl);

    if (output_event.Flags.Value != 0)
    {
        ntStatus = STATUS_SUCCESS;
        *output_data = output_event;
        bytes_returned = sizeof(*output_data);
    }
    else
    {
        LJB_VMON_WAIT_FOR_EVENT_REQ *   request;
        KIRQL old_irql;

        /* No flags changed, queue the request */
        request = LJB_VMON_GetPoolZero(sizeof(LJB_VMON_WAIT_FOR_EVENT_REQ));
        if (request == NULL)
        {
            LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
                (__FUNCTION__
                ": unable to allocate LJB_VMON_WAIT_FOR_EVENT_REQ?\n"
                ));

            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto exit;
        }

        InitializeListHead(&request->list_entry);
        request->Request = wdf_request;
        request->in_event_data = input_data;
        request->out_event_data = output_data;
        KeAcquireSpinLock(&dev_ctx->event_req_lock, &old_irql);
        InsertTailList(&dev_ctx->event_req_list, &request->list_entry);
        KeReleaseSpinLock(&dev_ctx->event_req_lock, old_irql);

        /* once we queue the request, do not complete it */
        return;
    }

exit:
    WdfRequestCompleteWithInformation(
        wdf_request,
        ntStatus,
        (ULONG_PTR) bytes_returned
        );
}

VOID
LJB_VMON_GetPointerShape(
    __in LJB_VMON_CTX *     dev_ctx,
    __in WDFREQUEST         wdf_request,
    __in size_t             input_buffer_length,
    __in size_t             output_buffer_length
    )
{
    POINTER_SHAPE_DATA *    pointer_shape_data;
    NTSTATUS                ntStatus = STATUS_SUCCESS;
    ULONG                   bytes_written = 0;
    KIRQL                   old_irql_ioctl;

    UNREFERENCED_PARAMETER(input_buffer_length);

    if (output_buffer_length < sizeof(POINTER_SHAPE_DATA))
    {
        LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
            (__FUNCTION__
            ": output_buffer_length(%u) too small?\n",
            output_buffer_length
            ));
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        goto exit;
    }

    ntStatus = WdfRequestRetrieveOutputBuffer(
            wdf_request,
            sizeof(POINTER_SHAPE_DATA),
            &pointer_shape_data,
            NULL);

    if (!NT_SUCCESS(ntStatus))
    {
        LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
            (__FUNCTION__
            ": WdfRequestRetrieveOutputBuffer failed with 0x%08x?\n",
            ntStatus
            ));
        goto exit;
    }

    KeAcquireSpinLock(&dev_ctx->ioctl_lock, &old_irql_ioctl);
    pointer_shape_data->Flags    = dev_ctx->PointerInfo.Flags;
    pointer_shape_data->Width    = dev_ctx->PointerInfo.Width;
    pointer_shape_data->Height   = dev_ctx->PointerInfo.Height;
    pointer_shape_data->Pitch    = dev_ctx->PointerInfo.Pitch;

    RtlCopyMemory(
        pointer_shape_data->Buffer,
        dev_ctx->PointerInfo.Bitmap,
        MAX_POINTER_SIZE
        );
    KeReleaseSpinLock(&dev_ctx->ioctl_lock, old_irql_ioctl);

    bytes_written = sizeof(POINTER_SHAPE_DATA);
    ntStatus = STATUS_SUCCESS;

exit:
    WdfRequestCompleteWithInformation(wdf_request, ntStatus, (ULONG_PTR) bytes_written);
}

VOID
LJB_VMON_BltBitmap(
    __in LJB_VMON_CTX *     dev_ctx,
    __in WDFREQUEST         wdf_request,
    __in size_t             input_buffer_length,
    __in size_t             output_buffer_length
    )
{
    LCI_GENERIC_INTERFACE * CONST   lci_interface = &dev_ctx->TargetGenericInterface;
    BLT_DATA *                      input_blt_data;
    BLT_DATA *                      output_blt_data;
    LJB_VMON_PRIMARY_SURFACE *      primary_surface;
    LJB_VMON_PRIMARY_SURFACE *      this_surface;
    LCI_USBAV_BLT_DATA              BltData;
    PMDL                            pMdl;
    ULONG                           FrameBufferSize;
    PVOID                           UserFrameBuffer;
    PVOID                           SystemFrameBuffer;
    NTSTATUS                        ntStatus = STATUS_SUCCESS;
    ULONG                           bytes_written = 0;
    ULONG                           bytes_return;
    LIST_ENTRY *                    list_head;
    LIST_ENTRY *                    list_entry;
    LIST_ENTRY *                    next_entry;
    KIRQL                           old_irql;

    if (input_buffer_length < sizeof(BLT_DATA))
    {
        LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
            (__FUNCTION__
            ": output_buffer_length(%u) too small?\n",
            input_buffer_length
            ));
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        goto exit;
    }

    if (output_buffer_length < sizeof(BLT_DATA))
    {
        LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
            (__FUNCTION__
            ": output_buffer_length(%u) too small?\n",
            output_buffer_length
            ));
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        goto exit;
    }

    ntStatus = WdfRequestRetrieveInputBuffer(
            wdf_request,
            sizeof(BLT_DATA),
            &input_blt_data,
            NULL);

    if (!NT_SUCCESS(ntStatus))
    {
        LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
            (__FUNCTION__
            ": WdfRequestRetrieveInputBuffer failed with 0x%08x?\n",
            ntStatus
            ));
        goto exit;
    }

    ntStatus = WdfRequestRetrieveOutputBuffer(
            wdf_request,
            sizeof(BLT_DATA),
            &output_blt_data,
            NULL);

    if (!NT_SUCCESS(ntStatus))
    {
        LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
            (__FUNCTION__
            ": WdfRequestRetrieveOutputBuffer failed with 0x%08x?\n",
            ntStatus
            ));
        goto exit;
    }

    list_head = &dev_ctx->surface_list;
    primary_surface = NULL;
    next_entry = NULL;
    KeAcquireSpinLock(&dev_ctx->surface_lock, &old_irql);
    for (list_entry = list_head->Flink;
        list_entry != list_head;
        list_entry = next_entry)
    {
        next_entry = list_entry->Flink;
        this_surface = CONTAINING_RECORD(
            list_entry,
            LJB_VMON_PRIMARY_SURFACE,
            list_entry
            );
        if (this_surface->hPrimarySurface == dev_ctx->hLatestPrimarySurface)
        {
            primary_surface = this_surface;
            break;
        }
    }
    KeReleaseSpinLock(&dev_ctx->surface_lock, old_irql);

    if (primary_surface == NULL)
    {
        LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
            (__FUNCTION__
            ": no primary_surface found?\n"
            ));
        ntStatus = STATUS_UNSUCCESSFUL;
        goto exit;
    }

    if (primary_surface->Width != input_blt_data->Width ||
        primary_surface->Height != input_blt_data->Height)
    {
        LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
            (__FUNCTION__
            ": input dimension(%u, %u) mismatch with surface dimenstion(%u, %u)?\n",
            input_blt_data->Width,
            input_blt_data->Height,
            primary_surface->Width,
            primary_surface->Height
            ));
        ntStatus = STATUS_UNSUCCESSFUL;
        goto exit;
    }

    pMdl = NULL;

    FrameBufferSize = input_blt_data->Width * input_blt_data->Height * 4;
    UserFrameBuffer = (PVOID) ((ULONG_PTR) input_blt_data->FrameBuffer);
    pMdl = IoAllocateMdl(
        UserFrameBuffer,
        FrameBufferSize,
        FALSE,
        FALSE,
        NULL
        );
    if (pMdl == NULL)
    {
        LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
            (__FUNCTION__
            ": input dimension(%u, %u) mismatch with surface dimension(%u, %u)?\n",
            input_blt_data->Width,
            input_blt_data->Height,
            primary_surface->Width,
            primary_surface->Height
            ));

        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }
    try
    {
        MmProbeAndLockPages(
            pMdl,
            UserMode,
            IoWriteAccess
            );
    }
    except (EXCEPTION_EXECUTE_HANDLER)
    {
        LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
            (__FUNCTION__ ": Unable to lock user buffer(%p)?\n",
            UserFrameBuffer
            ));
        IoFreeMdl(pMdl);
        pMdl = NULL;
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    SystemFrameBuffer = MmGetSystemAddressForMdlSafe(pMdl, NormalPagePriority);
    if (SystemFrameBuffer == NULL)
    {
        LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
            (__FUNCTION__
            ": MmGetSystemAddressForMdlSafe(%p) failed?\n",
            UserFrameBuffer
            ));

        MmUnlockPages(pMdl);
        IoFreeMdl(pMdl);
        pMdl = NULL;
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto exit;
    }

    /*
     * directly return framebuffer associated with FileObjectCtx->LastUpdatePrimarySurface
     */
    RtlZeroMemory(&BltData, sizeof(BltData));
    BltData.hPrimarySurface = primary_surface->hPrimarySurface;
    BltData.pPrimaryBuffer = primary_surface->remote_buffer;
    BltData.pShadowBuffer = SystemFrameBuffer;
    BltData.BufferSize = FrameBufferSize;
    (VOID) (*lci_interface->pfnGenericIoctl)(
        lci_interface->ProviderContext,
        LCI_USBAV_BLT_PRIMARY_TO_SHADOW,
        &BltData,
        sizeof(BltData),
        NULL,
        0,
        &bytes_return
        );

    output_blt_data->Width = primary_surface->Width;
    output_blt_data->Height = primary_surface->Height;
    output_blt_data->FrameId = dev_ctx->LatestFrameId;
    output_blt_data->FrameBufferSize = FrameBufferSize;
    output_blt_data->FrameBuffer = input_blt_data->FrameBuffer;

    MmUnlockPages(pMdl);
    IoFreeMdl(pMdl);
    ntStatus = STATUS_SUCCESS;
    bytes_written = sizeof(BLT_DATA);

exit:
    WdfRequestCompleteWithInformation(wdf_request, ntStatus, (ULONG_PTR) bytes_written);
}

VOID
LJB_VMON_LockBuffer(
    __in LJB_VMON_CTX *     dev_ctx,
    __in WDFREQUEST         wdf_request,
    __in size_t             input_buffer_length,
    __in size_t             output_buffer_length
    )
{
    /*
     * NOT YET IMPLEMENTED
     */
    UNREFERENCED_PARAMETER(dev_ctx);
    UNREFERENCED_PARAMETER(wdf_request);

    WdfRequestCompleteWithInformation(wdf_request, STATUS_SUCCESS, (ULONG_PTR) 0);
}

VOID
LJB_VMON_UnlockBuffer(
    __in LJB_VMON_CTX *     dev_ctx,
    __in WDFREQUEST         wdf_request,
    __in size_t             input_buffer_length,
    __in size_t             output_buffer_length
    )
{
    /*
     * NOT YET IMPLEMENTED
     */
    UNREFERENCED_PARAMETER(dev_ctx);
    UNREFERENCED_PARAMETER(wdf_request);

    WdfRequestCompleteWithInformation(wdf_request, STATUS_SUCCESS, (ULONG_PTR) 0);
}

