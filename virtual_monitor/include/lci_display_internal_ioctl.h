/*!
 	\file		lci_display_internal_ioctl.h
	\brief		Internal IOCTL definition used by ProxyKMD & USB AV module
	\details	Internal IOCTL definition used by ProxyKMD & USB AV module
	\authors	lucaslin
	\version	0.01a
	\date		June 28, 2013
	\todo		(Optional)
	\bug		(Optional)
	\warning	(Optional)
	\copyright	(c) 2013 Luminon Core Incorporated. All Rights Reserved.

	Revision Log
	+ 0.01a;	June 28, 2013;  lucaslin
	 - Created.

	+ 0.01a;    Sep. 4, 2013;   robert
     - Add visibility IOCTL support.

 */

#ifndef _LCI_USBAV_INTERNAL_IOCTL_H_
#define _LCI_USBAV_INTERNAL_IOCTL_H_

#include <dispmprt.h>

#define LCI_USBAV_INTERNAL_IOCTL_BASE   0x0000

/*!
 Name:  INTERNAL_IOCTL_QUERY_USB_MONITOR_INTERFACE

 \details
    LCI PROXY WDDM driver sends this internal IOCTL to USB monitor driver to obtain
    access to the USB Monitor device. The USB Monitor driver could only be acquired by one proxy
    driver. In an multiple display cards environment, the ProxyKmd driver
    registers Pnp interface notification for USB Monitor device on behalf of each
    adapter found in the system. This creates multiple accessing to the same USB
    AV device from multiple display adapter. The USB Monitor driver guards multiple
    access, and only allow the first ProxyKmd access.

    Once the access is granted by USB Monitor driver, the USB Monitor driver returns its
    generic access interface in OutputBuffer of the InternalIoctl.

    The ProxyKmd driver also provides a generic access interface in InputBuffer
    of the internal IOCTL.

 */
#define INTERNAL_IOCTL_QUERY_USB_MONITOR_INTERFACE  \
    CTL_CODE(FILE_DEVICE_UNKNOWN,                   \
    LCI_USBAV_INTERNAL_IOCTL_BASE,                  \
    METHOD_NEITHER,                                 \
    FILE_ANY_ACCESS)

typedef NTSTATUS
LCI_GENERIC_IOCTL(
    __in PVOID          ProviderContext,
    __in ULONG          IoctlCode,
    __in_opt PVOID      InputBuffer,
    __in SIZE_T         InputBufferSize,
    __out_opt PVOID     OutputBuffer,
    __in SIZE_T         OutputBufferSize,
    __out ULONG *       BytesReturned
    );

typedef VOID
LCI_RELEASE_INTERFACE(
    __in PVOID          ProviderContext
    );

typedef struct _LCI_GENERIC_INTERFACE
    {
    ULONG                   Version;
    SIZE_T                  Size;

    PVOID                   ProviderContext;
    LCI_GENERIC_IOCTL *     pfnGenericIoctl;
    LCI_RELEASE_INTERFACE * pfnReleaseInterface;

    } LCI_GENERIC_INTERFACE;

#define LCI_GENERIC_INTERFACE_V1    1

/*
 Generic IOCTL call from ProxyKmd to UsbAv driver. All generic IOCTL down calls
 has the prefix LCI_PROXYKMD_xxx
 */
#define LCI_PROXYKMD_GET_EDID                           0
#define LCI_DEFAULT_EDID_DATA_SIZE                      256

#define LCI_PROXYKMD_NOTIFY_PRIMARY_SURFACE_CREATE      1
typedef struct _LCI_PROXYKMD_PRIMARY_SURFACE_CREATE
    {
    HANDLE              hPrimarySurface;
    PVOID               pBuffer;
    SIZE_T              BufferSize;

    UINT                Width;
    UINT                Height;
    UINT                Pitch;
    UINT                BytesPerPixel;
    } LCI_PROXYKMD_PRIMARY_SURFACE_CREATE;

#define LCI_PROXYKMD_NOTIFY_PRIMARY_SURFACE_DESTROY     2
typedef struct _LCI_PROXYKMD_PRIMARY_SURFACE_DESTROY
    {
    HANDLE              hPrimarySurface;
    PVOID               pBuffer;
    SIZE_T              BufferSize;

    UINT                Width;
    UINT                Height;
    UINT                Pitch;
    UINT                BytesPerPixel;
    } LCI_PROXYKMD_PRIMARY_SURFACE_DESTROY;

#define LCI_PROXYKMD_NOTIFY_PRIMARY_SURFACE_UPDATE      3
typedef struct _LCI_PROXYKMD_PRIMARY_SURFACE_UPDATE
    {
    HANDLE              hPrimarySurface;
    PVOID               pBuffer;
    SIZE_T              BufferSize;
	ULONG				FrameId;

    UINT                Width;
    UINT                Height;
    UINT                Pitch;
    UINT                BytesPerPixel;
    } LCI_PROXYKMD_PRIMARY_SURFACE_UPDATE;

#define LCI_PROXYKMD_NOTIFY_CURSOR_UPDATE               4
typedef struct _LCI_PROXYKMD_CURSOR_UPDATE
    {
    CONST DXGKARG_SETPOINTERPOSITION *  pPositionUpdate;
    CONST DXGKARG_SETPOINTERSHAPE *     pShapeUpdate;
    } LCI_PROXYKMD_CURSOR_UPDATE;

#define LCI_PROXYKMD_NOTIFY_VISIBILITY_UPDATE			5
typedef struct _LCI_PROXYKMD_VISIBILITY_UPDATE
    {
	D3DDDI_VIDEO_PRESENT_SOURCE_ID VidPnSourceId;
	BOOLEAN                        Visible;
    } LCI_PROXYKMD_VISIBILITY_UPDATE;

#define LCI_PROXYKMD_NOTIFY_VSYNC                       6

#define LCI_PROXYKMD_NOTIFY_MEDIA_STATE                 7

#define LCI_PROXYKMD_NOTIFY_COMMIT_VIDPN                8
typedef struct _LCI_PROXYKMD_COMMIT_VIDPN
    {
    UINT                                            Width;
    UINT                                            Height;
    UINT                                            Pitch;
    UINT                                            BytesPerPixel;

    D3DKMDT_VIDPN_PRESENT_PATH_TRANSFORMATION       ContentTransformation;
    } LCI_PROXYKMD_COMMIT_VIDPN;

#define LCI_PROXYKMD_NOTIFY_IS_INTEL_INTEGRATED_GPU     9

/*
 Generic IOCTL call from UsbAV to ProxyKmd driver. All generic IOCTL up calls
 has the prefix LCI_USBAV_xxx.

 Currently the upcall IOCTLs are designed to support pixels update. 2 flavors
 of pixel update are defined:

 1. USB AV driver request ProxyKmd driver to copy pixels from primary surface
    buffer managed by ProxyKMD to shadow buffer allocated by USB AV driver.
    ProxyKmd ensures the update operation is tear-free. That is to say, when
    ProxyKmd is performing bits copy operation, ProxyKmd guards against
    concurrent access to the primary buffer surface. While ProxyKmd is accessing
    the primary buffer, DxgkDdiSubmitCommand processing is not allowed to change
    the contents of primary surface until after ProxyKmd  finishes the copy.

    This mechanism is simple to implement, but with the drawback of excessive
    memory copy.

 2. USB AV driver locks the primary surface until it finishes using the buffers.
    ProxyKmd keeps track of the lock state, and block-out the access to the
    primary surface. If the USB Monitor driver fails to unlock the primary buffer,
    it would causes system freeze, and timout recovery process in Direct X
    runtime.

    The suggested usage of this mechanism is to hold the lock as short as possible.
    For example, the USB Monitor driver could lock the buffer down, and perform
    kernel mode compression without involving user mode app, and unlock the
    surface in one routine. It is not suggested to lock and unlock the buffer
    from user mode app since the user mode app is not trusty.

 */
#define LCI_USBAV_BLT_PRIMARY_TO_SHADOW                 0
typedef struct _LCI_USBAV_BLT_DATA
    {
    HANDLE              hPrimarySurface;
    PVOID               pPrimaryBuffer;
    PVOID               pShadowBuffer;
    SIZE_T              BufferSize;
    ULONGLONG           FrameTimeStamp;
    } LCI_USBAV_BLT_DATA;

#define LCI_USBAV_LOCK_PRIMARY_SURFACE                  1
typedef struct _LCI_USBAV_LOCK_PRIMARY_SURFACE_DATA
    {
    HANDLE              hPrimarySurface;
    } LCI_USBAV_LOCK_PRIMARY_SURFACE_DATA;
#define LCI_USBAV_UNLOCK_PRIMARY_SURFACE                2

#endif