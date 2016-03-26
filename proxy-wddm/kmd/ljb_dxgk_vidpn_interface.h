/*
 * ljb_dxgk_vidpn_interface.h
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#ifndef _LJB_DXGK_VIDPN_INTERFACE_H_
#define _LJB_DXGK_VIDPN_INTERFACE_H_

/*
 * LJB_VIDPN object wraps around the VidPn object managed by VidPn manager
 */
#define LJB_VINPN_MAGIC     _MAKE_POOLTAG('V', 'I', 'D', 'P')

typedef struct _LJB_VIDPN_TOPOLOGY
    {
    LJB_ADAPTER *                           Adapter;
    CONST DXGK_VIDPNTOPOLOGY_INTERFACE *    VidPnTopologyInterface;
    D3DKMDT_HVIDPNTOPOLOGY                  hVidPnTopology;
    SIZE_T                                  NumPaths;
    D3DKMDT_VIDPN_PRESENT_PATH *            pPaths;
    } LJB_VIDPN_TOPOLOGY;

typedef struct _LJB_VIDPN
{
    ULONG                           MagicBegin;
    LJB_ADAPTER *                   Adapter;
    D3DKMDT_HVIDPN                  hVidPn;
    DXGK_VIDPN_INTERFACE_VERSION    VidPnInterfaceVersion;
    DXGK_VIDPN_INTERFACE *          VidPnInterface;

    LJB_VIDPN_TOPOLOGY              Topology;

    /*
     * Prefetched Path for internal use. The total number of Paths is depended
     * on the number of TargetId. We assume there are at most 64 targets in the system.
     */
    SIZE_T                          NumPaths;
    D3DKMDT_VIDPN_PRESENT_PATH      Paths[MAX_NUM_OF_INBOX_MONITOR+MAX_NUM_OF_USB_MONITOR];

}   LJB_VIDPN;

/*
 * C function declaration
 */
_C_BEGIN

LJB_VIDPN *
LJB_VIDPN_CreateVidPn(
    __in LJB_ADAPTER *  Adapter,
    __in D3DKMDT_HVIDPN hVidPn
    );

VOID
LJB_VIDPN_DestroyVidPn(
    __in LJB_VIDPN *    MyVidPn
    );

NTSTATUS
LJB_VIDPN_PrefetchTopology(
    __in LJB_VIDPN *    MyVidPn
    );
UINT
LJB_VIDPN_GetNumberOfUsbTarget(
    __in LJB_VIDPN *    MyVidPn
    );
UINT
LJB_VIDPN_GetNumberOfInboxTarget(
    __in LJB_VIDPN *    MyVidPn
    );

NTSTATUS
LJB_DXGKCB_QueryVidPnInterface(
    __in CONST D3DKMDT_HVIDPN               hVidPn,
    __in CONST DXGK_VIDPN_INTERFACE_VERSION VidPnInterfaceVersion,
    __out CONST DXGK_VIDPN_INTERFACE**      ppVidPnInterface
    );

NTSTATUS
LJB_VIDPN_GetTopology(
    __in CONST D3DKMDT_HVIDPN                   hVidPn,
    __out D3DKMDT_HVIDPNTOPOLOGY*               phVidPnTopology,
    __out CONST DXGK_VIDPNTOPOLOGY_INTERFACE**  ppVidPnTopologyInterface
    );
NTSTATUS
LJB_VIDPN_AcquireSourceModeSet(
    __in CONST D3DKMDT_HVIDPN                       hVidPn,
    __in CONST D3DDDI_VIDEO_PRESENT_SOURCE_ID       VidPnSourceId,
    __out D3DKMDT_HVIDPNSOURCEMODESET*              phVidPnSourceModeSet,
    __out CONST DXGK_VIDPNSOURCEMODESET_INTERFACE** ppVidPnSourceModeSetInterface
    );
NTSTATUS
LJB_VIDPN_ReleaseSourceModeSet(
    __in CONST D3DKMDT_HVIDPN               hVidPn,
    __in CONST D3DKMDT_HVIDPNSOURCEMODESET  hVidPnSourceModeSet
    );
NTSTATUS
LJB_VIDPN_CreateNewSourceModeSet(
    __in CONST D3DKMDT_HVIDPN                       hVidPn,
    __in CONST D3DDDI_VIDEO_PRESENT_SOURCE_ID       VidPnSourceId,
    __out D3DKMDT_HVIDPNSOURCEMODESET*              phNewVidPnSourceModeSet,
    __out CONST DXGK_VIDPNSOURCEMODESET_INTERFACE** ppVidPnSourceModeSetInterface
    );
NTSTATUS
LJB_VIDPN_AssignSourceModeSet(
    __in D3DKMDT_HVIDPN                         hVidPn,
    __in CONST D3DDDI_VIDEO_PRESENT_SOURCE_ID   VidPnSourceId,
    __in CONST D3DKMDT_HVIDPNSOURCEMODESET      hVidPnSourceModeSet
    );
NTSTATUS
LJB_VIDPN_AssignMultiSampleSourceModeSet(
    __in D3DKMDT_HVIDPN                         hVidPn,
    __in CONST D3DDDI_VIDEO_PRESENT_SOURCE_ID   VidPnSourceId,
    __in CONST SIZE_T                           NumMethods,
    __in CONST D3DDDI_MULTISAMPLINGMETHOD*      pSupportedMethodSet
    );
NTSTATUS
LJB_VIDPN_AcquireTargetModeSet(
    __in CONST D3DKMDT_HVIDPN                       hVidPn,
    __in CONST D3DDDI_VIDEO_PRESENT_TARGET_ID       VidPnTargetId,
    __out D3DKMDT_HVIDPNTARGETMODESET*              phVidPnTargetModeSet,
    __out CONST DXGK_VIDPNTARGETMODESET_INTERFACE** ppVidPnTargetModeSetInterface
    );
NTSTATUS
LJB_VIDPN_ReleaseTargetModeSet(
    __in CONST D3DKMDT_HVIDPN               hVidPn,
    __in CONST D3DKMDT_HVIDPNTARGETMODESET  hVidPnTargetModeSet
    );
NTSTATUS
LJB_VIDPN_CreateNewTargetModeSet(
    __in CONST D3DKMDT_HVIDPN                       hVidPn,
    __in CONST D3DDDI_VIDEO_PRESENT_TARGET_ID       VidPnTargetId,
    __out D3DKMDT_HVIDPNTARGETMODESET*              phNewVidPnTargetModeSet,
    __out CONST DXGK_VIDPNTARGETMODESET_INTERFACE** ppVidPnTargetModeSetInterface
    );
NTSTATUS
LJB_VIDPN_AssignTargetModeSet(
    __in D3DKMDT_HVIDPN                         hVidPn,
    __in CONST D3DDDI_VIDEO_PRESENT_TARGET_ID   VidPnTargetId,
    __in CONST D3DKMDT_HVIDPNTARGETMODESET      hVidPnTargetModeSet
    );

extern CONST DXGK_VIDPNTOPOLOGY_INTERFACE MyTopologyInterface;
VOID
LJB_VIDPN_TOPOLOGY_Initialize(
    __in LJB_ADAPTER *          Adapter,
    __in LJB_VIDPN_TOPOLOGY *   VidPnTopology
    );
NTSTATUS
LJB_VIDPN_TOPOLOGY_GetNumPaths(
	__in CONST D3DKMDT_HVIDPNTOPOLOGY   hVidPnTopology,
    __out SIZE_T*                       pNumPaths
    );
NTSTATUS
LJB_VIDPN_TOPOLOGY_GetNumPathsFromSource(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY           hVidPnTopology,
    __in CONST D3DDDI_VIDEO_PRESENT_SOURCE_ID   VidPnSourceId,
    __out SIZE_T*                               pNumPathsFromSource
    );
NTSTATUS
LJB_VIDPN_TOPOLOGY_EnumPathTargetsFromSource(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY           hVidPnTopology,
    __in CONST D3DDDI_VIDEO_PRESENT_SOURCE_ID   VidPnSourceId,
    __in CONST D3DKMDT_VIDPN_PRESENT_PATH_INDEX VidPnPresentPathIndex,
    __out D3DDDI_VIDEO_PRESENT_TARGET_ID*       pVidPnTargetId
    );
NTSTATUS
LJB_VIDPN_TOPOLOGY_GetPathSourceFromTarget(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY           hVidTopology,
    __in CONST D3DDDI_VIDEO_PRESENT_TARGET_ID   VidPnTargetId,
    __out D3DDDI_VIDEO_PRESENT_SOURCE_ID*       pVidPnSourceId
    );
NTSTATUS
LJB_VIDPN_TOPOLOGY_AcquirePathInfo(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY           hVidPnTopology,
    __in CONST D3DDDI_VIDEO_PRESENT_SOURCE_ID   VidPnSourceId,
    __in CONST D3DDDI_VIDEO_PRESENT_TARGET_ID   VidPnTargetId,
    __out CONST D3DKMDT_VIDPN_PRESENT_PATH**    pVidPnPresentPathInfo
    );
NTSTATUS
LJB_VIDPN_TOPOLOGY_AcquireFirstPathInfo(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY           hVidPnTopology,
    __out CONST D3DKMDT_VIDPN_PRESENT_PATH**    ppFirstVidPnPresentPathInfo
    );
NTSTATUS
LJB_VIDPN_TOPOLOGY_AcquireNextPathInfo(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY               hVidPnTopology,
    __in CONST D3DKMDT_VIDPN_PRESENT_PATH* CONST    pVidPnPresentPathInfo,
    __out CONST D3DKMDT_VIDPN_PRESENT_PATH**        ppNextVidPnPresentPathInfo
    );
NTSTATUS
LJB_VIDPN_TOPOLOGY_UpdatePathSupportInfo(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY               hVidPnTopology,
    __in CONST D3DKMDT_VIDPN_PRESENT_PATH* CONST    pVidPnPresentPathInfo
    );
NTSTATUS
LJB_VIDPN_TOPOLOGY_ReleasePathInfo(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY               hVidPnTopology,
    __in CONST D3DKMDT_VIDPN_PRESENT_PATH* CONST    pVidPnPresentPathInfo
    );
NTSTATUS
LJB_VIDPN_TOPOLOGY_CreateNewPathInfo(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY   hVidPnTopology,
    __out D3DKMDT_VIDPN_PRESENT_PATH**  ppNewVidPnPresentPathInfo
    );
NTSTATUS
LJB_VIDPN_TOPOLOGY_AddPath(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY       hVidPnTopology,
    __in D3DKMDT_VIDPN_PRESENT_PATH* CONST  pVidPnPresentPath
    );
NTSTATUS
LJB_VIDPN_TOPOLOGY_RemovePath(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY           hVidPnTopology,
    __in CONST D3DDDI_VIDEO_PRESENT_SOURCE_ID   VidPnSourceId,
    __in CONST D3DDDI_VIDEO_PRESENT_TARGET_ID   VidPnTargetId
    );

_C_END

#endif
