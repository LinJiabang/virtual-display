/*!
    \file       lci_gcloud_compression_thread.c
    \brief      gcloud specific
    \details    gcloud specific
    \authors    lucaslin
    \version    0.01a
    \date       July 22, 2013
    \todo       (Optional)
    \bug        (Optional)
    \warning    (Optional)
    \copyright  (c) 2013 Luminon Core Incorporated. All Rights Reserved.

    Revision Log
    + 0.01a;    July 22, 2013;   lucaslin
     - Created.

 */

#include "lci_gcloud.h"
//#include "lci_usbav_ioctl.h"
//#include "lci_usbav_private.h"

//static MSTAR_WRAPPER_JPE_CALLBACK   LCI_GCLOUD_CompressionCallback;
//#define BITS_PER_PIXEL_TO_FORMAT(bpp)	((bpp == 24) ? JPE_INPUT_FORMAT_BGR888 : JPE_INPUT_FORMAT_BGRA8888)


/*
 Name:  LCI_GCLOUD_CompressionCallback

 Definition:
    MSTAR_WRAPPER_JPE_CALLBACK        LCI_GCLOUD_CompressionCallback;

 Description:
    This routine is triggered from MSTAR internal thread. If IsLast is NON-ZERO,
    we are safe to commit the compressed buffer to USB AV device.

 Return Value:
    None.

 */
static
VOID
__cdecl
LCI_GCLOUD_CompressionCallback(
    __in UCHAR *    pOutputBuffer,
    __in ULONG      nOffset,
    __in ULONG      nSize,
    __in ULONG      IsLast,
    __in PVOID      pCtx
    )
    {
	LCI_GCLOUD_CTX * CONST	pGcloudCtx = pCtx;
	LCI_VMON_DEV_CTX * CONST    pDevCtx = pGcloudCtx->pDevCtx;
	LCI_VMON_REQ_CTX * CONST	pBitmapReq = pGcloudCtx->pCurrentBitMapCtx;
	PDEVICE_INFO 		pDeviceInfo;

	pDeviceInfo = pDevCtx->pDeviceInfo;

    DBG_PRINT((" " __FUNCTION__ ":\n"
        "\t pOutputBuffer(%p), nOffset(0x%x), nSize(0x%x), IsLast(%u),"
        " pBitmapReq(%p)\n",
        pOutputBuffer,
        nOffset,
        nSize,
        IsLast,
        pBitmapReq
        ));

		
	
	if (IsLast)
		{
	//	HDC hdc;
		/*
		Remove pBitmapReq from CompressionPendingList
		Since we are enforcing synchronization in such a way that after the
		bitmap is schedule for compression, we won't schedule another bitmap
		unless the previous compression task is completed. Therefore, we don't
		really to guard against concurrent access to
		CompressionPendingListHead.
		*/
		RemoveEntryList(&pBitmapReq->ListEntry);

#if 0	// release in WM_PAINT.
		if (pBitmapReq->pCompressionDoneFn != NULL)
			{
			/*
			 Release FrameBuffer
			*/
			(*pBitmapReq->pCompressionDoneFn)(
				pDevCtx,
				pBitmapReq->pCompressionDoneCtx
				);
			}
#endif 		
		/*
		 Assign to notify structure.
		*/
		pDeviceInfo->BitmapBuffer = pBitmapReq->BitmapBuffer;
		pDeviceInfo->Width = pBitmapReq->Width;
		pDeviceInfo->Height = pBitmapReq->Height;
		pDeviceInfo->pCompressionDoneFn = pBitmapReq->pCompressionDoneFn;
		pDeviceInfo->pCompressionDoneCtx = pBitmapReq->pCompressionDoneCtx;
		pDeviceInfo->pDevCtx = pDevCtx;
		//output now.
		SendMessage(pDeviceInfo->hParentWnd, WM_PAINT, 333, 0);
	
#ifdef LCI_VMON_COUNT_FRAME_RATE
		{
		LARGE_INTEGER liCurrentTime;
		float fTimeDiff = 0.0;

		QueryPerformanceCounter(&liCurrentTime);

		if (pDevCtx->llFrequency != 0.0)
			{
			pDevCtx->uTempFrameCount++;
			fTimeDiff = (float)
				(liCurrentTime.QuadPart - pDevCtx->llCurrenTime)/pDevCtx->llFrequency;
			pDevCtx->flElapsedTime += fTimeDiff;

			if (pDevCtx->blFrameRateControl == TRUE)
				{
				if (fTimeDiff < EXPECTED_TIME_PER_FRAME)
					{
					DBG_PRINT(("" __FUNCTION__ ":"
						" Sleep %d ms.\n",
						(int)(EXPECTED_TIME_PER_FRAME - fTimeDiff)));
					Sleep((DWORD)(EXPECTED_TIME_PER_FRAME - fTimeDiff));
					}
				}

			pDevCtx->llCurrenTime = liCurrentTime.QuadPart;
			if (pDevCtx->flElapsedTime >= 3.0)
				{
				pDevCtx->uFrameCount =
						(UINT) (pDevCtx->uTempFrameCount/pDevCtx->flElapsedTime);
				DBG_PRINT_ALWAYS((" %d fps\n", pDevCtx->uFrameCount));
				pDevCtx->uTempFrameCount = 0;
				pDevCtx->flElapsedTime = 0.0;
				}
			}
		else
			{
			DBG_PRINT(("?" __FUNCTION__ ":"
				" Performance counter error, frequency is 0.\n"));
			}
		}
#endif
	
	
	
	
		
		
#if 0
		LCI_USBAV_OutputFrame(
			pOutputBuffer,
			nOffset,
			nSize,
			pGcloudCtx,
			pBitmapReq
			);
#endif

		//	(VOID) SetEvent(pGcloudCtx->CurrentCompressionDoneEvent);
		}
    }


/*
 Name:  LCI_GCLOUD_CompressionThread

 Definition:
    PTHREAD_START_ROUTINE        LCI_GCLOUD_CompressionThread;

 Description:
    Create an initalize JPE engine related context. Then loop for waiting
    incoming compression tasks, and kick off compression.

 Return Value:
    TRUE if thread ended with success, FALSE on error.

 */

DWORD
LCI_GCLOUD_CompressionThread(
    __in LPVOID     lpThreadParameter
    )
    {
    LCI_GCLOUD_CTX * CONST    		pGcloudCtx = lpThreadParameter;   
    LCI_VMON_REQ_CTX *           	pBitmapReq;
    LIST_ENTRY *                    pListHead;
    LIST_ENTRY *                    pListEntry;
    DWORD                           dwRetValue;

#if 0	
	PVOID                           pJpeCtx;
	JpeInput_t                      JpeInput;
	JpeRECT_t                       Rect;
	PVOID				 		   	pAvRequest;

	pAvRequest = NULL;
	pJpeCtx = NULL;
    if (MSTAR_WRAPPER_JpeCreate(&pJpeCtx) != JPE_RESULT_OK)
        {
        DBG_PRINT(("?" __FUNCTION__
            ": JpeCreate failed?\n"
            ));
        return FALSE;
        }
    if (pJpeCtx == NULL)
        {
        DBG_PRINT(( "?" __FUNCTION__
            ": JpeCreate succeeded, but returned bad pJpeCtx?\n"
            ));
        return FALSE;
        }

    if (MSTAR_WRAPPER_JpeRegisterCallback(
            pJpeCtx, &LCI_GCLOUD_CompressionCallback, pGcloudCtx
        ) != JPE_RESULT_OK)
        {
        DBG_PRINT(( "?" __FUNCTION__ ": JpeRegisterCallback failed ?\n"));
        (VOID) MSTAR_WRAPPER_JpeDestroy(pJpeCtx);
        return FALSE;
        }
    pGcloudCtx->pJpeCtx = pJpeCtx;
#endif


	dwRetValue = FALSE;
    while (TRUE)
        {
        BOOLEAN         bCompressionReadyListEmpty;

        /*
         Check for pending list first. If we found no item to be processed,
         then we wait for CompressionReadyListEvent
         */
        (VOID) WaitForSingleObject(
            pGcloudCtx->CompressionReadyListMutex,
            INFINITE
            );
        bCompressionReadyListEmpty = IsListEmpty(
            &pGcloudCtx->CompressionReadyListHead
            );
        ReleaseMutex(pGcloudCtx->CompressionReadyListMutex);

        if (bCompressionReadyListEmpty)
            {
            DBG_PRINT((" " __FUNCTION__ ": "
            "Waiting for CompressionReadyListEvent.\n"));
            dwRetValue = WaitForSingleObject(
                pGcloudCtx->CompressionReadyListEvent,
                INFINITE
                );
            if (dwRetValue != WAIT_OBJECT_0)
                {
                DBG_PRINT(("?" __FUNCTION__
                    ": WaitForSingleObject return 0x%x?\n",
                    dwRetValue
                    ));
                dwRetValue = FALSE;
                break;
                }
            }

        /*
         Check if we are awake for suicide
         */
        if (pGcloudCtx->bTerminateCompressionThread)
            {
            DBG_PRINT(( " " __FUNCTION__
                ": somebody wake me up for suicide!\n"
                ));
            dwRetValue = TRUE;
            break;
            }

        /*
         Ok, there must be Compression task to be processed. Check out
         the latest BitmapReq
         */
        pBitmapReq = NULL;
        pListHead = &pGcloudCtx->CompressionReadyListHead;
        dwRetValue = WaitForSingleObject(
            pGcloudCtx->CompressionReadyListMutex,
            INFINITE
            );
        if (dwRetValue != WAIT_OBJECT_0)
            {
            /*
             something bad happens?
             */
            DBG_PRINT(("?" __FUNCTION__ ":WaitForSingleObject"
                "(CompressionReadyListMutex) return 0x%x?\n",
                dwRetValue
                ));
            dwRetValue = FALSE;
            break;
            }
        if (!IsListEmpty(pListHead))
            {
            pListEntry = RemoveHeadList(pListHead);
            pBitmapReq = CONTAINING_RECORD(
                pListEntry,
                LCI_VMON_REQ_CTX,
                ListEntry
                );
            }
        if (!ReleaseMutex(pGcloudCtx->CompressionReadyListMutex))
            {
            DBG_PRINT(( "?" __FUNCTION__
                ": ReleaseMutx(CompressionReadyListMutx) failed?\n"));
            dwRetValue = FALSE;
            break;
            }

        /*
         we were waken up, but we have nothing to process!
         */
        if (pBitmapReq == NULL)
            {
            DBG_PRINT((" " __FUNCTION__ 
                ": no pBitmapReq in queue, continue!\n"));
            continue;
            }
		
        /*
         now we have a BitmapReq, check if it is valid. If not valid, spit out
         the error, and free it to free list
         */
        DBG_PRINT((" " __FUNCTION__ ":\n"
            "scheduling pBitmapReq(%p) BitMapBuffer(%p), size(0x%x), "
            "Width(%u), Height(%u), BitsPerPixel(0x%x)\n",
            pBitmapReq,
            pBitmapReq->BitmapBuffer,
            pBitmapReq->BitmapSize,
            pBitmapReq->Width,
            pBitmapReq->Height,
            pBitmapReq->BitsPerPixel
            ));
        if (pBitmapReq->BitmapSize == 0 ||
            pBitmapReq->BitmapBuffer == NULL ||
            pBitmapReq->Width == 0
            )
            {
            DBG_PRINT(( "?" __FUNCTION__
                "? invalid bitmap parameters?\n"
                ));
            LCI_VMON_Req_Complete(
				pGcloudCtx->pDevCtx,
                pBitmapReq
                );
            /*
             skip this bitmap ctx, and continue next
             */
            continue;
            }

        /*
         Do not call JpeInit multiple times. JpeInit shall be paired with
         JpeCreate/JpeDestroy
         */
        if (!pGcloudCtx->LastInputConfigured)
            {
#if 0			
			JpeInput.nWidth = (u16)pBitmapReq->Width;
            JpeInput.nHeight = (u16)pBitmapReq->Height;
            JpeInput.nUsbHeaderSize = USB_AV_HEADER_SIZE;
            JpeInput.eFormat = BITS_PER_PIXEL_TO_FORMAT(pBitmapReq->BitsPerPixel);
            (VOID) MSTAR_WRAPPER_JpeInit(
                pJpeCtx,
                JpeInput
                );
#endif				
				
            pGcloudCtx->LastInputConfigured = TRUE;
            pGcloudCtx->LastInputWidth = (USHORT)pBitmapReq->Width;
            pGcloudCtx->LastInputHeight = (USHORT)pBitmapReq->Height;
            }
        else
            {
            /*
             Check if input frame size changed from last configuration
             */
            if (pGcloudCtx->LastInputWidth != pBitmapReq->Width ||
                pGcloudCtx->LastInputHeight != pBitmapReq->Height)
                {
                /*
                 Shall we wait for all pending compression tasks done???
                 In current implementation, this thread wait for previous
                 pending compression before schedule next compression tasks.
                 If the future compression library support parallel compression,
                 we probably need to do some kind of synchronization.
                 */
                DBG_PRINT(("?" __FUNCTION__
                    ": size changed LastInputWidth(%u), LastInputHeight(%u)"
                    ", this Width(%u) , this Height(%u)\n",
                    pGcloudCtx->LastInputWidth,
                    pGcloudCtx->LastInputHeight,
                    pBitmapReq->Width,
                    pBitmapReq->Height
                    ));

#if 0					
                (VOID) MSTAR_WRAPPER_JpeDestroy(pGcloudCtx->pJpeCtx);
                pGcloudCtx->pJpeCtx = NULL;
                if (MSTAR_WRAPPER_JpeCreate(&pJpeCtx) != JPE_RESULT_OK)
                    {
                    DBG_PRINT(("?" __FUNCTION__
                        ": JpeCreate failed?\n"
                        ));
                    return FALSE;
                    }
                pGcloudCtx->pJpeCtx = pJpeCtx;

                if (MSTAR_WRAPPER_JpeRegisterCallback(
                        pJpeCtx,
                        &LCI_GCLOUD_CompressionCallback,
                        pGcloudCtx
                        ) != JPE_RESULT_OK)
                    {
                    DBG_PRINT(( "?" __FUNCTION__ ": JpeRegisterCallback failed ?\n"));
                    (VOID) MSTAR_WRAPPER_JpeDestroy(pJpeCtx);
                    return FALSE;
                    }

                JpeInput.nWidth = (u16)pBitmapReq->Width;
                JpeInput.nHeight = (u16)pBitmapReq->Height;
                JpeInput.nUsbHeaderSize = USB_AV_HEADER_SIZE;
                JpeInput.eFormat = BITS_PER_PIXEL_TO_FORMAT(pBitmapReq->BitsPerPixel);
                (VOID) MSTAR_WRAPPER_JpeInit(
                    pJpeCtx,
                    JpeInput
                    );
#endif					
                pGcloudCtx->LastInputConfigured = TRUE;
                pGcloudCtx->LastInputWidth = (USHORT)pBitmapReq->Width;
                pGcloudCtx->LastInputHeight = (USHORT)pBitmapReq->Height;
                }
            }
	
        /*
         move pBitmapReq to CompressionPendingList.
         Since we are enforcing synchronization in such a way that after the
         bitmap is schedule for compression, we won't schedule another bitmap
         unless the previous compression task is completed. Therefore, we don't
         really to guard against concurrent access to
         CompressionPendingListHead.
         */
        pListHead = &pGcloudCtx->CompressionPendingListHead;
        InsertTailList(pListHead, &pBitmapReq->ListEntry);

#if 0
        /*
         finally, we are ready to kick off encoding engine
         */
		Rect.X = 0;
        Rect.Y = 0;
        Rect.W = (u16)pBitmapReq->Width;
        Rect.H = (u16)pBitmapReq->Height;
        Rect.pNext = NULL;
#endif
		
        pGcloudCtx->pCurrentBitMapCtx = pBitmapReq;
        (VOID) ResetEvent(pGcloudCtx->CurrentCompressionDoneEvent);


#if 0		
        if (MSTAR_WRAPPER_JpeEncodeOneFrame(
                pJpeCtx,
                pBitmapReq->BitmapBuffer,
                pBitmapReq->OutBuffer,
                &Rect
                ) != JPE_RESULT_OK)
            {
            DBG_PRINT(("?" __FUNCTION__
                ": JpeEncodeOneFrame failed?\n"
                ));
            /* remove pBitmapReq from CompressionPendingList */
            RemoveEntryList(&pBitmapReq->ListEntry);
            LCI_VMON_Req_Complete(
                pGcloudCtx->pDevCtx,
                pBitmapReq
                );
            /*
             skip this bitmap ctx, and continue next
             */
            continue;
            }			
			
        /*
         Wait for current compression completion!
         */
        DBG_PRINT((" " __FUNCTION__
            ": wait for pBitmapReq(%p) compression.\n",
            pBitmapReq
            ));
        dwRetValue = WaitForSingleObject(
            pGcloudCtx->CurrentCompressionDoneEvent,
            INFINITE
            );
        if (dwRetValue != WAIT_OBJECT_0)
            {
            /*
             something bad happens?
             */
            DBG_PRINT(("?" __FUNCTION__ ":WaitForSingleObject"
                "(CurrentCompressionDoneEvent) return 0x%x?\n",
                dwRetValue
                ));
            dwRetValue = FALSE;
            break;
            }
        DBG_PRINT((" " __FUNCTION__ 
            ": pBitmapReq(%p) compression done!\n",
            pBitmapReq
            ));

        (VOID) MSTAR_WRAPPER_JpeFreeBuffer(pGcloudCtx->pJpeCtx);
#endif
		
		// Gcloud has no compression. Call callback() directly.
		
		LCI_GCLOUD_CompressionCallback(
			pBitmapReq->BitmapBuffer,					//__in UCHAR *    pOutputBuffer,
			0,											//__in ULONG      nOffset,
			pBitmapReq->Width * pBitmapReq->Height *4,	//__in ULONG      nSize,
			1,											//__in ULONG      IsLast,
			pGcloudCtx									//__in PVOID      pCtx
			);
        } //end while()
		
#if 0
    if (pGcloudCtx->pJpeCtx != NULL)
        {
        /*
         Wait for all pending IO requests
         */
        LCI_USBAV_WaitForAllBitmapCompletion(pGcloudCtx);
		
		
        (VOID) MSTAR_WRAPPER_JpeDestroy(pGcloudCtx->pJpeCtx);
        pGcloudCtx->pJpeCtx = NULL;
        }
#endif		
    DBG_PRINT((" " __FUNCTION__ ": terminated\n"));
    return dwRetValue;
    }


/*
 Name:  LCI_GCLOUD_StartBitmapReq

 Definition:
    BOOL
    LCI_GCLOUD_StartBitmapReq(
        __in PVOID   				pClientCompressionCtx,
        __in LCI_VMON_REQ_CTX *  	pBitmapReq
        );

 Description:
    Enqueue the pBitmapReq into pGcloudCtx->CompressionReadyListHead
    and signal pGcloudCtx->CompressionReadyListEvent.

 Return Value:
    TRUE if thread ended with success, FALSE on error.

 */

BOOL
LCI_GCLOUD_StartBitmapReq(
    __in PVOID   				pClientCompressionCtx,
    __in LCI_VMON_REQ_CTX *		pBitmapReq
    )
    {
	LCI_GCLOUD_CTX * CONST pGcloudCtx = (LCI_GCLOUD_CTX *)pClientCompressionCtx;
    LIST_ENTRY * CONST  pListHead = &pGcloudCtx->CompressionReadyListHead;
    DWORD               dwRetValue;
    
    dwRetValue = WaitForSingleObject(
        pGcloudCtx->CompressionReadyListMutex,
        INFINITE
        );
    if (dwRetValue != WAIT_OBJECT_0)
        {
        /*
         something bad happens?
         */
        DBG_PRINT(("?" __FUNCTION__ ":WaitForSingleObject"
            "(CompressionReadyListMutex) return 0x%x?\n",
            dwRetValue
            ));
        dwRetValue = FALSE;
        return FALSE;
        }
    InsertTailList(pListHead, &pBitmapReq->ListEntry);

    if (!ReleaseMutex(pGcloudCtx->CompressionReadyListMutex))
        {
        DBG_PRINT(( "?" __FUNCTION__
            ": ReleaseMutx(CompressionReadyListMutx) failed?\n"));
        return FALSE;
        }

    (VOID) SetEvent(pGcloudCtx->CompressionReadyListEvent);
    
    return TRUE;
    }	
