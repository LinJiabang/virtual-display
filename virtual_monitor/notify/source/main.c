/*!
    \file       main.c
    \brief      entry point of lci_usbav_test.exe
    \details    entry point of lci_usbav_test.exe
    \authors    lucaslin
    \version    0.01a
    \date       May 25, 2013
    \todo       (Optional)
    \bug        (Optional)
    \warning    (Optional)
    \copyright  (c) 2013 Luminon Core Incorporated. All Rights Reserved.

    Revision Log
    + 0.01a;    May 25, 2013;   lucaslin
     - Created.

 */

#include "lci_vmon.h"

DWORD
LCI_VMON_Main(
	__in LPVOID     lpThreadParameter
    )
    {
	PDEVICE_INFO 				pDeviceInfo;
    HANDLE CONST                hDefaultHeap = GetProcessHeap();
    LCI_VMON_DEV_CTX *         pDevCtx;
    BOOL                        bRet;
	
	pDeviceInfo = lpThreadParameter;
    pDevCtx = HeapAlloc(hDefaultHeap, HEAP_ZERO_MEMORY, sizeof(*pDevCtx));
    
	
	DBG_PRINT(("+" __FUNCTION__
                ": Start thread.\n"));
	
	if (pDevCtx == NULL)
        {
        printf("unable to get pDevCtx?\n");
        return 0;
        }
	/*
	 Check the existence of the device
	*/
    bRet = LCI_VMON_GetDeviceHandle(
			pDevCtx
			);
    
    if (bRet)
        {
		pDevCtx->pDeviceInfo = pDeviceInfo;
		pDeviceInfo->pDevCtx = pDevCtx;
		pDevCtx->bStartVMONThread = TRUE;
        bRet = LCI_VMON_PixelMain(pDevCtx);
        
		// stop
		LCI_VMON_CloseDeviceHandle(pDevCtx);
        }
    HeapFree(hDefaultHeap, 0, pDevCtx);
	
	DBG_PRINT(("-" __FUNCTION__
                ": End thread.\n"));
    return 1;
    }

    
