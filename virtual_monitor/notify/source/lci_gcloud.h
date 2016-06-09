/*!
    \file       lci_gcloud.h
    \brief      gcloud output device specific
    \details    virtual monitor output device specific
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

#ifndef _LCI_GCLOUD_H_
#define _LCI_GCLOUD_H_

#include "lci_vmon.h"

/*
 Function declaration
*/ 

BOOL
LCI_GCLOUD_StartBitmapReq(
    __in PVOID 					pClientCompressionCtx,
    __in LCI_VMON_REQ_CTX *   	pBitmapReq
    );

BOOL
LCI_GCLOUD_CompressionInit(
    __in LCI_VMON_DEV_CTX *            pDevCtx
    );

VOID
LCI_GCLOUD_CompressionDeInit(
    __in LCI_VMON_DEV_CTX *            pDevCtx
    );
	
DWORD
LCI_GCLOUD_CompressionThread(
    __in LPVOID                         lpThreadParameter
    );
	
/*
 This structure is allocated in call to LCI_VMON_CLIENT_CREATE.
*/
typedef struct _LCI_GCLOUD_CTX
    {
	LCI_VMON_DEV_CTX *			pDevCtx;
	PVOID 	        			pBusCtx; // can be differnt transport ctx. Ex. ethernet
    HANDLE                      CurrentCompressionDoneEvent;
    LCI_VMON_REQ_CTX *        	pCurrentBitMapCtx;
 //   PVOID                       pJpeCtx; use as a flag before terminating the thread, need to check further.
    USHORT                      LastInputWidth;
    USHORT                      LastInputHeight;
    BOOLEAN                     LastInputConfigured;


    /*
     before a pBitmapReq is to be compressed, the grabber thread put the
     pBitmapReq into CompressionReadyListHead
     */
    LIST_ENTRY                  CompressionReadyListHead;
    HANDLE                      CompressionReadyListMutex;
    HANDLE                      CompressionReadyListEvent;

    /*
     The compressor thread upon receiving a BitmapReq, grabs a BitmapReq from
     CompressionReadyList and puts it to CompressionPendingList before
     submitting for compression via MSTAR_WRAPPER_JpeEncodeOneFrame
     */
    LIST_ENTRY                  CompressionPendingListHead;
    HANDLE                      CompressionPendingListMutex;

    /*
     After the BitmapReq is compression-done, the MSTAR_WRAPPER_JPE_CALLBACK
     takes out the compression-completed BitmapReq from
     CompressionPendingListHead, and put it to AvIoPendingList. The USBAV IO is
     triggered via DeviceIoControl(), and there is no callback mechanism
     provided. As a result, there is a "garbage-collection" thread that
     monitors the completed IOCTL. Once an IOCTL is completed, the completion
     routine is triggered to remove the BitmapReq from AvIoPendingList and
     return it to FreeBitMapCtxList.
     */

    HANDLE                      CompressionThread;
    ULONG                       CompressionThreadId;

    BOOLEAN                     bTerminateCompressionThread;
    } LCI_GCLOUD_CTX;


	
#endif /* _LCI_GCLOUD_H_ */