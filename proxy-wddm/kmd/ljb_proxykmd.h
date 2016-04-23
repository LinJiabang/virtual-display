/*
 * ljb_proxykmd.h
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */

#ifndef _LJB_PROXYKMD_H_
#define _LJB_PROXYKMD_H_

#include <ntddk.h>
#include <dispmprt.h>
#include "ljb_monitor_interface.h"

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

#define MY_USER_MODE_DRIVER_NAME            L"ljb_umd.dll\0"
#define MY_USER_MODE_DRIVER_NAME_WOW        L"ljb_umd32.dll\0"
#define MY_USER_MODE_DRIVER_NAME_FULL       L"ljb_umd.dll\0ljb_umd.dll\0ljb_umd.dll\0"
#define MY_USER_MODE_DRIVER_NAME_WOW_FULL   L"ljb_umd32.dll\0ljb_umd32.dll\0ljb_umd32.dll\0"
#define NUM_OF_UMD_ENTRIES                  3

#define IOCTL_GET_DXGK_INITIALIZE_WIN7          0x23003F
#define IOCTL_GET_DXGK_INITIALIZE_DISPLAY_ONLY  0x230043
#define IOCTL_GET_DXGK_INITIALIZE_WIN8          0x230047

#define MAX_NUM_OF_USB_MONITOR          6
#define MAX_NUM_OF_INBOX_MONITOR        64
#define MAX_NUM_OF_SEGMENT_DESC         16
#define MAX_NUM_OF_POWER_COMPONENTS     32
#define MAX_NUM_OF_NODE                 32
#define MAX_NUM_OF_ENGINE               8

/*
 * PoolTag macro
 */
#define _MAKE_POOLTAG(d, c, b, a)       \
    ((a << 24) | (b << 16) | (c << 8) | (d))

#define LJB_POOL_TAG    _MAKE_POOLTAG('L', 'J', 'B', ' ')

__checkReturn
PVOID
FORCEINLINE
LJB_GetPoolZero(
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

#define LJB_FreePool(p) ExFreePoolWithTag(p, LJB_POOL_TAG)


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
#define DBGLVL_DEFAULT      (DBGLVL_ERROR | DBGLVL_PNP | DBGLVL_POWER | DBGLVL_INFO | DBGLVL_FLOW | DBGLVL_PRESENT)

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
    LARGE_INTEGER                       RegistryCallbackCookie;

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
    LIST_ENTRY                          ListEntry;
    ULONG                               DxgkAddDeviceTag;
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

    LIST_ENTRY                          ClientDeviceListHead;
    LONG                                ClientDeviceListCount;
    KSPIN_LOCK                          ClientDeviceListLock;

    LIST_ENTRY                          ClientContextListHead;
    LONG                                ClientContextListCount;
    KSPIN_LOCK                          ClientContextListLock;

    LJB_DRIVER_BINDING_TAG              DriverBindingPool[4];
    LIST_ENTRY                          DriverBindingHead;
    LONG                                DriverBindingCount;
    KSPIN_LOCK                          DriverBindingLock;

}   LJB_GLOBAL_DRIVER_DATA;

extern LJB_GLOBAL_DRIVER_DATA   GlobalDriverData;

typedef struct _LJB_CLIENT_DRIVER_DATA
{
    LIST_ENTRY                          ListEntry;
    DRIVER_OBJECT *                     DriverObject;
    UNICODE_STRING                      RegistryPath;
    WCHAR                               RegistryPathBuffer[MAX_PATH];
    ULONG                               DxgkAddDeviceTag;
    LJB_DRIVER_BINDING_TAG *            DriverBindingTag;
    LONG                                ReferenceCount;
    DRIVER_INITIALIZATION_DATA          DriverInitData;
}   LJB_CLIENT_DRIVER_DATA;

typedef struct _LJB_ENGINE_INFO
{
    ULONGLONG                           DependentNodeOrdinalMask;
    DXGK_ENGINESTATUS                   EngineStatus;
    ULONG                               LastSubmittedFenceId;
    ULONG                               LastCompletedFenceId;
    ULONG                               LastPreemptedFenceId;
    ULONG                               LastAbortedFenceId;
    DXGKARG_CALIBRATEGPUCLOCK           CalibrateGpuClock;
}   LJB_ENGINE_INFO;

typedef struct _LJB_ADAPTER
{
    LIST_ENTRY                          ListEntry;
    PDEVICE_OBJECT                      PhysicalDeviceObject;
    PVOID                               hAdapter;
    LJB_CLIENT_DRIVER_DATA *            ClientDriverData;
    ULONG                               DebugMask;

    /*
     * Test purpose only
     */
    BOOLEAN                             FakeMonitorEnabled;

    /*
     * UserModeDriverName hacking
     */
    KEY_NAME_INFORMATION                DriverKeyNameInfo;
    WCHAR                               DriverKeyNameBuffer0[MAX_PATH];
    WCHAR *                             DriverKeyNameBuffer;

    WCHAR                               UserModeDriverName[MAX_PATH];
    WCHAR                               UserModeDriverNameWow[MAX_PATH];
    ULONG                               UserModeDriverNameSize;
    ULONG                               UserModeDriverNameWowSize;

    /*
     * information obtained from DxgkDdiStartDevice
     */
    DXGK_START_INFO                     DxgkStartInfo;
    DXGKRNL_INTERFACE                   DxgkInterface;
    ULONG                               NumberOfVideoPresentSources;
    ULONG                               NumberOfChildren;
    USHORT                              PciVendorId;

    /*
     * USB MONITOR PNP bookkeeping stuff
     */
    LIST_ENTRY                          MonitorNodeListHead;
    LONG                                MonitorNodeListCount;
    KSPIN_LOCK                          MonitorNodeListLock;
    PVOID                               NotificationHandle;
    ULONG                               MonitorNodeMask;

    /*
     * information obtained from DxgkDdiQueryChildRelations/DxgkDdiQueryChildStatus
     */
    DXGK_CHILD_DESCRIPTOR               ChildRelations[MAX_NUM_OF_INBOX_MONITOR];
    BOOLEAN                             ChildConnectionStatus[MAX_NUM_OF_INBOX_MONITOR];
    D3DDDI_VIDEO_PRESENT_TARGET_ID      UsbTargetIdBase;
    ULONG                               ActualNumberOfChildren;

    /*
     * information obtained from DxgkDdiQueryAdapterInfo
     */
    DXGK_DRIVERCAPS                     DriverCaps;
    DXGK_QUERYSEGMENTOUT                SegmentOut;
    DXGK_SEGMENTDESCRIPTOR              SegmentDescriptors[MAX_NUM_OF_SEGMENT_DESC];

    /*
     * allocations created by DxgkDdiCreateAllocations
     */
    LIST_ENTRY                          AllocationListHead;
    KSPIN_LOCK                          AllocationListLock;
    LONG                                AllocationListCount;

    LIST_ENTRY                          OpenedAllocationListHead;
    KSPIN_LOCK                          OpenedAllocationListLock;
    LONG                                OpenedAllocationListCount;

    /*
     * Standard allocation Data
     */
    LIST_ENTRY                          StdAllocationInfoListHead;
    KSPIN_LOCK                          StdAllocationInfoListLock;
    LONG                                StdAllocationInfoListCount;

    /*
     * Aperture Mapping entry
     */
    LIST_ENTRY                          ApertureMappingListHead;
    KSPIN_LOCK                          ApertureMappingListLock;
    LONG                                ApertureMappingListCount;
    /*
     * track power component Fstate
     */
    UINT                                FState[MAX_NUM_OF_POWER_COMPONENTS];

    /*
     * Engine information
     */
    LJB_ENGINE_INFO                     EngineInfo[MAX_NUM_OF_NODE][MAX_NUM_OF_ENGINE];
    DXGKARG_GETNODEMETADATA             NodeMetaData[MAX_NUM_OF_NODE];

    /*
     * VidPn related
     */
    BOOLEAN                             FirstVidPnArrived;
    DXGKARG_COMMITVIDPN                 LastCommitVidPn;
    SIZE_T                              NumPathsCommitted;
    D3DKMDT_VIDPN_PRESENT_PATH          PathsCommitted[MAX_NUM_OF_INBOX_MONITOR+MAX_NUM_OF_USB_MONITOR];
    SIZE_T                              PrevNumPathsCommitted;
    D3DKMDT_VIDPN_PRESENT_PATH          PrevPathsCommitted[MAX_NUM_OF_INBOX_MONITOR+MAX_NUM_OF_USB_MONITOR];

}   LJB_ADAPTER;

/*
 * created by DxgkDdiCreateDevice
 */
typedef struct _LJB_DEVICE
{
    LIST_ENTRY              ListEntry;
    LJB_ADAPTER *           Adapter;
    HANDLE                  hRTDevice;
    DXGKARG_CREATEDEVICE    CreateDevice;

    /*
     * output from miniport driver
     */
    HANDLE                  hDevice;        // miniport driver output
    DXGK_DEVICEINFO         DeviceInfo;
} LJB_DEVICE;

/*
 * created by DxgkDdiCreateContext
 */
typedef struct _LJB_CONTEXT
{
    LIST_ENTRY              ListEntry;
    LJB_ADAPTER *           Adapter;
    LJB_DEVICE *            MyDevice;
    DXGKARG_CREATECONTEXT   CreateContext;

    /*
     * output from miniport driver
     */
    HANDLE                  hContext;        // miniport driver output
    DXGK_CONTEXTINFO        ContextInfo;
} LJB_CONTEXT;

typedef struct _LJB_STD_ALLOCATION_INFO
{
    LIST_ENTRY                              ListEntry;
    DXGKARG_GETSTANDARDALLOCATIONDRIVERDATA DriverData;
    D3DKMDT_SHAREDPRIMARYSURFACEDATA        PrimarySurfaceData;
    D3DKMDT_SHADOWSURFACEDATA               ShadowSurfaceData;
} LJB_STD_ALLOCATION_INFO;

extern CONST CHAR * StdAllocationTypeString[];

typedef struct _LJB_ALLOCATION
{
    LIST_ENTRY                          ListEntry;
    LJB_ADAPTER *                       Adapter;
    HANDLE                              hAllocation;
    DXGK_ALLOCATIONINFO                 AllocationInfo;
    LJB_STD_ALLOCATION_INFO *           StdAllocationInfo;

    D3DDDI_VIDEO_PRESENT_SOURCE_ID      VidPnSourceId;
    PVOID                               KmBuffer;
    ULONG                               KmBufferSize;

    PMDL                                UmMdl;
    PVOID                               UmBuffer;
    ULONG                               UmBufferSize;
} LJB_ALLOCATION;

typedef struct _LJB_OPENED_ALLOCATION
{
    LIST_ENTRY                          ListEntry;
    D3DKMT_HANDLE                       hKmHandle;
    HANDLE                              hDeviceSpecificAllocation;
    HANDLE                              hAllocation;
    LJB_ALLOCATION *                    MyAllocation;
} LJB_OPENED_ALLOCATION;

typedef struct _LJB_APERTURE_MAPPING
{
    LIST_ENTRY                          ListEntry;
    HANDLE                              hDevice;
    HANDLE                              hAllocation;
    UINT                                SegmentId;
    SIZE_T                              OffsetInPages;
    SIZE_T                              NumberOfPages;
    PMDL                                pMdl;
    DXGK_MAPAPERTUREFLAGS               Flags;
    ULONG                               MdlOffset;

    LONG                                ReferenceCount;
    PMDL                                PartialMdl;
    PVOID                               MappedSystemMemory;
}   LJB_APERTURE_MAPPING;

#define MAX_NUM_MONITOR_MODES           32
typedef struct _LJB_MONITOR_NODE
{
    LIST_ENTRY                  ListEntry;
    LJB_ADAPTER *               Adapter;
    UNICODE_STRING              SymbolicLink;
    WCHAR                       NameBuffer[MAX_PATH];
    FILE_OBJECT *               FileObject;
    DEVICE_OBJECT *             FDO;
    DEVICE_OBJECT *             PDO;
    PVOID                       NotificationHandle;
    ULONG                       ChildUid;
    LJB_MONITOR_INTERFACE       MonitorInterface;
    LONG                        ReferenceCount;
    UCHAR                       Edid[LJB_DEFAULT_EDID_DATA_SIZE];

    SIZE_T                      NumModes;
    D3DKMDT_MONITOR_SOURCE_MODE MonitorModes[MAX_NUM_MONITOR_MODES];
} LJB_MONITOR_NODE;

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

NTSTATUS
LJB_PROXYKMD_CreateAndAttachDxgkFilter(
    __in PDRIVER_OBJECT     DriverObject,
    __in DEVICE_OBJECT *    DeviceObject
    );

// from ljb_proxykmd_utility.c
VOID
LJB_PROXYKMD_DelayMs(
    __in LONG  DelayInMs
    );

// DXGK related.
DXGK_INTIALIZE                          LJB_DXGK_InitializeWin7;
DXGK_INTIALIZE                          LJB_DXGK_InitializeWin8;
DXGKDDI_ADD_DEVICE                      LJB_DXGK_AddDevice0;
DXGKDDI_ADD_DEVICE                      LJB_DXGK_AddDevice1;
DXGKDDI_ADD_DEVICE                      LJB_DXGK_AddDevice2;
DXGKDDI_ADD_DEVICE                      LJB_DXGK_AddDevice3;
DXGKDDI_START_DEVICE                    LJB_DXGK_StartDevice;
DXGKDDI_STOP_DEVICE                     LJB_DXGK_StopDevice;
DXGKDDI_REMOVE_DEVICE                   LJB_DXGK_RemoveDevice;
DXGKDDI_DISPATCH_IO_REQUEST             LJB_DXGK_DispatchIoRequest;
DXGKDDI_INTERRUPT_ROUTINE               LJB_DXGK_InterruptRoutine;
DXGKDDI_DPC_ROUTINE                     LJB_DXGK_DpcRoutine;
DXGKDDI_QUERY_CHILD_RELATIONS           LJB_DXGK_QueryChildRelations;
DXGKDDI_QUERY_CHILD_STATUS              LJB_DXGK_QueryChildStatus;
DXGKDDI_QUERY_DEVICE_DESCRIPTOR         LJB_DXGK_QueryDeviceDescriptor;
DXGKDDI_SET_POWER_STATE                 LJB_DXGK_SetPowerState;
DXGKDDI_NOTIFY_ACPI_EVENT               LJB_DXGK_NotifyAcpiEvent;
DXGKDDI_RESET_DEVICE                    LJB_DXGK_ResetDevice;
DXGKDDI_QUERYADAPTERINFO                LJB_DXGK_QueryAdapterInfo;
DXGKDDI_CREATEDEVICE                    LJB_DXGK_CreateDevice;
DXGKDDI_CREATEALLOCATION                LJB_DXGK_CreateAllocation;
DXGKDDI_DESTROYALLOCATION               LJB_DXGK_DestroyAllocation;
DXGKDDI_DESCRIBEALLOCATION              LJB_DXGK_DescribeAllocation;
DXGKDDI_GETSTANDARDALLOCATIONDRIVERDATA LJB_DXGK_GetStdAllocationDrvData;
DXGKDDI_ACQUIRESWIZZLINGRANGE           LJB_DXGK_AcquireSwizzlingRange;
DXGKDDI_RELEASESWIZZLINGRANGE           LJB_DXGK_ReleaseSwizzlingRange;
DXGKDDI_PATCH                           LJB_DXGK_Patch;
DXGKDDI_SUBMITCOMMAND                   LJB_DXGK_SubmitCommand;
DXGKDDI_PREEMPTCOMMAND                  LJB_DXGK_PreemptCommand;
DXGKDDI_BUILDPAGINGBUFFER               LJB_DXGK_BuildPagingBuffer;
DXGKDDI_SETPOINTERPOSITION              LJB_DXGK_SetPointerPosition;
DXGKDDI_SETPOINTERSHAPE                 LJB_DXGK_SetPointerShape;
DXGKDDI_RESETFROMTIMEOUT                LJB_DXGK_ResetFromTimeout;
DXGKDDI_RESTARTFROMTIMEOUT              LJB_DXGK_RestartFromTimeout;
DXGKDDI_ESCAPE                          LJB_DXGK_Escape;
DXGKDDI_QUERYCURRENTFENCE               LJB_DXGK_QueryCurrentFence;
DXGKDDI_ISSUPPORTEDVIDPN                LJB_DXGK_IsSupportedVidPn;
DXGKDDI_ENUMVIDPNCOFUNCMODALITY         LJB_DXGK_EnumVidPnCofuncModality;
DXGKDDI_SETVIDPNSOURCEADDRESS           LJB_DXGK_SetVidPnSourceAddress;
DXGKDDI_SETVIDPNSOURCEVISIBILITY        LJB_DXGK_SetVidPnSourceVisibility;
DXGKDDI_COMMITVIDPN                     LJB_DXGK_CommitVidPn;
DXGKDDI_UPDATEACTIVEVIDPNPRESENTPATH    LJB_DXGK_UpdateActiveVidPnPresentPath;
DXGKDDI_RECOMMENDMONITORMODES           LJB_DXGK_RecommendMonitorModes;
DXGKDDI_RECOMMENDVIDPNTOPOLOGY          LJB_DXGK_RecommendVidPnTopology;
DXGKDDI_GETSCANLINE                     LJB_DXGK_GetScanLine;
DXGKDDI_CONTROLINTERRUPT                LJB_DXGK_ControlInterrupt;
DXGKDDI_CREATEOVERLAY                   LJB_DXGK_CreateOverlay;
DXGKDDI_DESTROYDEVICE                   LJB_DXGK_DestroyDevice;
DXGKDDI_OPENALLOCATIONINFO              LJB_DXGK_OpenAllocation;
DXGKDDI_CLOSEALLOCATION                 LJB_DXGK_CloseAllocation;
DXGKDDI_RENDER                          LJB_DXGK_Render;
DXGKDDI_RENDERKM                        LJB_DXGK_RenderKm;
DXGKDDI_PRESENT                         LJB_DXGK_Present;
DXGKDDI_CREATECONTEXT                   LJB_DXGK_CreateContext;
DXGKDDI_DESTROYCONTEXT                  LJB_DXGK_DestroyContext;
DXGKDDI_QUERYVIDPNHWCAPABILITY          LJB_DXGK_QueryVidPnHWCapability;

/*
 * Win8 or above DDI
 */
DXGKDDISETPOWERCOMPONENTFSTATE          LJB_DXGK_SetPowerComponentFState;
DXGKDDI_QUERYDEPENDENTENGINEGROUP       LJB_DXGK_QueryDependentEngineGroup;
DXGKDDI_QUERYENGINESTATUS               LJB_DXGK_QueryEngineStatus;
DXGKDDI_RESETENGINE                     LJB_DXGK_ResetEngine;
DXGKDDI_STOP_DEVICE_AND_RELEASE_POST_DISPLAY_OWNERSHIP
                                        LJB_DXGK_StopDeviceAndReleasePostDisplayOwnership;
DXGKDDI_SYSTEM_DISPLAY_ENABLE           LJB_DXGK_SystemDisplayEnable;
DXGKDDI_SYSTEM_DISPLAY_WRITE            LJB_DXGK_SystemDisplayWrite;
DXGKDDI_CANCELCOMMAND                   LJB_DXGK_CancelCommand;
DXGKDDI_GET_CHILD_CONTAINER_ID          LJB_DXGK_GetChildContainerId;
DXGKDDIPOWERRUNTIMECONTROLREQUEST       LJB_DXGK_PowerRuntimeControlRequest;
DXGKDDI_CHECKMULTIPLANEOVERLAYSUPPORT   LJB_DXGK_CheckMultiPlaneOverlaySupport;
DXGKDDI_SETVIDPNSOURCEADDRESSWITHMULTIPLANEOVERLAY
                                        LJB_DXGK_SetVidPnSourceAddressWithMultiPlaneOverlay;
DXGKDDI_NOTIFY_SURPRISE_REMOVAL         LJB_DXGK_NotifySurpriseRemoval;
DXGKDDI_GETNODEMETADATA                 LJB_DXGK_GetNodeMetadata;
DXGKDDI_CONTROLINTERRUPT2               LJB_DXGK_ControlInterrupt2;
DXGKDDI_CALIBRATEGPUCLOCK               LJB_DXGK_CalibrateGpuClock;

/*
 * WDDM 2.0 DDI
 */
DXGKDDI_SUBMITCOMMANDVIRTUAL            LJB_DXGK_SubmitCommandVirtual;
DXGKDDI_CHECKMULTIPLANEOVERLAYSUPPORT2  LJB_DXGK_CheckMultiPlaneOverlaySupport2;
DXGKDDI_SETVIDPNSOURCEADDRESSWITHMULTIPLANEOVERLAY2
                                        LJB_DXGK_SetVidPnSourceAddressWithMultiPlaneOverlay2;

NTSTATUS
LJB_PROXYKMD_PassDown (
    __in PDEVICE_OBJECT DeviceObject,
    __in PIRP Irp
    );

LJB_ADAPTER *
LJB_DXGK_FindAdapterByDriverAdapter(
    __in PVOID hAdapter
    );

LJB_ADAPTER *
LJB_DXGK_FindAdapterByDriverAdapterAtDIRQL(
    __in PVOID hAdapter
    );

LJB_DEVICE *
LJB_DXGK_FindDevice(
    __in HANDLE     hDevice
    );

LJB_CONTEXT *
LJB_DXGK_FindContext(
    __in HANDLE     hContext
    );

LJB_STD_ALLOCATION_INFO *
LJB_DXGK_FindStdAllocationInfo(
    __in LJB_ADAPTER *  Adapter,
    __in PVOID          pAllocationPrivateDriverData,
    __in UINT           AllocationPrivateDriverDataSize
    );

LJB_ALLOCATION *
LJB_DXGK_FindAllocation(
    __in LJB_ADAPTER*   Adapter,
    __in HANDLE         hAllocation
    );

LJB_OPENED_ALLOCATION *
LJB_DXGK_FindOpenedAllocation(
    __in LJB_ADAPTER*   Adapter,
    __in HANDLE         hDeviceSpecificAllocation
    );

LJB_APERTURE_MAPPING *
LJB_DXGK_FindApertureMapping(
    __in LJB_ADAPTER *      Adapter,
    __in PVOID              hAllocation,
    __in UINT               SegmentId
    );

VOID
LJB_DXGK_OpenApertureMapping(
    __in LJB_ADAPTER *          Adapter,
    __in LJB_APERTURE_MAPPING * ApertureMapping
    );

VOID
LJB_DXGK_CloseApertureMapping(
    __in LJB_ADAPTER *          Adapter,
    __in LJB_APERTURE_MAPPING * ApertureMapping
    );

PVOID
LJB_AcquireHandleData(
    __in DXGKRNL_INTERFACE *            DxgkInterface,
    IN_CONST_PDXGKARGCB_GETHANDLEDATA   GetHandleData
    );

#define FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter) LJB_DXGK_FindAdapterByDriverAdapter(hAdapter)
#define FIND_ADAPTER_AT_DIRQL(hAdapter) LJB_DXGK_FindAdapterByDriverAdapterAtDIRQL(hAdapter)

DRIVER_NOTIFICATION_CALLBACK_ROUTINE    LBJ_PROXYKMD_PnpNotifyInterfaceChange;
DRIVER_NOTIFICATION_CALLBACK_ROUTINE    LJB_PROXYKMD_PnpNotifyDeviceChange;

NTSTATUS
LJB_PROXYKMD_PnpStart(
    __in LJB_ADAPTER *      Adapter
    );

VOID
LJB_PROXYKMD_PnpStop(
    __in LJB_ADAPTER *      Adapter
    );

LJB_MONITOR_NODE *
LJB_PROXYKMD_FindMonitorNode(
    __in LJB_ADAPTER *      Adapter,
    __in PUNICODE_STRING    SymbolicLinkName
    );

IO_WORKITEM_ROUTINE_EX  LJB_PROXYKMD_OpenTargetDeviceWorkItem;

NTSTATUS
LJB_PROXYKMD_OpenTargetDevice(
    __in LJB_MONITOR_NODE * MonitorNode
    );

NTSTATUS
LJB_PROXYKMD_GetTargetDevicePdo(
    __in PDEVICE_OBJECT DeviceObject,
    __out PDEVICE_OBJECT *PhysicalDeviceObject
    );

VOID
LJB_PROXYKMD_CloseTargetDevice(
    __in __drv_freesMem(MonitorNode) LJB_MONITOR_NODE * MonitorNode
    );

LJB_MONITOR_NODE *
LJB_GetMonitorNodeFromChildUid(
    __in LJB_ADAPTER *      Adapter,
    __in ULONG              ChildUid
    );
VOID
LJB_DereferenceMonitorNode(
    __in LJB_MONITOR_NODE * MonitorNode
    );

BOOLEAN
LJB_DXGK_IsSourceConnectedToInboxTarget(
    __in LJB_ADAPTER *                  Adapter,
    __in D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId
    );
BOOLEAN
LJB_DXGK_IsSourceConnectedToUsbTarget(
    __in LJB_ADAPTER *                  Adapter,
    __in D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId
    );

static FORCEINLINE
NTSTATUS
SaveSseState(
    _In_ ULONG64 Mask,
    _Out_ PXSTATE_SAVE XStateSave
    )
{
#if defined (_X86_)
    return KeSaveExtendedProcessorState( Mask, XStateSave );
#else
    UNREFERENCED_PARAMETER(Mask);
    UNREFERENCED_PARAMETER(XStateSave);
    return STATUS_SUCCESS;
#endif
}

static FORCEINLINE
VOID
RestoreSseState(
    _In_ PXSTATE_SAVE XStateSave
    )
    {
#if defined (_X86_)
    KeRestoreExtendedProcessorState( XStateSave );
#else
    UNREFERENCED_PARAMETER(XStateSave);
#endif
    }

SIZE_T
LJB_PROXYKMD_FastMemCpy(
    __out PVOID pDst,
    __in PVOID  pSrc,
    __in SIZE_T len
    );

_C_END

#endif