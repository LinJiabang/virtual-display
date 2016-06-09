/*!
    \file       lci_usbav_release_bmp_req.c
    \brief      LCI_VMON_ReleaseBitmapReq() implementation
    \details    LCI_VMON_ReleaseBitmapReq() implementation
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

#include "lci_vmon.h"
//#include "lci_maanshan.h"
//#include "lci_usbav_private.h"


 /*
 Name:  LCI_VMON_ReleaseBitmapReq

 Definition:

 Description:
	Release pBusRequest first and then pBitmapReq.

 Return Value:
    None

 */

VOID
LCI_VMON_ReleaseBitmapReq(
	__in LCI_VMON_DEV_CTX *  pDevCtx,
    __in LCI_VMON_REQ_CTX *	 pBitmapReq
    )
    {
    LIST_ENTRY * CONST  pListHead = &pDevCtx->FreeBitmapReqListHead;
    LIST_ENTRY * CONST  pListEntry = &pBitmapReq->ListEntry;
    DWORD               dwRetValue;

    dwRetValue = WaitForSingleObject(
        pDevCtx->FreeBitmapReqListMutex,
        INFINITE
        );
    if (dwRetValue != WAIT_OBJECT_0)
            {
            /*
             something bad happens?
             */
            DBG_PRINT(("?" __FUNCTION__ ":WaitForSingleObject"
                "(FreeBitmapReqListMutex) return 0x%x?\n",
                dwRetValue
                ));
            }
    InsertTailList(pListHead, pListEntry);
    if (!ReleaseMutex(pDevCtx->FreeBitmapReqListMutex))
        {
        DBG_PRINT(( "?" __FUNCTION__
            ": ReleaseMutx(FreeBitmapReqListMutex) failed?\n"));
        }
    }