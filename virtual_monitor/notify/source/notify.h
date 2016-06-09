/*++
Copyright (c) 1990-2000    Microsoft Corporation All Rights Reserved

Module Name:

    notify.h

Abstract:


Author:

     Eliyas Yakub   Nov 23, 1999

Environment:


Revision History:


--*/

#ifndef __NOTIFY_H
#define __NOTIFY_H

/*
|| High performance counter.
*/
#define LCI_VMON_COUNT_FRAME_RATE


//
// Copied Macros from ntddk.h
//

#define CONTAINING_RECORD(address, type, field) ((type *)( \
                          (PCHAR)(address) - \
                          (ULONG_PTR)(&((type *)0)->field)))


#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))


#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

// Copy VMON stuff header.	
typedef struct _LCI_VMON_DEV_CTX      	LCI_VMON_DEV_CTX;
typedef struct _LCI_VMON_REQ_CTX      	LCI_VMON_REQ_CTX;
typedef struct _DEVICE_INFO      		DEVICE_INFO;

#define LCI_MAX_NUM_OF_BITMAP_CTX   4
#define LCI_MAX_NUM_OF_VIRTUAL_MEM	32

typedef VOID
LCI_VMON_GENERIC_DONE(
    __in LCI_VMON_DEV_CTX *    	pDevCtx,
    __in PVOID                 	pDoneCtx
    );
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
	
	DEVICE_INFO *						pDeviceInfo;
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


typedef enum {

    PLUGIN = 1,
    UNPLUG,
    EJECT

} USER_ACTION_TYPE;

typedef struct _DIALOG_RESULT
{
    ULONG    SerialNo;
    PWCHAR  DeviceId;
} DIALOG_RESULT, *PDIALOG_RESULT;

#define ID_EDIT 1

#define  IDM_OPEN       100
#define  IDM_CLOSE      101
#define  IDM_EXIT       102
#define  IDM_HIDE       103
#define  IDM_PLUGIN     104
#define  IDM_UNPLUG     105
#define  IDM_EJECT      106
#define  IDM_ENABLE     107
#define  IDM_DISABLE    108
#define  IDM_CLEAR      109
#define  IDM_IOCTL      110
#define  IDM_VERBOSE    111

#define IDD_DIALOG                     115
#define IDD_DIALOG1                    116
#define IDD_DIALOG2                    117
#define ID_OK                          118
#define ID_CANCEL                      119
#define IDC_SERIALNO                   1000
#define IDC_DEVICEID                   1001
#define IDC_STATIC                      -1

#define IDI_CLASS_ICON                 200

LRESULT FAR PASCAL
WndProc (
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

BOOLEAN EnumExistingDevices(
    HWND   hWnd
    );

BOOL HandleDeviceInterfaceChange(
    HWND hwnd,
    DWORD evtype,
    PDEV_BROADCAST_DEVICEINTERFACE dip
    );

BOOL HandleDeviceChange(
    HWND hwnd,
    DWORD evtype,
    PDEV_BROADCAST_HANDLE dhp
    );

LRESULT
HandleCommands(
    HWND     hWnd,
    UINT     uMsg,
    WPARAM   wParam,
    LPARAM   lParam
    );

BOOLEAN Cleanup(
    HWND hWnd
    );

BOOL
GetDeviceDescription(
    __in LPTSTR DevPath,
    __out_bcount_full(OutBufferLen) LPTSTR OutBuffer,
    __in ULONG OutBufferLen,
    __in PULONG SerialNo
    );

BOOLEAN
OpenBusInterface (
    __in ULONG SerialNo,
    __in_opt LPWSTR DeviceId,
    __in USER_ACTION_TYPE Action
    );


INT_PTR CALLBACK
DlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam);

void
SendIoctlToFilterDevice();

// VMON functions
DWORD
LCI_VMON_Main(
    __in LPVOID     lpThreadParameter
    );

VOID
DBG_PRINT(
    __in_z __drv_formatString(printf) LPWSTR format,
    ...
    );	

#endif


