#if !defined(_LJB_VMON_PRIVATE_H_)
#define _LJB_VMON_PRIVATE_H_

#include <ntddk.h>
#include <wdf.h>
#include <dispmprt.h>

#include "wmilib.h"
#include "driver.h"
#include "public.h"
#include "ljb_vmon_ioctl.h"
#include "lci_display_internal_ioctl.h"

#define LJB_VMON_POOL_TAG (ULONG) 'VMON'

#define MOFRESOURCENAME L"VMON_FUNC_Wmi"

__checkReturn
PVOID
FORCEINLINE
LJB_VMON_GetPoolZero(
    __in SIZE_T NumberOfBytes
    )
{
    PVOID   buffer;

    buffer = ExAllocatePoolWithTag(
        NonPagedPool,
        NumberOfBytes,
        LJB_VMON_POOL_TAG
        );
    if (buffer != NULL)
        RtlZeroMemory(buffer, NumberOfBytes);
    return buffer;
}

#define LJB_VMON_FreePool(p) ExFreePoolWithTag(p, LJB_VMON_POOL_TAG)

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
#define DBGLVL_FNENTRY      (1 << 5)

#define DBGLVL_DEFAULT      (DBGLVL_ERROR | DBGLVL_PNP | DBGLVL_POWER)

#if DBG
#define LJB_VMON_Printf(dev_ctx, Mask, _x_)         \
    if (dev_ctx->DebugLevel & Mask)                 \
        {                                           \
        DbgPrint(" LJB_VMON:");                     \
        DbgPrint _x_;                               \
        }
#else
#define LJB_VMON_Printf(dev_ctx, Mask, _x_)
#endif

#define LJB_VMON_PrintfAlways(dev_ctx, Mask, _x_)   \
        DbgPrint(" LJB_VMON:");                     \
        DbgPrint _x_;



#define MAX_POINTER_SIZE            (256*256*4)   /* width(256)/height(256)/4Byte */
#define FRAME_UPDATE_WINDOW         (0x10000)

typedef struct _LJB_VMON_PRIMARY_SURFACE
    {
    LIST_ENTRY                  list_entry;
    HANDLE                      hPrimarySurface;
    PVOID                       remote_buffer;

    SIZE_T                      BufferSize;

    UINT                        Width;
    UINT                        Height;
    UINT                        Pitch;
    UINT                        BytesPerPixel;

    LONG                        reference_count;
    } LJB_VMON_PRIMARY_SURFACE;

typedef struct _LJB_POINTER_INFO
    {
    /*
     * updated by DxgiDdiSetPointerPosition
     */
    INT                             X;
    INT                             Y;
    BOOLEAN                         Visible;

    /*
     Updated by DxgkDdiSetPointerShape
     */
    DXGK_POINTERFLAGS               Flags;
    UINT                            Width;
    UINT                            Height;
    UINT                            Pitch;
    UCHAR                           Bitmap[MAX_POINTER_SIZE];
    UINT                            XHot;
    UINT                            YHot;
    } LJB_POINTER_INFO;

typedef struct _LJB_VMON_WAIT_FOR_EVENT_REQ
    {
    LIST_ENTRY                      list_entry;
    WDFREQUEST                      Request;
    LJB_VMON_MONITOR_EVENT *        in_event_data;
    LJB_VMON_MONITOR_EVENT *        out_event_data;
    } LJB_VMON_WAIT_FOR_EVENT_REQ;

typedef struct _LJB_VMON_CTX
    {
    WDFWMIINSTANCE                  WmiDeviceArrivalEvent;
    BOOLEAN                         WmiPowerDeviceEnableRegistered;
    TOASTER_INTERFACE_STANDARD      BusInterface;
    ULONG                           DebugLevel;

    /*
     * EDID
     */
    UCHAR                           EdidBlock[128];

    LCI_GENERIC_INTERFACE           TargetGenericInterface;
    LONG                            InterfaceReferenceCount;

    KSPIN_LOCK                      surface_lock;
    LIST_ENTRY                      surface_list;
    LONG                            PrimarySurfaceListCount;

    KSPIN_LOCK                      event_req_lock;
    LIST_ENTRY                      event_req_list;
    LONG                            event_req_count;

    KSPIN_LOCK                      ioctl_lock;

    ULONG                           LatestFrameId;
    ULONG			                LastSentFrameId;
    PVOID                           hLatestPrimarySurface;

	/*
	 * Store pointer shape and position
	 */
	LJB_POINTER_INFO				            PointerInfo;
    LJB_POINTER_INFO                            TempPointerInfo;
    BOOLEAN                                     PointerShapeChanged;
    
    /*
     * VidPn related
     */
    D3DDDI_VIDEO_PRESENT_SOURCE_ID              VidPnSourceId;
    BOOLEAN                                     VidPnVisible;
    UINT                                        Width;
    UINT                                        Height;
    UINT                                        Pitch;
    UINT                                        BytesPerPixel;

    D3DKMDT_VIDPN_PRESENT_PATH_TRANSFORMATION   ContentTransformation;


    } LJB_VMON_CTX;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(LJB_VMON_CTX, LJB_VMON_GetVMonCtx)

//
// Connector Types
//

#define TOASTER_WMI_STD_I8042 0
#define TOASTER_WMI_STD_SERIAL 1
#define TOASTER_WMI_STD_PARALEL 2
#define TOASTER_WMI_STD_USB 3

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD       LJB_VMON_EvtDeviceAdd;

EVT_WDF_DEVICE_CONTEXT_CLEANUP  LJB_VMON_EvtDeviceContextCleanup;
EVT_WDF_DEVICE_D0_ENTRY         LJB_VMON_EvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT          LJB_VMON_EvtDeviceD0Exit;
EVT_WDF_DEVICE_PREPARE_HARDWARE LJB_VMON_EvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE LJB_VMON_EvtDeviceReleaseHardware;
EVT_WDF_DEVICE_SURPRISE_REMOVAL	LJB_VMON_EvtDeviceSurpriseRemoval;

EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT LJB_VMON_EvtDeviceSelfManagedIoInit;

//
// Io events callbacks.
//
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL
                                    LJB_VMON_InternalDeviceIoControl;
EVT_WDF_IO_IN_CALLER_CONTEXT        LJB_VMON_IoInCallerContext;
EVT_WDF_IO_QUEUE_IO_READ            LJB_VMON_EvtIoRead;
EVT_WDF_IO_QUEUE_IO_STOP            LJB_VMON_EvtIoStop;

EVT_WDF_IO_QUEUE_IO_WRITE           LJB_VMON_EvtIoWrite;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL  LJB_VMON_EvtIoDeviceControl;
EVT_WDF_DEVICE_FILE_CREATE          LJB_VMON_EvtDeviceFileCreate;
EVT_WDF_FILE_CLOSE                  LJB_VMON_EvtFileClose;

NTSTATUS
LJB_VMON_GenericIoctl(
    __in PVOID          ProviderContext,
    __in ULONG          IoctlCode,
    __in_opt PVOID      InputBuffer,
    __in SIZE_T         InputBufferSize,
    __out_opt PVOID     OutputBuffer,
    __in SIZE_T         OutputBufferSize,
    __out ULONG *       BytesReturned
    );

NTSTATUS
VMON_WmiRegistration(
    __in WDFDEVICE Device
    );

//
// Power events callbacks
//
EVT_WDF_DEVICE_ARM_WAKE_FROM_S0         LJB_VMON_EvtDeviceArmWakeFromS0;
EVT_WDF_DEVICE_ARM_WAKE_FROM_SX         LJB_VMON_EvtDeviceArmWakeFromSx;
EVT_WDF_DEVICE_DISARM_WAKE_FROM_S0      LJB_VMON_EvtDeviceDisarmWakeFromS0;
EVT_WDF_DEVICE_DISARM_WAKE_FROM_SX      LJB_VMON_EvtDeviceDisarmWakeFromSx;
EVT_WDF_DEVICE_WAKE_FROM_S0_TRIGGERED   LJB_VMON_EvtDeviceWakeFromS0Triggered;
EVT_WDF_DEVICE_WAKE_FROM_SX_TRIGGERED   LJB_VMON_EvtDeviceWakeFromSxTriggered;

PCHAR
DbgDevicePowerString(
    IN WDF_POWER_DEVICE_STATE Type
    );

//
// WMI event callbacks
//
EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE     EvtWmiInstanceStdDeviceDataQueryInstance;
EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE     EvtWmiInstanceToasterControlQueryInstance;
EVT_WDF_WMI_INSTANCE_SET_INSTANCE       EvtWmiInstanceStdDeviceDataSetInstance;
EVT_WDF_WMI_INSTANCE_SET_INSTANCE       EvtWmiInstanceToasterControlSetInstance;
EVT_WDF_WMI_INSTANCE_SET_ITEM           EvtWmiInstanceToasterControlSetItem;
EVT_WDF_WMI_INSTANCE_SET_ITEM           EvtWmiInstanceStdDeviceDataSetItem;
EVT_WDF_WMI_INSTANCE_EXECUTE_METHOD     EvtWmiInstanceToasterControlExecuteMethod;

NTSTATUS
LJB_VMON_FireArrivalEvent(
    __in WDFDEVICE  Device
    );

VOID
LJB_VMON_WaitForMonitorEvent(
    __in LJB_VMON_CTX *     dev_ctx,
    __in WDFREQUEST         wdf_request,
    __in size_t             InputBufferLength,
    __in size_t             OutputBufferLength
    );

VOID
LJB_VMON_GetPointerShape(
    __in LJB_VMON_CTX *     dev_ctx,
    __in WDFREQUEST         wdf_request,
    __in size_t             InputBufferLength,
    __in size_t             OutputBufferLength
    );

VOID
LJB_VMON_BltBitmap(
    __in LJB_VMON_CTX *     dev_ctx,
    __in WDFREQUEST         wdf_request,
    __in size_t             InputBufferLength,
    __in size_t             OutputBufferLength
    );

VOID
LJB_VMON_LockBuffer(
    __in LJB_VMON_CTX *     dev_ctx,
    __in WDFREQUEST         wdf_request,
    __in size_t             InputBufferLength,
    __in size_t             OutputBufferLength
    );

VOID
LJB_VMON_UnlockBuffer(
    __in LJB_VMON_CTX *     dev_ctx,
    __in WDFREQUEST         wdf_request,
    __in size_t             InputBufferLength,
    __in size_t             OutputBufferLength
    );

#endif  // _LJB_VMON_PRIVATE_H_

