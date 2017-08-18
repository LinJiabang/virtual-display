#include "ljb_vmon_private.h"

#define NUM_OF_BUFFERS_PER_SURFACE      1

NTSTATUS
LJB_VMON_GenericIoctl(
    __in PVOID          ProviderContext,
    __in ULONG          IoctlCode,
    __in_opt PVOID      InputBuffer,
    __in SIZE_T         InputBufferSize,
    __out_opt PVOID     OutputBuffer,
    __in SIZE_T         OutputBufferSize,
    __out ULONG *       BytesReturned
    )
{
    LJB_VMON_CTX * CONST                    dev_ctx = ProviderContext;
    LCI_PROXYKMD_PRIMARY_SURFACE_CREATE *   pCreateData;
    LCI_PROXYKMD_PRIMARY_SURFACE_DESTROY *  destroy_data;
    LCI_PROXYKMD_PRIMARY_SURFACE_UPDATE *   surface_update;
    LCI_PROXYKMD_CURSOR_UPDATE *            cursor_update;
    LCI_PROXYKMD_VISIBILITY_UPDATE *        visibility_update;
    LCI_PROXYKMD_COMMIT_VIDPN *             commit_vidpn;
    LJB_VMON_PRIMARY_SURFACE *              primary_surface;
    LJB_VMON_WAIT_FOR_EVENT_REQ *           wait_event_req;
    LIST_ENTRY *                            list_head;
    LIST_ENTRY *                            list_entry;
    LIST_ENTRY *                            next_entry;
    KIRQL                                   old_irql;
    KIRQL                                   old_irql_ioctl;
    LONG                                    reference_count;
    NTSTATUS                                ntStatus;
    UINT                                    i;
    LJB_VMON_PRIMARY_SURFACE *              this_surface;

    this_surface = NULL;
    *BytesReturned = 0;
    ntStatus = STATUS_NOT_SUPPORTED;
    switch (IoctlCode)
    {
    case LCI_PROXYKMD_GET_EDID:
        if (OutputBufferSize < 128)
        {
            LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
                (__FUNCTION__
                ": OutputBufferSize(0x%x) too small, required 0x%x bytes?\n",
                OutputBufferSize,
                128
                ));
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        RtlCopyMemory(
            OutputBuffer,
            dev_ctx->EdidBlock,
            128
            );
        ntStatus = STATUS_SUCCESS;
        break;

    case LCI_PROXYKMD_NOTIFY_PRIMARY_SURFACE_CREATE:
        if (InputBufferSize < sizeof(LCI_PROXYKMD_PRIMARY_SURFACE_CREATE))
        {
            LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
                (__FUNCTION__
                ": InputBufferSize(0x%x) too small, required 0x%x bytes?\n",
                InputBufferSize,
                sizeof(LCI_PROXYKMD_PRIMARY_SURFACE_CREATE)
                ));
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        pCreateData = InputBuffer;
        for (i = 0; i < NUM_OF_BUFFERS_PER_SURFACE; i++)
        {
            primary_surface = LJB_VMON_GetPoolZero(sizeof(*primary_surface));
            if (primary_surface == NULL)
            {
                LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
                    (__FUNCTION__ ": no primary_surface created?\n"
                    ));
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            primary_surface->hPrimarySurface    = pCreateData->hPrimarySurface;
            primary_surface->remote_buffer      = pCreateData->pBuffer;
            primary_surface->BufferSize         = pCreateData->BufferSize;
            primary_surface->Width              = pCreateData->Width;
            primary_surface->Height             = pCreateData->Height;
            primary_surface->Pitch              = pCreateData->Pitch;
            primary_surface->BytesPerPixel      = pCreateData->BytesPerPixel;
            primary_surface->reference_count    = 1;

            list_head = &dev_ctx->surface_list;
            KeAcquireSpinLock(&dev_ctx->surface_lock, &old_irql);
            InsertTailList(
                list_head,
                &primary_surface->list_entry
                );
            KeReleaseSpinLock(&dev_ctx->surface_lock, old_irql);
            LJB_VMON_Printf(dev_ctx, DBGLVL_FLOW,
                (__FUNCTION__
                ": primary_surface(%p) created, "
                "Width(%u)/Height(%u)/Size(0x%x)\n",
                primary_surface,
                primary_surface->Width,
                primary_surface->Height,
                primary_surface->BufferSize
                ));
        }
        break;

    case LCI_PROXYKMD_NOTIFY_PRIMARY_SURFACE_DESTROY:
        if (InputBufferSize < sizeof(LCI_PROXYKMD_PRIMARY_SURFACE_DESTROY))
        {
            LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
                (__FUNCTION__
                ": InputBufferSize(0x%x) too small, required 0x%x bytes?\n",
                InputBufferSize,
                sizeof(LCI_PROXYKMD_PRIMARY_SURFACE_DESTROY)
                ));
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        /*
         * Locate primary_surface with matched hPrimarySurface
         */
        destroy_data = InputBuffer;
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
            if (this_surface->hPrimarySurface == destroy_data->hPrimarySurface)
            {
                /*
                 * If nobody is holding a reference on the object, we are safe
                 * to remove it from the list
                 */
                reference_count = InterlockedDecrement(&this_surface->reference_count);
                if (reference_count == 0)
                {
                    RemoveEntryList(&this_surface->list_entry);
                    primary_surface = this_surface;

                    LJB_VMON_Printf(dev_ctx, DBGLVL_FLOW,
                        (__FUNCTION__
                        ": primary_surface(%p) destroyed\n",
                        primary_surface
                        ));
                    LJB_VMON_FreePool(primary_surface);
                }
                else
                {
                    LJB_VMON_Printf(dev_ctx, DBGLVL_FLOW,
                        (__FUNCTION__
                        ": this_surface(%p) has reference_count(%d), "
                        "Defer destruction to later.\n",
                        this_surface,
                        reference_count
                        ));
                }
            }
        }
        KeReleaseSpinLock(&dev_ctx->surface_lock, old_irql);

        ntStatus = STATUS_SUCCESS;
        break;

    case LCI_PROXYKMD_NOTIFY_PRIMARY_SURFACE_UPDATE:
        if (InputBufferSize < sizeof(LCI_PROXYKMD_PRIMARY_SURFACE_UPDATE))
        {
            LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
                (__FUNCTION__
                ":InputBufferSize(0x%x) too small, required 0x%x bytes?\n",
                InputBufferSize,
                sizeof(LCI_PROXYKMD_PRIMARY_SURFACE_UPDATE)
                ));
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        surface_update = InputBuffer;
        KeAcquireSpinLock(&dev_ctx->ioctl_lock, &old_irql_ioctl);
        dev_ctx->LatestFrameId = surface_update->FrameId;
        dev_ctx->hLatestPrimarySurface = surface_update->hPrimarySurface;

        LJB_VMON_Printf(dev_ctx, DBGLVL_FLOW,
            (__FUNCTION__
            ": LCI_PROXYKMD_NOTIFY_PRIMARY_SURFACE_UPDATE, FrameId(%d), "
            "hLatestPrimarySurface(0x%p)\n",
            dev_ctx->LatestFrameId,
            dev_ctx->hLatestPrimarySurface
            ));

        /*
         * Check if there is pending LJB_VMON_WAIT_FOR_EVENT_REQ request
         */
        list_head = &dev_ctx->event_req_list;
        do
        {
            LJB_VMON_WAIT_FOR_EVENT_REQ *  this_request;
            LJB_VMON_MONITOR_EVENT * in_event_data;
            LJB_VMON_MONITOR_EVENT * out_event_data;

            wait_event_req = NULL;
            KeAcquireSpinLock(&dev_ctx->event_req_lock, &old_irql);
            for (list_entry = list_head->Flink;
                 list_entry != list_head;
                 list_entry = list_entry->Flink)
            {
                this_request = CONTAINING_RECORD(
                    list_entry,
                    LJB_VMON_WAIT_FOR_EVENT_REQ,
                    list_entry
                    );
                in_event_data = this_request->in_event_data;
                if (in_event_data->Flags.VidPnSourceBitmapChange)
                {
                    RemoveEntryList(&this_request->list_entry);
                    wait_event_req = this_request;
                    break;
                }
            }
            KeReleaseSpinLock(&dev_ctx->event_req_lock, old_irql);
            if (wait_event_req == NULL)
                break;

            in_event_data = wait_event_req->in_event_data;
            out_event_data = wait_event_req->out_event_data;
            RtlZeroMemory(out_event_data, sizeof(LJB_VMON_MONITOR_EVENT));
            out_event_data->Flags.VidPnSourceBitmapChange = 1;
            out_event_data->FrameId = dev_ctx->LatestFrameId;

            LJB_VMON_Printf(dev_ctx, DBGLVL_FLOW,
                (__FUNCTION__ ": complete Request(%p), FrameId(0x%x).\n",
                wait_event_req,
                dev_ctx->LatestFrameId
                ));
            WdfRequestCompleteWithInformation(
                wait_event_req->Request,
                STATUS_SUCCESS,
                sizeof(*out_event_data)
                );
            LJB_VMON_FreePool(wait_event_req);
        } while (wait_event_req != NULL);
        KeReleaseSpinLock(&dev_ctx->ioctl_lock, old_irql_ioctl);
        break;

    case LCI_PROXYKMD_NOTIFY_CURSOR_UPDATE:
        if (InputBufferSize < sizeof(LCI_PROXYKMD_CURSOR_UPDATE))
        {
            LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
                (__FUNCTION__
                ": InputBufferSize(0x%x) too small, required 0x%x bytes?\n",
                InputBufferSize,
                sizeof(LCI_PROXYKMD_CURSOR_UPDATE)
                ));
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        cursor_update = InputBuffer;

        /*
         * caveat. the DXGKARG_SETPOINTERPOSITION/DXGKARG_SETPOINTERSHAPE is in
         * PAGEABLE area. We couldn't retrieve pSetPointerShape with lock held.
         * Do a shadow copy first.
         */
        if (cursor_update->pPositionUpdate != NULL)
        {
            CONST DXGKARG_SETPOINTERPOSITION * pSetPointerPosition;

            pSetPointerPosition = cursor_update->pPositionUpdate;
            dev_ctx->TempPointerInfo.X         = pSetPointerPosition->X;
            dev_ctx->TempPointerInfo.Y         = pSetPointerPosition->Y;
            dev_ctx->TempPointerInfo.Visible   = (pSetPointerPosition->Flags.Visible == 1);
        }

        if (cursor_update->pShapeUpdate != NULL)
        {
            ULONG   BitmapSize;
            CONST DXGKARG_SETPOINTERSHAPE * pSetPointerShape;

            pSetPointerShape = cursor_update->pShapeUpdate;
            BitmapSize = pSetPointerShape->Pitch * pSetPointerShape->Height;

            if (pSetPointerShape->Flags.Monochrome)
                BitmapSize *= 2;

            dev_ctx->TempPointerInfo.Flags     = pSetPointerShape->Flags;
            dev_ctx->TempPointerInfo.Width     = pSetPointerShape->Width;
            dev_ctx->TempPointerInfo.Height    = pSetPointerShape->Height;
            dev_ctx->TempPointerInfo.Pitch     = pSetPointerShape->Pitch;
            dev_ctx->TempPointerInfo.XHot      = pSetPointerShape->XHot;
            dev_ctx->TempPointerInfo.YHot      = pSetPointerShape->YHot;
            ASSERT(BitmapSize <= MAX_POINTER_SIZE);
            RtlCopyMemory(
                dev_ctx->TempPointerInfo.Bitmap,
                pSetPointerShape->pPixels,
                BitmapSize
                );
        }

        /*
         * now the cursor data is copied to non-pageable area. we are safe
         * to access it with spinlock held
         */
        KeAcquireSpinLock(&dev_ctx->ioctl_lock, &old_irql_ioctl);
        if (cursor_update->pPositionUpdate != NULL)
        {
            dev_ctx->PointerInfo.X         = dev_ctx->TempPointerInfo.X;
            dev_ctx->PointerInfo.Y         = dev_ctx->TempPointerInfo.Y;
            dev_ctx->PointerInfo.Visible   = dev_ctx->TempPointerInfo.Visible;
        }

        if (cursor_update->pShapeUpdate != NULL)
        {
            ULONG   BitmapSize;

            BitmapSize = dev_ctx->TempPointerInfo.Pitch * dev_ctx->TempPointerInfo.Height;

            if (dev_ctx->TempPointerInfo.Flags.Monochrome)
                BitmapSize *= 2;

            dev_ctx->PointerInfo.Flags     = dev_ctx->TempPointerInfo.Flags;
            dev_ctx->PointerInfo.Width     = dev_ctx->TempPointerInfo.Width;
            dev_ctx->PointerInfo.Height    = dev_ctx->TempPointerInfo.Height;
            dev_ctx->PointerInfo.Pitch     = dev_ctx->TempPointerInfo.Pitch;
            dev_ctx->PointerInfo.XHot      = dev_ctx->TempPointerInfo.XHot;
            dev_ctx->PointerInfo.YHot      = dev_ctx->TempPointerInfo.YHot;
            ASSERT(BitmapSize <= MAX_POINTER_SIZE);
            RtlCopyMemory(
                dev_ctx->PointerInfo.Bitmap,
                dev_ctx->TempPointerInfo.Bitmap,
                BitmapSize
                );
        }

        list_head = &dev_ctx->event_req_list;
        do
        {
            LJB_VMON_WAIT_FOR_EVENT_REQ *   this_request;
            LJB_VMON_MONITOR_EVENT *        in_event_data;
            LJB_VMON_MONITOR_EVENT *        out_event_data;

            wait_event_req = NULL;
            KeAcquireSpinLock(&dev_ctx->event_req_lock, &old_irql);
            for (list_entry = list_head->Flink;
                list_entry != list_head;
                list_entry = list_entry->Flink)
            {
                this_request = CONTAINING_RECORD(
                    list_entry,
                    LJB_VMON_WAIT_FOR_EVENT_REQ,
                    list_entry
                    );
                if (this_request->in_event_data == NULL)
                    continue;

                in_event_data = this_request->in_event_data;
                if ((in_event_data->Flags.PointerPositionChange &&
                    cursor_update->pPositionUpdate != NULL) ||
                    (in_event_data->Flags.PointerShapeChange &&
                    cursor_update->pShapeUpdate != NULL))
                {
                    RemoveEntryList(&this_request->list_entry);
                    wait_event_req = this_request;
                    break;
                }
            }
            KeReleaseSpinLock(&dev_ctx->event_req_lock, old_irql);
            if (wait_event_req == NULL)
                break;

            in_event_data = wait_event_req->in_event_data;
            out_event_data = wait_event_req->out_event_data;
            RtlZeroMemory(out_event_data, sizeof(*out_event_data));
            if (cursor_update->pPositionUpdate != NULL)
            {
                out_event_data->Flags.PointerPositionChange = 1;
                out_event_data->PointerPositionData.X = dev_ctx->PointerInfo.X;
                out_event_data->PointerPositionData.Y = dev_ctx->PointerInfo.Y;
                out_event_data->PointerPositionData.Visible = dev_ctx->PointerInfo.Visible;
            }

            if (cursor_update->pShapeUpdate != NULL)
            {
                out_event_data->Flags.PointerShapeChange = 1;
            }
            LJB_VMON_Printf(dev_ctx, DBGLVL_FLOW,
                (__FUNCTION__
                ": complete Request(%p), PointerPositionChange(%u), PointerShapeChange(%u)\n",
                wait_event_req,
                out_event_data->Flags.PointerPositionChange,
                out_event_data->Flags.PointerShapeChange
                ));
            WdfRequestCompleteWithInformation(
                wait_event_req->Request,
                STATUS_SUCCESS,
                sizeof(*out_event_data)
                );
            LJB_VMON_FreePool(wait_event_req);
        } while (wait_event_req != NULL);

        KeReleaseSpinLock(&dev_ctx->ioctl_lock, old_irql_ioctl);
        break;

    case LCI_PROXYKMD_NOTIFY_VISIBILITY_UPDATE:
        if (InputBufferSize < sizeof(LCI_PROXYKMD_VISIBILITY_UPDATE))
        {
            LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
                (__FUNCTION__
                ": InputBufferSize(0x%x) too small, required 0x%x bytes?\n",
                InputBufferSize,
                sizeof(LCI_PROXYKMD_VISIBILITY_UPDATE)
                ));
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        visibility_update = InputBuffer;
        dev_ctx->VidPnSourceId = visibility_update->VidPnSourceId;
        dev_ctx->VidPnVisible = visibility_update->Visible;

        list_head = &dev_ctx->event_req_list;
        do
        {
            LJB_VMON_WAIT_FOR_EVENT_REQ *   this_request;
            LJB_VMON_MONITOR_EVENT *        in_event_data;
            LJB_VMON_MONITOR_EVENT *        out_event_data;

            wait_event_req = NULL;
            KeAcquireSpinLock(&dev_ctx->event_req_lock, &old_irql);
            for (list_entry = list_head->Flink;
                list_entry != list_head;
                list_entry = list_entry->Flink)
            {
                this_request = CONTAINING_RECORD(
                    list_entry,
                    LJB_VMON_WAIT_FOR_EVENT_REQ,
                    list_entry
                    );
                if (this_request->in_event_data == NULL)
                    continue;

                in_event_data = this_request->in_event_data;
                if (in_event_data->Flags.VidPnSourceVisibilityChange)
                {
                    RemoveEntryList(&this_request->list_entry);
                    wait_event_req = this_request;
                    break;
                }
            }
            KeReleaseSpinLock(&dev_ctx->event_req_lock, old_irql);
            if (wait_event_req == NULL)
                break;

            in_event_data = wait_event_req->in_event_data;
            out_event_data = wait_event_req->out_event_data;
            RtlZeroMemory(out_event_data, sizeof(*out_event_data));
            out_event_data->VidPnSourceVisibilityData.Visible = visibility_update->Visible;
            out_event_data->Flags.VidPnSourceVisibilityChange = 1;
            LJB_VMON_Printf(dev_ctx, DBGLVL_FLOW,
                (__FUNCTION__
                ": complete Request(%p), VidPnSourceVisibilityChange(%u),Visible(%u)\n",
                wait_event_req,
                out_event_data->Flags.VidPnSourceVisibilityChange,
                visibility_update->Visible
                ));
            WdfRequestCompleteWithInformation(
                wait_event_req->Request,
                STATUS_SUCCESS,
                sizeof(*out_event_data)
                );
            LJB_VMON_FreePool(wait_event_req);
        } while (wait_event_req != NULL);
        break;

    case LCI_PROXYKMD_NOTIFY_COMMIT_VIDPN:
        if (InputBufferSize < sizeof(LCI_PROXYKMD_COMMIT_VIDPN))
        {
            LJB_VMON_Printf(dev_ctx, DBGLVL_ERROR,
                (__FUNCTION__
                ": InputBufferSize(0x%x) too small, required 0x%x bytes?\n",
                InputBufferSize,
                sizeof(LCI_PROXYKMD_COMMIT_VIDPN)
                ));
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        commit_vidpn = InputBuffer;
        dev_ctx->Width = commit_vidpn->Width;
        dev_ctx->Height = commit_vidpn->Height;
        dev_ctx->Pitch = commit_vidpn->Pitch;
        dev_ctx->BytesPerPixel = commit_vidpn->BytesPerPixel;
        dev_ctx->ContentTransformation = commit_vidpn->ContentTransformation;

        list_head = &dev_ctx->event_req_list;
        do
        {
            LJB_VMON_WAIT_FOR_EVENT_REQ *   this_request;
            LJB_VMON_MONITOR_EVENT *        in_event_data;
            LJB_VMON_MONITOR_EVENT *        out_event_data;

            wait_event_req = NULL;
            KeAcquireSpinLock(&dev_ctx->event_req_lock, &old_irql);
            for (list_entry = list_head->Flink;
                list_entry != list_head;
                list_entry = list_entry->Flink)
            {
                this_request = CONTAINING_RECORD(
                    list_entry,
                    LJB_VMON_WAIT_FOR_EVENT_REQ,
                    list_entry
                    );
                if (this_request->in_event_data == NULL)
                    continue;

                in_event_data = this_request->in_event_data;
                if (in_event_data->Flags.ModeChange)
                {
                    RemoveEntryList(&this_request->list_entry);
                    wait_event_req = this_request;
                    break;
                }
            }
            KeReleaseSpinLock(&dev_ctx->event_req_lock, old_irql);
            if (wait_event_req == NULL)
                break;

            in_event_data = wait_event_req->in_event_data;
            out_event_data = wait_event_req->out_event_data;
            RtlZeroMemory(out_event_data, sizeof(*out_event_data));
            out_event_data->TargetModeData.Enabled = commit_vidpn->Width != 0;
            out_event_data->TargetModeData.Width = commit_vidpn->Width;
            out_event_data->TargetModeData.Height = commit_vidpn->Height;
            out_event_data->TargetModeData.Rotation = commit_vidpn->ContentTransformation.Rotation;
            out_event_data->Flags.ModeChange = 1;
            LJB_VMON_Printf(dev_ctx, DBGLVL_FLOW,
                (__FUNCTION__
                ": complete Request(%p): ModeChange, Width(%u), Height(%u)\n",
                wait_event_req,
                commit_vidpn->Width,
                commit_vidpn->Height
                ));
            WdfRequestCompleteWithInformation(
                wait_event_req->Request,
                STATUS_SUCCESS,
                sizeof(*out_event_data)
                );
            LJB_VMON_FreePool(wait_event_req);
        } while (wait_event_req != NULL);
        break;

    default:
        break;
    }

    return ntStatus;
}