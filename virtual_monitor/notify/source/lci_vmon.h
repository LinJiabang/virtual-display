/*!
    \file       lci_vmon.h
    \brief      virtual monitor structures definitions
    \details    This files defines the private data structures, function
                declarations that are only used within VMON module.
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

	+ 0.01a;	August 14, 2013;	viking
	- Add frame rate control and frame rate display by registry.

	 */

#ifndef _LCI_VMON_H_
#define _LCI_VMON_H_

#pragma warning(disable:4127)

#include <windows.h>
#include <winioctl.h>
#include <strsafe.h>
#include <setupapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <usb.h>

/*
|| High performance counter.
*/
#define LCI_VMON_COUNT_FRAME_RATE
#ifdef	LCI_VMON_COUNT_FRAME_RATE
#define	EXPECTED_TIME_PER_FRAME		30.0	/*in milliseconds.*/
#endif

#define	LCI_VMON_SERVICENAME	TEXT("LCI_SVC")
#define	LCI_VMON_REGISTRY_PATH	\
		"SYSTEM\\CurrentControlSet\\Services\\" ## LCI_VMON_SERVICENAME

/*
|| High resolution performance counter.
*/
#ifdef	LCI_VMON_COUNT_FRAME_RATE
#define LCI_VMON_TIMER_INIT						\
		LARGE_INTEGER	liFrequency;				\
		LARGE_INTEGER liCurrentTime;				\
		QueryPerformanceFrequency(&liFrequency);

#define LCI_VMON_TIMER_COUNT					\
		QueryPerformanceCounter(&liCurrentTime);
#endif
	
/*
 Forward declaration
 */
typedef struct _LCI_VMON_DEV_CTX      	LCI_VMON_DEV_CTX;
typedef struct _LCI_VMON_REQ_CTX      	LCI_VMON_REQ_CTX;

#define LCI_MAX_NUM_OF_BITMAP_CTX   4
#define LCI_MAX_NUM_OF_VIRTUAL_MEM	32

typedef DWORD
LCI_VMON_CLIENT_CREATE(
	__in LCI_VMON_DEV_CTX * 	pDevCtx,
	__out PVOID *				ppClientCtx
	);

typedef DWORD
LCI_VMON_CLIENT_DESTROY(
	__in LCI_VMON_DEV_CTX * 	pDevCtx,
	__in PVOID 					pClientCtx
	);

typedef VOID
LCI_VMON_SEND_REQ(
	__in PVOID					pClientCtx,
	__in LCI_VMON_REQ_CTX *		pRequest
	);

typedef VOID
LCI_VMON_GENERIC_DONE(
    __in LCI_VMON_DEV_CTX *    	pDevCtx,
    __in PVOID                 	pDoneCtx
    );
	
#define	LCI_VMON_CLIENT_IFC_VERSION1	1

typedef struct _LCI_VMON_CLIENT_IFC_ {
	ULONG						Version;
	LCI_VMON_CLIENT_CREATE *	pClientCreate;
	LCI_VMON_CLIENT_DESTROY *	pClientDestroy;
	LCI_VMON_SEND_REQ *			pSendReq;
	} LCI_VMON_CLIENT_IFC;

	
	
// From notify.h	
typedef struct _DEVICE_INFO
{
   HANDLE       hDevice; // file handle
   HDEVNOTIFY   hHandleNotification; // notification handle
   WCHAR        DeviceName[MAX_PATH];// friendly name of device description
   WCHAR        DevicePath[MAX_PATH];//
   ULONG        SerialNo; // Serial number of the device.
   LIST_ENTRY   ListEntry;
   HANDLE       VMONThread;
   ULONG        VMONThreadId;
   PVOID		BitmapBuffer;
   ULONG		Width;
   ULONG		Height;
   HWND        	hWndList;
   HWND			hParentWnd;
   LCI_VMON_GENERIC_DONE *        		pCompressionDoneFn;
   PVOID                           		pCompressionDoneCtx;   
   LCI_VMON_DEV_CTX *					pDevCtx;
} DEVICE_INFO, *PDEVICE_INFO;
	
/*
 Virtual monitor context area
 */
typedef struct _LCI_VMON_REQ_CTX
    {

    LIST_ENTRY                      	ListEntry;
    LCI_VMON_GENERIC_DONE *        		pCompressionDoneFn;
    PVOID                           	pCompressionDoneCtx;
    PVOID                           	pClientCtx; // FrameInfo
    DWORD                            	CompletionStatus;

    /*	
     The original Bitmap buffer	
     */	
    ULONG                           	BitmapSize;
    PVOID                           	BitmapBuffer;
    ULONG                           	Width;
    ULONG                           	Height;
	ULONG								BitsPerPixel;	/* 32 or 24 */

	ULONG             		            OutBufferSize;
    PVOID                   	        OutBuffer;
    ULONG                       	    ActualBufferSize;
	
    PVOID					            pBusRequest; //pAvRequest for maanshan project
    } LCI_VMON_REQ_CTX;

typedef struct _LCI_VMON_DEV_CTX
    {
    HANDLE                              hDevice;
    HDEVINFO                            HardwareDeviceInfo;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    pDevIfcDetailData;
    LCI_VMON_REQ_CTX                    BitmapReqPool[LCI_MAX_NUM_OF_BITMAP_CTX];
	LONG                        		PendingBitmapReqCount;
    LIST_ENTRY                  		FreeBitmapReqListHead;
    HANDLE                      		FreeBitmapReqListMutex;
    HANDLE                      		FreeBitmapReqListEvent;
    PVOID				               	pClientCompressionCtx;
	
	HANDLE                              VirtualMemMutex;
    ULONG                               VirtualMemFlag[LCI_MAX_NUM_OF_VIRTUAL_MEM];
    ULONG                               VirtualMemSize[LCI_MAX_NUM_OF_VIRTUAL_MEM];
    PVOID                               VirtualMemBuffer[LCI_MAX_NUM_OF_VIRTUAL_MEM];
	
	PDEVICE_INFO						pDeviceInfo;
	BOOL								bStartVMONThread;

#ifdef	LCI_VMON_COUNT_FRAME_RATE
	LONGLONG							llFrequency;
	LONGLONG							llCurrenTime;
	float								flElapsedTime;
	UINT								uFrameCount;
	UINT								uTempFrameCount;
	BOOL								blDisplayFrameRateNum;
	BOOL								blFrameRateControl;
#endif
    } LCI_VMON_DEV_CTX;	

typedef BOOL CLIENT_INIT_FN(
		__in LCI_VMON_DEV_CTX *	pDevCtx
		);
typedef CLIENT_INIT_FN *PCLIENT_INIT_FN;

typedef VOID CLIENT_DEINIT_FN(
		__in LCI_VMON_DEV_CTX *	pDevCtx
		);
typedef CLIENT_DEINIT_FN *PCLIENT_DEINIT_FN;

typedef BOOL CLIENT_ENQUEUE_BITMAP_REQ_FN(
		__in PVOID   				pClientCompressionCtx,
        __in LCI_VMON_REQ_CTX *  	pBitmapReq
		);
typedef CLIENT_ENQUEUE_BITMAP_REQ_FN *PCLIENT_ENQUEUE_BITMAP_REQ_FN;

/*
 Client switch
*/
typedef struct _CLIENT_SWITCH
	{
	CLIENT_INIT_FN	*					pInitFn;
	CLIENT_DEINIT_FN *					pDeInitFn;
	CLIENT_ENQUEUE_BITMAP_REQ_FN *		pEnqueueBitmapReqFn;
	} CLIENT_SWITCH;

#define	CLIENT_SWITCH_DECL_V1(			\
	pInitFn,							\
	pDeInitFn,							\
	pEnqueueBitmapReqFn					\
	) 									\
	CLIENT_INIT_FN						pInitFn; 		\
	CLIENT_DEINIT_FN					pDeInitFn;		\
	CLIENT_ENQUEUE_BITMAP_REQ_FN		pEnqueueBitmapReqFn;

#define	CLIENT_SWITCH_INIT_V1( 			\
	pInitFn,							\
	pDeInitFn,							\
	pEnqueueBitmapReqFn					\
	)									\
	{ 									\
	pInitFn,							\
	pDeInitFn,							\
	pEnqueueBitmapReqFn					\
	}

/*
 CLIENT_SWITCH_DEFINE_V1(LCI_MAANSHAN, gk_ClientSwitch);
*/
#define CLIENT_SWITCH_DEFINE_V1(pfx, switchname)		\
	CLIENT_SWITCH_DECL_V1( 				\
			pfx ## _CompressionInit,	\
			pfx ## _CompressionDeInit,	\
			pfx ## _StartBitmapReq		\
			)							\
	const CLIENT_SWITCH switchname =	\
		CLIENT_SWITCH_INIT_V1(			\
			pfx ## _CompressionInit,	\
			pfx ## _CompressionDeInit,	\
			pfx ## _StartBitmapReq		\
			)	

extern CONST CLIENT_SWITCH	gk_ClientSwitch;
			
/*
 Macro, inline functions borrowed from wdm.h
 */
FORCEINLINE
VOID
InitializeListHead(
    __out PLIST_ENTRY ListHead
    )
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

__checkReturn
BOOLEAN
FORCEINLINE
IsListEmpty(
    __in const LIST_ENTRY * ListHead
    )
{
    return (BOOLEAN)(ListHead->Flink == ListHead);
}

FORCEINLINE
BOOLEAN
RemoveEntryList(
    __in PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Flink;

    Flink = Entry->Flink;
    Blink = Entry->Blink;
    Blink->Flink = Flink;
    Flink->Blink = Blink;
    return (BOOLEAN)(Flink == Blink);
}

FORCEINLINE
PLIST_ENTRY
RemoveHeadList(
    __inout PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Flink;
    Flink = Entry->Flink;
    ListHead->Flink = Flink;
    Flink->Blink = ListHead;
    return Entry;
}



FORCEINLINE
PLIST_ENTRY
RemoveTailList(
    __inout PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Blink;
    Blink = Entry->Blink;
    ListHead->Blink = Blink;
    Blink->Flink = ListHead;
    return Entry;
}


FORCEINLINE
VOID
InsertTailList(
    __inout PLIST_ENTRY ListHead,
    __inout __drv_aliasesMem PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Blink;

    Blink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = Blink;
    Blink->Flink = Entry;
    ListHead->Blink = Entry;
}


FORCEINLINE
VOID
InsertHeadList(
    __inout PLIST_ENTRY ListHead,
    __inout __drv_aliasesMem PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Flink;

    Flink = ListHead->Flink;
    Entry->Flink = Flink;
    Entry->Blink = ListHead;
    Flink->Blink = Entry;
    ListHead->Flink = Entry;
}

FORCEINLINE
VOID
AppendTailList(
    __inout PLIST_ENTRY ListHead,
    __inout PLIST_ENTRY ListToAppend
    )
{
    PLIST_ENTRY ListEnd = ListHead->Blink;

    ListHead->Blink->Flink = ListToAppend;
    ListHead->Blink = ListToAppend->Blink;
    ListToAppend->Blink->Flink = ListHead;
    ListToAppend->Blink = ListEnd;
}




PVOID
LCI_VMON_AllocateVirtualMem(
    __in LCI_VMON_DEV_CTX *		pDevCtx,
    __in SIZE_T                 Size
    );

VOID
LCI_VMON_ReleaseVirtualMem(
    __in LCI_VMON_DEV_CTX *    	pDevCtx,
    __in PVOID                  pVirtualMem
    );	

__checkReturn
LCI_VMON_REQ_CTX *
LCI_VMON_AllocateBitmapReq(
    __in LCI_VMON_DEV_CTX *  	pDevCtx
    );

VOID
LCI_VMON_ReleaseBitmapReq(
    __in LCI_VMON_DEV_CTX *  	pDevCtx,
    __in LCI_VMON_REQ_CTX *		pBitmapReq
    );
	
__checkReturn
LCI_VMON_REQ_CTX *
LCI_VMON_AllocateRequest(
    __in LCI_VMON_DEV_CTX *   	pDevCtx
    );

VOID
LCI_VMON_ReleaseRequest(
    __in LCI_VMON_DEV_CTX *   	pDevCtx,
    __in LCI_VMON_REQ_CTX *   	pRequest
    );

VOID
LCI_VMON_CompleteBitmapReq(
    __in LCI_VMON_DEV_CTX *   	pDevCtx,
    __in LCI_VMON_REQ_CTX *   	pBitmapReq
    );

__checkReturn
BOOL
LCI_VMON_GetDeviceHandle(
    __in LCI_VMON_DEV_CTX *   	pDevCtx
    );

__checkReturn
VOID
LCI_VMON_CloseDeviceHandle(
    __in LCI_VMON_DEV_CTX *  	pDevCtx
    );	
    	
BOOL
LCI_VMON_PixelMain(
    __in LCI_VMON_DEV_CTX *   	pDevCtx
    );

BOOL
LCI_VMON_PixelMainTest(
    __in LCI_VMON_DEV_CTX *    	pDevCtx
    );

VOID
LCI_VMON_ServiceRegistryControl(
	__in LCI_VMON_DEV_CTX * pDevCtx
	);

VOID
LCI_VMON_DumpBuffer(
	__in UCHAR  *       		pBuf,
    __in ULONG          		BufSize
    );
	
VOID
__cdecl
LCI_VMON_DbgPrint(
    __in_z __drv_formatString(printf) PCSTR format,
    ...
    );

VOID
LCI_VMON_Req_Complete(
	__in LCI_VMON_DEV_CTX * 	pDevCtx,
    __in LCI_VMON_REQ_CTX * 	pRequest
    );

#define DBG_PRINT_ALWAYS(x)             LCI_VMON_DbgPrint x
#if (DBG)
#define DBG_PRINT(x)                    LCI_VMON_DbgPrint x
#define DUMP_BUF(buf, size)             LCI_VMON_DumpBuffer(buf, size);
#else
#define DBG_PRINT(x)
#define DUMP_BUF(buf, size)
#endif

#endif /* _LCI_VMON_H_ */