#ifndef _LJB_VMON_H_
#define _LJB_VMON_H_

#pragma warning(disable:4127)

#include <windows.h>
#include <winioctl.h>
#include <strsafe.h>
#include <setupapi.h>
#include <stdio.h>
#include <stdlib.h>

/*
 Forward declaration
 */
typedef struct _LJB_VMON_DEV_CTX    LJB_VMON_DEV_CTX;

#define LPARAM_NOTIFY_FRAME_UPDATE  0x12345678

// From notify.h
typedef struct _DEVICE_INFO
{
   HANDLE                       hDevice; // file handle
   HDEVNOTIFY                   hHandleNotification; // notification handle
   WCHAR                        DeviceName[MAX_PATH];// friendly name of device description
   WCHAR                        DevicePath[MAX_PATH];//
   ULONG                        SerialNum; // Serial number of the device.
   LIST_ENTRY                   ListEntry;
   HANDLE                       VMONThread;
   ULONG                        VMONThreadId;
   PVOID                        BitmapBuffer;
   ULONG                        Width;
   ULONG                        Height;
   HWND                         hWndList;
   HWND                         hParentWnd;
   LJB_VMON_DEV_CTX *           dev_ctx;
} DEVICE_INFO, *PDEVICE_INFO;

typedef struct _LJB_VMON_DEV_CTX
    {
    HANDLE                              hDevice;
    HDEVINFO                            HardwareDeviceInfo;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    pDevIfcDetailData;

    PDEVICE_INFO                        pDeviceInfo;
    BOOL                                exit_vmon_thread;

    } LJB_VMON_DEV_CTX;

/*
 * Macro, inline functions borrowed from wdm.h
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

__checkReturn
BOOL
LJB_VMON_GetDeviceHandle(
    __in LJB_VMON_DEV_CTX *     dev_ctx
    );

__checkReturn
VOID
LJB_VMON_CloseDeviceHandle(
    __in LJB_VMON_DEV_CTX *     dev_ctx
    );

VOID
LJB_VMON_PixelMain(
    __in LJB_VMON_DEV_CTX *     dev_ctx
    );

VOID
LJB_VMON_DumpBuffer(
    __in UCHAR  *               pBuf,
    __in ULONG                  BufSize
    );

VOID
__cdecl
LJB_VMON_DbgPrint(
    __in_z __drv_formatString(printf) PCSTR format,
    ...
    );


#define DBG_PRINT_ALWAYS(x)             LJB_VMON_DbgPrint x
#if (DBG)
#define DBG_PRINT(x)                    LJB_VMON_DbgPrint x
#define DUMP_BUF(buf, size)             LJB_VMON_DumpBuffer(buf, size);
#else
#define DBG_PRINT(x)
#define DUMP_BUF(buf, size)
#endif

#endif /* _LJB_VMON_H_ */