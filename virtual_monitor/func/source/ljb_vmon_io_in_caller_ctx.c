#include "ljb_vmon_private.h"
#include "ljb_vmon_ioctl.h"

static BOOLEAN
LJB_VMON_IoctlGenericIoPreProcess(
    __in WDFDEVICE                  WdfDevice,
    __in WDFREQUEST                 WdfRequest,
    __in WDF_REQUEST_PARAMETERS     Params
    );

static BOOLEAN
LJB_VMON_IoctlAcquireFramePreProcess(
    __in WDFDEVICE                  WdfDevice,
    __in WDFREQUEST                 WdfRequest,
    __in WDF_REQUEST_PARAMETERS     Params
    );

static BOOLEAN
LJB_VMON_IoctlReleaseFramePreProcess(
    __in WDFDEVICE                  WdfDevice,
    __in WDFREQUEST                 WdfRequest,
    __in WDF_REQUEST_PARAMETERS     Params
    );

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
    LJB_VMON_CTX * CONST    pDevCtx = LJB_VMON_GetVMonCtx(WdfDevice);
    WDF_REQUEST_PARAMETERS  params;
    NTSTATUS                ntStatus;
    BOOLEAN                 bRequestHasBeenCompleted;

    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(WdfRequest, &params);

    /*
     * any request type other than DeviceControl is not of our interest.
     */
    if (params.Type != WdfRequestTypeDeviceControl)
    {
        ntStatus = WdfDeviceEnqueueRequest(WdfDevice, WdfRequest);
        if (!NT_SUCCESS(ntStatus))
        {
            LJB_VMON_Printf(pDevCtx, DBGLVL_ERROR,
                ("?" __FUNCTION__
                ": WdfDeviceEnqueueRequest failed with ntStatus(0x%08x)?\n",
                ntStatus
                ));
            WdfRequestComplete(WdfRequest, ntStatus);
        }
        return;
    }

    bRequestHasBeenCompleted = FALSE;
    switch (params.Parameters.DeviceIoControl.IoControlCode)
    {
    case IOCTL_LJB_VMON_ACQUIRE_FRAME:
        if (pDevCtx->DeviceDead)
        {
            LJB_VMON_Printf(pDevCtx, DBGLVL_ERROR,
                ("?" __FUNCTION__
                ": DeviceGone? fail IOCTL_LJB_VMON_ACQUIRE_FRAME directly.\n"
                ));
            ntStatus = STATUS_DEVICE_REMOVED;
            WdfRequestComplete(WdfRequest, ntStatus);
            return;
        }
        bRequestHasBeenCompleted = LJB_VMON_IoctlAcquireFramePreProcess(
            WdfDevice,
            WdfRequest,
            params
            );
        break;

    case IOCTL_LJB_VMON_RELEASE_FRAME:
        bRequestHasBeenCompleted = LJB_VMON_IoctlReleaseFramePreProcess(
            WdfDevice,
            WdfRequest,
            params
            );
        break;

    default:
        break;
    }

    /*
     * If we are able to get input and/or output buffer, put it back to request
     * queue.
     */
    if (!bRequestHasBeenCompleted)
    {
        ntStatus = WdfDeviceEnqueueRequest(WdfDevice, WdfRequest);
        if (!NT_SUCCESS(ntStatus))
        {
            WdfRequestComplete(WdfRequest, ntStatus);
        }
    }
}

/*
 * Name:  LJB_VMON_DrawCursor
 *
 * Definition:
 *    static VOID
 *    LJB_VMON_DrawCursor(
 *        __in WDFDEVICE                      WdfDevice,
 *        __in LJB_VMON_PRIMARY_SURFACE *       pPrimarySurface
 *        );
 *
 * Description:
 *    Draw cursor on the frame buffer upon IOCTL_LJB_VMON_ACQUIRE_FRAME request
 *    is received.
 *
 * Return Value:
 *    TRUE if the request has been completed by this routine (if an error occurs),
 *    FALSE if the request is successfully processed, and request further enqueue.
 *
 */
static VOID
LJB_VMON_DrawCursor(
    __in WDFDEVICE                      WdfDevice,
    __in LJB_VMON_PRIMARY_SURFACE *     pPrimarySurface
    )
{
    LJB_VMON_CTX * CONST    pDevCtx = LJB_VMON_GetVMonCtx(WdfDevice);
    ULONG SurfWidth;
    ULONG SurfHeight;
    INT CurPosX;
    INT CurPosY;
    UINT CurWidth;
    UINT CurHeight;
    UINT CurPitch;
    UINT EffCurWidth;
    UINT EffCurHeight;
    UCHAR * pCurBitmap;
    UCHAR * pSurfBitmap;

    UCHAR * pFinalBlendCurBuffer;
    UCHAR * pFinalBlendCurBufferStart;
    UCHAR * pOrigSurfPos;
    UCHAR * pOrigSurfPosStart;

    UINT row;

    SurfWidth = pPrimarySurface->Width;
    SurfHeight = pPrimarySurface->Height;

    CurPosX = pDevCtx->PointerInfo.X;
    CurPosY = pDevCtx->PointerInfo.Y;

    CurWidth = pDevCtx->PointerInfo.Width;
    CurHeight = pDevCtx->PointerInfo.Height;
    CurPitch = pDevCtx->PointerInfo.Pitch;

    EffCurWidth = CurWidth;
    EffCurHeight = CurHeight;
    pCurBitmap = pDevCtx->PointerInfo.Bitmap;

    /*
     * Calculate effective cursor Width/Height/Position.
     */
    if (CurPosX >= 0)
    {
        if ((CurPosX + CurWidth) >= (SurfWidth - 1))
            EffCurWidth = SurfWidth - 1 - CurPosX;
    }
    else
    {
        EffCurWidth = CurWidth + CurPosX;
        CurPosX = 0;
    }

    if (CurPosY >= 0)
    {
        if ((CurPosY + CurHeight) >= (SurfHeight - 1))
            EffCurHeight = SurfHeight - 1 - CurPosY;
    }
    else
    {
        EffCurHeight = CurHeight + CurPosY;
        CurPosY = 0;
    }

    if (EffCurWidth == 0 || EffCurHeight == 0)
    {
        LJB_VMON_Printf(pDevCtx, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": EffCurWidth or EffCurHeight is 0.\n"));
        return;
    }

    pFinalBlendCurBufferStart = LJB_VMON_GetPoolZero(
        EffCurWidth * EffCurHeight * 4//CurPitch
        );

    if (pFinalBlendCurBufferStart == NULL)
    {
        LJB_VMON_Printf(pDevCtx, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": unable to allocate pFinalBlendCurBufferStart?\n"));
        return;
    }

    pFinalBlendCurBuffer = pFinalBlendCurBufferStart;
    pOrigSurfPos = pOrigSurfPosStart =
        (UCHAR *)pPrimarySurface->pBuffer +
        CurPosY * pPrimarySurface->Pitch + CurPosX * 4;

    /*
     * http://msdn.microsoft.com/en-us/library/windows/hardware/ff559481(v=vs.85).aspx
     * 1. Monochrome cursor:
     * A monochrome bitmap whose size is specified by Width and Height in a 1 bits
     * per pixel (bpp) DIB format AND mask that is followed by another 1 bpp DIB
     * format XOR mask of the same size.
     */
    if (pDevCtx->PointerInfo.Flags.Value == 1)
    {
        UCHAR * pANDMask;
        UCHAR * pXORMask;
        UINT iByte;
        UINT iBit;

        for(row = 0; row < EffCurHeight; row++)
        {
            pANDMask = pCurBitmap;
            pXORMask = pCurBitmap + CurHeight * CurPitch;
            for (iByte = 0, iBit = 7; iByte < EffCurWidth * 4; iByte +=4, iBit--)
            {
                /*
                 AND first and then XOR.
                */
                if ((*pANDMask >> iBit) & 1)
                {
                    *((ULONG *) (pFinalBlendCurBuffer + iByte)) =
                        *((ULONG *) (pOrigSurfPos + iByte)) & 0xFFFFFFFF;
                }
                else
                {
                    *((ULONG *) (pFinalBlendCurBuffer + iByte)) =
                        *((ULONG *) (pOrigSurfPos + iByte)) & 0;
                }

                if ((*pXORMask >> iBit) & 1)
                {
                    *((ULONG *) (pFinalBlendCurBuffer + iByte)) ^= 0xFFFFFFFF;
                }
                else
                {
                    *((ULONG *) (pFinalBlendCurBuffer + iByte)) ^= 0;
                }

                if (iBit == 0)
                {
                    pANDMask++;
                    pXORMask++;
                    iBit = 8;
                }
            }
            pOrigSurfPos += SurfWidth * 4;
            pFinalBlendCurBuffer += EffCurWidth * 4;
            pCurBitmap += CurPitch;
        }
    }
    else
    {
        /*
         * When the cursor is on the boundary of the screen, CurPosX/CurPosY
         * can be negative. So the start address of the cursor bitmap need to
         * be re-calculated for these special cases.
         */
        UCHAR CurA;
        SIZE_T AbsCurPosX;
        SIZE_T AbsCurPosY;
        UINT i;

        CurPosX = pDevCtx->PointerInfo.X;
        CurPosY = pDevCtx->PointerInfo.Y;

        AbsCurPosX = abs(CurPosX);
        AbsCurPosY = abs(CurPosY);

        if (CurPosX < 0 && CurPosY < 0)
        {
            pCurBitmap += AbsCurPosY * CurPitch + AbsCurPosX * 4;
        }

        else if (CurPosX < 0)
        {
            pCurBitmap += AbsCurPosX * 4;
        }

        else if (CurPosY < 0)
        {
            pCurBitmap += AbsCurPosY * CurPitch;
        }

        /*
         * 2. Color cursor:
         * A color bitmap whose size is specified by Width and Height in a 32 bpp
         * ARGB device independent bitmap (DIB) format.
         */
        if (pDevCtx->PointerInfo.Flags.Value == 2)
        {
            UCHAR CurR, CurG, CurB;
            UCHAR SurfR, SurfG, SurfB;

            for(row = 0; row < EffCurHeight; ++row)
            {
                for (i = 0; i < EffCurWidth * 4; i +=4)
                {
                    SurfB = pOrigSurfPos[i];
                    SurfG = pOrigSurfPos[i + 1];
                    SurfR = pOrigSurfPos[i + 2];

                    CurB = pCurBitmap[i];
                    CurG = pCurBitmap[i + 1];
                    CurR = pCurBitmap[i + 2];
                    CurA = pCurBitmap[i + 3];

                    pFinalBlendCurBuffer[i] = SurfB + (((CurB - SurfB) * CurA) / 255);
                    pFinalBlendCurBuffer[i + 1] = SurfG + (((CurG - SurfG) * CurA) / 255);
                    pFinalBlendCurBuffer[i + 2] = SurfR + (((CurR - SurfR) * CurA) / 255);
                    pFinalBlendCurBuffer[i + 3] = pOrigSurfPos[i + 3];
                }
                pOrigSurfPos += SurfWidth * 4;
                pCurBitmap += CurPitch;
                pFinalBlendCurBuffer += EffCurWidth * 4;
            }
        }

        /*
         * 3. Masked color cursor:
         * A 32-bpp ARGB format bitmap with the mask value in the alpha bits. The only
         * allowed mask values are 0 and 0xFF. When the mask value is 0, the RGB value
         * should replace the screen pixel. When the mask value is 0xFF, an XOR operation
         * is performed on the RGB value and the screen pixel; the result should replace
         * the screen pixel.
         */
        if (pDevCtx->PointerInfo.Flags.Value == 4)
        {
            for(row = 0; row < EffCurHeight; ++row)
            {
                for (i = 0; i < EffCurWidth * 4; i +=4)
                {
                    CurA = pCurBitmap[i + 3];
                    if (CurA == 0)
                    {
                        *((ULONG *) (pFinalBlendCurBuffer + i)) =
                            *((ULONG *) (pCurBitmap + i));
                    }
                    else
                    {
                        *((ULONG *) (pFinalBlendCurBuffer + i)) =
                            *((ULONG *) (pOrigSurfPos + i)) ^ *((ULONG *) (pCurBitmap + i));
                    }
                    pFinalBlendCurBuffer[i+3] = pOrigSurfPos[i+3];
                }
                pCurBitmap += CurPitch;
                pOrigSurfPos += SurfWidth * 4;
                pFinalBlendCurBuffer += EffCurWidth *4;
            }
        }
    }

    /*
     * Back up the replaced section of primarySurface before putting the final
     * cursor image on the primarySurface.
     */
    pPrimarySurface->pOrigSurfPosStart = pOrigSurfPosStart;
    pPrimarySurface->EffCurWidth = EffCurWidth;
    pPrimarySurface->EffCurHeight = EffCurHeight;
    pPrimarySurface->bIsCursorDrew = TRUE;
    pSurfBitmap = pPrimarySurface->SurfBitmap;

    pOrigSurfPos = pOrigSurfPosStart;
    pFinalBlendCurBuffer = pFinalBlendCurBufferStart;
    for(row = 0; row < EffCurHeight; ++row)
    {
        /*
         * Back up original primarySurface.
         */
        RtlCopyMemory(
            pSurfBitmap,
            pOrigSurfPos,
            EffCurWidth * 4
            );
        pSurfBitmap += EffCurWidth * 4;

        /*
         * Start filling cursor.
         */
        RtlCopyMemory(
            pOrigSurfPos,
            pFinalBlendCurBuffer,
            EffCurWidth * 4
            );
        pOrigSurfPos += SurfWidth * 4;
        pFinalBlendCurBuffer += EffCurWidth * 4;
    }
    LJB_VMON_FreePool(pFinalBlendCurBufferStart);

}

/*
 * Name:  LJB_VMON_IoctlAcquireFramePreProcess
 *
 * Definition:
 *    static BOOLEAN
 *    LJB_VMON_IoctlAcquireFramePreProcess(
 *        __in WDFDEVICE                  WdfDevice,
 *        __in WDFREQUEST                 WdfRequest,
 *        __in WDF_REQUEST_PARAMETERS     Params
 *        );
 *
 * Description:
 *    IOCTL_LJB_VMON_ACQUIRE_FRAME request, we find a free LJB_VMON_PRIMARY_SURFACE
 *    and map its kernel buffer to user space. If the mapping is successful,
 *    the routine updates the pixel contents.
 *
 * Return Value:
 *    TRUE if the request has been completed by this routine (if an error occurs),
 *    FALSE if the request is successfully processed, and request further enqueue.
 *
 */
static BOOLEAN
LJB_VMON_IoctlAcquireFramePreProcess(
    __in WDFDEVICE                  WdfDevice,
    __in WDFREQUEST                 WdfRequest,
    __in WDF_REQUEST_PARAMETERS     Params
    )
{
    LJB_VMON_CTX * CONST            pDevCtx = LJB_VMON_GetVMonCtx(WdfDevice);
    LCI_GENERIC_INTERFACE * CONST   pTargetInterface = &pDevCtx->TargetGenericInterface;
    LIST_ENTRY * CONST              pListHead = &pDevCtx->PrimarySurfaceListHead;
    WDFFILEOBJECT CONST             UserFileObject = WdfRequestGetFileObject(WdfRequest);
    LJB_VMON_FRAME_INFO *           pFrameInfo;
    LCI_USBAV_BLT_DATA              MyBltData;
    LJB_VMON_PRIMARY_SURFACE *      pPrimarySurface;
    KIRQL                           OldIrql;
    LJB_VMON_WAIT_FOR_UPDATE_REQ *  pWaitRequest;
    LIST_ENTRY *                    pListEntry;
    PVOID                           pOutputBuffer;
    size_t                          OutBufSize;
    NTSTATUS                        ntStatus;
    LJB_VMON_PRIMARY_SURFACE *      pThisSurface;
    UINT                            m;
    ULONG                           MaxFrameIdInList;
    LJB_VMON_PRIMARY_SURFACE *      pMaxFrameIdSurface;
    BOOLEAN                         bCursorUpdate;

    pThisSurface = NULL;
    m = 0;
    MaxFrameIdInList = 0;
    pMaxFrameIdSurface = NULL;
    bCursorUpdate = FALSE;

    if (pTargetInterface->ProviderContext == NULL ||
        pTargetInterface->pfnGenericIoctl == NULL)
    {
        LJB_VMON_Printf(pDevCtx, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": "
            "bad pTargetInterface?\n"
            ));
        ntStatus = STATUS_UNSUCCESSFUL;
        WdfRequestComplete(WdfRequest, ntStatus);
        return TRUE;
    }

    if (Params.Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(LJB_VMON_FRAME_INFO))
    {
        LJB_VMON_Printf(pDevCtx, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": "
            "OutputBufferLength (0x%x) too small? Need %u bytes.\n",
            Params.Parameters.DeviceIoControl.OutputBufferLength,
            sizeof(LJB_VMON_FRAME_INFO)
            ));
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        WdfRequestComplete(WdfRequest, ntStatus);
        return TRUE;
    }

    ntStatus = WdfRequestRetrieveOutputBuffer(
        WdfRequest,
        sizeof(LJB_VMON_FRAME_INFO),
        &pOutputBuffer,
        &OutBufSize
        );
    if(!NT_SUCCESS(ntStatus))
    {
        LJB_VMON_Printf(pDevCtx, DBGLVL_ERROR,
            ("?" __FUNCTION__
            ": unable to get OutputBuffer?\n"
            ));
        WdfRequestComplete(WdfRequest, ntStatus);
        return TRUE;
    }

    pPrimarySurface = NULL;
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

        /*
         * First filter out invalid Surfaces and then find the smallest FrameId.
         */
        if (pThisSurface->bIsAcquiredByUserMode || 
            pThisSurface->bTransferDone ||
            (pDevCtx->FrameIdSent >= pThisSurface->FrameId))
        {
            continue;
        }

        if (m == 0)
        {
            pPrimarySurface = pThisSurface;
            m++;
            continue;
        }

        if (pThisSurface->FrameId < pPrimarySurface->FrameId)
        {
            pPrimarySurface = pThisSurface;
        }
    }

    if (pPrimarySurface != NULL)
    {
        InterlockedIncrement(&pPrimarySurface->ReferenceCount);
        pPrimarySurface->bIsAcquiredByUserMode = TRUE;
        pPrimarySurface->UserFileObject = UserFileObject;
    }

    KeReleaseSpinLock(&pDevCtx->PrimarySurfaceListLock, OldIrql);

    if (pPrimarySurface == NULL)
    {
        /*
         * Check if it is cursor update or frame update with no free buffer
         */

        /*
         * Mouse event is handled here.
         */
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

            /*
             * Find surface with largest frame Id.
             */
            if (pThisSurface->FrameId > MaxFrameIdInList)
            {
                MaxFrameIdInList = pThisSurface->FrameId;
                pMaxFrameIdSurface = pThisSurface;
            }
        }

        pPrimarySurface = NULL;
        if ((pMaxFrameIdSurface != NULL) && (pMaxFrameIdSurface->FrameId > 0))
        {
            /*
             * Find used surface with smaller frameId to copy.
             */
            for (pListEntry = pListHead->Flink;
                pListEntry != pListHead;
                pListEntry = pListEntry->Flink)
            {
                pThisSurface = CONTAINING_RECORD(
                    pListEntry,
                    LJB_VMON_PRIMARY_SURFACE,
                    ListEntry
                    );
                /*
                 * hPrimarySurface must be matched to make sure both src and
                 * dst surfaces are same height and width. If not, BSOD during
                 * resolution switch.
                 */
                if ((pThisSurface->FrameId < pMaxFrameIdSurface->FrameId) &&
                    (pThisSurface->hPrimarySurface == pMaxFrameIdSurface->hPrimarySurface) &&
                    !pThisSurface->bIsAcquiredByUserMode &&
                    !pThisSurface->bBusyBltting)
                {
                    pPrimarySurface = pThisSurface;
                    break;
                }
            }

            if (pPrimarySurface != NULL)
            {
                InterlockedIncrement(&pPrimarySurface->ReferenceCount);
                InterlockedIncrement(&pMaxFrameIdSurface->ReferenceCount);
                pPrimarySurface->bIsAcquiredByUserMode = TRUE;
                pPrimarySurface->UserFileObject = UserFileObject;
            }
        }
        KeReleaseSpinLock(&pDevCtx->PrimarySurfaceListLock, OldIrql);

        if (pPrimarySurface != NULL)
        {
            LCI_USBAV_BLT_DATA  MyBltData;
            ULONG               BytesReturned;

            /*
             * For cursor update, don't update FrameId.
             */
            RtlZeroMemory(&MyBltData, sizeof(MyBltData));
            MyBltData.hPrimarySurface   = pMaxFrameIdSurface->hPrimarySurface;
            MyBltData.pPrimaryBuffer    = pMaxFrameIdSurface->pRemoteBuffer;
            MyBltData.pShadowBuffer     = pPrimarySurface->pBuffer;
            MyBltData.BufferSize        = pPrimarySurface->BufferSize;
            pPrimarySurface->bBusyBltting = TRUE;
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
                (__FUNCTION__ ": Cursor update: Blt from FrameId(%d) to "
                "FrameId(%d) done, count(%d).\n",
                pMaxFrameIdSurface->FrameId,
                pPrimarySurface->FrameId,
                pDevCtx->AcquirelistCount
                ));

            pPrimarySurface->bBusyBltting = FALSE;
            pPrimarySurface->bTransferDone = FALSE;
            InterlockedDecrement(&pMaxFrameIdSurface->ReferenceCount);
            bCursorUpdate = TRUE;
        }

        /*
         * If can't find available pPrimarySurface for cursor update,
         * complete immediately.
         */
        else
        {
            ntStatus = STATUS_UNSUCCESSFUL;
            WdfRequestComplete(WdfRequest, ntStatus);
            return TRUE;
        }
    }

    /*
     * Copy pixels from ProxyKMD to pPrimarySurface->pBuffer , and map the
     * pPrimarySurface->pBuffer to user space.
     */
    ASSERT(pPrimarySurface->pUserBuffer == NULL);
    pPrimarySurface->pUserBuffer = MmMapLockedPagesSpecifyCache(
        pPrimarySurface->pMdl,
        UserMode,
        MmCached,
        NULL,
        FALSE,
        NormalPagePriority
        );
    if (pPrimarySurface->pUserBuffer == NULL)
    {
        pPrimarySurface->bIsAcquiredByUserMode = FALSE;
        InterlockedDecrement(&pPrimarySurface->ReferenceCount);
        LJB_VMON_Printf(pDevCtx, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": No pUserBuffer mapped?\n"));
        ntStatus = STATUS_UNSUCCESSFUL;
        WdfRequestComplete(WdfRequest, ntStatus);
        return TRUE;
    }

    if (bCursorUpdate)
    {
        /*
         * Don't change pDevCtx->FrameIdSent for cursor update. It will alter
         * FrameIdSent saved in Pixel Service.
         */
        LJB_VMON_PrintfAlways(pDevCtx, DBGLVL_FLOW,
            (__FUNCTION__ ": Cursor update: FrameId(%d) acquired.\n",
            pPrimarySurface->FrameId
            ));
    }
    else
    {
        LJB_VMON_PrintfAlways(pDevCtx, DBGLVL_FLOW,
            (__FUNCTION__ ": FrameId(%d) acquired.\n",
            pPrimarySurface->FrameId
            ));

        pDevCtx->FrameIdSent = pPrimarySurface->FrameId;
    }

    /*
     * Save original background and then draw cursor.
     */
    pPrimarySurface->bIsCursorDrew  = FALSE;
    if (pDevCtx->PointerInfo.Visible)
    {
        LJB_VMON_DrawCursor(
            WdfDevice,
            pPrimarySurface
            );
        pPrimarySurface->bIsCursorDrew  = TRUE;
    }

    /*
     * Finally, fill in pFrameInfo and complete the IOCTL here with SUCCESS!
     */
    pFrameInfo = pOutputBuffer;
    RtlZeroMemory(pFrameInfo, sizeof(*pFrameInfo));
    pFrameInfo->KernelFrameHandle   = (ULONG64) ((ULONG_PTR) pPrimarySurface);
    pFrameInfo->UserFrameAddress    = (ULONG64) ((ULONG_PTR) pPrimarySurface->pUserBuffer);
    pFrameInfo->FrameSize           = (ULONG) pPrimarySurface->BufferSize;
    pFrameInfo->Width               = pPrimarySurface->Width;
    pFrameInfo->Height              = pPrimarySurface->Height;
    pFrameInfo->Pitch               = pPrimarySurface->Pitch;
    pFrameInfo->BytesPerPixel       = pPrimarySurface->BytesPerPixel;
    pFrameInfo->FrameIdAcquired     = pDevCtx->FrameIdSent;

    LJB_VMON_Printf(pDevCtx, DBGLVL_FLOW,
        (__FUNCTION__ ": "
        "pPrimarySurface->pBuffer(%p) mapped to pUserBuffer(%p)\n",
        pPrimarySurface->pBuffer,
        pPrimarySurface->pUserBuffer
        ));

    WdfRequestCompleteWithInformation(WdfRequest, STATUS_SUCCESS, sizeof(*pFrameInfo));
    return TRUE;
}

/*
 * Name:  LJB_VMON_IoctlReleaseFramePreProcess
 *
 * Definition:
 *    static BOOLEAN
 *    LJB_VMON_IoctlReleaseFramePreProcess(
 *        __in WDFDEVICE                  WdfDevice,
 *        __in WDFREQUEST                 WdfRequest,
 *        __in WDF_REQUEST_PARAMETERS     Params
 *        );
 *
 * Description:
 *    Unmap the user buffer previously mapped in LJB_VMON_IoctlAcquireFramePreProcess
 *    and decrement the reference count.
 *
 * Return Value:
 *    TRUE if the request has been completed by this routine (if an error occurs),
 *    FALSE if the request is successfully processed, and request further enqueue.
 *
 */
static BOOLEAN
LJB_VMON_IoctlReleaseFramePreProcess(
    __in WDFDEVICE                  WdfDevice,
    __in WDFREQUEST                 WdfRequest,
    __in WDF_REQUEST_PARAMETERS     Params
    )
{
    LJB_VMON_CTX * CONST            pDevCtx = LJB_VMON_GetVMonCtx(WdfDevice);
    LJB_VMON_FRAME_INFO *           pFrameInfo;
    LJB_VMON_PRIMARY_SURFACE *      pPrimarySurface;
    PVOID                           pInputBuffer;
    size_t                          InputBufferSize;
    LONG                            ReferenceCount;
    NTSTATUS                        ntStatus;
    LIST_ENTRY *                    pListHead;
    LIST_ENTRY *                    pListEntry;
    LJB_VMON_WAIT_FOR_UPDATE_REQ *  pWaitRequest;
    LJB_VMON_WAIT_FOR_UPDATE_REQ *  pThisRequest;
    WDFREQUEST                      ThisWdfRequest;
    WDF_REQUEST_PARAMETERS          ThisParams;
    KIRQL                           OldIrql;

    if (Params.Parameters.DeviceIoControl.InputBufferLength <
        sizeof(LJB_VMON_FRAME_INFO))
    {
        LJB_VMON_Printf(pDevCtx, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": "
            "InputBufferLength (0x%x) too small? Need %u bytes.\n",
            Params.Parameters.DeviceIoControl.InputBufferLength,
            sizeof(LJB_VMON_FRAME_INFO)
            ));
        ntStatus = STATUS_BUFFER_TOO_SMALL;
        WdfRequestComplete(WdfRequest, ntStatus);
        return TRUE;
    }

    ntStatus = WdfRequestRetrieveInputBuffer(
        WdfRequest,
        sizeof(LJB_VMON_FRAME_INFO),
        &pInputBuffer,
        &InputBufferSize
        );
    if(!NT_SUCCESS(ntStatus))
    {
        LJB_VMON_Printf(pDevCtx, DBGLVL_ERROR,
            ("?" __FUNCTION__
            ": unable to get InputBuffer?\n"
            ));
        WdfRequestComplete(WdfRequest, ntStatus);
        return TRUE;
    }

    pFrameInfo = pInputBuffer;
    pPrimarySurface = (LJB_VMON_PRIMARY_SURFACE *)
        ((ULONG_PTR) pFrameInfo->KernelFrameHandle);

    ASSERT(pFrameInfo->UserFrameAddress == (ULONG64)
           ((ULONG_PTR) pPrimarySurface->pUserBuffer));
    ASSERT(pPrimarySurface->bIsAcquiredByUserMode == TRUE);

    /*
     * Restore partial background.
     */
    if (pPrimarySurface->bIsCursorDrew)
    {
        UINT    row;
        UINT    EffCurWidth;
        UINT    EffCurHeight;
        UCHAR * pSurfBitmap;
        UCHAR * pOrigSurfPos;

        pOrigSurfPos = pPrimarySurface->pOrigSurfPosStart;
        pSurfBitmap = pPrimarySurface->SurfBitmap;
        EffCurWidth = pPrimarySurface->EffCurWidth;
        EffCurHeight = pPrimarySurface->EffCurHeight;

        for(row = 0; row < EffCurHeight; ++row)
        {
            RtlCopyMemory(
                pOrigSurfPos,
                pSurfBitmap,
                EffCurWidth * 4
                );

            pOrigSurfPos += pPrimarySurface->Width * 4;
            pSurfBitmap += EffCurWidth * 4;
        }
        pPrimarySurface->bIsCursorDrew = FALSE;
    }

    LJB_VMON_Printf(pDevCtx, DBGLVL_FLOW,
        (__FUNCTION__ ": "
        "pUserBuffer(%p) unmapped.\n",
        pPrimarySurface->pUserBuffer
        ));

    InterlockedDecrement(&pDevCtx->AcquirelistCount);

    LJB_VMON_PrintfAlways(pDevCtx, DBGLVL_ERROR,
        (__FUNCTION__ ": FrameId(%d) transfer done, remaining count(%d).\n",
        pPrimarySurface->FrameId,
        pDevCtx->AcquirelistCount
        ));

    MmUnmapLockedPages(pPrimarySurface->pUserBuffer, pPrimarySurface->pMdl);

    ReferenceCount = InterlockedDecrement(&pPrimarySurface->ReferenceCount);
    if (ReferenceCount == 0)
    {
        KIRQL   OldIrql;

        LJB_VMON_Printf(pDevCtx, DBGLVL_FLOW,
            (__FUNCTION__ ": "
            "Deferred destruction of pPrimarySurface(%p)/pMdl(%p)\n",
            pPrimarySurface,
            pPrimarySurface->pMdl
            ));

        KeAcquireSpinLock(&pDevCtx->PrimarySurfaceListLock, &OldIrql);
        RemoveEntryList(&pPrimarySurface->ListEntry);
        KeReleaseSpinLock(&pDevCtx->PrimarySurfaceListLock, OldIrql);
        IoFreeMdl(pPrimarySurface->pMdl);
        LJB_VMON_FreePool(pPrimarySurface->pBuffer);
        LJB_VMON_FreePool(pPrimarySurface);
        pPrimarySurface = NULL;
    }

    if (pPrimarySurface)
    {
        pPrimarySurface->pUserBuffer = NULL;
        pPrimarySurface->bIsAcquiredByUserMode  = FALSE;
        pPrimarySurface->bTransferDone          = TRUE;
    }

    WdfRequestComplete(WdfRequest, STATUS_SUCCESS);

    /*
     Check if there is pending AcquireFrameRequest
     */
    pListHead = &pDevCtx->WaitRequestListHead;
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
        ThisWdfRequest = pThisRequest->Request;

        WDF_REQUEST_PARAMETERS_INIT(&ThisParams);
        WdfRequestGetParameters(ThisWdfRequest, &ThisParams);

        if (ThisParams.Type != WdfRequestTypeDeviceControl)
            continue;
        if (ThisParams.Parameters.DeviceIoControl.IoControlCode !=
            IOCTL_LJB_VMON_ACQUIRE_FRAME)
            continue;

        RemoveEntryList(&pThisRequest->ListEntry);
        pWaitRequest = pThisRequest;
        break;
    }
    KeReleaseSpinLock(&pDevCtx->WaitRequestListLock, OldIrql);

    if (pWaitRequest != NULL)
    {
        WDF_REQUEST_PARAMETERS_INIT(&ThisParams);
        WdfRequestGetParameters(pWaitRequest->Request, &ThisParams);

        LJB_VMON_Printf(pDevCtx, DBGLVL_FLOW,
            (__FUNCTION__
            ": Re-Process previous pWaitRequest(%p)->Request(%p)\n",
            pWaitRequest,
            pWaitRequest->Request
            ));
        LJB_VMON_IoctlAcquireFramePreProcess(
            WdfDevice,
            pWaitRequest->Request,
            ThisParams
            );
        LJB_VMON_FreePool(pWaitRequest);
    }

    return TRUE;
}
