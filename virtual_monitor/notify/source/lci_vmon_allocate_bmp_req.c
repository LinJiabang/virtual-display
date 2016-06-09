/*!
    \file       lci_vmon_allocate_bmp_req.c
    \brief      LCI_VMON_AllocateBitmapReq() implementation
    \details    LCI_VMON_AllocateBitmapReq() implementation
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

 /*
 Name:  LCI_VMON_AllocateBitmapReq

 Definition:

 Description:

 Return Value:
    None

 */

__checkReturn
LCI_VMON_REQ_CTX *
LCI_VMON_AllocateBitmapReq(
    __in LCI_VMON_DEV_CTX *  pDevCtx
    )
    {
	LIST_ENTRY * CONST          pListHead = &pDevCtx->FreeBitmapReqListHead;
    LIST_ENTRY *                pListEntry;
    LCI_VMON_REQ_CTX *        	pBitmapReq;
    DWORD                       dwRetValue;

    pBitmapReq = NULL;
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
            return NULL;
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
    if (!ReleaseMutex(pDevCtx->FreeBitmapReqListMutex))
        {
        DBG_PRINT(("?" __FUNCTION__
            ": ReleaseMutx(FreeBitmapReqListMutex) failed?\n"));
        }
    if (pBitmapReq != NULL)
        {
        ZeroMemory(pBitmapReq, sizeof(*pBitmapReq));
		}
    return pBitmapReq;
    }