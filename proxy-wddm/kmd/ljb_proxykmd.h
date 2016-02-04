/*
 * ljb_proxykmd.h
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _LJB_PROXYKMD_H_
#define _LJB_PROXYKMD_H_

#include <ntddk.h>
#include <dispmprt.h>

/*
 * C/CPP linkage macro.
 */
#ifdef __cplusplus
#define _C_BEGIN extern "C" {
#define _C_END   }
#else
#define _C_BEGIN
#define _C_END
#endif

#pragma warning(disable:4201) /* allow nameless struct/union */

#define DXGK_SVC_NAME   L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\DXGKrnl"
#define DXGK_DEV_NAME   L"\\Device\\DxgKrnl"

#define DXGKRNL_SYS_STR                 L"dxgkrnl.sys"
#define USER_MODE_DRIVER_NAME           L"UserModeDriverName"
#define USER_MODE_DRIVER_NAME_WOW       L"UserModeDriverNameWow"

#define MY_USER_MODE_DRIVER_NAME        L"ljb_umd.dll\0ljb_umd.dll\0ljb_umd.dll\0"
#define MY_USER_MODE_DRIVER_NAME_WOW    L"ljb_umd32.dll\0ljb_umd32.dll\0ljb_umd32.dll\0"

#define IOCTL_GET_DXGK_INITIALIZE_WIN7          0x23003F
#define IOCTL_GET_DXGK_INITIALIZE_DISPLAY_ONLY  0x230043
#define IOCTL_GET_DXGK_INITIALIZE_WIN8          0x230047

#define USB_MONITOR_MAX                 6

/*
 * PoolTag macro
 */
#define _MAKE_POOLTAG(d, c, b, a)       \
    ((a << 24) | (b << 16) | (c << 8) | (d))

#define LJB_POOL_TAG    _MAKE_POOLTAG('L', 'J', 'B', ' ')

__checkReturn
PVOID
FORCEINLINE
LJB_PROXYKMD_GetPoolZero(
    __in SIZE_T NumberOfBytes
    )
    {
    PVOID   Buffer;

    Buffer = ExAllocatePoolWithTag(
        NonPagedPoolNx,
        NumberOfBytes,
        LJB_POOL_TAG
        );
    if (Buffer != NULL)
        RtlZeroMemory(Buffer, NumberOfBytes);
    return Buffer;
    }

#define LJB_PROXYKMD_FreePool(p) ExFreePoolWithTag(p, LJB_POOL_TAG)


/*
 * Debug Print macro.
 * To enable debugging message, set DEFAULT registry value
 * to 1 under "Debug Print Filter", and set DebugMask registry
 * value under driver's software key.
 */
#define DBGLVL_ERROR        (1 << 0)
#define DBGLVL_PNP          (1 << 1)
#define DBGLVL_POWER        (1 << 2)
#define DBGLVL_FLOW         (1 << 3)
#define DBGLVL_INFO         (1 << 4)
#define DBGLVL_ALLOCATION   (1 << 5)
#define DBGLVL_PRESENT      (1 << 6)
#define DBGLVL_FENCEID      (1 << 7)
#define DBGLVL_VIDPN        (1 << 8)
#define DBGLVL_VSYNC        (1 << 9)
#define DBGLVL_DEFAULT      (DBGLVL_ERROR | DBGLVL_PNP | DBGLVL_POWER)

#if (DBG)
#define DBG_PRINT(adapter, mask, arg)           \
    if (adapter->DebugMask & mask)              \
    {                                           \
        DbgPrint arg;                           \
    }
#else
#define DBG_PRINT(adapter, mask, arg)
#endif

typedef enum _DEVICE_TYPE
{
    DEVICE_TYPE_INVALID,
    DEVICE_TYPE_FDO,
    DEVICE_TYPE_FILTER
}   DEVICE_TYPE;

typedef enum _PNP_STATE
{
    PNP_NOT_STARTED,
    PNP_STARTED,
    PNP_STOP_PENDING,
    PNP_STOPPED,
    PNP_REMOVE_PENDING,
    PNP_SURPRISE_REMOVE_PENDING,
    PNP_DELETED
}   PNP_STATE;

#define INITIALIZE_PNP_STATE(_Data_)    \
        (_Data_)->DevicePnPState =  PNP_NOT_STARTED;\
        (_Data_)->PreviousPnPState = PNP_NOT_STARTED;

#define SET_NEW_PNP_STATE(_Data_, _state_) \
        (_Data_)->PreviousPnPState =  (_Data_)->DevicePnPState;\
        (_Data_)->DevicePnPState = (_state_);

#define RESTORE_PREVIOUS_PNP_STATE(_Data_)   \
        (_Data_)->DevicePnPState =   (_Data_)->PreviousPnPState;\


typedef struct _LJB_DEVICE_EXTENSION
{
    DEVICE_OBJECT *                     DeviceObject;
    DEVICE_OBJECT *                     NextLowerDriver;
    DEVICE_OBJECT *                     PhysicalDeviceObject;
    DEVICE_OBJECT *                     FilterDeviceObject;
    UNICODE_STRING                      InterfaceName;
    RTL_OSVERSIONINFOW                  RtlOsVersion;

    DEVICE_TYPE                         DeviceType;
    PNP_STATE                           DevicePnPState;
    PNP_STATE                           PreviousPnPState;
    IO_REMOVE_LOCK                      RemoveLock;

    PFILE_OBJECT                        DxgkFileObject;

    ULONG                               DebugMask;
}   LJB_DEVICE_EXTENSION;

typedef NTSTATUS
DXGK_INTIALIZE(
    __in PDRIVER_OBJECT                 DriverObject,
    __in PUNICODE_STRING                RegistryPath,
    __in PDRIVER_INITIALIZATION_DATA    DriverInitializationData
    );
typedef DXGK_INTIALIZE *PFN_DXGK_INITIALIZE;

typedef struct _LJB_DRIVER_BINDING_TAG
{
    LIST_ENTRY                              ListEntry;
    ULONG                                   DxgkAddDeviceTag;
} LJB_DRIVER_BINDING_TAG;

typedef struct _LJB_GLOBAL_DRIVER_DATA
{
    DEVICE_OBJECT *                     DeviceObject;
    PDRIVER_OBJECT                      DriverObject;
    ULONG                               DebugMask;

    RTL_OSVERSIONINFOW                  RtlOsVersion;
    PFN_DXGK_INITIALIZE                 DxgkInitializeWin7;
    PFN_DXGK_INITIALIZE                 DxgkInitializeWin8;

    LIST_ENTRY                          ClientDriverListHead;
    LONG                                ClientDriverListCount;
    KSPIN_LOCK                          ClientDriverListLock;

    LIST_ENTRY                          ClientAdapterListHead;
    LONG                                ClientAdapterListCount;
    KSPIN_LOCK                          ClientAdapterListLock;

    LJB_DRIVER_BINDING_TAG              DriverBindingPool[4];
    LIST_ENTRY                          DriverBindingHead;
    LONG                                DriverBindingCount;
    KSPIN_LOCK                          DriverBindingLock;

}   LJB_GLOBAL_DRIVER_DATA;

extern LJB_GLOBAL_DRIVER_DATA   GlobalDriverData;

typedef struct _LJB_CLIENT_DRIVER_DATA
{
    LIST_ENTRY                              ListEntry;
    DRIVER_OBJECT *                         DriverObject;
    UNICODE_STRING                          RegistryPath;
    WCHAR                                   RegistryPathBuffer[MAX_PATH];
    ULONG                                   DxgkAddDeviceTag;
    LJB_DRIVER_BINDING_TAG *                DriverBindingTag;
    LONG                                    ReferenceCount;
    DRIVER_INITIALIZATION_DATA              DriverInitData;
}   LJB_CLIENT_DRIVER_DATA;

typedef struct _LJB_ADAPTER
{
    LIST_ENTRY                              ListEntry;
    PDEVICE_OBJECT                          PhysicalDeviceObject;
    PVOID                                   hAdapter;
    LJB_CLIENT_DRIVER_DATA *                ClientDriverData;
    ULONG                                   DebugMask;

    /*
     * information obtained from DxgkDdiStartDevice
     */
    DXGK_START_INFO                         DxgkStartInfo;
    DXGKRNL_INTERFACE                       DxgkInterface;
    ULONG                                   NumberOfVideoPresentSources;
    ULONG                                   NumberOfChildren;
    USHORT                                  PciVendorId;

    /*
     * information obtained from DxgkDdiQueryChildRelations
     */
    D3DDDI_VIDEO_PRESENT_TARGET_ID          UsbTargetIdBase;
    ULONG                                   ActualNumberOfChildren;


}   LJB_ADAPTER;

/*
 * C function declaration
 */
_C_BEGIN

DRIVER_INITIALIZE           DriverEntry;
DRIVER_ADD_DEVICE           LJB_PROXYKMD_AddDevice;
DRIVER_DISPATCH             LJB_PROXYKMD_DispatchPnp;
DRIVER_DISPATCH             LJB_PROXYKMD_DispatchPower;
DRIVER_DISPATCH             LJB_PROXYKMD_DispatchCreate;
DRIVER_DISPATCH             LJB_PROXYKMD_DispatchClose;
DRIVER_DISPATCH             LJB_PROXYKMD_DispatchInternalIoctl;
DRIVER_DISPATCH             LJB_PROXYKMD_DispatchIoctl;
DRIVER_DISPATCH             LJB_PROXYKMD_PassDown;
DRIVER_UNLOAD               LJB_PROXYKMD_Unload;

DXGK_INTIALIZE              LJB_DXGK_InitializeWin7;
DXGK_INTIALIZE              LJB_DXGK_InitializeWin8;
DXGKDDI_ADD_DEVICE          LJB_DXGK_AddDevice0;
DXGKDDI_ADD_DEVICE          LJB_DXGK_AddDevice1;
DXGKDDI_ADD_DEVICE          LJB_DXGK_AddDevice2;
DXGKDDI_ADD_DEVICE          LJB_DXGK_AddDevice3;
DXGKDDI_START_DEVICE        LJB_DXGK_StartDevice;

NTSTATUS
LJB_PROXYKMD_PassDown (
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP Irp
    );

LJB_ADAPTER *
LJB_DXGK_FindAdapterByDriverAdapter(
    __in PVOID hAdapter
    );
#define FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter) LJB_DXGK_FindAdapterByDriverAdapter(hAdapter)

_C_END

#endif