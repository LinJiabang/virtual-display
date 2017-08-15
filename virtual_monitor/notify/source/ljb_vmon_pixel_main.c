#include <conio.h>
//#include <dwmapi.h>
#include "ljb_vmon.h"


/*
 * The following EDID data is borrowed from Samsung SyncMaster 2343
 */
UCHAR CONST EdidTemplate[128] = {
//    00    01    02    03    04    05    06    07    08    09    0A    0B    0C    0D    0E    0F
    0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x4C, 0x2D, 0x85, 0x05, 0x33, 0x32, 0x59, 0x4D,
    0x01, 0x14, 0x01, 0x03, 0x80, 0x33, 0x1D, 0x78, 0x2A, 0xEE, 0x91, 0xA3, 0x54, 0x4C, 0x99, 0x26,
    0x0F, 0x50, 0x54, 0x23, 0x08, 0x00, 0x81, 0x80, 0x81, 0x40, 0x81, 0x00, 0x95, 0x00, 0xB3, 0x00,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x3B, 0x3D, 0x00, 0xA0, 0x80, 0x80, 0x21, 0x40, 0x30, 0x20,
    0x35, 0x00, 0xFE, 0x1F, 0x11, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00, 0xFD, 0x00, 0x38, 0x3C, 0x1E,
    0x51, 0x10, 0x00, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFC, 0x00, 0x53,
    0x79, 0x6E, 0x63, 0x4D, 0x61, 0x73, 0x74, 0x65, 0x72, 0x0A, 0x20, 0x20, 0x00, 0x00, 0x00, 0xFF,
    0x00, 0x48, 0x56, 0x4D, 0x5A, 0x31, 0x30, 0x30, 0x30, 0x38, 0x30, 0x0A, 0x20, 0x20, 0x01, 0x76,

};

static VOID
SetEdid(
    __out UCHAR MyEDID[128]
    );

static VOID
LJB_VMON_DrawCursorOnFrameBuffer(
    __in LJB_VMON_DEV_CTX *             dev_ctx,
    __in TARGET_MODE_DATA               *TargetModeData,
    __in VIDPN_SOURCE_VISIBILITY_DATA   *VisibilityData,
    __in POINTER_POSITION_DATA          *PointerPositionData,
    __in POINTER_SHAPE_DATA             *PointerShapeData,
    __out PVOID                         FrameBuffer
    );

static VOID
LJB_VMON_RestoreFrameBuffer(
    __in LJB_VMON_DEV_CTX *             dev_ctx,
    __in TARGET_MODE_DATA               *TargetModeData,
    __out PVOID                         FrameBuffer
    );

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
        (LPGUID)&LJB_MONITOR_INTERFACE_GUID
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
typedef HRESULT WINAPI DWM_ENABLE_MMCSS(
   BOOL fEnableMMCSS
);
BOOLEAN
LJB_VMON_PixelMain_Init(
    __in LJB_VMON_DEV_CTX *    dev_ctx
    )
{
    HMODULE CONST       hDwmApiDll = LoadLibrary("dwmapi.dll");
    DWM_ENABLE_MMCSS *  DwmEnableMMCSSFn;

    if (hDwmApiDll == NULL)
    {
        DBG_PRINT(("?" __FUNCTION__ ": unable to load dwmapi.dll?\n"));
        return FALSE;
    }

    DwmEnableMMCSSFn = (DWM_ENABLE_MMCSS*) GetProcAddress(
        hDwmApiDll,
        TEXT("DwmEnableMMCSS")
        );
    if (DwmEnableMMCSSFn == NULL)
    {
        DBG_PRINT(("?" __FUNCTION__ ": No DwmEnableMMCSSFn?\n"));
        return FALSE;
    }

    (*DwmEnableMMCSSFn)(TRUE);

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
    LJB_VMON_MONITOR_EVENT          MonitorEvent;
    BOOL                            io_ret;
    BOOLEAN                         ret;
    BOOLEAN                         ExitLoop;
    RTL_NT_STATUS_TO_DOS_ERROR *    RtlNtStatusToDosErrorFn;
    PDEVICE_INFO                    pDeviceInfo;
    PVOID                           FrameBuffer;
    UINT                            OutputFrameId;
    UCHAR                           MyEDID[128];
    ULONG                           bytes_returned;
    BOOLEAN                         PointerPositionChanged;
    UINT32                          VidPnTargetId;

    RtlCopyMemory(MyEDID, EdidTemplate, 128);
    SetEdid(MyEDID);

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

    RtlZeroMemory(&dev_ctx->TargetModeData, sizeof(TARGET_MODE_DATA));
    RtlZeroMemory(&dev_ctx->VisibilityData, sizeof(VIDPN_SOURCE_VISIBILITY_DATA));
    RtlZeroMemory(&dev_ctx->PointerPositionData, sizeof(POINTER_POSITION_DATA));
    OutputFrameId = 0;
    FrameBuffer = NULL;

    io_ret = DeviceIoControl(
        dev_ctx->hDevice,
        IOCTL_LJB_VMON_PLUGIN_MONITOR,
        MyEDID,
        sizeof(MyEDID),
        &VidPnTargetId,
        sizeof(VidPnTargetId),
        &bytes_returned,
        NULL
        );
    if (!io_ret)
    {
        DBG_PRINT((__FUNCTION__": IOCTL_LJB_VMON_PLUGIN_MONITOR failed\n"));
        return;
    }

    DBG_PRINT((__FUNCTION__": VidPnTargetId(0x%x) returned\n", VidPnTargetId));

    ExitLoop = FALSE;
    while (!ExitLoop)
    {
        LJB_VMON_WAIT_FLAGS         OutputFlags;
        BOOLEAN                     NeedUpdateImage;

        if (dev_ctx->exit_vmon_thread)
            break;

        PointerPositionChanged = 0;
        MonitorEvent.Flags.ModeChange = 1;
        MonitorEvent.Flags.VidPnSourceVisibilityChange = 1;
        MonitorEvent.Flags.VidPnSourceBitmapChange = 1;
        MonitorEvent.Flags.PointerPositionChange = 1;
        MonitorEvent.Flags.PointerShapeChange = 1;
        MonitorEvent.TargetModeData = dev_ctx->TargetModeData;
        MonitorEvent.VidPnSourceVisibilityData = dev_ctx->VisibilityData;
        MonitorEvent.FrameId = OutputFrameId;
        MonitorEvent.PointerPositionData = dev_ctx->PointerPositionData;

        io_ret = DeviceIoControl(
            dev_ctx->hDevice,
            IOCTL_LJB_VMON_WAIT_FOR_MONITOR_EVENT,
            &MonitorEvent,
            sizeof(MonitorEvent),
            &MonitorEvent,
            sizeof(MonitorEvent),
            &bytes_returned,
            NULL
            );

        /*
         * ioctl returns failure. it could be device removed
         */
        if (!io_ret)
        {
            DWORD CONST LastError = GetLastError();

            if (LastError == (*RtlNtStatusToDosErrorFn)(STATUS_NO_SUCH_DEVICE) ||
                LastError == (*RtlNtStatusToDosErrorFn)(STATUS_DEVICE_REMOVED))
            {
                DBG_PRINT(("?" __FUNCTION__ ": "
                    "IOCTL_LJB_VMON_WAIT_FOR_MONITOR_EVENT failed. "
                    "Device unplugged, LastError(0x%x)\n",
                    LastError));
                break;
            }

            /*
             * if IOCTL_LJB_VMON_WAIT_FOR_MONITOR_EVENT failed for some other
             * reason, retry
             */
            DBG_PRINT(("?" __FUNCTION__ ": "
                "IOCTL_LJB_VMON_WAIT_FOR_UPDATE failed. LastError(0x%x)\n",
                LastError));
            continue;
        }

        /*
         * Check each output flags
         */
        OutputFlags = MonitorEvent.Flags;
        if (OutputFlags.ModeChange)
        {
            BOOLEAN ResolutionChanged = FALSE;

            DBG_PRINT((__FUNCTION__
                ": ModeChange, previous mode(%u, %u), new mode(%u, %u)\n",
                dev_ctx->TargetModeData.Width,
                dev_ctx->TargetModeData.Height,
                MonitorEvent.TargetModeData.Width,
                MonitorEvent.TargetModeData.Height
                ));

            /*
             * check if resolution change
             */
            if (dev_ctx->TargetModeData.Width != MonitorEvent.TargetModeData.Width ||
                dev_ctx->TargetModeData.Height != MonitorEvent.TargetModeData.Height)
                ResolutionChanged = TRUE;

            if (ResolutionChanged)
            {
                LOCK_BUFFER_DATA    LockBufferData;

                if (FrameBuffer != NULL)
                {
                    RtlZeroMemory(&LockBufferData, sizeof(LockBufferData));
                    LockBufferData.FrameBuffer = (UINT64)((ULONG_PTR) FrameBuffer);
                    LockBufferData.FrameBufferSize =
                        dev_ctx->TargetModeData.Width *
                        dev_ctx->TargetModeData.Height * 4;
                    DeviceIoControl(
                        dev_ctx->hDevice,
                        IOCTL_LJB_VMON_UNLOCK_BUFFER,
                        &LockBufferData,
                        sizeof(LockBufferData),
                        NULL,
                        0,
                        &bytes_returned,
                        NULL
                        );
                    HeapFree(hDefaultHeap, 0, FrameBuffer);
                    FrameBuffer = NULL;
                }

                dev_ctx->TargetModeData = MonitorEvent.TargetModeData;
                if (dev_ctx->TargetModeData.Width != 0 &&
                    dev_ctx->TargetModeData.Height != 0)
                {
                    FrameBuffer = HeapAlloc(hDefaultHeap, HEAP_ZERO_MEMORY,
                        dev_ctx->TargetModeData.Width *
                        dev_ctx->TargetModeData.Height * 4);
                    if (FrameBuffer == NULL)
                    {
                        DBG_PRINT((__FUNCTION__
                            ": no FrameBuffer allocated for Width=%u, Height=%u?\n",
                            dev_ctx->TargetModeData.Width,
                            dev_ctx->TargetModeData.Height));
                        break;
                    }
                    DBG_PRINT((__FUNCTION__
                        ": FrameBuffer(%p) allocated for Width=%u, Height=%u\n",
                        FrameBuffer,
                        dev_ctx->TargetModeData.Width,
                        dev_ctx->TargetModeData.Height));

                    RtlZeroMemory(&LockBufferData, sizeof(LockBufferData));
                    LockBufferData.FrameBuffer = (UINT64)((ULONG_PTR) FrameBuffer);
                    LockBufferData.FrameBufferSize =
                        dev_ctx->TargetModeData.Width *
                        dev_ctx->TargetModeData.Height * 4;
                    DeviceIoControl(
                        dev_ctx->hDevice,
                        IOCTL_LJB_VMON_LOCK_BUFFER,
                        &LockBufferData,
                        sizeof(LockBufferData),
                        NULL,
                        0,
                        &bytes_returned,
                        NULL
                        );
                }
            }
        }
        if (OutputFlags.VidPnSourceVisibilityChange)
        {
            DBG_PRINT((__FUNCTION__": VisibilityChange (%u => %u)\n",
                dev_ctx->VisibilityData.Visible,
                MonitorEvent.VidPnSourceVisibilityData.Visible
                ));
            dev_ctx->VisibilityData = MonitorEvent.VidPnSourceVisibilityData;
        }

        if (OutputFlags.VidPnSourceBitmapChange)
        {
            BLT_DATA    BltData;

            //DBG_PRINT((__FUNCTION__": Bitmap Changed, FrameId(%u => %u)\n",
            //    OutputFrameId,
            //    MonitorEvent.FrameId
            //    ));
            OutputFrameId = MonitorEvent.FrameId;
            dev_ctx->FrameBufferIsDirty = FALSE;

            /*
             * acquire bitmap from kmd
             */
            if (FrameBuffer != NULL)
            {
                RtlZeroMemory(&BltData, sizeof(BltData));
                BltData.Width           = dev_ctx->TargetModeData.Width;
                BltData.Height          = dev_ctx->TargetModeData.Height;
                BltData.FrameId         = OutputFrameId;
                BltData.FrameBufferSize = dev_ctx->TargetModeData.Width *
                                          dev_ctx->TargetModeData.Height * 4;
                BltData.FrameBuffer     = (UINT64)((ULONG_PTR) FrameBuffer);

                DeviceIoControl(
                    dev_ctx->hDevice,
                    IOCTL_LJB_VMON_BLT_BITMAP,
                    &BltData,
                    sizeof(BltData),
                    &BltData,
                    sizeof(BltData),
                    &bytes_returned,
                    NULL
                    );
            }
        }

        if (OutputFlags.PointerPositionChange)
        {
            //DBG_PRINT((__FUNCTION__
            //    ": PointerPosition Changed: (%d, %d) Visible(%u) => (%d, %d) Visible(%u)\n",
            //    dev_ctx->PointerPositionData.X,
            //    dev_ctx->PointerPositionData.Y,
            //    dev_ctx->PointerPositionData.Visible,
            //    MonitorEvent.PointerPositionData.X,
            //    MonitorEvent.PointerPositionData.Y,
            //    MonitorEvent.PointerPositionData.Visible
            //    ));
            PointerPositionChanged =
                (dev_ctx->PointerPositionData.X != MonitorEvent.PointerPositionData.X) ||
                (dev_ctx->PointerPositionData.Y != MonitorEvent.PointerPositionData.Y) ||
                (dev_ctx->PointerPositionData.Visible != MonitorEvent.PointerPositionData.Visible);

            dev_ctx->PointerPositionData = MonitorEvent.PointerPositionData;
        }

        if (OutputFlags.PointerShapeChange)
        {
            DBG_PRINT((__FUNCTION__": PointerShape Changed\n"));
            DeviceIoControl(
                dev_ctx->hDevice,
                IOCTL_LJB_VMON_GET_POINTER_SHAPE,
                NULL,
                0,
                &dev_ctx->PointerShapeData,
                sizeof(POINTER_SHAPE_DATA),
                &bytes_returned,
                NULL
                );
        }

        /*
         * now update the final image. If the monitor is set to invisible,
         * don't update
         */
        NeedUpdateImage = FALSE;
        if (OutputFlags.VidPnSourceBitmapChange ||
            (OutputFlags.PointerShapeChange && dev_ctx->PointerPositionData.Visible) ||
            PointerPositionChanged)
            NeedUpdateImage = TRUE;

        if (FrameBuffer != NULL &&
            dev_ctx->VisibilityData.Visible &&
            NeedUpdateImage)
        {
            /*
             * overlay cursor image to the final image. Recover previous image first.
             */
            if (dev_ctx->FrameBufferIsDirty)
            {
                LJB_VMON_RestoreFrameBuffer(
                    dev_ctx,
                    &dev_ctx->TargetModeData,
                    FrameBuffer
                    );
            }

            if (dev_ctx->PointerPositionData.Visible)
            {
                LJB_VMON_DrawCursorOnFrameBuffer(
                    dev_ctx,
                    &dev_ctx->TargetModeData,
                    &dev_ctx->VisibilityData,
                    &dev_ctx->PointerPositionData,
                    &dev_ctx->PointerShapeData,
                    FrameBuffer
                    );
            }

            pDeviceInfo->BitmapBuffer   = FrameBuffer;
            pDeviceInfo->Width          = dev_ctx->TargetModeData.Width;
            pDeviceInfo->Height         = dev_ctx->TargetModeData.Height;
            pDeviceInfo->dev_ctx        = dev_ctx;

            //output now.
            SendMessage(pDeviceInfo->hParentWnd, WM_PAINT, LPARAM_NOTIFY_FRAME_UPDATE, 0);
        }
    } /* end of while */

    DeviceIoControl(
        dev_ctx->hDevice,
        IOCTL_LJB_VMON_UNPLUG_MONITOR,
        NULL,
        0,
        NULL,
        0,
        &bytes_returned,
        NULL
        );

    (VOID) FreeLibrary(hNtDll);
    LJB_VMON_PixelMain_DeInit(dev_ctx);
    DBG_PRINT(("-" __FUNCTION__": leaving.\n"));
}

#define EDID_RATIO_16_10            0
#define EDID_RATIO_4_3              1
#define EDID_RATIO_5_4              2
#define EDID_RATIO_16_9             3

BOOLEAN CONST  Enable_640_480   = TRUE;
BOOLEAN CONST  Enable_800_600   = TRUE;
BOOLEAN CONST  Enable_1024_768  = TRUE;
BOOLEAN CONST  Enable_1280_960  = TRUE;
BOOLEAN CONST  Enable_1280_1024 = TRUE;
BOOLEAN CONST  Enable_1400_1050 = TRUE;
BOOLEAN CONST  Enable_1440_900  = TRUE;
BOOLEAN CONST  Enable_1600_900  = TRUE;
BOOLEAN CONST  Enable_1680_1050 = TRUE;
BOOLEAN CONST  Enable_1920_1080 = TRUE;
BOOLEAN CONST  Enable_1920_1200 = TRUE;

typedef struct _DETAILED_TIMING_MODE
{
    UINT    Width;
    UINT    Height;
}   DETAILED_TIMING_MODE;

DETAILED_TIMING_MODE    DetailedTimingTable[] =
{
    {2048, 1152},
};
#define NUM_OF_DETAILED_TIMING  (sizeof(DetailedTimingTable)/sizeof(DETAILED_TIMING_MODE))

static VOID
SetDetailTimingDesc(
    __out UCHAR MyEDID[128],
    __in  UINT  Index,
    __in  UINT  Width,
    __in  UINT  Height
    )
{
    UINT CONST HBlank = Width / 16;
    UINT CONST VBlank = Height / 32;
    UINT CONST PixelClock = (Width + HBlank) * (Height + VBlank) * 60;
    UINT CONST EdidIndex = 54 + Index * 18;
    UINT CONST HSyncOffset = HBlank / 4;
    UINT CONST HSyncPulse = HBlank / 4;
    UINT CONST VSyncOffset = VBlank / 4;
    UINT CONST VSyncPulse = VBlank / 4;
    UINT CONST HDisplayMm = 510;        // hardcoded common monitor size
    UINT CONST VDisplayMm = 287;        // hardcoded common monitor size

    MyEDID[EdidIndex + 0]   = (PixelClock / 10000) & 0xFF;
    MyEDID[EdidIndex + 1]   = ((PixelClock / 10000) >> 8) & 0xFF;
    MyEDID[EdidIndex + 2]   = Width & 0xFF;   // Horizontal Active pixels
    MyEDID[EdidIndex + 3]   = HBlank & 0xFF;  // Horizontal Blanking pixels
    MyEDID[EdidIndex + 4]   = ((Width >> 8) & 0x0F) << 4 | (HBlank >> 8) & 0x0F;
    MyEDID[EdidIndex + 5]   = Height & 0xFF;  // Vertical Active Lines
    MyEDID[EdidIndex + 6]   = VBlank & 0xFF;  // Vertical blanking Lines
    MyEDID[EdidIndex + 7]   = ((Height >> 8) & 0x0F) << 4 | (VBlank >> 8) & 0x0F;
    MyEDID[EdidIndex + 8]   = HSyncOffset & 0xFF;   // Horizontal sync offset
    MyEDID[EdidIndex + 9]   = HSyncPulse & 0xFF;   // HSyncOffset Pulse
    MyEDID[EdidIndex + 10]  = ((VSyncOffset & 0x0F) << 4) | (VSyncPulse & 0x0F);
    MyEDID[EdidIndex + 11]  = ((HSyncOffset >> 8) & 0x03) << 6 |
                              ((HSyncPulse >> 8) & 0x03) << 4 |
                              ((VSyncOffset >> 4) & 0x03) << 2 |
                              ((VSyncPulse >> 4) & 0x03) << 0;
    MyEDID[EdidIndex + 12]  = HDisplayMm & 0xFF;
    MyEDID[EdidIndex + 13]  = VDisplayMm & 0xFF;
    MyEDID[EdidIndex + 14]  = ((HDisplayMm >> 8) & 0x0F) << 4 |
                              ((VDisplayMm >> 8) & 0x0F);
    MyEDID[EdidIndex + 15]  = 0;        // horizontal border
    MyEDID[EdidIndex + 16]  = 0;        // vertical border
    MyEDID[EdidIndex + 17]  = 0x1A;
}

#define TO_PNP(c)   (c - 64)
CONST UCHAR Mfg[3] = {
    TO_PNP('L'),
    TO_PNP('J'),
    TO_PNP('B'),
};
static VOID
SetEdid(
    __out UCHAR MyEDID[128]
    )
{
    UINT NumOfStdTiming;
    UINT NumOfDetailedTiming;
    UINT i;
    UCHAR checksum;

    // Copy EDID template to our MyEDID
    //
    NumOfStdTiming = 0;
    NumOfDetailedTiming = 0;

    MyEDID[8] = Mfg[0] << 2 | (Mfg[1] >> 3) & 3;
    MyEDID[9] = (Mfg[1] & 0x07) << 5 | (Mfg[2] & 0x1F);
    MyEDID[10] = 0x16;
    MyEDID[11] = 0x20;

    // Established timing bitmap. Supported bitmap for (formerly) very common timing modes.
    // Byte 35 Bit 0    800X600 @ 60 Hz
    //         Bit 5    640X480 @ 60 Hz
    // Byte 36 Bit 3    1024X768 @ 60 Hz
    //
    MyEDID[35] = Enable_800_600 << 0 | Enable_640_480 << 5;
    MyEDID[36] = Enable_1024_768 << 3;

    // Byte 38-53, Standard timing information. Up to 8 2-byte fields describing standard display modes.
    // Unused fields are filled with 01 01
    // Byte 0 = X resolution, divided by 8, less 31 (256-2288 pixels, value 00 is reserved and should not be used)
    // Byte 1 bits 7-6 = X:Y pixel ratio: 00=16:10; 01=4:3; 10=5:4; 11=16:9.
    // Byte 1 bits 5-0 = Vertical frequency, less 60 (60-123 Hz)
    // 1366x768 couldn't fit into standard timing information block. It goes to detailed timing descriptor.
    //
    if (Enable_1280_960)
    {
        //Ratio 4:3 @ 60Hz.
        MyEDID[38 + NumOfStdTiming * 2] = (1280/8) - 31;
        MyEDID[39 + NumOfStdTiming * 2] = (EDID_RATIO_4_3 << 6);
        NumOfStdTiming++;
        DBG_PRINT((__FUNCTION__ ":1280_960 enabled, NumOfStdTiming(%u)\n",
            NumOfStdTiming));
    }
    if (Enable_1280_1024)
    {
        //Ratio 5:4 @ 60Hz.
        MyEDID[38 + NumOfStdTiming * 2] = (1280/8) - 31;
        MyEDID[39 + NumOfStdTiming * 2] = (EDID_RATIO_5_4 << 6);
        NumOfStdTiming++;
        DBG_PRINT((__FUNCTION__ ":1280_1024 enabled, NumOfStdTiming(%u)\n",
            NumOfStdTiming));
    }
    if (Enable_1400_1050)
    {
        //Ratio 4:3 @ 60Hz.
        MyEDID[38 + NumOfStdTiming * 2] = (1400/8) - 31;
        MyEDID[39 + NumOfStdTiming * 2] = (EDID_RATIO_4_3 << 6);
        NumOfStdTiming++;
        DBG_PRINT((__FUNCTION__ ":1400_1050 enabled, NumOfStdTiming(%u)\n",
            NumOfStdTiming));
    }
    if (Enable_1440_900)
    {
        //Ratio 16:10 @ 60Hz.
        MyEDID[38 + NumOfStdTiming * 2] = (1440/8) - 31;
        MyEDID[39 + NumOfStdTiming * 2] = (EDID_RATIO_16_10 << 6);
        NumOfStdTiming++;
        DBG_PRINT((__FUNCTION__ ":1440_900 enabled, NumOfStdTiming(%u)\n",
            NumOfStdTiming));
    }
    if (Enable_1600_900)
    {
        //Ratio 16:9 @ 60Hz.
        MyEDID[38 + NumOfStdTiming * 2] = (1600/8) - 31;
        MyEDID[39 + NumOfStdTiming * 2] = (EDID_RATIO_16_9 << 6);
        NumOfStdTiming++;
        DBG_PRINT((__FUNCTION__ ":1600_900 enabled, NumOfStdTiming(%u)\n",
            NumOfStdTiming));
    }
    if (Enable_1680_1050)
    {
        //Ratio 16:10 @ 60Hz.
        MyEDID[38 + NumOfStdTiming * 2] = (1680/8) - 31;
        MyEDID[39 + NumOfStdTiming * 2] = (EDID_RATIO_16_10 << 6);
        NumOfStdTiming++;
        DBG_PRINT((__FUNCTION__ ":1680_1050 enabled, NumOfStdTiming(%u)\n",
            NumOfStdTiming));
    }
    if (Enable_1920_1080)
    {
        //Ratio 16:9 @ 60Hz.
        MyEDID[38 + NumOfStdTiming * 2] = (1920/8) - 31;
        MyEDID[39 + NumOfStdTiming * 2] = (EDID_RATIO_16_9 << 6);
        NumOfStdTiming++;
        DBG_PRINT((__FUNCTION__ ":1920_1080 enabled, NumOfStdTiming(%u)\n",
            NumOfStdTiming));
    }
    if (Enable_1920_1200)
    {
        //Ratio 16:10 @ 60Hz.
        MyEDID[38 + NumOfStdTiming * 2] = (1920/8) - 31;
        MyEDID[39 + NumOfStdTiming * 2] = (EDID_RATIO_16_10 << 6);
        NumOfStdTiming++;
        DBG_PRINT((__FUNCTION__ ":1920_1200 enabled, NumOfStdTiming(%u)\n",
            NumOfStdTiming));
    }

    while (NumOfStdTiming < 8)
    {
        MyEDID[38 + NumOfStdTiming * 2] = 1;
        MyEDID[39 + NumOfStdTiming * 2] = 1;
        DBG_PRINT((__FUNCTION__ ":StdTimingEntry[%u] unused\n", NumOfStdTiming));
        NumOfStdTiming++;
    }

    // Update EDID Detailed Timing Descriptor
    // 54-71 = Descriptor 1, 72-89 = Descriptor 2, 90-107 = Descriptor 3, 108-125 = Descriptor = 4
    for (i = 0; i < NUM_OF_DETAILED_TIMING; i++)
    {
        SetDetailTimingDesc(
            MyEDID,
            i,
            DetailedTimingTable[i].Width,
            DetailedTimingTable[i].Height
            );
        NumOfDetailedTiming++;
    }

    // Monitor range limits (required)
    // byte[0-1] = zero, indicating not a detailed timing descriptor
    // byte[2] = 0.
    // byte[3] = Descriptor type. 0xFD = Monitor Range Descriptor
    // byte[4] = zero.
    // byte[5] = Minimum vertical field rate (1-255 Hz)
    // byte[6] = Maximum vertical field rate (1-255 Hz)
    // byte[7] = Minimum horizontal line rate (1-255 kHz)
    // byte[8] = Maximum horizontal line rate (1-255 kHz)
    // byte[9] = Maximum pixel clock rate, rounded up to 10 MHz multiple (10-2550 MHz)
    // byte[10]= Extended timing information type:
    //           00: No information, padded with 0A 20 20 20 20 20 20.
    //           02: Secondary GTF supported, parameters as follows.
    // 00 01 02 03 04 05 06 07-08 09 0A 0B 0C 0D 0E 0F
    // ===============================================
    // 00 00 00 FD 00 38 4B 1F-53 0E 00 0A 20 20 20 20
    // 20 20
    MyEDID[54 + NumOfDetailedTiming*18 + 0] = 0;
    MyEDID[54 + NumOfDetailedTiming*18 + 1] = 0;
    MyEDID[54 + NumOfDetailedTiming*18 + 2] = 0;
    MyEDID[54 + NumOfDetailedTiming*18 + 3] = 0xFD;
    MyEDID[54 + NumOfDetailedTiming*18 + 4] = 0;
    MyEDID[54 + NumOfDetailedTiming*18 + 5] = 56;       //56 Hz
    MyEDID[54 + NumOfDetailedTiming*18 + 6] = 60;       //60 Hz
    MyEDID[54 + NumOfDetailedTiming*18 + 7] = 30;       //30 KHz
    MyEDID[54 + NumOfDetailedTiming*18 + 8] = 81;       //81 KHz
    MyEDID[54 + NumOfDetailedTiming*18 + 9] = 160;      //Maximum Pixel Clock = 160MHz
    MyEDID[54 + NumOfDetailedTiming*18 + 10]= 0x00;
    MyEDID[54 + NumOfDetailedTiming*18 + 11]= 0x0A;
    MyEDID[54 + NumOfDetailedTiming*18 + 12]= ' ';
    MyEDID[54 + NumOfDetailedTiming*18 + 13]= ' ';
    MyEDID[54 + NumOfDetailedTiming*18 + 14]= ' ';
    MyEDID[54 + NumOfDetailedTiming*18 + 15]= ' ';
    MyEDID[54 + NumOfDetailedTiming*18 + 16]= ' ';
    MyEDID[54 + NumOfDetailedTiming*18 + 17]= ' ';
    NumOfDetailedTiming++;
    DBG_PRINT((__FUNCTION__
        ":  Populate MonitorRangeLimits, NumOfDetailedTiming(%u)\n",
        NumOfDetailedTiming));

    // 0xFC: Monitor name (text), padded with 0A 20 20.
    // Let's have "LuminonCore"
    // 00 01 02 03 04 05 06 07-08 09 0A 0B 0C 0D 0E 0F
    // ===============================================
    // 00 00 00 FC 00 41 63 65-72 20 50 31 39 35 48 51
    // 4C 0A
    MyEDID[54 + NumOfDetailedTiming*18 + 0] = 0;
    MyEDID[54 + NumOfDetailedTiming*18 + 1] = 0;
    MyEDID[54 + NumOfDetailedTiming*18 + 2] = 0;
    MyEDID[54 + NumOfDetailedTiming*18 + 3] = 0xFC;
    MyEDID[54 + NumOfDetailedTiming*18 + 4] = 0;
    MyEDID[54 + NumOfDetailedTiming*18 + 5] = 'L';
    MyEDID[54 + NumOfDetailedTiming*18 + 6] = 'J';
    MyEDID[54 + NumOfDetailedTiming*18 + 7] = 'B';
    MyEDID[54 + NumOfDetailedTiming*18 + 8] = 'V';
    MyEDID[54 + NumOfDetailedTiming*18 + 9] = 'M';
    MyEDID[54 + NumOfDetailedTiming*18 + 10]= 'O';
    MyEDID[54 + NumOfDetailedTiming*18 + 11]= 'N';
    MyEDID[54 + NumOfDetailedTiming*18 + 12]= 0x0A;
    MyEDID[54 + NumOfDetailedTiming*18 + 13]= 0x20;
    MyEDID[54 + NumOfDetailedTiming*18 + 14]= 0x20;
    MyEDID[54 + NumOfDetailedTiming*18 + 15]= 0x20;
    MyEDID[54 + NumOfDetailedTiming*18 + 16]= 0x20;
    MyEDID[54 + NumOfDetailedTiming*18 + 17]= 0x20;
    NumOfDetailedTiming++;
    DBG_PRINT((
        __FUNCTION__
        ": Populated monitor name = 'LJBVMON', NumOfDetailedTiming(%u)\n",
        NumOfDetailedTiming));

    // Serial Number: "LJB2016"
    // 00 01 02 03 04 05 06 07-08 09 0A 0B 0C 0D 0E 0F
    // ===============================================
    // 00 00 00 FF 00 41 42 30-30 30 30 30 30 30 30 30,
    // 31 0A
    MyEDID[54 + NumOfDetailedTiming*18 + 0] = 0;
    MyEDID[54 + NumOfDetailedTiming*18 + 1] = 0;
    MyEDID[54 + NumOfDetailedTiming*18 + 2] = 0;
    MyEDID[54 + NumOfDetailedTiming*18 + 3] = 0xFF;
    MyEDID[54 + NumOfDetailedTiming*18 + 4] = 0;
    MyEDID[54 + NumOfDetailedTiming*18 + 5] = 'L';
    MyEDID[54 + NumOfDetailedTiming*18 + 6] = 'J';
    MyEDID[54 + NumOfDetailedTiming*18 + 7] = 'B';
    MyEDID[54 + NumOfDetailedTiming*18 + 8] = '2';
    MyEDID[54 + NumOfDetailedTiming*18 + 9] = '0';
    MyEDID[54 + NumOfDetailedTiming*18 + 10]= '1';
    MyEDID[54 + NumOfDetailedTiming*18 + 11]= '6';
    MyEDID[54 + NumOfDetailedTiming*18 + 12]= 0x0A;
    MyEDID[54 + NumOfDetailedTiming*18 + 13]= ' ';
    MyEDID[54 + NumOfDetailedTiming*18 + 14]= ' ';
    MyEDID[54 + NumOfDetailedTiming*18 + 15]= ' ';
    MyEDID[54 + NumOfDetailedTiming*18 + 16]= ' ';
    MyEDID[54 + NumOfDetailedTiming*18 + 17]= ' ';
    NumOfDetailedTiming++;
    DBG_PRINT((__FUNCTION__
        ": Serial Number='LJB2016', NumOfDetailedTiming(%u)\n",
        NumOfDetailedTiming));

    // Calculate Checksum
    //
    checksum = 0;
    for (i = 0; i < 127; i++)
    {
        checksum += MyEDID[i];
    }
    MyEDID[127] = -checksum;

    DUMP_BUF(MyEDID, 128);
}

static VOID
LJB_VMON_DrawCursorOnFrameBuffer(
    __in LJB_VMON_DEV_CTX *             dev_ctx,
    __in TARGET_MODE_DATA               *TargetModeData,
    __in VIDPN_SOURCE_VISIBILITY_DATA   *VisibilityData,
    __in POINTER_POSITION_DATA          *PointerPositionData,
    __in POINTER_SHAPE_DATA             *PointerShapeData,
    __out PVOID                         FrameBuffer
    )
{
    HANDLE CONST    hDefaultHeap = GetProcessHeap();
    UINT32 CONST    SurfaceWidth = TargetModeData->Width;
    UINT32 CONST    SurfaceHeight = TargetModeData->Height;
    UINT32 CONST    SurfacePitch = SurfaceWidth * 4;
    INT             CurPosX;
    INT             CurPosY;
    UINT            CurWidth;
    UINT            CurHeight;
    UINT            CurPitch;
    UINT            ShadowCursorWidth;
    UINT            ShadowCursorHeight;
    UCHAR *         pCurBitmap;
    UCHAR *         pSurfBitmap;
    UCHAR *         pFinalBlendCurBuffer;
    UCHAR *         pFinalBlendCurBufferStart;
    UCHAR *         pOrigSurfPos;
    UCHAR *         ShadowBitmapPosition;
    UINT            row;

    CurPosX = PointerPositionData->X;
    CurPosY = PointerPositionData->Y;

    /*
     * If the target monitor is not visible, do not bother to draw cursor.
     */
    if (!VisibilityData->Visible)
        return;

    /*
     * If cursor is not visible, do not bother to draw cursor.
     */
    if (!PointerPositionData->Visible)
        return;

    /*
     * sanity check
     */
    if (CurPosX > 0)
    {
       if ((UINT)CurPosX >= SurfaceWidth)
           return;
    }
    if (CurPosY > 0)
    {
       if ((UINT)CurPosY >= SurfaceHeight)
           return;
    }

    CurWidth    = PointerShapeData->Width;
    CurHeight   = PointerShapeData->Height;
    CurPitch    = PointerShapeData->Pitch;

    ShadowCursorWidth = CurWidth;
    ShadowCursorHeight= CurHeight;
    pCurBitmap  = PointerShapeData->Buffer;

    // Calculate effective cursor Width/Height/Position.
    //
    if (CurPosX >= 0)
    {
        if ((CurPosX + CurWidth) >= (SurfaceWidth - 1))
            ShadowCursorWidth = SurfaceWidth - 1 - CurPosX;
    }
    else
    {
        ShadowCursorWidth = CurWidth + CurPosX;
        CurPosX = 0;
    }

    if (CurPosY >= 0)
    {
        if ((CurPosY + CurHeight) >= (SurfaceHeight - 1))
            ShadowCursorHeight = SurfaceHeight - 1 - CurPosY;
    }
    else
    {
        ShadowCursorHeight = CurHeight + CurPosY;
        CurPosY = 0;
    }

    if (ShadowCursorWidth == 0 || ShadowCursorHeight == 0 || ShadowCursorWidth > CurWidth || ShadowCursorHeight > CurHeight)
    {
        return;
    }

    pFinalBlendCurBufferStart = HeapAlloc(
        hDefaultHeap,
        HEAP_ZERO_MEMORY,
        ShadowCursorWidth * ShadowCursorHeight * 4
        );

    if (pFinalBlendCurBufferStart == NULL)
    {
        DBG_PRINT((__FUNCTION__
            ":unable to allocate pFinalBlendCurBufferStart?\n"));
        return;
    }

    pFinalBlendCurBuffer = pFinalBlendCurBufferStart;
    ShadowBitmapPosition= (UCHAR *)FrameBuffer + CurPosY * SurfacePitch + CurPosX * 4;
    pOrigSurfPos = ShadowBitmapPosition;

    // http://msdn.microsoft.com/en-us/library/windows/hardware/ff559481(v=vs.85).aspx
    // 1. Monochrome cursor:
    // A monochrome bitmap whose size is specified by Width and Height in a 1 bits
    // per pixel (bpp) DIB format AND mask that is followed by another 1 bpp DIB
    // format XOR mask of the same size.
    //
    if (PointerShapeData->Flags.Value == 1)
    {
        UCHAR * pANDMask;
        UCHAR * pXORMask;
        UINT iByte;
        UINT iBit;

        for(row = 0; row < ShadowCursorHeight; row++)
        {
            pANDMask = pCurBitmap;
            pXORMask = pCurBitmap + CurHeight * CurPitch;
            for (iByte = 0, iBit = 7; iByte < ShadowCursorWidth * 4; iByte +=4, iBit--)
                {
                // AND first and then XOR.
                //
                if ((*pANDMask >> iBit) & 1)
                {
                    *((ULONG *) (pFinalBlendCurBuffer + iByte)) =
                        *((ULONG *) (pOrigSurfPos + iByte)) & 0xFFFFFFFF;
                }
                else
                {
                    *((ULONG *) (pFinalBlendCurBuffer + iByte)) =
                        *((ULONG *) (pOrigSurfPos + iByte)) & 0;
                }

                if ((*pXORMask >> iBit) & 1)
                {
                    *((ULONG *) (pFinalBlendCurBuffer + iByte)) ^= 0xFFFFFFFF;
                }
                else
                {
                    *((ULONG *) (pFinalBlendCurBuffer + iByte)) ^= 0;
                }

                if (iBit == 0)
                {
                    pANDMask++;
                    pXORMask++;
                    iBit = 8;
                }
            }
            pOrigSurfPos += SurfaceWidth * 4;
            pFinalBlendCurBuffer += ShadowCursorWidth * 4;
            pCurBitmap += CurPitch;
        }
    }
    else
    {
        // When the cursor is on the boundary of the screen, CurPosX/CurPosY
        // can be negative. So the start address of the cursor bitmap need to
        // be re-calculated for these special cases.
        //
        UCHAR CurA;
        SIZE_T AbsCurPosX;
        SIZE_T AbsCurPosY;
        UINT i;

        CurPosX = PointerPositionData->X;
        CurPosY = PointerPositionData->Y;

        AbsCurPosX = abs(CurPosX);
        AbsCurPosY = abs(CurPosY);

        if (CurPosX < 0 && CurPosY < 0)
        {
            pCurBitmap += AbsCurPosY * CurPitch + AbsCurPosX * 4;
        }

        else if (CurPosX < 0)
        {
            pCurBitmap += AbsCurPosX * 4;
        }

        else if (CurPosY < 0)
        {
            pCurBitmap += AbsCurPosY * CurPitch;
        }

        // 2. Color cursor:
        // A color bitmap whose size is specified by Width and Height in a 32 bpp
        // ARGB device independent bitmap (DIB) format.
        //
        if (PointerShapeData->Flags.Value == 2)
        {
            UCHAR CurR, CurG, CurB;
            UCHAR SurfR, SurfG, SurfB;

            for(row = 0; row < ShadowCursorHeight; ++row)
            {
                for (i = 0; i < ShadowCursorWidth * 4; i +=4)
                {
                    SurfB = pOrigSurfPos[i];
                    SurfG = pOrigSurfPos[i + 1];
                    SurfR = pOrigSurfPos[i + 2];

                    CurB = pCurBitmap[i];
                    CurG = pCurBitmap[i + 1];
                    CurR = pCurBitmap[i + 2];
                    CurA = pCurBitmap[i + 3];

                    pFinalBlendCurBuffer[i] = SurfB + (((CurB - SurfB) * CurA) / 255);
                    pFinalBlendCurBuffer[i + 1] = SurfG + (((CurG - SurfG) * CurA) / 255);
                    pFinalBlendCurBuffer[i + 2] = SurfR + (((CurR - SurfR) * CurA) / 255);
                    pFinalBlendCurBuffer[i + 3] = pOrigSurfPos[i + 3];
                }
                pOrigSurfPos += SurfaceWidth * 4;
                pCurBitmap += CurPitch;
                pFinalBlendCurBuffer += ShadowCursorWidth * 4;
            }
        }

        // 3. Masked color cursor:
        // A 32-bpp ARGB format bitmap with the mask value in the alpha bits. The only
        // allowed mask values are 0 and 0xFF. When the mask value is 0, the RGB value
        // should replace the screen pixel. When the mask value is 0xFF, an XOR operation
        // is performed on the RGB value and the screen pixel; the result should replace
        // the screen pixel.
        //
        if (PointerShapeData->Flags.Value == 4)
        {
            for(row = 0; row < ShadowCursorHeight; ++row)
            {
                for (i = 0; i < ShadowCursorWidth * 4; i +=4)
                {
                    CurA = pCurBitmap[i + 3];
                    if (CurA == 0)
                    {
                        *((ULONG *) (pFinalBlendCurBuffer + i)) =
                            *((ULONG *) (pCurBitmap + i));
                    }
                    else
                    {
                        *((ULONG *) (pFinalBlendCurBuffer + i)) =
                            *((ULONG *) (pOrigSurfPos + i)) ^ *((ULONG *) (pCurBitmap + i));
                    }
                    pFinalBlendCurBuffer[i+3] = pOrigSurfPos[i+3];
                }
                pCurBitmap += CurPitch;
                pOrigSurfPos += SurfaceWidth * 4;
                pFinalBlendCurBuffer += ShadowCursorWidth *4;
            }
        }
    }

    // Back up the replaced section of PrimarySurface before putting the final
    // cursor image on the PrimarySurface.
    //
    dev_ctx->ShadowBitmapPosition = ShadowBitmapPosition;
    dev_ctx->ShadowCursorWidth = ShadowCursorWidth;
    dev_ctx->ShadowCursorHeight = ShadowCursorHeight;
    dev_ctx->FrameBufferIsDirty = TRUE;
    pSurfBitmap = dev_ctx->ShadowBitmapBuffer;

    pOrigSurfPos = ShadowBitmapPosition;
    pFinalBlendCurBuffer = pFinalBlendCurBufferStart;
    for(row = 0; row < ShadowCursorHeight; ++row)
    {
        // Back up original PrimarySurface.
        //
        RtlCopyMemory(
            pSurfBitmap,
            pOrigSurfPos,
            ShadowCursorWidth * 4
            );
        pSurfBitmap += ShadowCursorWidth * 4;

        //Start filling cursor.
        //
        RtlCopyMemory(
            pOrigSurfPos,
            pFinalBlendCurBuffer,
            ShadowCursorWidth * 4
            );
        pOrigSurfPos += SurfaceWidth * 4;
        pFinalBlendCurBuffer += ShadowCursorWidth * 4;
    }

    HeapFree(hDefaultHeap, 0 , pFinalBlendCurBufferStart);
}

static VOID
LJB_VMON_RestoreFrameBuffer(
    __in LJB_VMON_DEV_CTX *             dev_ctx,
    __in TARGET_MODE_DATA               *TargetModeData,
    __out PVOID                         FrameBuffer
    )
{
    UINT    row;
    UINT    ShadowCursorWidth;
    UINT    ShadowCursorHeight;
    UCHAR * pSurfBitmap;
    UCHAR * pOrigSurfPos;

    if (!dev_ctx->FrameBufferIsDirty)
        return;

    pOrigSurfPos = dev_ctx->ShadowBitmapPosition;
    pSurfBitmap = dev_ctx->ShadowBitmapBuffer;
    ShadowCursorWidth = dev_ctx->ShadowCursorWidth;
    ShadowCursorHeight = dev_ctx->ShadowCursorHeight;

    for(row = 0; row < ShadowCursorHeight; ++row)
        {
        RtlCopyMemory(
            pOrigSurfPos,
            pSurfBitmap,
            ShadowCursorWidth * 4
            );

        pOrigSurfPos += TargetModeData->Width * 4;
        pSurfBitmap += ShadowCursorWidth * 4;
        }
    dev_ctx->FrameBufferIsDirty = FALSE;
}

