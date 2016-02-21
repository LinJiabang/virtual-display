/*
 * ljb_dxgk_initialize.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

/*
 * The following structure is copied from WDK7. To cope with different driver binary
 * built by different WDK, we need to know exactly what the client driver WDDM version.
 * Typically it is examined by the Version field. If Version is DXGKDDI_INTERFACE_VERSION_WIN7_GOLD
 * or DXGKDDI_INTERFACE_VERSION_WIN7, we will use our Win7 table.
 */
#define DXGKDDI_INTERFACE_VERSION_WIN7_GOLD     0x2004
typedef struct _DRIVER_INITIALIZATION_DATA_WIN7
{
    ULONG                                   Version;
    PDXGKDDI_ADD_DEVICE                     DxgkDdiAddDevice;
    PDXGKDDI_START_DEVICE                   DxgkDdiStartDevice;
    PDXGKDDI_STOP_DEVICE                    DxgkDdiStopDevice;
    PDXGKDDI_REMOVE_DEVICE                  DxgkDdiRemoveDevice;
    PDXGKDDI_DISPATCH_IO_REQUEST            DxgkDdiDispatchIoRequest;
    PDXGKDDI_INTERRUPT_ROUTINE              DxgkDdiInterruptRoutine;
    PDXGKDDI_DPC_ROUTINE                    DxgkDdiDpcRoutine;
    PDXGKDDI_QUERY_CHILD_RELATIONS          DxgkDdiQueryChildRelations;
    PDXGKDDI_QUERY_CHILD_STATUS             DxgkDdiQueryChildStatus;
    PDXGKDDI_QUERY_DEVICE_DESCRIPTOR        DxgkDdiQueryDeviceDescriptor;
    PDXGKDDI_SET_POWER_STATE                DxgkDdiSetPowerState;
    PDXGKDDI_NOTIFY_ACPI_EVENT              DxgkDdiNotifyAcpiEvent;
    PDXGKDDI_RESET_DEVICE                   DxgkDdiResetDevice;
    PDXGKDDI_UNLOAD                         DxgkDdiUnload;
    PDXGKDDI_QUERY_INTERFACE                DxgkDdiQueryInterface;
    PDXGKDDI_CONTROL_ETW_LOGGING            DxgkDdiControlEtwLogging;

    PDXGKDDI_QUERYADAPTERINFO               DxgkDdiQueryAdapterInfo;
    PDXGKDDI_CREATEDEVICE                   DxgkDdiCreateDevice;
    PDXGKDDI_CREATEALLOCATION               DxgkDdiCreateAllocation;
    PDXGKDDI_DESTROYALLOCATION              DxgkDdiDestroyAllocation;
    PDXGKDDI_DESCRIBEALLOCATION             DxgkDdiDescribeAllocation;
    PDXGKDDI_GETSTANDARDALLOCATIONDRIVERDATA DxgkDdiGetStandardAllocationDriverData;
    PDXGKDDI_ACQUIRESWIZZLINGRANGE          DxgkDdiAcquireSwizzlingRange;
    PDXGKDDI_RELEASESWIZZLINGRANGE          DxgkDdiReleaseSwizzlingRange;
    PDXGKDDI_PATCH                          DxgkDdiPatch;
    PDXGKDDI_SUBMITCOMMAND                  DxgkDdiSubmitCommand;
    PDXGKDDI_PREEMPTCOMMAND                 DxgkDdiPreemptCommand;
    PDXGKDDI_BUILDPAGINGBUFFER              DxgkDdiBuildPagingBuffer;
    PDXGKDDI_SETPALETTE                     DxgkDdiSetPalette;
    PDXGKDDI_SETPOINTERPOSITION             DxgkDdiSetPointerPosition;
    PDXGKDDI_SETPOINTERSHAPE                DxgkDdiSetPointerShape;
    PDXGKDDI_RESETFROMTIMEOUT               DxgkDdiResetFromTimeout;
    PDXGKDDI_RESTARTFROMTIMEOUT             DxgkDdiRestartFromTimeout;
    PDXGKDDI_ESCAPE                         DxgkDdiEscape;
    PDXGKDDI_COLLECTDBGINFO                 DxgkDdiCollectDbgInfo;
    PDXGKDDI_QUERYCURRENTFENCE              DxgkDdiQueryCurrentFence;
    PDXGKDDI_ISSUPPORTEDVIDPN               DxgkDdiIsSupportedVidPn;
    PDXGKDDI_RECOMMENDFUNCTIONALVIDPN       DxgkDdiRecommendFunctionalVidPn;
    PDXGKDDI_ENUMVIDPNCOFUNCMODALITY        DxgkDdiEnumVidPnCofuncModality;
    PDXGKDDI_SETVIDPNSOURCEADDRESS          DxgkDdiSetVidPnSourceAddress;
    PDXGKDDI_SETVIDPNSOURCEVISIBILITY       DxgkDdiSetVidPnSourceVisibility;
    PDXGKDDI_COMMITVIDPN                    DxgkDdiCommitVidPn;
    PDXGKDDI_UPDATEACTIVEVIDPNPRESENTPATH   DxgkDdiUpdateActiveVidPnPresentPath;
    PDXGKDDI_RECOMMENDMONITORMODES          DxgkDdiRecommendMonitorModes;
    PDXGKDDI_RECOMMENDVIDPNTOPOLOGY         DxgkDdiRecommendVidPnTopology;
    PDXGKDDI_GETSCANLINE                    DxgkDdiGetScanLine;
    PDXGKDDI_STOPCAPTURE                    DxgkDdiStopCapture;
    PDXGKDDI_CONTROLINTERRUPT               DxgkDdiControlInterrupt;
    PDXGKDDI_CREATEOVERLAY                  DxgkDdiCreateOverlay;

    //
    // Device functions
    //

    PDXGKDDI_DESTROYDEVICE                  DxgkDdiDestroyDevice;
    PDXGKDDI_OPENALLOCATIONINFO             DxgkDdiOpenAllocation;
    PDXGKDDI_CLOSEALLOCATION                DxgkDdiCloseAllocation;
    PDXGKDDI_RENDER                         DxgkDdiRender;
    PDXGKDDI_PRESENT                        DxgkDdiPresent;

    //
    // Overlay functions
    //

    PDXGKDDI_UPDATEOVERLAY                  DxgkDdiUpdateOverlay;
    PDXGKDDI_FLIPOVERLAY                    DxgkDdiFlipOverlay;
    PDXGKDDI_DESTROYOVERLAY                 DxgkDdiDestroyOverlay;

    //
    // Context supports.
    //

    PDXGKDDI_CREATECONTEXT                  DxgkDdiCreateContext;
    PDXGKDDI_DESTROYCONTEXT                 DxgkDdiDestroyContext;

    //
    // Linked Display Adapter support.
    //

    PDXGKDDI_LINK_DEVICE                    DxgkDdiLinkDevice;
    PDXGKDDI_SETDISPLAYPRIVATEDRIVERFORMAT  DxgkDdiSetDisplayPrivateDriverFormat;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN7)
    //
    // Extended for WDDM 2.0
    //
    PVOID                                   DxgkDdiDescribePageTable;
    PVOID                                   DxgkDdiUpdatePageTable;
    PVOID                                   DxgkDdiUpdatePageDirectory;
    PVOID                                   DxgkDdiMovePageDirectory;

    PVOID                                   DxgkDdiSubmitRender;
    PVOID                                   DxgkDdiCreateAllocation2;

    //
    // GDI acceleration. Extended for WDDM 1.0
    //
    PDXGKDDI_RENDER                         DxgkDdiRenderKm;

    //
    // New DMM DDIs for CCD support
    //
    VOID*                                   Reserved;
    PDXGKDDI_QUERYVIDPNHWCAPABILITY         DxgkDdiQueryVidPnHWCapability;

#endif // DXGKDDI_INTERFACE_VERSION

}   DRIVER_INITIALIZATION_DATA_WIN7;

static CONST DRIVER_INITIALIZATION_DATA_WIN7    DriverInitTableWin7 =
{
    DXGKDDI_INTERFACE_VERSION_WIN7,
    &LJB_DXGK_AddDevice0,
    &LJB_DXGK_StartDevice,
    &LJB_DXGK_StopDevice,
    &LJB_DXGK_RemoveDevice,
    &LJB_DXGK_DispatchIoRequest,
    &LJB_DXGK_InterruptRoutine,
    &LJB_DXGK_DpcRoutine,
    &LJB_DXGK_QueryChildRelations,
    &LJB_DXGK_QueryChildStatus,
    &LJB_DXGK_QueryDeviceDescriptor,
    &LJB_DXGK_SetPowerState,
    &LJB_DXGK_NotifyAcpiEvent,
    &LJB_DXGK_ResetDevice,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_Unload,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_QueryInterface,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_ControlEtwLogging,
    &LJB_DXGK_QueryAdapterInfo,
    &LJB_DXGK_CreateDevice,
    &LJB_DXGK_CreateAllocation,
    &LJB_DXGK_DestroyAllocation,
    &LJB_DXGK_DescribeAllocation,
    &LJB_DXGK_GetStdAllocationDrvData,
    &LJB_DXGK_AcquireSwizzlingRange,
    &LJB_DXGK_ReleaseSwizzlingRange,
    &LJB_DXGK_Patch,
    &LJB_DXGK_SubmitCommand,
    &LJB_DXGK_PreemptCommand,
    &LJB_DXGK_BuildPagingBuffer,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_SetPalette,
    &LJB_DXGK_SetPointerPosition,
    &LJB_DXGK_SetPointerShape,
    &LJB_DXGK_ResetFromTimeout,
    &LJB_DXGK_RestartFromTimeout,
    &LJB_DXGK_Escape,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_CollectDbgInfo,
    &LJB_DXGK_QueryCurrentFence,
    &LJB_DXGK_IsSupportedVidPn,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_RecommendFunctionalVidPn,
    &LJB_DXGK_EnumVidPnCofuncModality,
    &LJB_DXGK_SetVidPnSourceAddress,
    &LJB_DXGK_SetVidPnSourceVisibility,
    &LJB_DXGK_CommitVidPn,
    &LJB_DXGK_UpdateActiveVidPnPresentPath,
    &LJB_DXGK_RecommendMonitorModes,
    &LJB_DXGK_RecommendVidPnTopology,
    &LJB_DXGK_GetScanLine,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_StopCapture,
    &LJB_DXGK_ControlInterrupt,
    &LJB_DXGK_CreateOverlay,
    &LJB_DXGK_DestroyDevice,
    &LJB_DXGK_OpenAllocation,
    &LJB_DXGK_CloseAllocation,
    &LJB_DXGK_Render,
    &LJB_DXGK_Present,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_UpdateOverlay,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_FlipOverlay,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_DestroyOverlay,
    &LJB_DXGK_CreateContext,
    &LJB_DXGK_DestroyContext,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_LinkDevice,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_SetDisplayPrivateDriverFormat,

    /*
     * The following fields are reserved. Do NOT implement!
     * PDXGKDDI_DESCRIBEPAGETABLE              DxgkDdiDescribePageTable;
     * PDXGKDDI_UPDATEPAGETABLE                DxgkDdiUpdatePageTable;
     * PDXGKDDI_UPDATEPAGEDIRECTORY            DxgkDdiUpdatePageDirectory;
     * PDXGKDDI_MOVEPAGEDIRECTORY              DxgkDdiMovePageDirectory;
     * PDXGKDDI_SUBMITRENDER                   DxgkDdiSubmitRender;
     * PDXGKDDI_CREATEALLOCATION2              DxgkDdiCreateAllocation2;
     */
    NULL,                   /* DxgkDdiDescribePageTable */
    NULL,                   /* DxgkDdiUpdatePageTable */
    NULL,                   /* DxgkDdiUpdatePageDirectory */
    NULL,                   /* DxgkDdiMovePageDirectory */
    NULL,                   /* DxgkDdiSubmitRender */
    NULL,                   /* DxgkDdiCreateAllocation2 */
    &LJB_DXGK_RenderKm,
    NULL,                   /* VOID* Reserved; */
    &LJB_DXGK_QueryVidPnHWCapability
};

/*
 * DRIVER_INITIALIZATION from WDK8
 */
typedef struct _DRIVER_INITIALIZATION_DATA_WIN8
{
    ULONG                                   Version;
    PDXGKDDI_ADD_DEVICE                     DxgkDdiAddDevice;
    PDXGKDDI_START_DEVICE                   DxgkDdiStartDevice;
    PDXGKDDI_STOP_DEVICE                    DxgkDdiStopDevice;
    PDXGKDDI_REMOVE_DEVICE                  DxgkDdiRemoveDevice;
    PDXGKDDI_DISPATCH_IO_REQUEST            DxgkDdiDispatchIoRequest;
    PDXGKDDI_INTERRUPT_ROUTINE              DxgkDdiInterruptRoutine;
    PDXGKDDI_DPC_ROUTINE                    DxgkDdiDpcRoutine;
    PDXGKDDI_QUERY_CHILD_RELATIONS          DxgkDdiQueryChildRelations;
    PDXGKDDI_QUERY_CHILD_STATUS             DxgkDdiQueryChildStatus;
    PDXGKDDI_QUERY_DEVICE_DESCRIPTOR        DxgkDdiQueryDeviceDescriptor;
    PDXGKDDI_SET_POWER_STATE                DxgkDdiSetPowerState;
    PDXGKDDI_NOTIFY_ACPI_EVENT              DxgkDdiNotifyAcpiEvent;
    PDXGKDDI_RESET_DEVICE                   DxgkDdiResetDevice;
    PDXGKDDI_UNLOAD                         DxgkDdiUnload;
    PDXGKDDI_QUERY_INTERFACE                DxgkDdiQueryInterface;
    PDXGKDDI_CONTROL_ETW_LOGGING            DxgkDdiControlEtwLogging;

    PDXGKDDI_QUERYADAPTERINFO               DxgkDdiQueryAdapterInfo;
    PDXGKDDI_CREATEDEVICE                   DxgkDdiCreateDevice;
    PDXGKDDI_CREATEALLOCATION               DxgkDdiCreateAllocation;
    PDXGKDDI_DESTROYALLOCATION              DxgkDdiDestroyAllocation;
    PDXGKDDI_DESCRIBEALLOCATION             DxgkDdiDescribeAllocation;
    PDXGKDDI_GETSTANDARDALLOCATIONDRIVERDATA DxgkDdiGetStandardAllocationDriverData;
    PDXGKDDI_ACQUIRESWIZZLINGRANGE          DxgkDdiAcquireSwizzlingRange;
    PDXGKDDI_RELEASESWIZZLINGRANGE          DxgkDdiReleaseSwizzlingRange;
    PDXGKDDI_PATCH                          DxgkDdiPatch;
    PDXGKDDI_SUBMITCOMMAND                  DxgkDdiSubmitCommand;
    PDXGKDDI_PREEMPTCOMMAND                 DxgkDdiPreemptCommand;
    PDXGKDDI_BUILDPAGINGBUFFER              DxgkDdiBuildPagingBuffer;
    PDXGKDDI_SETPALETTE                     DxgkDdiSetPalette;
    PDXGKDDI_SETPOINTERPOSITION             DxgkDdiSetPointerPosition;
    PDXGKDDI_SETPOINTERSHAPE                DxgkDdiSetPointerShape;
    PDXGKDDI_RESETFROMTIMEOUT               DxgkDdiResetFromTimeout;
    PDXGKDDI_RESTARTFROMTIMEOUT             DxgkDdiRestartFromTimeout;
    PDXGKDDI_ESCAPE                         DxgkDdiEscape;
    PDXGKDDI_COLLECTDBGINFO                 DxgkDdiCollectDbgInfo;
    PDXGKDDI_QUERYCURRENTFENCE              DxgkDdiQueryCurrentFence;
    PDXGKDDI_ISSUPPORTEDVIDPN               DxgkDdiIsSupportedVidPn;
    PDXGKDDI_RECOMMENDFUNCTIONALVIDPN       DxgkDdiRecommendFunctionalVidPn;
    PDXGKDDI_ENUMVIDPNCOFUNCMODALITY        DxgkDdiEnumVidPnCofuncModality;
    PDXGKDDI_SETVIDPNSOURCEADDRESS          DxgkDdiSetVidPnSourceAddress;
    PDXGKDDI_SETVIDPNSOURCEVISIBILITY       DxgkDdiSetVidPnSourceVisibility;
    PDXGKDDI_COMMITVIDPN                    DxgkDdiCommitVidPn;
    PDXGKDDI_UPDATEACTIVEVIDPNPRESENTPATH   DxgkDdiUpdateActiveVidPnPresentPath;
    PDXGKDDI_RECOMMENDMONITORMODES          DxgkDdiRecommendMonitorModes;
    PDXGKDDI_RECOMMENDVIDPNTOPOLOGY         DxgkDdiRecommendVidPnTopology;
    PDXGKDDI_GETSCANLINE                    DxgkDdiGetScanLine;
    PDXGKDDI_STOPCAPTURE                    DxgkDdiStopCapture;
    PDXGKDDI_CONTROLINTERRUPT               DxgkDdiControlInterrupt;
    PDXGKDDI_CREATEOVERLAY                  DxgkDdiCreateOverlay;

    //
    // Device functions
    //

    PDXGKDDI_DESTROYDEVICE                  DxgkDdiDestroyDevice;
    PDXGKDDI_OPENALLOCATIONINFO             DxgkDdiOpenAllocation;
    PDXGKDDI_CLOSEALLOCATION                DxgkDdiCloseAllocation;
    PDXGKDDI_RENDER                         DxgkDdiRender;
    PDXGKDDI_PRESENT                        DxgkDdiPresent;

    //
    // Overlay functions
    //

    PDXGKDDI_UPDATEOVERLAY                  DxgkDdiUpdateOverlay;
    PDXGKDDI_FLIPOVERLAY                    DxgkDdiFlipOverlay;
    PDXGKDDI_DESTROYOVERLAY                 DxgkDdiDestroyOverlay;

    //
    // Context supports.
    //

    PDXGKDDI_CREATECONTEXT                  DxgkDdiCreateContext;
    PDXGKDDI_DESTROYCONTEXT                 DxgkDdiDestroyContext;

    //
    // Linked Display Adapter support.
    //

    PDXGKDDI_LINK_DEVICE                    DxgkDdiLinkDevice;
    PDXGKDDI_SETDISPLAYPRIVATEDRIVERFORMAT  DxgkDdiSetDisplayPrivateDriverFormat;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN7)
    //
    // Extended for WDDM 2.0
    //
    PVOID                                   DxgkDdiDescribePageTable;
    PVOID                                   DxgkDdiUpdatePageTable;
    PVOID                                   DxgkDdiUpdatePageDirectory;
    PVOID                                   DxgkDdiMovePageDirectory;

    PVOID                                   DxgkDdiSubmitRender;
    PVOID                                   DxgkDdiCreateAllocation2;

    //
    // GDI acceleration. Extended for WDDM 1.0
    //
    PDXGKDDI_RENDER                         DxgkDdiRenderKm;

    //
    // New DMM DDIs for CCD support
    //
    VOID*                                   Reserved;
    PDXGKDDI_QUERYVIDPNHWCAPABILITY         DxgkDdiQueryVidPnHWCapability;

#endif // DXGKDDI_INTERFACE_VERSION

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    PDXGKDDISETPOWERCOMPONENTFSTATE         DxgkDdiSetPowerComponentFState;

    //
    // New DDIs for TDR support.
    //
    PDXGKDDI_QUERYDEPENDENTENGINEGROUP      DxgkDdiQueryDependentEngineGroup;
    PDXGKDDI_QUERYENGINESTATUS              DxgkDdiQueryEngineStatus;
    PDXGKDDI_RESETENGINE                    DxgkDdiResetEngine;

    //
    // New DDIs for PnP stop/start support.
    //
    PDXGKDDI_STOP_DEVICE_AND_RELEASE_POST_DISPLAY_OWNERSHIP DxgkDdiStopDeviceAndReleasePostDisplayOwnership;

    //
    // New DDIs for system display support.
    //
    PDXGKDDI_SYSTEM_DISPLAY_ENABLE          DxgkDdiSystemDisplayEnable;
    PDXGKDDI_SYSTEM_DISPLAY_WRITE           DxgkDdiSystemDisplayWrite;

    PDXGKDDI_CANCELCOMMAND                  DxgkDdiCancelCommand;

    //
    // New DDI for the monitor container ID support.
    //
    PDXGKDDI_GET_CHILD_CONTAINER_ID         DxgkDdiGetChildContainerId;

    PDXGKDDIPOWERRUNTIMECONTROLREQUEST      DxgkDdiPowerRuntimeControlRequest;

    //
    // New DDI for multi plane overlay support.
    //
    PDXGKDDI_SETVIDPNSOURCEADDRESSWITHMULTIPLANEOVERLAY DxgkDdiSetVidPnSourceAddressWithMultiPlaneOverlay;

    //
    // New DDI for the surprise removal support.
    //
    PDXGKDDI_NOTIFY_SURPRISE_REMOVAL        DxgkDdiNotifySurpriseRemoval;

#endif // DXGKDDI_INTERFACE_VERSION

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    //
    // New DDI for querying node metadata
    //
    PDXGKDDI_GETNODEMETADATA            DxgkDdiGetNodeMetadata;

    //
    // New DDI for power management enhancements
    //
    PDXGKDDISETPOWERPSTATE              DxgkDdiSetPowerPState;
    PDXGKDDI_CONTROLINTERRUPT2          DxgkDdiControlInterrupt2;

    //
    // New DDI for multiplane overlay support
    //
    PDXGKDDI_CHECKMULTIPLANEOVERLAYSUPPORT  DxgkDdiCheckMultiPlaneOverlaySupport;

    //
    // New DDI for GPU clock calibration
    //
    PDXGKDDI_CALIBRATEGPUCLOCK          DxgkDdiCalibrateGpuClock;

    //
    // New DDI for history buffer formatting
    //
    PDXGKDDI_FORMATHISTORYBUFFER        DxgkDdiFormatHistoryBuffer;
#endif

//#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

    VOID *      NotImplemented0;                //PDXGKDDI_RENDERGDI                      DxgkDdiRenderGdi;
    VOID *      NotImplemented1;                //PDXGKDDI_SUBMITCOMMANDVIRTUAL           DxgkDdiSubmitCommandVirtual;
    VOID *      NotImplemented2;                //PDXGKDDI_SETROOTPAGETABLE               DxgkDdiSetRootPageTable;
    VOID *      NotImplemented3;                //PDXGKDDI_GETROOTPAGETABLESIZE           DxgkDdiGetRootPageTableSize;
    VOID *      NotImplemented4;                //PDXGKDDI_MAPCPUHOSTAPERTURE             DxgkDdiMapCpuHostAperture;
    VOID *      NotImplemented5;                //PDXGKDDI_UNMAPCPUHOSTAPERTURE           DxgkDdiUnmapCpuHostAperture;
    VOID *      NotImplemented6;                //PDXGKDDI_CHECKMULTIPLANEOVERLAYSUPPORT2 DxgkDdiCheckMultiPlaneOverlaySupport2;
    VOID *      NotImplemented7;                //PDXGKDDI_CREATEPROCESS                  DxgkDdiCreateProcess;
    VOID *      NotImplemented8;                //PDXGKDDI_DESTROYPROCESS                 DxgkDdiDestroyProcess;
    VOID *      NotImplemented9;                //PDXGKDDI_SETVIDPNSOURCEADDRESSWITHMULTIPLANEOVERLAY2    DxgkDdiSetVidPnSourceAddressWithMultiPlaneOverlay2;
    VOID *      NotImplemented10;               //void*                                   Reserved1;
    VOID *      NotImplemented11;               //void*                                   Reserved2;
    VOID *      NotImplemented12;               //PDXGKDDI_POWERRUNTIMESETDEVICEHANDLE    DxgkDdiPowerRuntimeSetDeviceHandle;
    VOID *      NotImplemented13;               //PDXGKDDI_SETSTABLEPOWERSTATE            DxgkDdiSetStablePowerState;
    VOID *      NotImplemented14;               //PDXGKDDI_SETVIDEOPROTECTEDREGION        DxgkDdiSetVideoProtectedRegion;

//#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

}   DRIVER_INITIALIZATION_DATA_WIN8;

static CONST DRIVER_INITIALIZATION_DATA_WIN8   DriverInitTableWin8 =
{
    DXGKDDI_INTERFACE_VERSION,
    &LJB_DXGK_AddDevice0,
    &LJB_DXGK_StartDevice,
    &LJB_DXGK_StopDevice,
    &LJB_DXGK_RemoveDevice,
    &LJB_DXGK_DispatchIoRequest,
    &LJB_DXGK_InterruptRoutine,
    &LJB_DXGK_DpcRoutine,
    &LJB_DXGK_QueryChildRelations,
    &LJB_DXGK_QueryChildStatus,
    &LJB_DXGK_QueryDeviceDescriptor,
    &LJB_DXGK_SetPowerState,
    &LJB_DXGK_NotifyAcpiEvent,
    &LJB_DXGK_ResetDevice,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_Unload,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_QueryInterface,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_ControlEtwLogging,
    &LJB_DXGK_QueryAdapterInfo,
    &LJB_DXGK_CreateDevice,
    &LJB_DXGK_CreateAllocation,
    &LJB_DXGK_DestroyAllocation,
    &LJB_DXGK_DescribeAllocation,
    &LJB_DXGK_GetStdAllocationDrvData,
    &LJB_DXGK_AcquireSwizzlingRange,
    &LJB_DXGK_ReleaseSwizzlingRange,
    &LJB_DXGK_Patch,
    &LJB_DXGK_SubmitCommand,
    &LJB_DXGK_PreemptCommand,
    &LJB_DXGK_BuildPagingBuffer,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_SetPalette,
    &LJB_DXGK_SetPointerPosition,
    &LJB_DXGK_SetPointerShape,
    &LJB_DXGK_ResetFromTimeout,
    &LJB_DXGK_RestartFromTimeout,
    &LJB_DXGK_Escape,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_CollectDbgInfo,
    &LJB_DXGK_QueryCurrentFence,
    &LJB_DXGK_IsSupportedVidPn,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_RecommendFunctionalVidPn,
    &LJB_DXGK_EnumVidPnCofuncModality,
    &LJB_DXGK_SetVidPnSourceAddress,
    &LJB_DXGK_SetVidPnSourceVisibility,
    &LJB_DXGK_CommitVidPn,
    &LJB_DXGK_UpdateActiveVidPnPresentPath,
    &LJB_DXGK_RecommendMonitorModes,
    &LJB_DXGK_RecommendVidPnTopology,
    &LJB_DXGK_GetScanLine,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_StopCapture,
    &LJB_DXGK_ControlInterrupt,
    &LJB_DXGK_CreateOverlay,
    &LJB_DXGK_DestroyDevice,
    &LJB_DXGK_OpenAllocation,
    &LJB_DXGK_CloseAllocation,
    &LJB_DXGK_Render,
    &LJB_DXGK_Present,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_UpdateOverlay,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_FlipOverlay,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_DestroyOverlay,
    &LJB_DXGK_CreateContext,
    &LJB_DXGK_DestroyContext,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_LinkDevice,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_SetDisplayPrivateDriverFormat,

    /*
     * The following fields are reserved. Do NOT implement!
     * PDXGKDDI_DESCRIBEPAGETABLE              DxgkDdiDescribePageTable;
     * PDXGKDDI_UPDATEPAGETABLE                DxgkDdiUpdatePageTable;
     * PDXGKDDI_UPDATEPAGEDIRECTORY            DxgkDdiUpdatePageDirectory;
     * PDXGKDDI_MOVEPAGEDIRECTORY              DxgkDdiMovePageDirectory;
     * PDXGKDDI_SUBMITRENDER                   DxgkDdiSubmitRender;
     * PDXGKDDI_CREATEALLOCATION2              DxgkDdiCreateAllocation2;
     */
    NULL,                   /* DxgkDdiDescribePageTable */
    NULL,                   /* DxgkDdiUpdatePageTable */
    NULL,                   /* DxgkDdiUpdatePageDirectory */
    NULL,                   /* DxgkDdiMovePageDirectory */
    NULL,                   /* DxgkDdiSubmitRender */
    NULL,                   /* DxgkDdiCreateAllocation2 */
    &LJB_DXGK_RenderKm,
    NULL,                   /* VOID* Reserved; */
    &LJB_DXGK_QueryVidPnHWCapability,

    /*
     *(DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
     */
    &LJB_DXGK_SetPowerComponentFState,
    &LJB_DXGK_QueryDependentEngineGroup,
    &LJB_DXGK_QueryEngineStatus,
    &LJB_DXGK_ResetEngine,
    &LJB_DXGK_StopDeviceAndReleasePostDisplayOwnership,
    &LJB_DXGK_SystemDisplayEnable,
    &LJB_DXGK_SystemDisplayWrite,
    &LJB_DXGK_CancelCommand,
    &LJB_DXGK_GetChildContainerId,
    &LJB_DXGK_PowerRuntimeControlRequest,
    &LJB_DXGK_SetVidPnSourceAddressWithMultiPlaneOverlay,
    &LJB_DXGK_NotifySurpriseRemoval,

    /*
     * (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
     */
    &LJB_DXGK_GetNodeMetadata,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_SetPowerPState,        // DxgkDdiSetPowerPState: This member is reserved and should be set to zero.
    &LJB_DXGK_ControlInterrupt2,     // MSDN says this shall be zero, but Dxgkrnl.sys is calling into it!
    &LJB_DXGK_CheckMultiPlaneOverlaySupport,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_CalibrateGpuClock,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_FormatHistoryBuffer,

    NULL,               //NotImplemented0
    NULL,               //NotImplemented1
    NULL,               //NotImplemented2
    NULL,               //NotImplemented3
    NULL,               //NotImplemented4
    NULL,               //NotImplemented5
    NULL,               //NotImplemented6
    NULL,               //NotImplemented7
    NULL,               //NotImplemented8
    NULL,               //NotImplemented9
    NULL,               //NotImplemented10
    NULL,               //NotImplemented11
    NULL,               //NotImplemented12
    NULL,               //NotImplemented13
    NULL                //NotImplemented14
};

/*
 * WIN10 DxgkInitialize
 */
#define DXGKDDI_INTERFACE_VERSION_WDDM2_0_PREVIEW   0x5020
typedef struct _DRIVER_INITIALIZATION_DATA_WIN10 {
    ULONG                                   Version;
    PDXGKDDI_ADD_DEVICE                     DxgkDdiAddDevice;
    PDXGKDDI_START_DEVICE                   DxgkDdiStartDevice;
    PDXGKDDI_STOP_DEVICE                    DxgkDdiStopDevice;
    PDXGKDDI_REMOVE_DEVICE                  DxgkDdiRemoveDevice;
    PDXGKDDI_DISPATCH_IO_REQUEST            DxgkDdiDispatchIoRequest;
    PDXGKDDI_INTERRUPT_ROUTINE              DxgkDdiInterruptRoutine;
    PDXGKDDI_DPC_ROUTINE                    DxgkDdiDpcRoutine;
    PDXGKDDI_QUERY_CHILD_RELATIONS          DxgkDdiQueryChildRelations;
    PDXGKDDI_QUERY_CHILD_STATUS             DxgkDdiQueryChildStatus;
    PDXGKDDI_QUERY_DEVICE_DESCRIPTOR        DxgkDdiQueryDeviceDescriptor;
    PDXGKDDI_SET_POWER_STATE                DxgkDdiSetPowerState;
    PDXGKDDI_NOTIFY_ACPI_EVENT              DxgkDdiNotifyAcpiEvent;
    PDXGKDDI_RESET_DEVICE                   DxgkDdiResetDevice;
    PDXGKDDI_UNLOAD                         DxgkDdiUnload;
    PDXGKDDI_QUERY_INTERFACE                DxgkDdiQueryInterface;
    PDXGKDDI_CONTROL_ETW_LOGGING            DxgkDdiControlEtwLogging;

    PDXGKDDI_QUERYADAPTERINFO               DxgkDdiQueryAdapterInfo;
    PDXGKDDI_CREATEDEVICE                   DxgkDdiCreateDevice;
    PDXGKDDI_CREATEALLOCATION               DxgkDdiCreateAllocation;
    PDXGKDDI_DESTROYALLOCATION              DxgkDdiDestroyAllocation;
    PDXGKDDI_DESCRIBEALLOCATION             DxgkDdiDescribeAllocation;
    PDXGKDDI_GETSTANDARDALLOCATIONDRIVERDATA DxgkDdiGetStandardAllocationDriverData;
    PDXGKDDI_ACQUIRESWIZZLINGRANGE          DxgkDdiAcquireSwizzlingRange;
    PDXGKDDI_RELEASESWIZZLINGRANGE          DxgkDdiReleaseSwizzlingRange;
    PDXGKDDI_PATCH                          DxgkDdiPatch;
    PDXGKDDI_SUBMITCOMMAND                  DxgkDdiSubmitCommand;
    PDXGKDDI_PREEMPTCOMMAND                 DxgkDdiPreemptCommand;
    PDXGKDDI_BUILDPAGINGBUFFER              DxgkDdiBuildPagingBuffer;
    PDXGKDDI_SETPALETTE                     DxgkDdiSetPalette;
    PDXGKDDI_SETPOINTERPOSITION             DxgkDdiSetPointerPosition;
    PDXGKDDI_SETPOINTERSHAPE                DxgkDdiSetPointerShape;
    PDXGKDDI_RESETFROMTIMEOUT               DxgkDdiResetFromTimeout;
    PDXGKDDI_RESTARTFROMTIMEOUT             DxgkDdiRestartFromTimeout;
    PDXGKDDI_ESCAPE                         DxgkDdiEscape;
    PDXGKDDI_COLLECTDBGINFO                 DxgkDdiCollectDbgInfo;
    PDXGKDDI_QUERYCURRENTFENCE              DxgkDdiQueryCurrentFence;
    PDXGKDDI_ISSUPPORTEDVIDPN               DxgkDdiIsSupportedVidPn;
    PDXGKDDI_RECOMMENDFUNCTIONALVIDPN       DxgkDdiRecommendFunctionalVidPn;
    PDXGKDDI_ENUMVIDPNCOFUNCMODALITY        DxgkDdiEnumVidPnCofuncModality;
    PDXGKDDI_SETVIDPNSOURCEADDRESS          DxgkDdiSetVidPnSourceAddress;
    PDXGKDDI_SETVIDPNSOURCEVISIBILITY       DxgkDdiSetVidPnSourceVisibility;
    PDXGKDDI_COMMITVIDPN                    DxgkDdiCommitVidPn;
    PDXGKDDI_UPDATEACTIVEVIDPNPRESENTPATH   DxgkDdiUpdateActiveVidPnPresentPath;
    PDXGKDDI_RECOMMENDMONITORMODES          DxgkDdiRecommendMonitorModes;
    PDXGKDDI_RECOMMENDVIDPNTOPOLOGY         DxgkDdiRecommendVidPnTopology;
    PDXGKDDI_GETSCANLINE                    DxgkDdiGetScanLine;
    PDXGKDDI_STOPCAPTURE                    DxgkDdiStopCapture;
    PDXGKDDI_CONTROLINTERRUPT               DxgkDdiControlInterrupt;
    PDXGKDDI_CREATEOVERLAY                  DxgkDdiCreateOverlay;

    //
    // Device functions
    //

    PDXGKDDI_DESTROYDEVICE                  DxgkDdiDestroyDevice;
    PDXGKDDI_OPENALLOCATIONINFO             DxgkDdiOpenAllocation;
    PDXGKDDI_CLOSEALLOCATION                DxgkDdiCloseAllocation;
    PDXGKDDI_RENDER                         DxgkDdiRender;
    PDXGKDDI_PRESENT                        DxgkDdiPresent;

    //
    // Overlay functions
    //

    PDXGKDDI_UPDATEOVERLAY                  DxgkDdiUpdateOverlay;
    PDXGKDDI_FLIPOVERLAY                    DxgkDdiFlipOverlay;
    PDXGKDDI_DESTROYOVERLAY                 DxgkDdiDestroyOverlay;

    //
    // Context supports.
    //

    PDXGKDDI_CREATECONTEXT                  DxgkDdiCreateContext;
    PDXGKDDI_DESTROYCONTEXT                 DxgkDdiDestroyContext;

    //
    // Linked Display Adapter support.
    //

    PDXGKDDI_LINK_DEVICE                    DxgkDdiLinkDevice;
    PDXGKDDI_SETDISPLAYPRIVATEDRIVERFORMAT  DxgkDdiSetDisplayPrivateDriverFormat;

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN7)
    //
    // Extended for WDDM 2.0
    //
    PVOID                                   DxgkDdiDescribePageTable;
    PVOID                                   DxgkDdiUpdatePageTable;
    PVOID                                   DxgkDdiUpdatePageDirectory;
    PVOID                                   DxgkDdiMovePageDirectory;

    PVOID                                   DxgkDdiSubmitRender;
    PVOID                                   DxgkDdiCreateAllocation2;

    //
    // GDI acceleration. Extended for WDDM 1.0
    //
    PDXGKDDI_RENDER                         DxgkDdiRenderKm;

    //
    // New DMM DDIs for CCD support
    //
    VOID*                                   Reserved;
    PDXGKDDI_QUERYVIDPNHWCAPABILITY         DxgkDdiQueryVidPnHWCapability;

#endif // DXGKDDI_INTERFACE_VERSION

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
    PDXGKDDISETPOWERCOMPONENTFSTATE         DxgkDdiSetPowerComponentFState;

    //
    // New DDIs for TDR support.
    //
    PDXGKDDI_QUERYDEPENDENTENGINEGROUP      DxgkDdiQueryDependentEngineGroup;
    PDXGKDDI_QUERYENGINESTATUS              DxgkDdiQueryEngineStatus;
    PDXGKDDI_RESETENGINE                    DxgkDdiResetEngine;

    //
    // New DDIs for PnP stop/start support.
    //
    PDXGKDDI_STOP_DEVICE_AND_RELEASE_POST_DISPLAY_OWNERSHIP DxgkDdiStopDeviceAndReleasePostDisplayOwnership;

    //
    // New DDIs for system display support.
    //
    PDXGKDDI_SYSTEM_DISPLAY_ENABLE          DxgkDdiSystemDisplayEnable;
    PDXGKDDI_SYSTEM_DISPLAY_WRITE           DxgkDdiSystemDisplayWrite;

    PDXGKDDI_CANCELCOMMAND                  DxgkDdiCancelCommand;

    //
    // New DDI for the monitor container ID support.
    //
    PDXGKDDI_GET_CHILD_CONTAINER_ID         DxgkDdiGetChildContainerId;

    PDXGKDDIPOWERRUNTIMECONTROLREQUEST      DxgkDdiPowerRuntimeControlRequest;

    //
    // New DDI for multi plane overlay support.
    //
    PDXGKDDI_SETVIDPNSOURCEADDRESSWITHMULTIPLANEOVERLAY DxgkDdiSetVidPnSourceAddressWithMultiPlaneOverlay;

    //
    // New DDI for the surprise removal support.
    //
    PDXGKDDI_NOTIFY_SURPRISE_REMOVAL        DxgkDdiNotifySurpriseRemoval;

#endif // DXGKDDI_INTERFACE_VERSION

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    //
    // New DDI for querying node metadata
    //
    PDXGKDDI_GETNODEMETADATA                DxgkDdiGetNodeMetadata;

    //
    // New DDI for power management enhancements
    //
    PDXGKDDISETPOWERPSTATE                  DxgkDdiSetPowerPState;
    PDXGKDDI_CONTROLINTERRUPT2              DxgkDdiControlInterrupt2;

    //
    // New DDI for multiplane overlay support
    //
    PDXGKDDI_CHECKMULTIPLANEOVERLAYSUPPORT  DxgkDdiCheckMultiPlaneOverlaySupport;

	//
	// New DDI for GPU clock calibration
	//
	PDXGKDDI_CALIBRATEGPUCLOCK              DxgkDdiCalibrateGpuClock;

	//
	// New DDI for history buffer formatting
	//
	PDXGKDDI_FORMATHISTORYBUFFER            DxgkDdiFormatHistoryBuffer;

#endif

#if (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

    PDXGKDDI_RENDERGDI                      DxgkDdiRenderGdi;
    PDXGKDDI_SUBMITCOMMANDVIRTUAL           DxgkDdiSubmitCommandVirtual;
    PDXGKDDI_SETROOTPAGETABLE               DxgkDdiSetRootPageTable;
    PDXGKDDI_GETROOTPAGETABLESIZE           DxgkDdiGetRootPageTableSize;
    PDXGKDDI_MAPCPUHOSTAPERTURE             DxgkDdiMapCpuHostAperture;
    PDXGKDDI_UNMAPCPUHOSTAPERTURE           DxgkDdiUnmapCpuHostAperture;
    PDXGKDDI_CHECKMULTIPLANEOVERLAYSUPPORT2 DxgkDdiCheckMultiPlaneOverlaySupport2;
    PDXGKDDI_CREATEPROCESS                  DxgkDdiCreateProcess;
    PDXGKDDI_DESTROYPROCESS                 DxgkDdiDestroyProcess;
    PDXGKDDI_SETVIDPNSOURCEADDRESSWITHMULTIPLANEOVERLAY2    DxgkDdiSetVidPnSourceAddressWithMultiPlaneOverlay2;
    void*                                   Reserved1;
    void*                                   Reserved2;
    PDXGKDDI_POWERRUNTIMESETDEVICEHANDLE    DxgkDdiPowerRuntimeSetDeviceHandle;
    PDXGKDDI_SETSTABLEPOWERSTATE            DxgkDdiSetStablePowerState;
    PDXGKDDI_SETVIDEOPROTECTEDREGION        DxgkDdiSetVideoProtectedRegion;

#endif // (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)

} DRIVER_INITIALIZATION_DATA_WIN10;

static CONST DRIVER_INITIALIZATION_DATA_WIN10   DriverInitTableWin10 =
{
    DXGKDDI_INTERFACE_VERSION,
    &LJB_DXGK_AddDevice0,
    &LJB_DXGK_StartDevice,
    &LJB_DXGK_StopDevice,
    &LJB_DXGK_RemoveDevice,
    &LJB_DXGK_DispatchIoRequest,
    &LJB_DXGK_InterruptRoutine,
    &LJB_DXGK_DpcRoutine,
    &LJB_DXGK_QueryChildRelations,
    &LJB_DXGK_QueryChildStatus,
    &LJB_DXGK_QueryDeviceDescriptor,
    &LJB_DXGK_SetPowerState,
    &LJB_DXGK_NotifyAcpiEvent,
    &LJB_DXGK_ResetDevice,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_Unload,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_QueryInterface,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_ControlEtwLogging,
    &LJB_DXGK_QueryAdapterInfo,
    &LJB_DXGK_CreateDevice,
    &LJB_DXGK_CreateAllocation,
    &LJB_DXGK_DestroyAllocation,
    &LJB_DXGK_DescribeAllocation,
    &LJB_DXGK_GetStdAllocationDrvData,
    &LJB_DXGK_AcquireSwizzlingRange,
    &LJB_DXGK_ReleaseSwizzlingRange,
    &LJB_DXGK_Patch,
    &LJB_DXGK_SubmitCommand,
    &LJB_DXGK_PreemptCommand,
    &LJB_DXGK_BuildPagingBuffer,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_SetPalette,
    &LJB_DXGK_SetPointerPosition,
    &LJB_DXGK_SetPointerShape,
    &LJB_DXGK_ResetFromTimeout,
    &LJB_DXGK_RestartFromTimeout,
    &LJB_DXGK_Escape,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_CollectDbgInfo,
    &LJB_DXGK_QueryCurrentFence,
    &LJB_DXGK_IsSupportedVidPn,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_RecommendFunctionalVidPn,
    &LJB_DXGK_EnumVidPnCofuncModality,
    &LJB_DXGK_SetVidPnSourceAddress,
    &LJB_DXGK_SetVidPnSourceVisibility,
    &LJB_DXGK_CommitVidPn,
    &LJB_DXGK_UpdateActiveVidPnPresentPath,
    &LJB_DXGK_RecommendMonitorModes,
    &LJB_DXGK_RecommendVidPnTopology,
    &LJB_DXGK_GetScanLine,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_StopCapture,
    &LJB_DXGK_ControlInterrupt,
    &LJB_DXGK_CreateOverlay,
    &LJB_DXGK_DestroyDevice,
    &LJB_DXGK_OpenAllocation,
    &LJB_DXGK_CloseAllocation,
    &LJB_DXGK_Render,
    &LJB_DXGK_Present,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_UpdateOverlay,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_FlipOverlay,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_DestroyOverlay,
    &LJB_DXGK_CreateContext,
    &LJB_DXGK_DestroyContext,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_LinkDevice,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_SetDisplayPrivateDriverFormat,

    /*
     * The following fields are reserved. Do NOT implement!
     * PDXGKDDI_DESCRIBEPAGETABLE              DxgkDdiDescribePageTable;
     * PDXGKDDI_UPDATEPAGETABLE                DxgkDdiUpdatePageTable;
     * PDXGKDDI_UPDATEPAGEDIRECTORY            DxgkDdiUpdatePageDirectory;
     * PDXGKDDI_MOVEPAGEDIRECTORY              DxgkDdiMovePageDirectory;
     * PDXGKDDI_SUBMITRENDER                   DxgkDdiSubmitRender;
     * PDXGKDDI_CREATEALLOCATION2              DxgkDdiCreateAllocation2;
     */
    NULL,                   /* DxgkDdiDescribePageTable */
    NULL,                   /* DxgkDdiUpdatePageTable */
    NULL,                   /* DxgkDdiUpdatePageDirectory */
    NULL,                   /* DxgkDdiMovePageDirectory */
    NULL,                   /* DxgkDdiSubmitRender */
    NULL,                   /* DxgkDdiCreateAllocation2 */
    &LJB_DXGK_RenderKm,
    NULL,                   /* VOID* Reserved; */
    &LJB_DXGK_QueryVidPnHWCapability,

    /*
     *(DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WIN8)
     */
    &LJB_DXGK_SetPowerComponentFState,
    &LJB_DXGK_QueryDependentEngineGroup,
    &LJB_DXGK_QueryEngineStatus,
    &LJB_DXGK_ResetEngine,
    &LJB_DXGK_StopDeviceAndReleasePostDisplayOwnership,
    &LJB_DXGK_SystemDisplayEnable,
    &LJB_DXGK_SystemDisplayWrite,
    &LJB_DXGK_CancelCommand,
    &LJB_DXGK_GetChildContainerId,
    &LJB_DXGK_PowerRuntimeControlRequest,
    &LJB_DXGK_SetVidPnSourceAddressWithMultiPlaneOverlay,
    &LJB_DXGK_NotifySurpriseRemoval,

    /*
     * (DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
     */
    &LJB_DXGK_GetNodeMetadata,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_SetPowerPState,        // DxgkDdiSetPowerPState: This member is reserved and should be set to zero.
    &LJB_DXGK_ControlInterrupt2,     // MSDN says this shall be zero, but Dxgkrnl.sys is calling into it!
    &LJB_DXGK_CheckMultiPlaneOverlaySupport,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_CalibrateGpuClock,
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_FormatHistoryBuffer,

    /*
     * DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_0
     */
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_RenderGdi,                                    //DxgkDdiRenderGdi
    &LJB_DXGK_SubmitCommandVirtual,                                //DxgkDdiSubmitCommandVirtual
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_SetRootPageTable,                             //DxgkDdiSetRootPageTable
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_GetRootPageTableSize,                         //DxgkDdiGetRootPageTableSize
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_MapCpuHostAperture,                           //DxgkDdiMapCpuHostAperture
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_UnmapCpuHostAperture,                         //DxgkDdiUnmapCpuHostAperture
    &LJB_DXGK_CheckMultiPlaneOverlaySupport2,               //DxgkDdiCheckMultiPlaneOverlaySupport2
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_CreateProcess,                                //DxgkDdiCreateProcess
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_DestroyProcess,                               //DxgkDdiDestroyProcess
    &LJB_DXGK_SetVidPnSourceAddressWithMultiPlaneOverlay2,  //DxgkDdiSetVidPnSourceAddressWithMultiPlaneOverlay2
    NULL,               //Reserved1
    NULL,               //Reserved2
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_PowerRuntimeSetDeviceHandle,                  //DxgkDdiPowerRuntimeSetDeviceHandle
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_SetStablePowerState,                          //DxgkDdiSetStablePowerState
    NULL, // NOT YET IMPLEMENTED &LJB_DXGK_SetVideoProtectedRegion,                      //DxgkDdiSetVideoProtectedRegion
};

static CONST PDXGKDDI_ADD_DEVICE DriverBindingTable[] =
    {
    &LJB_DXGK_AddDevice0,
    &LJB_DXGK_AddDevice1,
    &LJB_DXGK_AddDevice2,
    &LJB_DXGK_AddDevice3
    };

VOID
LJB_FilterPointers(
    __inout PVOID * DstPointers,
    __in PVOID *    SrcPointers,
    __in ULONG      NumOfPointers
    )
{
    UINT    i;

    for (i = 0; i < NumOfPointers; i++)
    {
        if (SrcPointers[i] == NULL)
            DstPointers[i] = NULL;
    }
}

NTSTATUS
LJB_DXGK_InitializeWin7(
    __in PDRIVER_OBJECT                     DriverObject,
    __in PUNICODE_STRING                    RegistryPath,
    __in PDRIVER_INITIALIZATION_DATA        DriverInitializationData
    )
{
    DRIVER_INITIALIZATION_DATA              MyDriverInitData;
    LJB_CLIENT_DRIVER_DATA *                ClientDriverData;
    ULONG                                   DriverInitDataLength;
    DECLARE_CONST_UNICODE_STRING(BasicRenderName, L"\\Driver\\BasicRender");

    /*
     * don't try to hack if BasicRender is calling us.
     */
    if (RtlEqualUnicodeString( &DriverObject->DriverName, &BasicRenderName, TRUE))
        return (*GlobalDriverData.DxgkInitializeWin7)(DriverObject, RegistryPath, DriverInitializationData);

    /*
     * Don't support Vista.
     */
    if (DriverInitializationData->Version < DXGKDDI_INTERFACE_VERSION_WIN7_GOLD)
        return (*GlobalDriverData.DxgkInitializeWin7)(DriverObject, RegistryPath, DriverInitializationData);

    ClientDriverData = LJB_PROXYKMD_GetPoolZero(sizeof(LJB_CLIENT_DRIVER_DATA));
    if (ClientDriverData == NULL)
    {
        KdPrint(("?" __FUNCTION__ ": "
            "Unable to allocate ClientDriverData?\n"
            ));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /*
     * track down the input RegistryPath
     */
    ASSERT(RegistryPath->Length <= MAX_PATH);
    ClientDriverData->RegistryPath.Length = RegistryPath->Length;
    ClientDriverData->RegistryPath.MaximumLength = MAX_PATH;
    ClientDriverData->RegistryPath.Buffer = ClientDriverData->RegistryPathBuffer;
    RtlCopyMemory(
        ClientDriverData->RegistryPathBuffer,
        RegistryPath->Buffer,
        RegistryPath->Length
        );
    ClientDriverData->DriverObject     = DriverObject;
    InsertTailList(
        &GlobalDriverData.ClientDriverListHead,
        &ClientDriverData->ListEntry
        );
    InterlockedIncrement(&GlobalDriverData.ClientDriverListCount);

    KdPrint((__FUNCTION__ ": "
        "Version=(0x%x), RegistryPath(%ws)\n",
        DriverInitializationData->Version,
        ClientDriverData->RegistryPathBuffer
        ));

    RtlZeroMemory(&MyDriverInitData, sizeof(MyDriverInitData));
    if (DriverInitializationData->Version < DXGKDDI_INTERFACE_VERSION_WIN8)
        {
        DriverInitDataLength = sizeof(DRIVER_INITIALIZATION_DATA_WIN7);
        RtlCopyMemory(
            &MyDriverInitData,
            &DriverInitTableWin7,
            DriverInitDataLength
            );
        }
    else if (DriverInitializationData->Version < DXGKDDI_INTERFACE_VERSION_WDDM2_0_PREVIEW)
        {
        /*
         * this is Win8 or Win8.1
         */
        DriverInitDataLength = sizeof(DRIVER_INITIALIZATION_DATA_WIN8);
        RtlCopyMemory(
            &MyDriverInitData,
            &DriverInitTableWin8,
            DriverInitDataLength
            );
        }
    else
        {
        /*
         * this is Win10 or above.
         */
        DriverInitDataLength = sizeof(DRIVER_INITIALIZATION_DATA_WIN10);
        RtlCopyMemory(
            &MyDriverInitData,
            &DriverInitTableWin10,
            DriverInitDataLength
            );
        }
    MyDriverInitData.Version = DriverInitializationData->Version;
    if (DriverInitializationData->Version > DXGKDDI_INTERFACE_VERSION)
        MyDriverInitData.Version = DXGKDDI_INTERFACE_VERSION;

    /*
     * special hack for DxgkDdiAddDevice
     */
    if (!IsListEmpty(&GlobalDriverData.DriverBindingHead))
    {
        LJB_DRIVER_BINDING_TAG *    DriverBindingTag;
        LIST_ENTRY *                listEntry;

        listEntry = RemoveHeadList(&GlobalDriverData.DriverBindingHead);
        DriverBindingTag = CONTAINING_RECORD(
            listEntry,
            LJB_DRIVER_BINDING_TAG,
            ListEntry
            );
        ClientDriverData->DxgkAddDeviceTag = DriverBindingTag->DxgkAddDeviceTag;
        ClientDriverData->DriverBindingTag = DriverBindingTag;
        MyDriverInitData.DxgkDdiAddDevice = DriverBindingTable[ClientDriverData->DxgkAddDeviceTag];
    }
    else
    {
        KdPrint(("?"__FUNCTION__ ": no DriverBindingTag left? No interception for this adapter!\n"));
        RemoveEntryList(&ClientDriverData->ListEntry);
        InterlockedDecrement(&GlobalDriverData.ClientDriverListCount);
        LJB_PROXYKMD_FreePool(ClientDriverData);
        return (*GlobalDriverData.DxgkInitializeWin7)(
            DriverObject,
            RegistryPath,
            DriverInitializationData
            );
    }

    /*
     * NO INTERCEPT DDI:
     * DxgkDdiControlEtwLogging, DxgkDdiUnload, DxgkDdiQueryInterface,
     * DxgkDdiSetPalette, DxgkDdiCollectDbgInfo, DxgkDdiRecommendFunctionalVidPn,
     * DxgkDdiStopCapture, DxgkDdiUpdateOverlay, DxgkDdiFlipOverlay, DxgkDdiDestroyOverlay,
     * DxgkDdiLinkDevice,DxgkDdiSetDisplayPrivateDriverFormat, DxgkDdiRenderKm,
     * DxgkDdiSetPowerPState, DxgkDdiCalibrateGpuClock,DxgkDdiFormatHistoryBuffer
     */
    MyDriverInitData.DxgkDdiControlEtwLogging = DriverInitializationData->DxgkDdiControlEtwLogging;
    MyDriverInitData.DxgkDdiUnload = DriverInitializationData->DxgkDdiUnload;
    MyDriverInitData.DxgkDdiQueryInterface = DriverInitializationData->DxgkDdiQueryInterface;
    MyDriverInitData.DxgkDdiSetPalette = DriverInitializationData->DxgkDdiSetPalette;
    MyDriverInitData.DxgkDdiCollectDbgInfo = DriverInitializationData->DxgkDdiCollectDbgInfo;
    MyDriverInitData.DxgkDdiRecommendFunctionalVidPn = DriverInitializationData->DxgkDdiRecommendFunctionalVidPn;
    MyDriverInitData.DxgkDdiStopCapture = DriverInitializationData->DxgkDdiStopCapture;
    MyDriverInitData.DxgkDdiUpdateOverlay = DriverInitializationData->DxgkDdiUpdateOverlay;
    MyDriverInitData.DxgkDdiFlipOverlay = DriverInitializationData->DxgkDdiFlipOverlay;
    MyDriverInitData.DxgkDdiDestroyOverlay = DriverInitializationData->DxgkDdiDestroyOverlay;
    MyDriverInitData.DxgkDdiLinkDevice = DriverInitializationData->DxgkDdiLinkDevice;
    MyDriverInitData.DxgkDdiSetDisplayPrivateDriverFormat  = DriverInitializationData->DxgkDdiSetDisplayPrivateDriverFormat;
    MyDriverInitData.DxgkDdiRenderKm = DriverInitializationData->DxgkDdiRenderKm;

    /*
     * WDDM1.3 or above
     */
    if (DriverInitializationData->Version >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    {
    MyDriverInitData.DxgkDdiSetPowerPState = DriverInitializationData->DxgkDdiSetPowerPState;
    MyDriverInitData.DxgkDdiCalibrateGpuClock = DriverInitializationData->DxgkDdiCalibrateGpuClock;
    MyDriverInitData.DxgkDdiFormatHistoryBuffer = DriverInitializationData->DxgkDdiFormatHistoryBuffer;
    }
    /*
     * WDDM2.0 or above
     */
    if (DriverInitializationData->Version >= DXGKDDI_INTERFACE_VERSION_WDDM2_0_PREVIEW)
    {
    MyDriverInitData.DxgkDdiRenderGdi = DriverInitializationData->DxgkDdiRenderGdi;
    }

    LJB_FilterPointers(
        (PVOID *) &MyDriverInitData,
        (PVOID *) DriverInitializationData,
        DriverInitDataLength / sizeof(PVOID)
        );

    RtlCopyMemory(
        &ClientDriverData->DriverInitData,
        DriverInitializationData,
        DriverInitDataLength
        );
    return (*GlobalDriverData.DxgkInitializeWin7)(
        DriverObject,
        RegistryPath,
        (DRIVER_INITIALIZATION_DATA *)&MyDriverInitData
        );
}

NTSTATUS
LJB_DXGK_InitializeWin8(
    __in PDRIVER_OBJECT                     DriverObject,
    __in PUNICODE_STRING                    RegistryPath,
    __in PDRIVER_INITIALIZATION_DATA        DriverInitializationData
    )
{
    DRIVER_INITIALIZATION_DATA              MyDriverInitData;
    LJB_CLIENT_DRIVER_DATA *                ClientDriverData;
    ULONG                                   DriverInitDataLength;
    DECLARE_CONST_UNICODE_STRING(BasicRenderName, L"\\Driver\\BasicRender");

    /*
     * don't try to hack if BasicRender is calling us.
     */
    if (RtlEqualUnicodeString( &DriverObject->DriverName, &BasicRenderName, TRUE))
        return (*GlobalDriverData.DxgkInitializeWin8)(DriverObject, RegistryPath, DriverInitializationData);

    ClientDriverData = LJB_PROXYKMD_GetPoolZero(sizeof(LJB_CLIENT_DRIVER_DATA));
    if (ClientDriverData == NULL)
    {
        KdPrint(("?" __FUNCTION__ ": "
            "Unable to allocate ClientDriverData?\n"
            ));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /*
     * track down the input RegistryPath
     */
    ASSERT(RegistryPath->Length <= MAX_PATH);
    ClientDriverData->RegistryPath.Length = RegistryPath->Length;
    ClientDriverData->RegistryPath.MaximumLength = MAX_PATH;
    ClientDriverData->RegistryPath.Buffer = ClientDriverData->RegistryPathBuffer;
    RtlCopyMemory(
        ClientDriverData->RegistryPathBuffer,
        RegistryPath->Buffer,
        RegistryPath->Length
        );
    ClientDriverData->DriverObject     = DriverObject;
    InsertTailList(
        &GlobalDriverData.ClientDriverListHead,
        &ClientDriverData->ListEntry
        );
    InterlockedIncrement(&GlobalDriverData.ClientDriverListCount);

    KdPrint((__FUNCTION__ ": "
        "Version=(0x%x), RegistryPath(%ws)\n",
        DriverInitializationData->Version,
        ClientDriverData->RegistryPathBuffer
        ));

    RtlZeroMemory(&MyDriverInitData, sizeof(MyDriverInitData));
    if (DriverInitializationData->Version < DXGKDDI_INTERFACE_VERSION_WIN8)
        {
        DriverInitDataLength = sizeof(DRIVER_INITIALIZATION_DATA_WIN7);
        RtlCopyMemory(
            &MyDriverInitData,
            &DriverInitTableWin7,
            DriverInitDataLength
            );
        }
    else if (DriverInitializationData->Version < DXGKDDI_INTERFACE_VERSION_WDDM2_0_PREVIEW)
        {
        /*
         * this is Win8 or Win8.1
         */
        DriverInitDataLength = sizeof(DRIVER_INITIALIZATION_DATA_WIN8);
        RtlCopyMemory(
            &MyDriverInitData,
            &DriverInitTableWin8,
            DriverInitDataLength
            );
        }
    else
        {
        /*
         * this is Win10 or above.
         */
        DriverInitDataLength = sizeof(DRIVER_INITIALIZATION_DATA_WIN10);
        RtlCopyMemory(
            &MyDriverInitData,
            &DriverInitTableWin10,
            DriverInitDataLength
            );
        }
    MyDriverInitData.Version = DriverInitializationData->Version;
    if (DriverInitializationData->Version > DXGKDDI_INTERFACE_VERSION)
        MyDriverInitData.Version = DXGKDDI_INTERFACE_VERSION;

    /*
     * special hack for DxgkDdiAddDevice
     */
    if (!IsListEmpty(&GlobalDriverData.DriverBindingHead))
    {
        LJB_DRIVER_BINDING_TAG *    DriverBindingTag;
        LIST_ENTRY *                listEntry;

        listEntry = RemoveHeadList(&GlobalDriverData.DriverBindingHead);
        DriverBindingTag = CONTAINING_RECORD(
            listEntry,
            LJB_DRIVER_BINDING_TAG,
            ListEntry
            );
        ClientDriverData->DxgkAddDeviceTag = DriverBindingTag->DxgkAddDeviceTag;
        ClientDriverData->DriverBindingTag = DriverBindingTag;
        MyDriverInitData.DxgkDdiAddDevice = DriverBindingTable[ClientDriverData->DxgkAddDeviceTag];
    }
    else
    {
        KdPrint(("?"__FUNCTION__ ": no DriverBindingTag left? No interception for this adapter!\n"));
        RemoveEntryList(&ClientDriverData->ListEntry);
        InterlockedDecrement(&GlobalDriverData.ClientDriverListCount);
        LJB_PROXYKMD_FreePool(ClientDriverData);
        return (*GlobalDriverData.DxgkInitializeWin7)(
            DriverObject,
            RegistryPath,
            DriverInitializationData
            );
    }

    /*
     * NO INTERCEPT DDI:
     * DxgkDdiControlEtwLogging, DxgkDdiUnload, DxgkDdiQueryInterface,
     * DxgkDdiSetPalette, DxgkDdiCollectDbgInfo, DxgkDdiRecommendFunctionalVidPn,
     * DxgkDdiStopCapture, DxgkDdiUpdateOverlay, DxgkDdiFlipOverlay, DxgkDdiDestroyOverlay,
     * DxgkDdiLinkDevice,DxgkDdiSetDisplayPrivateDriverFormat, DxgkDdiRenderKm,
     * DxgkDdiCalibrateGpuClock
     */
    MyDriverInitData.DxgkDdiControlEtwLogging = DriverInitializationData->DxgkDdiControlEtwLogging;
    MyDriverInitData.DxgkDdiUnload = DriverInitializationData->DxgkDdiUnload;
    MyDriverInitData.DxgkDdiQueryInterface = DriverInitializationData->DxgkDdiQueryInterface;
    MyDriverInitData.DxgkDdiSetPalette = DriverInitializationData->DxgkDdiSetPalette;
    MyDriverInitData.DxgkDdiCollectDbgInfo = DriverInitializationData->DxgkDdiCollectDbgInfo;
    MyDriverInitData.DxgkDdiRecommendFunctionalVidPn = DriverInitializationData->DxgkDdiRecommendFunctionalVidPn;
    MyDriverInitData.DxgkDdiStopCapture = DriverInitializationData->DxgkDdiStopCapture;
    MyDriverInitData.DxgkDdiUpdateOverlay = DriverInitializationData->DxgkDdiUpdateOverlay;
    MyDriverInitData.DxgkDdiFlipOverlay = DriverInitializationData->DxgkDdiFlipOverlay;
    MyDriverInitData.DxgkDdiDestroyOverlay = DriverInitializationData->DxgkDdiDestroyOverlay;
    MyDriverInitData.DxgkDdiLinkDevice = DriverInitializationData->DxgkDdiLinkDevice;
    MyDriverInitData.DxgkDdiSetDisplayPrivateDriverFormat  = DriverInitializationData->DxgkDdiSetDisplayPrivateDriverFormat;
    MyDriverInitData.DxgkDdiRenderKm = DriverInitializationData->DxgkDdiRenderKm;

    /*
     * WDDM1.3 or above
     */
    if (DriverInitializationData->Version >= DXGKDDI_INTERFACE_VERSION_WDDM1_3)
    {
    MyDriverInitData.DxgkDdiSetPowerPState = DriverInitializationData->DxgkDdiSetPowerPState;
    MyDriverInitData.DxgkDdiCalibrateGpuClock = DriverInitializationData->DxgkDdiCalibrateGpuClock;
    MyDriverInitData.DxgkDdiFormatHistoryBuffer = DriverInitializationData->DxgkDdiFormatHistoryBuffer;
    }
    /*
     * WDDM2.0 or above
     */
    if (DriverInitializationData->Version >= DXGKDDI_INTERFACE_VERSION_WDDM2_0_PREVIEW)
    {
    MyDriverInitData.DxgkDdiRenderGdi = DriverInitializationData->DxgkDdiRenderGdi;
    }

    LJB_FilterPointers(
        (PVOID *) &MyDriverInitData,
        (PVOID *) DriverInitializationData,
        DriverInitDataLength / sizeof(PVOID)
        );

    RtlCopyMemory(
        &ClientDriverData->DriverInitData,
        DriverInitializationData,
        DriverInitDataLength
        );
    return (*GlobalDriverData.DxgkInitializeWin8)(
        DriverObject,
        RegistryPath,
        (DRIVER_INITIALIZATION_DATA *)&MyDriverInitData
        );
}