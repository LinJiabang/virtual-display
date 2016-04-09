#ifndef _LJB_MONITOR_INTERFACE_H_
#define _LJB_MONITOR_INTERFACE_H_
#include <ntddk.h>

/*
 * LJB_MONITOR_INTERFACE is returned from USB monitor driver when proxy driver
 * query its interface.
 */
typedef NTSTATUS
LJB_GENERIC_IOCTL(
    __in PVOID          ProviderContext,
    __in ULONG          IoctlCode,
    __in_opt PVOID      InputBuffer,
    __in SIZE_T         InputBufferSize,
    __out_opt PVOID     OutputBuffer,
    __in SIZE_T         OutputBufferSize,
    __out ULONG *       BytesReturned
    );

typedef struct _LJB_MONITOR_INTERFACE
{
    USHORT                  Size;
    USHORT                  Version;
    PVOID                   Context;
    PINTERFACE_REFERENCE    InterfaceReference;
    PINTERFACE_DEREFERENCE  InterfaceDereference;
    //  interface-specific  entries go here

    LJB_GENERIC_IOCTL *     pfnGenericIoctl;
} LJB_MONITOR_INTERFACE, *PLJB_MONITOR_INTERFACE;

#define LJB_MONITOR_INTERFACE_V0    0

/*
 * FUNCTION:
 *  LJB_GENERIC_IOCTL_SET_PROXY_CALLBACK
 *
 * Parameters:
 *  InputBuffer = pointer to LJB_PROXY_CALLBACK
 *  InputBufferSize = sizeof(LJB_PROXY_CALLBACK)
 *  OutputBuffer = None
 *  OutputBufferSize = 0
 *  BytesReturned = don't care.
 *
 * Desciption:
 *  Notifies the USB monitor driver that a VidPn is committed.
 */
#define LJB_GENERIC_IOCTL_SET_PROXY_CALLBACK            0

typedef struct _LJB_PROXY_CALLBACK
{
    LJB_GENERIC_IOCTL *     pfnGenericIoctl;
    PVOID                   Context;
} LJB_PROXY_CALLBACK;

/*
 * FUNCTION:
 *  LJB_GENERIC_IOCTL_GET_EDID
 *
 * Parameters:
 *  InputBuffer = NULL
 *  InputBufferSize = 0
 *  OutputBuffer = pointer to 256-bytes buffer
 *  OutputBufferSize = 256
 *  BytesReturned = don't care.
 *
 * Desciption:
 *  Queries the EDID data from USB monitor driver.
 */
#define LJB_GENERIC_IOCTL_GET_EDID                      1
#define LJB_DEFAULT_EDID_DATA_SIZE                      256

/*
 * FUNCTION:
 *  LJB_GENERIC_IOCTL_COMMIT_VIDPN
 *
 * Parameters:
 *  InputBuffer = pointer to LJB_COMMIT_VIDPN
 *  InputBufferSize = sizeof(LJB_COMMIT_VIDPN)
 *  OutputBuffer = None
 *  OutputBufferSize = 0
 *  BytesReturned = don't care.
 *
 * Desciption:
 *  Notifies the USB monitor driver that a VidPn is committed.
 */
#define LJB_GENERIC_IOCTL_COMMIT_VIDPN                  2

typedef struct _LJB_COMMIT_VIDPN
{
    D3DKMDT_VIDPN_PRESENT_PATH  CommitPath;
    ULONG                       Width;
    ULONG                       Height;
    ULONG                       BitPerPixel;
} LJB_COMMIT_VIDPN;

/*
 * FUNCTION:
 *  LJB_GENERIC_IOCTL_CREATE_PRIMARY_SURFACE
 *
 * Parameters:
 *  InputBuffer = pointer to LJB_PRIMARY_SURFACE
 *  InputBufferSize = sizeof(LJB_PRIMARY_SURFACE)
 *  OutputBuffer = None
 *  OutputBufferSize = 0
 *  BytesReturned = don't care.
 *
 * Desciption:
 *  Notifies the USB monitor that a primary surface is created
 */
#define LJB_GENERIC_IOCTL_CREATE_PRIMARY_SURFACE        3
#define LJB_GENERIC_IOCTL_DESTROY_PRIMARY_SURFACE       4

typedef struct _LJB_PRIMARY_SURFACE
{
    ULONG                       Width;
    ULONG                       Height;
    ULONG                       BitPerPixel;

    ULONG                       BufferSize;
    PVOID                       RemoteBuffer;
    PVOID                       SurfaceHandle;
} LJB_PRIMARY_SURFACE;

/*
 * FUNCTION:
 *  LJB_GENERIC_IOCTL_SET_VIDPN_SOURCE_VISIBLE
 *
 * Parameters:
 *  InputBuffer = pointer to BOOLEAN
 *  InputBufferSize = sizeof(BOOLEAN)
 *  OutputBuffer = None
 *  OutputBufferSize = 0
 *  BytesReturned = don't care.
 *
 * Desciption:
 *  Notifies the USB monitor driver about the monitor visibility.
 */
#define LJB_GENERIC_IOCTL_SET_VIDPN_SOURCE_VISIBLE      5

#endif