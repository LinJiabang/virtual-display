#define INITGUID
#include <windows.h>
#include <winioctl.h>
#include <strsafe.h>
#include <setupapi.h>
#include <stdio.h>
#include <stdlib.h>

#include "ljb_proxykmd_ioctl.h"
#include "ljb_proxykmd_guid.h"

PCHAR
TEST_GetDevicePath(
    __in LPGUID InterfaceGuid
    )
{
    HDEVINFO HardwareDeviceInfo;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetailData = NULL;
    ULONG Length, RequiredLength = 0;
    BOOL bResult;

    HardwareDeviceInfo = SetupDiGetClassDevs(
                InterfaceGuid,
                NULL,
                NULL,
                (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));

    if (HardwareDeviceInfo == INVALID_HANDLE_VALUE)
    {
        printf("SetupDiGetClassDevs failed!\n");
        exit(1);
    }

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
            printf("Error: %s", (LPSTR)lpMsgBuf);
            LocalFree(lpMsgBuf);
        }

        printf("SetupDiEnumDeviceInterfaces failed.\n");
        SetupDiDestroyDeviceInfoList(HardwareDeviceInfo);
        exit(1);
        }

    SetupDiGetDeviceInterfaceDetail(
        HardwareDeviceInfo,
        &DeviceInterfaceData,
        NULL,
        0,
        &RequiredLength,
        NULL
        );

    DeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) LocalAlloc(LMEM_FIXED, RequiredLength);
    if (DeviceInterfaceDetailData == NULL)
    {
        SetupDiDestroyDeviceInfoList(HardwareDeviceInfo);
        printf("Failed to allocate memory.\n");
        exit(1);
    }

    DeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    Length = RequiredLength;
    bResult = SetupDiGetDeviceInterfaceDetail(
            HardwareDeviceInfo,
            &DeviceInterfaceData,
            DeviceInterfaceDetailData,
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

        printf( "SetupDiGetDeviceInterfaceDetail failed. Error: %s\n",
            (LPCTSTR) lpMsgBuf);

        LocalFree(lpMsgBuf);
        SetupDiDestroyDeviceInfoList(HardwareDeviceInfo);
        LocalFree(DeviceInterfaceDetailData);
        exit(1);
    }

    return DeviceInterfaceDetailData->DevicePath;
}


VOID
__cdecl
main(
    __in int argc,
    __in_ecount(argc) PSTR argv[]
    )
{
    ULONG               BytesReturned;
    HANDLE              hDevice;
    PCHAR               DevicePath;
    BOOLEAN             EnableFakeMonitor;
    BOOL                bRet;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s [attach | detach]\n",
            argv[0]
            );
        return;
    }
    if (_stricmp(argv[1], "attach") == 0)
    {
        EnableFakeMonitor = TRUE;
    }
    else if (_stricmp(argv[1], "detach") == 0)
    {
        EnableFakeMonitor = FALSE;
    }
    else
    {
        fprintf(stderr, "Usage: %s [attach | detach]\n",
            argv[0]
            );
        return;
    }

    DevicePath = TEST_GetDevicePath(
        (LPGUID)&LJB_PROXYKMD_INTERFACE_GUID
        );
    printf("DevicePath: %s\n", DevicePath);

    hDevice = CreateFile(
        DevicePath,
        GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
        );
    if (hDevice == INVALID_HANDLE_VALUE)
    {
        printf("Failed to open device. Error %d\n",GetLastError());
        exit(1);
    }

    printf("Opened device successfully\n");

    bRet = DeviceIoControl(
        hDevice,
        EnableFakeMonitor ? IOCTL_PROXYKMD_PLUGIN_FAKE_MONITOR : IOCTL_PROXYKMD_UNPLUG_FAKE_MONITOR,
        NULL,
        0,
        NULL,
        0,
        &BytesReturned,
        NULL
        );
    if (bRet == 0)
    {
        printf("Failed to %s device, Error %d\n",
        EnableFakeMonitor ? "attach" : "detach",
        GetLastError());
        exit(1);
    }

    /*
     * close
     */
    CloseHandle(hDevice);
    exit(0);
}