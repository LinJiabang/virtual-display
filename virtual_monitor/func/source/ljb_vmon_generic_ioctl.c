#include "ljb_vmon_private.h"

CONST UCHAR gkBuiltInEdidData[] = {
0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x1E, 0x6D, 0x41, 0x58, 0x3A, 0x3D, 0x00, 0x00,
0x08, 0x15, 0x01, 0x03, 0x80, 0x33, 0x1D, 0x78, 0xEA, 0xD9, 0x45, 0xA2, 0x55, 0x4D, 0xA0, 0x27,
0x12, 0x50, 0x54, 0xA7, 0x6B, 0x80, 0x71, 0x4F, 0x81, 0x80, 0x81, 0x40, 0xB3, 0x00, 0x01, 0x01,
0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x3A, 0x80, 0x18, 0x71, 0x38, 0x2D, 0x40, 0x58, 0x2C,
0x45, 0x00, 0xFE, 0x22, 0x11, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00, 0xFD, 0x00, 0x38, 0x4B, 0x1E,
0x53, 0x0F, 0x00, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x44,
0x32, 0x33, 0x34, 0x32, 0x50, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFF,
0x00, 0x31, 0x30, 0x38, 0x4E, 0x44, 0x59, 0x47, 0x30, 0x46, 0x36, 0x37, 0x34, 0x0A, 0x00, 0x8E,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define FAKE_EDID_SIZE      sizeof(gkBuiltInEdidData)
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
    LJB_VMON_CTX * CONST                    pDevCtx = ProviderContext;
    LCI_GENERIC_INTERFACE * CONST           pTargetInterface = &pDevCtx->TargetGenericInterface;
    LCI_PROXYKMD_PRIMARY_SURFACE_CREATE *   pCreateData;
    LCI_PROXYKMD_PRIMARY_SURFACE_DESTROY *  pDestroyData;
    LCI_PROXYKMD_PRIMARY_SURFACE_UPDATE *   pUpdateData;
    LCI_PROXYKMD_CURSOR_UPDATE *            pCursorData;
    LJB_VMON_PRIMARY_SURFACE *              pPrimarySurface;
    LJB_VMON_WAIT_FOR_UPDATE_REQ *          pWaitRequest;
    LIST_ENTRY *                            pListHead;
    LIST_ENTRY *                            pListEntry;
    LIST_ENTRY *                            pNextEntry;
    KIRQL                                   OldIrql;
    KIRQL                                   PendingIoctlIrql;
    LONG                                    ReferenceCount;
    NTSTATUS                                ntStatus;
    UINT                                    i;
    LJB_VMON_PRIMARY_SURFACE *              pThisSurface;

    UNREFERENCED_PARAMETER(OutputBuffer);
    UNREFERENCED_PARAMETER(OutputBufferSize);

    pThisSurface = NULL;
    *BytesReturned = 0;
    ntStatus = STATUS_NOT_SUPPORTED;
    switch (IoctlCode)
    {
    case LCI_PROXYKMD_GET_EDID:
        if (OutputBufferSize < FAKE_EDID_SIZE)
        {
            LJB_VMON_Printf(pDevCtx, DBGLVL_ERROR,
                ("?"__FUNCTION__ ": "
                "OutputBufferSize(0x%x) too small, required 0x%x bytes?\n",
                OutputBufferSize,
                FAKE_EDID_SIZE
                ));
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        RtlCopyMemory(
            OutputBuffer,
            gkBuiltInEdidData,
            FAKE_EDID_SIZE
            );
        ntStatus = STATUS_SUCCESS;
        break;

    case LCI_PROXYKMD_NOTIFY_PRIMARY_SURFACE_CREATE:
        if (InputBufferSize < sizeof(LCI_PROXYKMD_PRIMARY_SURFACE_CREATE))
        {
            LJB_VMON_Printf(pDevCtx, DBGLVL_ERROR,
                ("?"__FUNCTION__ ": "
                "InputBufferSize(0x%x) too small, required 0x%x bytes?\n",
                InputBufferSize,
                sizeof(LCI_PROXYKMD_PRIMARY_SURFACE_CREATE)
                ));
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        pCreateData = InputBuffer;
        for (i = 0; i < NUM_OF_BUFFERS_PER_SURFACE; i++)
        {
            pPrimarySurface = LJB_VMON_GetPoolZero(sizeof(*pPrimarySurface));
            if (pPrimarySurface == NULL)
            {
                LJB_VMON_Printf(pDevCtx, DBGLVL_ERROR,
                    ("?" __FUNCTION__ ": no pPrimarySurface created?\n"
                    ));
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            pPrimarySurface->pBuffer = LJB_VMON_GetPoolZero(
                pCreateData->BufferSize
                );
            if (pPrimarySurface->pBuffer == NULL)
            {
                LJB_VMON_Printf(pDevCtx, DBGLVL_ERROR,
                    ("?" __FUNCTION__ ": No pBuffer allocated?\n"
                    ));
                LJB_VMON_FreePool(pPrimarySurface);
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            pPrimarySurface->pMdl = IoAllocateMdl(
                pPrimarySurface->pBuffer,
                (ULONG) pCreateData->BufferSize,
                FALSE,
                FALSE,
                NULL
                );
            if (pPrimarySurface->pMdl == NULL)
            {
                LJB_VMON_Printf(pDevCtx, DBGLVL_ERROR,
                    ("?" __FUNCTION__ ": no pMdl allocated?\n"
                    ));
                LJB_VMON_FreePool(pPrimarySurface->pBuffer);
                LJB_VMON_FreePool(pPrimarySurface);
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            MmBuildMdlForNonPagedPool(pPrimarySurface->pMdl);

            pPrimarySurface->hPrimarySurface    = pCreateData->hPrimarySurface;
            pPrimarySurface->pRemoteBuffer      = pCreateData->pBuffer;
            pPrimarySurface->BufferSize         = pCreateData->BufferSize;
            pPrimarySurface->Width              = pCreateData->Width;
            pPrimarySurface->Height             = pCreateData->Height;
            pPrimarySurface->Pitch              = pCreateData->Pitch;
            pPrimarySurface->BytesPerPixel      = pCreateData->BytesPerPixel;
            pPrimarySurface->ReferenceCount     = 1;
            pPrimarySurface->bTransferDone      = TRUE;
            pPrimarySurface->bBusyBltting       = FALSE;
            pPrimarySurface->FrameId            = 0;

            pListHead = &pDevCtx->PrimarySurfaceListHead;
            KeAcquireSpinLock(&pDevCtx->PrimarySurfaceListLock, &OldIrql);
            InsertTailList(
                pListHead,
                &pPrimarySurface->ListEntry
                );
            KeReleaseSpinLock(&pDevCtx->PrimarySurfaceListLock, OldIrql);
            LJB_VMON_Printf(pDevCtx, DBGLVL_FLOW,
                (__FUNCTION__ ": "
                "pPrimarySurface(%p)/pMdl(%p) created, "
                "Width(%u)/Height(%u)/Size(0x%x)\n",
                pPrimarySurface,
                pPrimarySurface->pMdl,
                pPrimarySurface->Width,
                pPrimarySurface->Height,
                pPrimarySurface->BufferSize
                ));
        }
        break;

    case LCI_PROXYKMD_NOTIFY_PRIMARY_SURFACE_DESTROY:
        if (InputBufferSize < sizeof(LCI_PROXYKMD_PRIMARY_SURFACE_DESTROY))
        {
            LJB_VMON_Printf(pDevCtx, DBGLVL_ERROR,
                ("?"__FUNCTION__ ": "
                "InputBufferSize(0x%x) too small, required 0x%x bytes?\n",
                InputBufferSize,
                sizeof(LCI_PROXYKMD_PRIMARY_SURFACE_DESTROY)
                ));
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        /*
         * Locate pPrimarySurface with matched hPrimarySurface
         */
        pDestroyData = InputBuffer;
        pListHead = &pDevCtx->PrimarySurfaceListHead;
        pPrimarySurface = NULL;
        pNextEntry = NULL;
        KeAcquireSpinLock(&pDevCtx->PrimarySurfaceListLock, &OldIrql);
        for (pListEntry = pListHead->Flink;
            pListEntry != pListHead;
            pListEntry = pNextEntry)
        {
            pNextEntry = pListEntry->Flink;
            pThisSurface = CONTAINING_RECORD(
                pListEntry,
                LJB_VMON_PRIMARY_SURFACE,
                ListEntry
                );
            if (pThisSurface->hPrimarySurface == pDestroyData->hPrimarySurface)
            {
                /*
                 * If nobody is holding a reference on the object, we are safe
                 * to remove it from the list
                 */
                ReferenceCount = InterlockedDecrement(&pThisSurface->ReferenceCount);
                if (ReferenceCount == 0)
                {
                    RemoveEntryList(&pThisSurface->ListEntry);
                    pPrimarySurface = pThisSurface;

                    LJB_VMON_Printf(pDevCtx, DBGLVL_FLOW,
                        (__FUNCTION__ ": "
                        "pPrimarySurface(%p)/pMdl(%p) destroyed\n",
                        pPrimarySurface,
                        pPrimarySurface->pMdl
                        ));
                    IoFreeMdl(pPrimarySurface->pMdl);
                    LJB_VMON_FreePool(pPrimarySurface->pBuffer);
                    LJB_VMON_FreePool(pPrimarySurface);
                }
                else
                {
                    LJB_VMON_Printf(pDevCtx, DBGLVL_FLOW,
                        (__FUNCTION__ ": "
                        "pThisSurface(%p) has ReferenceCount(%d), "
                        "Defer destruction to later.\n",
                        pThisSurface,
                        ReferenceCount
                        ));
                }
            }
        }
        KeReleaseSpinLock(&pDevCtx->PrimarySurfaceListLock, OldIrql);

        ntStatus = STATUS_SUCCESS;
        break;

    case LCI_PROXYKMD_NOTIFY_PRIMARY_SURFACE_UPDATE:
        if (InputBufferSize < sizeof(LCI_PROXYKMD_PRIMARY_SURFACE_UPDATE))
        {
            LJB_VMON_Printf(pDevCtx, DBGLVL_ERROR,
                ("?"__FUNCTION__ ": "
                "InputBufferSize(0x%x) too small, required 0x%x bytes?\n",
                InputBufferSize,
                sizeof(LCI_PROXYKMD_PRIMARY_SURFACE_UPDATE)
                ));
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        KeAcquireSpinLock(&pDevCtx->PendingIoctlListLock, &PendingIoctlIrql);
        pUpdateData = InputBuffer;
        pDevCtx->LatestFrameId = pUpdateData->FrameId;
        pDevCtx->hLatestPrimarySurface = pUpdateData->hPrimarySurface;

        /*
         * ask KMD to copy pixels to our backbuffer, if one is found.
         */
        pPrimarySurface = NULL;
        pListHead = &pDevCtx->PrimarySurfaceListHead;
        KeAcquireSpinLock(&pDevCtx->PrimarySurfaceListLock, &OldIrql);
        for (pListEntry = pListHead->Flink;
            pListEntry != pListHead;
            pListEntry = pListEntry->Flink)
        {
            pThisSurface = CONTAINING_RECORD(
                pListEntry,
                LJB_VMON_PRIMARY_SURFACE,
                ListEntry
                );
            if (pThisSurface->hPrimarySurface != pUpdateData->hPrimarySurface)
                continue;
            if (pThisSurface->bTransferDone && !pThisSurface->bBusyBltting)
            {
                pPrimarySurface= pThisSurface;
                pPrimarySurface->bBusyBltting = TRUE;
                break;
            }
        }
        KeReleaseSpinLock(&pDevCtx->PrimarySurfaceListLock, OldIrql);
        if (pPrimarySurface == NULL)
        {
            LJB_VMON_PrintfAlways(pDevCtx, DBGLVL_ERROR,
                ("?" __FUNCTION__ ": No free AV buffer. Drop FrameId(%d).\n",
                pDevCtx->LatestFrameId
                ));
        }
        else
        {
            LCI_USBAV_BLT_DATA  MyBltData;
            ULONG               BytesReturned;

            pPrimarySurface->FrameId = pUpdateData->FrameId;
            RtlZeroMemory(&MyBltData, sizeof(MyBltData));
            MyBltData.hPrimarySurface   = pPrimarySurface->hPrimarySurface;
            MyBltData.pPrimaryBuffer    = pPrimarySurface->pRemoteBuffer;
            MyBltData.pShadowBuffer     = pPrimarySurface->pBuffer;
            MyBltData.BufferSize        = pPrimarySurface->BufferSize;
            (VOID) (*pTargetInterface->pfnGenericIoctl)(
                pTargetInterface->ProviderContext,
                LCI_USBAV_BLT_PRIMARY_TO_SHADOW,
                &MyBltData,
                sizeof(MyBltData),
                NULL,
                0,
                &BytesReturned
                );

            InterlockedIncrement(&pDevCtx->AcquirelistCount);
            LJB_VMON_PrintfAlways(pDevCtx, DBGLVL_FLOW,
                (__FUNCTION__ ": FrameId(%d) blt done, count(%d).\n",
                pDevCtx->LatestFrameId,
                pDevCtx->AcquirelistCount
                ));

            pPrimarySurface->bBusyBltting = FALSE;
            pPrimarySurface->bTransferDone = FALSE;
        }

        /*
         * Check if there is pending wait request
         */
        pListHead = &pDevCtx->WaitRequestListHead;
        do
        {
            LJB_VMON_WAIT_FOR_UPDATE_REQ *  pThisRequest;
            LJB_VMON_WAIT_FOR_UPDATE_DATA * pInputWaitUpdateData;
            LJB_VMON_WAIT_FOR_UPDATE_DATA * pOutputWaitUpdateData;

            pWaitRequest = NULL;
            KeAcquireSpinLock(&pDevCtx->WaitRequestListLock, &OldIrql);
            for (pListEntry = pListHead->Flink;
                 pListEntry != pListHead;
                 pListEntry = pListEntry->Flink)
            {
                pThisRequest = CONTAINING_RECORD(
                    pListEntry,
                    LJB_VMON_WAIT_FOR_UPDATE_REQ,
                    ListEntry
                    );
                if (pThisRequest->IoctlCode == IOCTL_LJB_VMON_WAIT_FOR_UPDATE &&
                    pThisRequest->pInputWaitUpdateData->Flags.Frame)
                {
                    RemoveEntryList(&pThisRequest->ListEntry);
                    pWaitRequest = pThisRequest;
                    break;
                }

                if (pThisRequest->IoctlCode == IOCTL_LJB_VMON_ACQUIRE_FRAME)
                {
                    RemoveEntryList(&pThisRequest->ListEntry);
                    pWaitRequest = pThisRequest;
                    break;
                }
            }
            KeReleaseSpinLock(&pDevCtx->WaitRequestListLock, OldIrql);
            if (pWaitRequest == NULL)
                break;

            if (pWaitRequest->IoctlCode == IOCTL_LJB_VMON_WAIT_FOR_UPDATE)
            {
                pInputWaitUpdateData = pWaitRequest->pInputWaitUpdateData;
                pOutputWaitUpdateData = pWaitRequest->pOutputWaitUpdateData;
                RtlZeroMemory(&pOutputWaitUpdateData->Flags, sizeof(LJB_WAIT_FLAGS));
                pOutputWaitUpdateData->Flags.Frame = 1;
                pOutputWaitUpdateData->FrameId = pDevCtx->LatestFrameId;
                /*
                 * check for bCursorUpdatePending
                 */
                if (pDevCtx->bCursorUpdatePending)
                {
                    pDevCtx->bCursorUpdatePending = FALSE;
                    pOutputWaitUpdateData->Flags.Cursor = 1;
                }
                LJB_VMON_Printf(pDevCtx, DBGLVL_FLOW,
                    (__FUNCTION__ ": complete Request(%p), FrameId(0x%x).\n",
                    pWaitRequest,
                    pDevCtx->LatestFrameId
                    ));
                WdfRequestCompleteWithInformation(
                    pWaitRequest->Request,
                    STATUS_SUCCESS,
                    sizeof(*pOutputWaitUpdateData)
                    );
                LJB_VMON_FreePool(pWaitRequest);
            }
            else if (pWaitRequest->IoctlCode == IOCTL_LJB_VMON_ACQUIRE_FRAME)
            {
                LJB_VMON_Printf(pDevCtx, DBGLVL_FLOW,
                    (__FUNCTION__ ": complete pWaitRequest(%p) with FAILURE.\n",
                    pWaitRequest
                    ));
                WdfRequestComplete(
                    pWaitRequest->Request,
                    STATUS_UNSUCCESSFUL
                    );
                LJB_VMON_FreePool(pWaitRequest);
            }
        } while (pWaitRequest != NULL);
        KeReleaseSpinLock(&pDevCtx->PendingIoctlListLock, PendingIoctlIrql);
        break;

    case LCI_PROXYKMD_NOTIFY_CURSOR_UPDATE:
        if (InputBufferSize < sizeof(LCI_PROXYKMD_CURSOR_UPDATE))
        {
            LJB_VMON_Printf(pDevCtx, DBGLVL_ERROR,
                ("?"__FUNCTION__ ": "
                "InputBufferSize(0x%x) too small, required 0x%x bytes?\n",
                InputBufferSize,
                sizeof(LCI_PROXYKMD_CURSOR_UPDATE)
                ));
            ntStatus = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        KeAcquireSpinLock(&pDevCtx->PendingIoctlListLock, &PendingIoctlIrql);
        pCursorData = InputBuffer;

        if (pCursorData->pPositionUpdate != NULL)
        {
            CONST DXGKARG_SETPOINTERPOSITION * pSetPointerPosition;

            pSetPointerPosition = pCursorData->pPositionUpdate;
            pDevCtx->PointerInfo.X         = pSetPointerPosition->X;
            pDevCtx->PointerInfo.Y         = pSetPointerPosition->Y;
            pDevCtx->PointerInfo.Visible   = (pSetPointerPosition->Flags.Visible == 1);
        }

        if (pCursorData->pShapeUpdate != NULL)
        {
            ULONG   BitmapSize;
            CONST DXGKARG_SETPOINTERSHAPE * pSetPointerShape;

            pSetPointerShape = pCursorData->pShapeUpdate;
            BitmapSize = pSetPointerShape->Pitch * pSetPointerShape->Height;

            if (pSetPointerShape->Flags.Monochrome)
                BitmapSize *= 2;

            pDevCtx->PointerInfo.Flags     = pSetPointerShape->Flags;
            pDevCtx->PointerInfo.Width     = pSetPointerShape->Width;
            pDevCtx->PointerInfo.Height    = pSetPointerShape->Height;
            pDevCtx->PointerInfo.Pitch     = pSetPointerShape->Pitch;
            pDevCtx->PointerInfo.XHot      = pSetPointerShape->XHot;
            pDevCtx->PointerInfo.YHot      = pSetPointerShape->YHot;
            ASSERT(BitmapSize <= MAX_POINTER_SIZE);
            RtlCopyMemory(
                pDevCtx->PointerInfo.Bitmap,
                pSetPointerShape->pPixels,
                BitmapSize
                );
        }

        pListHead = &pDevCtx->WaitRequestListHead;
        do
        {
            LJB_VMON_WAIT_FOR_UPDATE_REQ *      pThisRequest;
            LJB_VMON_WAIT_FOR_UPDATE_DATA *     pInputWaitUpdateData;
            LJB_VMON_WAIT_FOR_UPDATE_DATA *     pOutputWaitUpdateData;

            pWaitRequest = NULL;
            KeAcquireSpinLock(&pDevCtx->WaitRequestListLock, &OldIrql);
            for (pListEntry = pListHead->Flink;
                pListEntry != pListHead;
                pListEntry = pListEntry->Flink)
            {
                pThisRequest = CONTAINING_RECORD(
                    pListEntry,
                    LJB_VMON_WAIT_FOR_UPDATE_REQ,
                    ListEntry
                    );
                if (pThisRequest->pInputWaitUpdateData == NULL)
                    continue;

                if (pThisRequest->pInputWaitUpdateData->Flags.Cursor)
                {
                    RemoveEntryList(&pThisRequest->ListEntry);
                    pWaitRequest = pThisRequest;
                    break;
                }
            }
            KeReleaseSpinLock(&pDevCtx->WaitRequestListLock, OldIrql);
            if (pWaitRequest == NULL)
                break;

            pInputWaitUpdateData = pWaitRequest->pInputWaitUpdateData;
            pOutputWaitUpdateData = pWaitRequest->pOutputWaitUpdateData;
            pOutputWaitUpdateData->Flags.Cursor = 1;
            pOutputWaitUpdateData->FrameId = pDevCtx->LatestFrameId;
            LJB_VMON_Printf(pDevCtx, DBGLVL_FLOW,
                (__FUNCTION__ ": complete Request(%p), Cursor got updated. FrameId(0x%x).\n",
                pWaitRequest,
                pDevCtx->LatestFrameId
                ));
            WdfRequestCompleteWithInformation(
                pWaitRequest->Request,
                STATUS_SUCCESS,
                sizeof(*pOutputWaitUpdateData)
                );
            LJB_VMON_FreePool(pWaitRequest);
        } while (pWaitRequest != NULL);

        KeReleaseSpinLock(&pDevCtx->PendingIoctlListLock, PendingIoctlIrql);

        break;

    default:
        break;
    }

    return ntStatus;
}