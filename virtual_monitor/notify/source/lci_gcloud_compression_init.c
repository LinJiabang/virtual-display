/*!
    \file       lci_gcloud_compression_init.c
    \brief      Initialize Compression Context
    \details    Initialize Compression Context
    \authors    lucaslin
    \version    0.01a
    \date       May 31, 2013
    \todo       (Optional)
    \bug        (Optional)
    \warning    (Optional)
    \copyright  (c) 2013 Luminon Core Incorporated. All Rights Reserved.

    Revision Log
    + 0.01a;    May 31, 2013;   lucaslin
     - Created.

 */


#include "lci_gcloud.h"
//#include "lci_usbav_private.h"

/*
 Create client switch
*/
CLIENT_SWITCH_DEFINE_V1(LCI_GCLOUD, gk_ClientSwitch);

/*
 Name:  LCI_GCLOUD_CompressionInit

 Definition:
    BOOL
    LCI_GCLOUD_CompressionInit(
        __in LCI_VMON_DEV_CTX *            pDevCtx
        );

 Description:
    1.  Allocate LCI_GCLOUD_CTX.
    2.  Create and initailize associated mutexes, lists.

 Return Value:
    TRUE if success, FALSE on failure.

 */

BOOL
LCI_GCLOUD_CompressionInit(
    __in LCI_VMON_DEV_CTX *            pDevCtx
    )
    {
    HANDLE CONST       			hDefaultHeap = GetProcessHeap();
    LCI_GCLOUD_CTX  *          	pGcloudCtx;
//	BOOL						bRet;
	
	pGcloudCtx = HeapAlloc(
        hDefaultHeap,
        HEAP_ZERO_MEMORY,
        sizeof(LCI_GCLOUD_CTX)
        );
    if (pGcloudCtx == NULL)
        {
        DBG_PRINT(("?" __FUNCTION__ ": no pGcloudCtx?\n"));
        return FALSE;
        }

#if 0
	bRet = LCI_USBAV_Create(
        pGcloudCtx,
		pDevCtx->hDevice
		);
	
	if (bRet == 0)
		{
        DBG_PRINT(("?" __FUNCTION__
            ": LCI_USBAV_Create failed."
            ));
		
		LCI_USBAV_Destroy(pGcloudCtx);
		HeapFree(hDefaultHeap, 0, pGcloudCtx);
		return FALSE;
		}
#endif
		
    pGcloudCtx->pDevCtx = pDevCtx;
	pDevCtx->pClientCompressionCtx = pGcloudCtx;
	
    InitializeListHead(&pGcloudCtx->CompressionReadyListHead);
    InitializeListHead(&pGcloudCtx->CompressionPendingListHead);
    
    pGcloudCtx->CurrentCompressionDoneEvent = CreateEvent(
        NULL,
        FALSE,   /* auto Reset event */
        FALSE,  /* initial state = non-signaled */
        NULL    /* no lpName */
        );
    if (pGcloudCtx->CurrentCompressionDoneEvent == NULL)
        {
        DBG_PRINT(("not able to allocate CurrentCompressionDoneEvent?\n"));
        HeapFree(hDefaultHeap, 0, pGcloudCtx);
        pGcloudCtx = NULL;
        return FALSE;
        }

    pGcloudCtx->CompressionReadyListMutex = CreateMutex(NULL, FALSE, NULL);
    if (pGcloudCtx->CompressionReadyListMutex == NULL)
        {
        DBG_PRINT(("?" __FUNCTION__ 
            ":not able to allocate CompressionReadyListMutex?\n"));
        CloseHandle(pGcloudCtx->CurrentCompressionDoneEvent);
        HeapFree(hDefaultHeap, 0, pGcloudCtx);
        pGcloudCtx = NULL;
        return FALSE;
        }

    pGcloudCtx->CompressionReadyListEvent = CreateEvent(
        NULL,
        FALSE,   /* auto Reset event */
        FALSE,  /* initial state = non-signaled */
        NULL    /* no lpName */
        );
    if (pGcloudCtx->CompressionReadyListEvent == NULL)
        {
        DBG_PRINT(("not able to create CompressionReadyListEvent?\n"));
        CloseHandle(pGcloudCtx->CurrentCompressionDoneEvent);
        CloseHandle(pGcloudCtx->CompressionReadyListMutex);
        HeapFree(hDefaultHeap, 0, pGcloudCtx);
        pGcloudCtx = NULL;
        return FALSE;
        }

    pGcloudCtx->CompressionPendingListMutex = CreateMutex(NULL, FALSE, NULL);
    if (pGcloudCtx->CompressionPendingListMutex == NULL)
        {
        DBG_PRINT(("not able to create CompressionReadyListEvent?\n"));
        CloseHandle(pGcloudCtx->CurrentCompressionDoneEvent);
        CloseHandle(pGcloudCtx->CompressionReadyListMutex);
        CloseHandle(pGcloudCtx->CompressionReadyListEvent);
        HeapFree(hDefaultHeap, 0, pGcloudCtx);
        pGcloudCtx = NULL;
        return FALSE;
        }

    /*
     create Compression thread.
     */
    pGcloudCtx->CompressionThread = CreateThread(
        NULL,
        0,      /* use default statck size */
        &LCI_GCLOUD_CompressionThread,
        pGcloudCtx,
        0,       /* the thread runs after completion */
        &pGcloudCtx->CompressionThreadId
        );
    if (pGcloudCtx->CompressionThread == NULL)
        {
        DBG_PRINT(("?" __FUNCTION__
            ": CreateThread failed, LastError(%u)\n", GetLastError()
            ));
        CloseHandle(pGcloudCtx->CurrentCompressionDoneEvent);
        CloseHandle(pGcloudCtx->CompressionReadyListMutex);
        CloseHandle(pGcloudCtx->CompressionReadyListEvent);
        CloseHandle(pGcloudCtx->CompressionPendingListMutex);
        HeapFree(hDefaultHeap, 0, pGcloudCtx);
        pGcloudCtx = NULL;
        return FALSE;
        }        
			
    DBG_PRINT((" " __FUNCTION__ ": succeeded.\n"));
    return TRUE;
    }

 /*
 Name:  LCI_GCLOUD_CompressionDeInit

 Definition:
    VOID
    LCI_GCLOUD_CompressionDeInit(
        __in LCI_VMON_DEV_CTX *            pDevCtx
        );

 Description:
    release all resources that were previously allocated in
    LCI_GCLOUD_CompressionInit

 Return Value:
   None

 */
VOID
LCI_GCLOUD_CompressionDeInit(
    __in LCI_VMON_DEV_CTX *            pDevCtx
    )
    {
    HANDLE CONST                hDefaultHeap = GetProcessHeap();
    LCI_GCLOUD_CTX * CONST pGcloudCtx = pDevCtx->pClientCompressionCtx;

    if (pGcloudCtx == NULL)
        return;

    /*
     tell the compression thread to die
     */
    pGcloudCtx->bTerminateCompressionThread = TRUE;
    SetEvent(pGcloudCtx->CompressionReadyListEvent);
    
    /*
     Wait for compression thread
     */
    (VOID) WaitForSingleObject(
        pGcloudCtx->CompressionThread,
        INFINITE
        );

    if (pGcloudCtx->CompressionReadyListMutex != NULL)
        {
        CloseHandle(pGcloudCtx->CompressionReadyListMutex);
        pGcloudCtx->CompressionReadyListMutex = NULL;
        }
    if (pGcloudCtx->CompressionReadyListEvent != NULL)
        {
        CloseHandle(pGcloudCtx->CompressionReadyListEvent);
        pGcloudCtx->CompressionReadyListEvent = NULL;
        }
    if (pGcloudCtx->CompressionPendingListMutex != NULL)
        {
        CloseHandle(pGcloudCtx->CompressionPendingListMutex);
        pGcloudCtx->CompressionPendingListMutex = NULL;
        }

#if 0		
	/*
	 Clean up USBAV module.
	*/
	LCI_USBAV_Destroy(pGcloudCtx);
#endif

    HeapFree(hDefaultHeap, 0, pGcloudCtx);
    pDevCtx->pClientCompressionCtx = NULL;
    }