/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name: notify.c


Abstract:


Author:

     Eliyas Yakub   Nov 23, 1999

Environment:

    User mode only.

Revision History:

    Modified to use linked list for deviceInfo
    instead of arrays. (5/12/2000)

--*/
#define UNICODE
#define _UNICODE

//#define INITGUID

//
// Annotation to indicate to prefast that this is nondriver user-mode code.
//
#include <DriverSpecs.h>
__user_code

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <setupapi.h>
#include <dbt.h>
#include <winioctl.h>
#include <strsafe.h>
#include "public.h"
#include "notify.h"
#include "ljb_vmon.h"
#include <dontuse.h>

static PVOID g_pDebugHandler = NULL;
LONG
LJB_VMON_VectorHandler(
    struct _EXCEPTION_POINTERS *pExceptionInfo
    );

BOOL
HandlePowerBroadcast(
    HWND hWnd,
    WPARAM wParam,
    LPARAM lParam);

//
// Global variables
//
HINSTANCE   hInst;
HWND        hWndList;
TCHAR       szTitle[]=TEXT("Virtual Monitor Test Application");
LIST_ENTRY  ListHead;
HDEVNOTIFY  hInterfaceNotification;
UINT        ListBoxIndex = 0;
GUID        InterfaceGuid;// = LJB_MONITOR_INTERFACE_GUID;
BOOLEAN     Verbose= FALSE;
PDEVICE_INFO   gDeviceInfo = NULL;


_inline BOOLEAN
IsValid(
    ULONG No
    )
{
    PLIST_ENTRY thisEntry;
    PDEVICE_INFO deviceInfo;

    if(0==(No)) return TRUE; //special case

    for(thisEntry = ListHead.Flink; thisEntry != &ListHead;
                        thisEntry = thisEntry->Flink)
    {
            deviceInfo = CONTAINING_RECORD(thisEntry, DEVICE_INFO, ListEntry);
            if((No) == deviceInfo->SerialNum) {
                return TRUE;
        }
    }
    return FALSE;
}

void makebmp(BYTE* pBits, long width, long height, HDC hdc, HWND hWnd)
{
    int                 iRet;
    RECT                rect;
    BOOL                Status;
    BITMAPINFO          BitmapInfo;
    BITMAPINFO * CONST  pBMI = &BitmapInfo;

    memset(pBMI, 0, sizeof(BITMAPINFO));
    pBMI->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

    pBMI->bmiHeader.biWidth = width;
    pBMI->bmiHeader.biHeight = -height;  // negative means top down!!!!
    pBMI->bmiHeader.biPlanes = 1;
    pBMI->bmiHeader.biBitCount = 32;
    pBMI->bmiHeader.biCompression = BI_RGB;//BI_BITFIELDS;//BI_RGB;
    pBMI->bmiHeader.biSizeImage = width*height*4;
    pBMI->bmiHeader.biXPelsPerMeter = 0;
    pBMI->bmiHeader.biYPelsPerMeter = 0;
    pBMI->bmiHeader.biClrUsed = 0;
    pBMI->bmiHeader.biClrImportant = 0;

    //memcpy(pBMI+ sizeof(BITMAPINFOHEADER), bmiColors, sizeof(RGBQUAD)*3);
    Status = GetWindowRect(
        hWnd,
        &rect
        );

    if (Status == 0)
    {
        DBG_PRINT(("?"__FUNCTION__":Failed to get rect?\n"));
    }
    else
    {
        iRet = StretchDIBits(
            hdc,
            0,//rect.left,//0,
            0,//rect.top,//0,
            width,//rect.right - rect.left,//width,
            height,//rect.bottom - rect.top,//height,
            0,
            0,
            width,
            height,
            pBits,
            pBMI,
            DIB_RGB_COLORS,//DIB_PAL_COLORS,
            SRCCOPY
            );
    }
}

int PASCAL
WinMain (
    __in HINSTANCE hInstance,
    __in_opt HINSTANCE hPrevInstance,
    __in_opt LPSTR lpCmdLine,
    __in int nShowCmd
    )
{
    static    TCHAR szAppName[]=TEXT("LJB_VMON Notify");
    PDEVICE_INFO deviceInfo;
    HWND      hWnd;
    MSG       msg;
    WNDCLASS  wndclass;

    UNREFERENCED_PARAMETER( lpCmdLine );

    InterfaceGuid = LJB_MONITOR_INTERFACE_GUID;
    hInst=hInstance;

#if (DBG)
    g_pDebugHandler = AddVectoredExceptionHandler(1, LJB_VMON_VectorHandler);
#endif

    deviceInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DEVICE_INFO));
    if(!deviceInfo)
        return FALSE;

    InitializeListHead(&ListHead);
    InitializeListHead(&deviceInfo->ListEntry);
    InsertTailList(&ListHead, &deviceInfo->ListEntry);

    if (!hPrevInstance)
    {
         wndclass.style        =  CS_HREDRAW | CS_VREDRAW;
         wndclass.lpfnWndProc  =  WndProc;
         wndclass.cbClsExtra   =  0;
         wndclass.cbWndExtra   =  0;
         wndclass.hInstance    =  hInstance;
         wndclass.hIcon        =  LoadIcon (NULL, IDI_APPLICATION);
         wndclass.hCursor      =  LoadCursor(NULL, IDC_ARROW);
         wndclass.hbrBackground=  GetStockObject(WHITE_BRUSH);
         wndclass.lpszMenuName =  TEXT("GenericMenu");
         wndclass.lpszClassName=  szAppName;

         RegisterClass(&wndclass);
    }

    hWnd = CreateWindow(
        szAppName,
        szTitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        NULL,
        NULL,
        hInstance,
        NULL);

    /*
     * create VMON main thread.
     */
    deviceInfo->hWndList = hWndList;
    deviceInfo->hParentWnd = hWnd;
    gDeviceInfo = deviceInfo;
    deviceInfo->VMONThread = CreateThread(
        NULL,
        0,      /* use default statck size */
        &LJB_VMON_Main,
        deviceInfo,
        0,       /* the thread runs after completion */
        &deviceInfo->VMONThreadId
        );

    ShowWindow(hWnd, nShowCmd);
    UpdateWindow(hWnd);

    while (GetMessage (&msg, NULL, 0,0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (0);
}


LRESULT
FAR PASCAL
WndProc(
    HWND hWnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    DWORD nEventType = (DWORD)wParam;
    PDEV_BROADCAST_HDR p = (PDEV_BROADCAST_HDR) lParam;
    DEV_BROADCAST_DEVICEINTERFACE filter;
    // For WM_PAINT
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_PAINT:
        if (gDeviceInfo != NULL && gDeviceInfo->BitmapBuffer != NULL &&
            wParam == LPARAM_NOTIFY_FRAME_UPDATE)
        {
            //hdc = BeginPaint(hWndList, &ps);
            hdc = GetDC(hWndList);
            makebmp(
                gDeviceInfo->BitmapBuffer,  // pBits,
                gDeviceInfo->Width,         //width,
                gDeviceInfo->Height,        //height,
                hdc,
                hWndList
                );
            ReleaseDC(
                hWndList,
                hdc
                );
            //EndPaint(hWndList, &ps);
        }
        return DefWindowProc(hWnd,message, wParam, lParam);

    case WM_COMMAND:
            HandleCommands(hWnd, message, wParam, lParam);
            return 0;

    case WM_CREATE:
            //
            // Load and set the icon of the program
            //
            SetClassLongPtr(hWnd, GCLP_HICON,
                (LONG_PTR)LoadIcon((HINSTANCE)lParam,MAKEINTRESOURCE(IDI_CLASS_ICON)));

            hWndList = CreateWindow (TEXT("listbox"),
                         NULL,
                         WS_CHILD|WS_VISIBLE|LBS_NOTIFY |
                         WS_VSCROLL | WS_BORDER,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         CW_USEDEFAULT,
                         hWnd,
                         (HMENU)ID_EDIT,
                         hInst,
                         NULL);
            return 0;

    case WM_SIZE:
        MoveWindow(hWndList, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
        return 0;

    case WM_SETFOCUS:
        SetFocus(hWndList);
        return 0;

    case WM_DEVICECHANGE:
        //
        // The DBT_DEVNODES_CHANGED broadcast message is sent
        // everytime a device is added or removed. This message
        // is typically handled by Device Manager kind of apps,
        // which uses it to refresh window whenever something changes.
        // The lParam is always NULL in this case.
        //
        if(DBT_DEVNODES_CHANGED == wParam) {
            DBG_PRINT(("Received DBT_DEVNODES_CHANGED broadcast message"));
            return 0;
        }
        return 0;

    case WM_POWERBROADCAST:
        HandlePowerBroadcast(hWnd, wParam, lParam);
        return 0;

    case WM_CLOSE:
        Cleanup(hWnd);
        UnregisterDeviceNotification(hInterfaceNotification);
        return  DefWindowProc(hWnd,message, wParam, lParam);

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd,message, wParam, lParam);
}


LRESULT
HandleCommands(
    HWND     hWnd,
    UINT     uMsg,
    WPARAM   wParam,
    LPARAM   lParam
    )
{
    PDIALOG_RESULT result = NULL;

    UNREFERENCED_PARAMETER( uMsg );
    UNREFERENCED_PARAMETER( lParam );

    switch (wParam) {
    case IDM_OPEN:
        break;

    case IDM_CLOSE:
        Cleanup(hWnd);
        break;

    case IDM_HIDE:
        break;

    case IDM_PLUGIN:
        break;

    case IDM_UNPLUG:
        break;

    case IDM_EJECT:
        break;

    case IDM_CLEAR:
        break;

    case IDM_IOCTL:
        break;

    case IDM_VERBOSE:
        break;

    case IDM_EXIT:
        PostQuitMessage(0);
        break;

    default:
        break;
    }

    if(result)
    {
        HeapFree(GetProcessHeap(), 0, result);
    }

    return TRUE;
}

INT_PTR CALLBACK
DlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    BOOL            success;
    PDIALOG_RESULT  dialogResult = NULL;

    UNREFERENCED_PARAMETER( lParam );

    switch(message)
    {
    case WM_INITDIALOG:
        SetDlgItemText(hDlg, IDC_DEVICEID, BUS_HARDWARE_IDS);
        return TRUE;

    case WM_COMMAND:
        switch( wParam)
        {
        case ID_OK:
            dialogResult = HeapAlloc(GetProcessHeap(),
                                    HEAP_ZERO_MEMORY,
                                    (sizeof(DIALOG_RESULT) + MAX_PATH * sizeof(WCHAR)));
            if(dialogResult)
            {
                dialogResult->DeviceId = (PWCHAR)((PCHAR)dialogResult + sizeof(DIALOG_RESULT));
                dialogResult->SerialNum = GetDlgItemInt(hDlg,IDC_SERIALNO, &success, FALSE );
                GetDlgItemText(hDlg, IDC_DEVICEID, dialogResult->DeviceId, MAX_PATH-1 );
            }
            EndDialog(hDlg, (UINT_PTR)dialogResult);
            return TRUE;

        case ID_CANCEL:
            EndDialog(hDlg, 0);
            return TRUE;
        }
        break;
    }

    return FALSE;
}

BOOLEAN Cleanup(HWND hWnd)
{
    PDEVICE_INFO    deviceInfo = NULL;
    PLIST_ENTRY     thisEntry;

    UNREFERENCED_PARAMETER(hWnd);

    while (!IsListEmpty(&ListHead))
    {
        thisEntry = RemoveHeadList(&ListHead);
        deviceInfo = CONTAINING_RECORD(thisEntry, DEVICE_INFO, ListEntry);
        if (deviceInfo->hHandleNotification)
        {
            UnregisterDeviceNotification(deviceInfo->hHandleNotification);
            deviceInfo->hHandleNotification = NULL;
        }

        if (deviceInfo->hDevice != INVALID_HANDLE_VALUE &&
                deviceInfo->hDevice != NULL)
        {
            CloseHandle(deviceInfo->hDevice);
            deviceInfo->hDevice = INVALID_HANDLE_VALUE;
            DBG_PRINT(("Closed handle to device %ws",
                deviceInfo->DeviceName));
        }
        HeapFree(GetProcessHeap(), 0, deviceInfo);
    }
    return TRUE;
}

BOOL
HandlePowerBroadcast(
    HWND hWnd,
    WPARAM wParam,
    LPARAM lParam)
{
    BOOL fRet = TRUE;

    UNREFERENCED_PARAMETER(hWnd);
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    return fRet;
}

void
SendIoctlToFilterDevice()
{
#define IOCTL_CUSTOM_CODE CTL_CODE(FILE_DEVICE_UNKNOWN, 0, METHOD_BUFFERED, FILE_READ_DATA)

    HANDLE hControlDevice;
    ULONG  bytes;

    //
    // Open handle to the control device. Please note that even
    // a non-admin user can open handle to the device with
    // FILE_READ_ATTRIBUTES | SYNCHRONIZE DesiredAccess and send IOCTLs if the
    // IOCTL is defined with FILE_ANY_ACCESS. So for better security avoid
    // specifying FILE_ANY_ACCESS in your IOCTL defintions.
    // If the IOCTL is defined to have FILE_READ_DATA access rights, you can
    // open the device with GENERIC_READ and call DeviceIoControl.
    // If the IOCTL is defined to have FILE_WRITE_DATA access rights, you can
    // open the device with GENERIC_WRITE and call DeviceIoControl.
    //
    hControlDevice = CreateFile ( TEXT("\\\\.\\ToasterFilter"),
                        GENERIC_READ, // Only read access
                        0, // FILE_SHARE_READ | FILE_SHARE_WRITE
                        NULL, // no SECURITY_ATTRIBUTES structure
                        OPEN_EXISTING, // No special create flags
                        0, // No special attributes
                        NULL); // No template file

    if (INVALID_HANDLE_VALUE == hControlDevice)
    {
        DBG_PRINT(("Failed to open ToasterFilter device"));
    }
    else
    {
        if (!DeviceIoControl (hControlDevice,
                              IOCTL_CUSTOM_CODE,
                              NULL, 0,
                              NULL, 0,
                              &bytes, NULL)) {
            DBG_PRINT(("Ioctl to ToasterFilter device failed\n"));
        } else {
            DBG_PRINT(("Ioctl to ToasterFilter device succeeded\n"));
        }
        CloseHandle(hControlDevice);
    }
    return;
}

LONG
LJB_VMON_VectorHandler(
    struct _EXCEPTION_POINTERS *pExceptionInfo
    )
    {
    PEXCEPTION_RECORD CONST pExceptionRecord = pExceptionInfo->ExceptionRecord;
    LONG    ReturnCode;

    ReturnCode = EXCEPTION_CONTINUE_SEARCH;
    switch (pExceptionRecord->ExceptionCode)
        {
    case 0x40010006:
        /*
         trigger by OutputDebugString
         */
        break;

    case 0xe06d7363:
        /*
         a Microsoft C++ exception
         */
        break;

    case 0xC0000005:
        DBG_PRINT(("? EXCEPTION_ACCESS_VIOLATION:\n"
            "\tExceptionFlag(%s)\n"
            "\tExceptionAddress(%p)\n",
            (pExceptionRecord->ExceptionFlags == 0) ?
            "EXCEPTION_CONTINUABLE" : "EXCEPTION_NONCONTINUABLE",
            pExceptionRecord->ExceptionAddress
            ));
        __debugbreak();
        break;
    default:
        //DBG_PRINT((
        //    "\t? ExceptionCode(0x%x)\n"
        //    "\tExceptionFlag(%s)\n"
        //    "\tExceptionAddress(%p)\n",
        //    pExceptionRecord->ExceptionCode,
        //    (pExceptionRecord->ExceptionFlags == 0) ?
        //    "EXCEPTION_CONTINUABLE" : "EXCEPTION_NONCONTINUABLE",
        //    pExceptionRecord->ExceptionAddress
        //    ));

        break;
        }
    return ReturnCode;
    }
