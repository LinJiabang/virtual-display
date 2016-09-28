#include <conio.h>
#include "ljb_vmon.h"
#include "ljb_vmon_ioctl.h"
#include "lci_usbav_guid.h"

/*
 * borrow STATUS_NO_SUCH_DEVICE definition from ntstatus.h
 */
#ifndef STATUS_NO_SUCH_DEVICE
#define STATUS_NO_SUCH_DEVICE            (0xC000000EL)
#endif
#ifndef STATUS_DEVICE_REMOVED
#define STATUS_DEVICE_REMOVED            (0xC00002B6L)
#endif
typedef ULONG WINAPI RTL_NT_STATUS_TO_DOS_ERROR(ULONG ntStatus);

/*
 * Name:  LJB_VMON_GetDevicePath
 *
 * Definition:
 *    PCHAR
 *    LJB_VMON_GetDevicePath(
 *        __in  LPGUID InterfaceGuid
 *        );
 *
 * Description:
 *    Given an interface GUID, this routine enumerate the associated interface
 *    in the system, and return the device path.
 *
 * Return Value:
 *    pointer to device path.
 *
 */
__checkReturn
PCHAR
LJB_VMON_GetDevicePath(
    __in LJB_VMON_DEV_CTX *             dev_ctx,
    __in LPGUID                         InterfaceGuid
    )
{
    HDEVINFO                            HardwareDeviceInfo;
    SP_DEVICE_INTERFACE_DATA            DeviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    pDevIfcDetailData;
    ULONG                               Length, RequiredLength = 0;
    BOOL                                bResult;

    dev_ctx->pDevIfcDetailData = NULL;
    dev_ctx->HardwareDeviceInfo = INVALID_HANDLE_VALUE;

    HardwareDeviceInfo = SetupDiGetClassDevs(
        InterfaceGuid,
        NULL,
        NULL,
        (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));

    if (HardwareDeviceInfo == INVALID_HANDLE_VALUE)
    {
        DBG_PRINT(("?" __FUNCTION__ ": SetupDiGetClassDevs failed!\n"));
        return NULL;
    }

    dev_ctx->HardwareDeviceInfo = HardwareDeviceInfo;
    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    bResult = SetupDiEnumDeviceInterfaces(
        HardwareDeviceInfo,
        0,
        InterfaceGuid,
        0,
        &DeviceInterfaceData
        );

    if (bResult == FALSE)
    {
        LPVOID lpMsgBuf;

        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR) &lpMsgBuf,
            0,
            NULL
            ))
            {
            DBG_PRINT(("?" __FUNCTION__ ":%s", (LPSTR)lpMsgBuf));
            LocalFree(lpMsgBuf);
            }

        DBG_PRINT(("?" __FUNCTION__ ":SetupDiEnumDeviceInterfaces failed.\n"));
        SetupDiDestroyDeviceInfoList(HardwareDeviceInfo);
        dev_ctx->HardwareDeviceInfo = INVALID_HANDLE_VALUE;
        return NULL;
    }

    SetupDiGetDeviceInterfaceDetail(
        HardwareDeviceInfo,
        &DeviceInterfaceData,
        NULL,
        0,
        &RequiredLength,
        NULL
        );

    pDevIfcDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)
        LocalAlloc(LMEM_FIXED, RequiredLength);
    if (pDevIfcDetailData == NULL)
    {
        SetupDiDestroyDeviceInfoList(HardwareDeviceInfo);
        dev_ctx->HardwareDeviceInfo = INVALID_HANDLE_VALUE;
        DBG_PRINT(("?" __FUNCTION__ ": Failed to allocate memory?\n"));
        return  NULL;
    }

    pDevIfcDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    Length = RequiredLength;
    bResult = SetupDiGetDeviceInterfaceDetail(
            HardwareDeviceInfo,
            &DeviceInterfaceData,
            pDevIfcDetailData,
            Length,
            &RequiredLength,
            NULL);

    if (bResult == FALSE)
    {
        LPVOID lpMsgBuf;

        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPSTR) &lpMsgBuf,
            0,
            NULL
            );

        DBG_PRINT((
            "?" __FUNCTION__
            ": SetupDiGetDeviceInterfaceDetail failed. Error: %s\n",
            (LPCTSTR) lpMsgBuf
            ));

        LocalFree(lpMsgBuf);
        dev_ctx->HardwareDeviceInfo = INVALID_HANDLE_VALUE;
        SetupDiDestroyDeviceInfoList(HardwareDeviceInfo);
        LocalFree(pDevIfcDetailData);
        return NULL;
    }

    dev_ctx->pDevIfcDetailData    = pDevIfcDetailData;
    return pDevIfcDetailData->DevicePath;
}

/*
 * Name:  LJB_VMON_CloseDeviceHandle
 *
 * Definition:
 *    VOID
 *    LJB_VMON_CloseDeviceHandle(
 *        __in LJB_VMON_DEV_CTX *        dev_ctx
 *        );
 *
 * Description:
 *    Close device handle.
 *
 * Return Value:
 *    None.
 *
 */
__checkReturn
VOID
LJB_VMON_CloseDeviceHandle(
    __in LJB_VMON_DEV_CTX *        dev_ctx
    )
    {
    if (dev_ctx == NULL)
        return;

    if (dev_ctx->HardwareDeviceInfo != INVALID_HANDLE_VALUE)
    {
        SetupDiDestroyDeviceInfoList(dev_ctx->HardwareDeviceInfo);
        dev_ctx->HardwareDeviceInfo = INVALID_HANDLE_VALUE;
    }

    if (dev_ctx->pDevIfcDetailData != NULL)
    {
        LocalFree(dev_ctx->pDevIfcDetailData);
        dev_ctx->pDevIfcDetailData = NULL;
    }

    if (dev_ctx->hDevice != INVALID_HANDLE_VALUE)
    {
        CloseHandle(dev_ctx->hDevice);
        dev_ctx->hDevice = INVALID_HANDLE_VALUE;
    }
}

/*
 * Name:  LJB_VMON_GetDeviceHandle
 *
 * Definition:
 *    BOOL
 *    LJB_VMON_GetDeviceHandle(
 *        __in LJB_VMON_DEV_CTX *        dev_ctx
 *        );
 *
 * Description:
 *    Get device handle.
 *
 * Return Value:
 *    Return TRUE if success. Return FALSE otherwise.
 *
 */
__checkReturn
BOOL
LJB_VMON_GetDeviceHandle(
    __in LJB_VMON_DEV_CTX *     dev_ctx
    )
{
    HANDLE                      hDevice;
    PCHAR                       pDevicePath;

    dev_ctx->hDevice = INVALID_HANDLE_VALUE;
    pDevicePath = LJB_VMON_GetDevicePath(
        dev_ctx,
        (LPGUID)&GUID_LCI_USBAV
        );
    if (pDevicePath == NULL)
    {
        fprintf(stderr, "VMON device not present?\n");
        LJB_VMON_CloseDeviceHandle(dev_ctx);
        return FALSE;
    }

    printf("Found USB AV DevicePath: %s\n", pDevicePath);
    hDevice = CreateFile(
        pDevicePath,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL
        );
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        fprintf(stderr, "Failed to open device. Error %d\n",
            GetLastError()
            );
        LJB_VMON_CloseDeviceHandle(dev_ctx);
        return FALSE;
    }

    dev_ctx->hDevice = hDevice;
    return TRUE;
}

/*
 * Name:  LJB_VMON_PixelMain_Init
 *
 * Definition:
 *    BOOLEAN
 *    LJB_VMON_PixelMain_Init(
 *        __in LJB_VMON_DEV_CTX *    dev_ctx
 *        );
 *
 * Description:
 *    Initialize all parameters for VMON thread.
 *
 * Return Value:
 *    Return TRUE if success. Return FALSE otherwise.
 *
 */
BOOLEAN
LJB_VMON_PixelMain_Init(
    __in LJB_VMON_DEV_CTX *    dev_ctx
    )
{
    return TRUE;
}

/*
 * Name:  LJB_VMON_PixelMain_DeInit
 *
 * Definition:
 *    VOID
 *    LJB_VMON_PixelMain_DeInit(
 *        __in LJB_VMON_DEV_CTX *    dev_ctx
 *        )
 *
 * Description:
 *    De-initialize all parameters for VMON thread.
 *
 * Return Value:
 *     Nothing.
 *
 */
VOID
LJB_VMON_PixelMain_DeInit(
    __in LJB_VMON_DEV_CTX *    dev_ctx
    )
{
}

/*
 * Name:  LJB_VMON_PixelMain
 *
 * Definition:
 *    VOID
 *    LJB_VMON_PixelMain(
 *        __in LJB_VMON_DEV_CTX *            dev_ctx
 *        );
 *
 * Description:
 *    This routine is start up pixel service function.
 *    1. Start a new thread for compression tasks.
 *    2. Wait for primary surface update, and acquire primary surface.
 *
 * Return Value:
 *    Nothing
 *
 */
VOID
LJB_VMON_PixelMain(
    __in LJB_VMON_DEV_CTX *    dev_ctx
    )
    {
    HANDLE CONST                    hDefaultHeap = GetProcessHeap();
    HMODULE CONST                   hNtDll = LoadLibrary("ntdll.dll");
    LJB_VMON_WAIT_FOR_UPDATE_DATA   wait_for_update_data;
    BOOL                            io_ret;
    BOOLEAN                         ret;
    BOOLEAN                         exit_loop;
    UINT                            LastOutputFrameId;
    RTL_NT_STATUS_TO_DOS_ERROR *    RtlNtStatusToDosErrorFn;
    BOOL                            bypass_wait_for_update;
    PDEVICE_INFO                    pDeviceInfo;
    PVOID                           frame_buffer;

    pDeviceInfo = dev_ctx->pDeviceInfo;
    if (hNtDll == NULL)
    {
        DBG_PRINT(("?" __FUNCTION__ ": unable to load ntdll.dll?\n"));
        return;
    }

    RtlNtStatusToDosErrorFn = (RTL_NT_STATUS_TO_DOS_ERROR*) GetProcAddress(
        hNtDll,
        TEXT("RtlNtStatusToDosError")
        );
    if (RtlNtStatusToDosErrorFn == NULL)
    {
        DBG_PRINT(("?" __FUNCTION__ ": No RtlNtStatusToDosError?\n"));
        return;
    }

    ret = LJB_VMON_PixelMain_Init(dev_ctx);
    if (ret == FALSE)
    {
        DBG_PRINT(("?" __FUNCTION__ ": LJB_VMON_PixelMain_Init failed.\n"));
        LJB_VMON_PixelMain_DeInit(dev_ctx);
        return;
    }

    wait_for_update_data.Flags.Frame = 1;
    LastOutputFrameId = 0;

    exit_loop = FALSE;
    bypass_wait_for_update = FALSE;
    while (!exit_loop)
    {
        OVERLAPPED              Overlapped;
        LJB_VMON_FRAME_INFO *   frame_info;
        ULONG                   bytes_returned;

        if (dev_ctx->exit_vmon_thread)
            break;

        if (!bypass_wait_for_update)
        {
            wait_for_update_data.FrameId = LastOutputFrameId;
            wait_for_update_data.Flags.Frame = 1;
            wait_for_update_data.Flags.Cursor = 1;
            ZeroMemory(&Overlapped, sizeof(Overlapped));
            Overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            io_ret = DeviceIoControl(
                dev_ctx->hDevice,
                IOCTL_LJB_VMON_WAIT_FOR_UPDATE,
                &wait_for_update_data,
                sizeof(wait_for_update_data),
                &wait_for_update_data,
                sizeof(wait_for_update_data),
                &bytes_returned,
                &Overlapped
                );

            /*
             * ioctl returns failure. could be io pending, or device removed
             */
            if (!io_ret)
            {
                DWORD CONST LastError = GetLastError();

                if (LastError == (*RtlNtStatusToDosErrorFn)(STATUS_NO_SUCH_DEVICE) ||
                    LastError == (*RtlNtStatusToDosErrorFn)(STATUS_DEVICE_REMOVED))
                {
                    DBG_PRINT(("?" __FUNCTION__ ": "
                        "IOCTL_LJB_VMON_WAIT_FOR_UPDATE failed. "
                        "Device unplugged, LastError(0x%x)\n",
                        LastError));
                    CloseHandle(Overlapped.hEvent);
                    break;
                }

                if (LastError != ERROR_IO_PENDING)
                {
                    DBG_PRINT(("?" __FUNCTION__ ": "
                        "IOCTL_LJB_VMON_WAIT_FOR_UPDATE failed? LastError(0x%x)\n",
                        LastError));
                    CloseHandle(Overlapped.hEvent);
                    continue;
                    }

                /*
                 * wait for io complete
                 */
                io_ret = GetOverlappedResult(
                    dev_ctx->hDevice,
                    &Overlapped,
                    &bytes_returned,
                    TRUE
                    );
                }
            CloseHandle(Overlapped.hEvent);
        }

        bypass_wait_for_update = FALSE;
        frame_info = HeapAlloc(
            hDefaultHeap,
            HEAP_ZERO_MEMORY,
            sizeof(*frame_info)
            );
        if (frame_info == NULL)
        {
            DBG_PRINT(("?" __FUNCTION__
                ": unable to allocate frame_info?\n"));
            continue;
        }

        /*
         * acquire uncompressed frame buffer
         */
        ZeroMemory(&Overlapped, sizeof(Overlapped));
        Overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        io_ret = DeviceIoControl(
            dev_ctx->hDevice,
            IOCTL_LJB_VMON_ACQUIRE_FRAME,
            NULL,
            0,
            frame_info,
            sizeof(*frame_info),
            &bytes_returned,
            &Overlapped
            );
            
        if (!io_ret)
        {
            DWORD CONST LastError = GetLastError();

            if (LastError == (*RtlNtStatusToDosErrorFn)(STATUS_NO_SUCH_DEVICE) ||
                LastError == (*RtlNtStatusToDosErrorFn)(STATUS_DEVICE_REMOVED))
            {
                DBG_PRINT(("?" __FUNCTION__ ": "
                    "IOCTL_LJB_VMON_ACQUIRE_FRAME failed, "
                    "Device unplugged, LastError(0x%x)\n",
                    LastError));
                HeapFree(hDefaultHeap, 0, frame_info);
                CloseHandle(Overlapped.hEvent);
                break;
            }
            
            if (LastError != ERROR_IO_PENDING)
            {
                DBG_PRINT(("?" __FUNCTION__
                    ": IOCTL_LJB_VMON_ACQUIRE_FRAME failed, LastError(0x%x)?\n",
                    GetLastError()
                    ));
                HeapFree(hDefaultHeap, 0, frame_info);
                CloseHandle(Overlapped.hEvent);
                bypass_wait_for_update = TRUE;
                continue;
            }
            
            io_ret = GetOverlappedResult(
                dev_ctx->hDevice,
                &Overlapped,
                &bytes_returned,
                TRUE
                );
        }
        CloseHandle(Overlapped.hEvent);

        /*
         * now the frame is acquired, send it out
         */
        frame_buffer = (PVOID) ((ULONG_PTR) frame_info->UserFrameAddress);
        if (frame_buffer == NULL)
        {
            DBG_PRINT(("?" __FUNCTION__
                ": unable to acquire BitmapBuffer?\n"));

            HeapFree(hDefaultHeap, 0, frame_info);
            Sleep(5);
            continue;
        }
        DBG_PRINT((__FUNCTION__ ": pFrameBuffer(%p) mapped from kernel.\n",
            frame_buffer));

        /*
         * Save mark returned frameId from Acquire Ioctl.
         */
        LastOutputFrameId = frame_info->FrameIdAcquired;
        DBG_PRINT((" " __FUNCTION__ ": LastOutputFrameId(0x%x) updated\n", LastOutputFrameId));

		pDeviceInfo->BitmapBuffer   = frame_buffer;
		pDeviceInfo->Width          = frame_info->Width;
		pDeviceInfo->Height         = frame_info->Height;
		pDeviceInfo->dev_ctx        = dev_ctx;

		//output now.
		SendMessage(pDeviceInfo->hParentWnd, WM_PAINT, LPARAM_NOTIFY_FRAME_UPDATE, 0);
    } /* end of while */

    (VOID) FreeLibrary(hNtDll);
    LJB_VMON_PixelMain_DeInit(dev_ctx);
    DBG_PRINT(("-" __FUNCTION__": leaving.\n"));
}
