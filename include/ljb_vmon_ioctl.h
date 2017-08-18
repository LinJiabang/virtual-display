/*!
 	\file		ljb_vmon_ioctl.h
	\brief		IOCTL definition for VMON module
	\details	IOCTL definition for VMON module
	\authors	lucaslin
	\version	0.01a
	\date		June 19, 2017
	\todo		(Optional)
	\bug		(Optional)
	\warning	(Optional)
	\copyright	(c) 2013 Luminon Core Incorporated. All Rights Reserved.

	Revision Log
	+ 0.01a;	June 19, 2017;	lucaslin
	 - Created.

 */

#ifndef _LJB_VMON_IOCTL_H_
#define _LJB_VMON_IOCTL_H_

#pragma warning(disable:4201) /* allow nameless struct/union */
/*
 * definitions borrowed from d3dkmdt.h
 */
#ifndef _D3DKMDT_H
typedef enum _D3DKMDT_VIDPN_PRESENT_PATH_ROTATION
{
    D3DKMDT_VPPR_UNINITIALIZED = 0,

    // Source content is not modified in any way.
    D3DKMDT_VPPR_IDENTITY      = 1,

    // Source content is rotated 90 degrees.
    D3DKMDT_VPPR_ROTATE90      = 2,

    // Source content is rotated 180 degrees.
    D3DKMDT_VPPR_ROTATE180     = 3,

    // Source content is rotated 270 degrees.
    D3DKMDT_VPPR_ROTATE270     = 4,

} D3DKMDT_VIDPN_PRESENT_PATH_ROTATION;
#endif

/*
 * definitions borrowed from d3dkmddi.h
 */
#ifndef _D3DKMDDI_H_
typedef struct _DXGK_POINTERFLAGS
{
    union
    {
        struct
        {
            UINT    Monochrome      : 1;    // 0x00000001
            UINT    Color           : 1;    // 0x00000002
            UINT    MaskedColor     : 1;    // 0x00000004
            UINT    Reserved        :29;    // 0xFFFFFFF8
        };
        UINT        Value;
    };
} DXGK_POINTERFLAGS;
#endif

#define LJB_VMON_IOCTL_BASE        0x0000

/*
 * Theory of Operation
 *
 * This document describes how to use VMON desktop manipulation API to attach/detatch
 * a virtual monitor, and how to grab the pixels of the monitor contents, as well
 * as obtaining cursor events for composing final desktop image. The target audiences
 * are USB/Ethernet display application developer.
 *
 * 1. The user app first locates LJB_VMON driver instance by RegisterDeviceNotification
 *    or  SetupDiGetClassDevs.
 *
 * 2. The user app opens a file handle to LJB_VMON driver by CreateFile
 *    routine.
 *
 * 3. The user app sends IOCTL_LJB_VMON_PLUGIN_MONITOR to kernel driver.
 *    The ljb_vmon.sys creats a virtual monitor and indicates a monitor
 *    arrival event to the OS.
 *
 * 4. If the monitor is no longer in use, the user app sends IOCTL_LJB_VMON_UNPLUG_MONITOR
 *    to signal a monitor departing event. If the user app fails to send
 *    the IOCTL, the kernel mode driver sends the monitor departure event
 *    to OS when the user mode app close the file handle. That is, if user mode
 *    app crashes, the ljb_vmon.sys automatically detaches all virtual monitors
 *    previously attached.
 *
 * 5. Kernel driver associates only 1 virtual monitor with each opened file hanlde
 *    returned from CreateFile(). If the user app want to use multiple monitors,
 *    the user app should call CreateFile() multiple times, and for each file hanle,
 *    the user app sends only 1 IOCTL_LJB_VMON_PLUGIN_MONITOR to the driver.
 *
 * 6. When user app close the file handle, the kernel driver detaches the monitor
 *    if not previously detached.
 *
 * 7. After a file handle is created, the user app waits for monitor event
 *    event in a loop by IOCTL_LJB_VMON_WAIT_FOR_MONITOR_EVENT. Kernel mode
 *    driver (kmd) indicates various monitor events in LJB_VMON_MONITOR_EVENT.
 *    The user mode app could check LJB_VMON_MONITOR_EVENT.flags and determine
 *    which kind of event needs to be processed.
 */

/*
 * Name:  IOCTL_LJB_VMON_PLUGIN_MONITOR
 *
 * details
 *  This IOCTL mimics a monitor plug-in event. The user mode app sends down
 *  EDID data(128 Bytes) to ljb_vmon driver. The LJB_VMON driver supports only
 *  one virtual monitor. The kernel driver fails the request there is already a
 *  IOCTL_LJB_VMON_PLUGIN_MONITOR call previously.
 *
 * parameters
 *    InputBuffer:        A buffer of 128 bytes
 *    InputBufferSize:    128
 *    OutputBuffer:       NULL
 *    OutputBufferSize:   0
 *
 */

#define IOCTL_LJB_VMON_PLUGIN_MONITOR               \
    CTL_CODE(FILE_DEVICE_UNKNOWN,                   \
    LJB_VMON_IOCTL_BASE,                            \
    METHOD_BUFFERED,                                \
    FILE_ANY_ACCESS)

/*
 * Name:  IOCTL_LJB_VMON_UNPLUG_MONITOR
 *
 * details
 *  This IOCTL mimics a monitor unplugn event. The kernel mode driver find the
 *  virtual monitor previously attached by IOCTL_LJB_VMON_PLUGIN_MONITOR and
 *  indicate a monitor departure event.
 *
 * parameters
 *    InputBuffer:        NULL
 *    InputBufferSize:    0
 *    OutputBuffer:       NULL
 *    OutputBufferSize:   0
 *
 */
#define IOCTL_LJB_VMON_UNPLUG_MONITOR               \
    CTL_CODE(FILE_DEVICE_UNKNOWN,                   \
    (LJB_VMON_IOCTL_BASE + 1),                      \
    METHOD_BUFFERED,                                \
    FILE_ANY_ACCESS)

/*
 * Name:  IOCTL_LJB_VMON_WAIT_FOR_MONITOR_EVENT
 *
 * details
 *  The kernel mode driver keeps track of the following events:
 *  1. A particular monitor resolution is committed to the system.
 *  2. VidPnSource visibility change event.
 *  3. screen bitmap change event associated with a monitor.
 *  3. Cursor visibility change event.
 *  4. Cursor shape change event.
 *
 *  User mode app typically sends the IOCTL by turning on all wait flags
 *  in LJB_VMON_WAIT_FLAGS. If any of the event is triggered, the kernel
 *  driver completes the request immediately. Otherwise the caller is blocked
 *  until the kernel driver completes the request.
 *
 *  When the IOCTL is made, kernel mode driver checks Flags member of LJB_VMON_MONIOR_EVENT
 *  structure as input. The input flags instructs kernel driver on how to complete
 *  the request. For example, if user mode driver turns ModeChange flag on
 *  as input, the kernel driver would not complete the request if monitor mode
 *  resolution change event does not occur.  The kernel mode driver checks
 *  every flag that is enabled by the user app, and completes the request if
 *  any of the event is triggered. For example, if user mode enable VidPnSourceBitmapChange
 *  and PointerVisibilityChange flags, kernel driver would would complete the
 *  the request if cursor movement event is detected while the monitor bitmap
 *  isn't changed.
 *
 *  When completing the request, the kernel driver reflects the event in
 *  the output flags. For example, if monitor bitmap has occurred, the kernel
 *  driver would set VidPnSourceBitmapChange flag in the output field.
 *
 *  The user app shall check output flags for determining which event is causing
 *  the completion.
 *
 *  The detailed behavior of event handling is as following:
 *
 *  ModeChange:
 *    Upon input, this flag instruct kernel driver whether there is target mode
 *    change. A mode change occurs when OS calls DxgkDdiCommitVidPn. If a particular
 *    monitor is enabled or disabled in the current VidPn Topology, the kernel driver
 *    completes the pending IOCTL_LJB_VMON_WAIT_FOR_MONITOR_EVENT request.
 *    Upon output, kernel driver set ModeChange flag to indicates that a ModeChange
 *    event occurs.
 *
 *    The kernel driver will also check input TARGET_MODE_DATA. If the input TargetModeData
 *    mismatch the current committed TarggetModeData, the IOCTL is completed
 *    immediately.
 *
 *    When the IOCTL_LJB_VMON_WAIT_FOR_MONITOR_EVENT returns, the user app
 *    should check if ModeChange bit is set. If so, the TargetModeData reflects
 *    the current target mode. The target monitor might not always stayed in
 *    ENABLED state.  User mode app should not try to acquire bitmap if the
 *    monitor mode is not enabled. When the monitor is set to enabled state, the
 *    resolution is set to Width * Height @ 32bpp. User mode
 *    app could allocate buffer for holding the bitmap data.
 *
 *  VidPnSourceVisibilityChange:
 *    The kernel driver detects VidPnSourceVisibilityChange event when OS calls
 *    kernel driver's DxgkDdiSetVidPnSourceVisibility. Upon request completion,
 *    user app should check if VidPnSourceVisibilityChange flag is set. If so,
 *    kernel driver also output VidPnSourceVisibilityData field, which is
 *    described by VIDPN_SOURCE_VISIBILITY_DATA structure.
 *
 *    kernel driver checks also the input VidPnSourceVisibilityData. If the input setting
 *    mismatches the current monitor visibility data, the kernel driver completes the request
 *    immediately.
 *
 *  VidPnSourceBitmapChange:
 *    Kernel driver detects a frame update if the primary surface is updated. For
 *    each update, the kernel driver maintains a incremental FrameId to indicate
 *    the change counts. If VidPnSourceBitmapChange is set upon output, the kernel
 *    driver also update FrameId field. User mode app could detect whether there
 *    is any frame update by comparing the user app's internal FrameId and the
 *    returned FrameId.
 *
 *    kernel driver checks the input FrameId field against kernel driver's internal LastUpdateFrameId
 *    variable. If the 2 variables mismatch, the request is completed immediately.
 *    Otherwise, the request is completed until the LastUpdateFrameId is updated.
 *
 *  PointerPositionChange:
 *    This event is updated when OS calls kernel driver's DxgkDdiSetPointerPosition. Upon
 *    completion, kernel driver updates output PointerPositionData. NOTE that CURSOR POSITION
 *    COULD START AT NEGATIVE VALUE(eg. X = -1, Y= 0) to indicate that cursor
 *    is positioned at CROSS-MONITOR area. User app MUST CALCULATE APPROPRIATE
 *    OFFSET to overlay the cursor image.
 *
 *    NOTE: kernel driver does NOT check input PointerPositionData. The request is always
 *    completed until next cursor movement event.
 *
 *  PointerShapeChange:
 *    The kernel driver update this flag if OS calls into kernel driver's DxgkDdiSetPointerShape.
 *    If kenel mode driver reflects cursor pointer change event, the user app could
 *    issue IOCTL_LJB_VMON_GET_POINTER_SHAPE to query the current cursor shape
 *    data.
 *
 *  The kernel driver only allows 1 IOCTL_LJB_VMON_WAIT_FOR_MONITOR_EVENT at a time
 *  for each opened file handle. If user app sends more than 1 such a request,
 *  the kernel driver fails the 2nd request immediately. User app should send the request
 *  if previous IOCTL_LJB_VMON_WAIT_FOR_MONITOR_EVENT is completed. Typically
 *  user app sends the request in a loop, and wait for the request completion
 *  before starting next IOCTL_LJB_VMON_WAIT_FOR_MONITOR_EVENT.
 *
 * parameters
 *    InputBuffer:        pointer to LJB_VMON_MONITOR_EVENT
 *    InputBufferSize:    sizeof(LJB_VMON_MONITOR_EVENT)
 *    OutputBuffer:       pointer to LJB_VMON_MONITOR_EVENT
 *    OutputBufferSize:   sizeof(LJB_VMON_MONITOR_EVENT)
 *
 */
#define IOCTL_LJB_VMON_WAIT_FOR_MONITOR_EVENT       \
    CTL_CODE(FILE_DEVICE_UNKNOWN,                   \
    LJB_VMON_IOCTL_BASE + 2,                        \
    METHOD_BUFFERED,                                \
    FILE_ANY_ACCESS)

typedef struct _LJB_VMON_WAIT_FLAGS
{
    union
    {
        struct
        {
        UINT    ModeChange: 1;
        UINT    VidPnSourceVisibilityChange: 1;
        UINT    VidPnSourceBitmapChange: 1;
        UINT    PointerPositionChange: 1;
        UINT    PointerShapeChange:1;
        };
        UINT    Value;
    };
} LJB_VMON_WAIT_FLAGS;

typedef struct _TARGET_MODE_DATA
{
    UINT32                                  Enabled: 1;
    UINT32                                  Width;
    UINT32                                  Height;
    D3DKMDT_VIDPN_PRESENT_PATH_ROTATION     Rotation;
} TARGET_MODE_DATA;

typedef struct _VIDPN_SOURCE_VISIBILITY_DATA
{
    BOOLEAN         Visible;
} VIDPN_SOURCE_VISIBILITY_DATA;

typedef struct _POINTER_POSITION_DATA
{
    INT             X;
    INT             Y;
    BOOLEAN         Visible;
} POINTER_POSITION_DATA;

typedef struct _LJB_VMON_MONITOR_EVENT
    {
    LJB_VMON_WAIT_FLAGS             Flags;
    TARGET_MODE_DATA                TargetModeData;
    VIDPN_SOURCE_VISIBILITY_DATA    VidPnSourceVisibilityData;
    ULONG                           FrameId;
    POINTER_POSITION_DATA           PointerPositionData;
    } LJB_VMON_MONITOR_EVENT;

/*
 * this structure is mapped from DXGKARG_SETPOINTERSHAPE. Look for MSDN
 * for detailed descriptions.
 */
#define MAXIMUM_POINTER_WIDTH       256
#define MAXIMUM_POINTER_HEIGHT      256
typedef struct _POINTER_SHAPE_DATA
{
    DXGK_POINTERFLAGS   Flags;
    UINT                Width;
    UINT                Height;
    UINT                Pitch;
    UCHAR               Buffer[MAXIMUM_POINTER_WIDTH * MAXIMUM_POINTER_HEIGHT * 4];
} POINTER_SHAPE_DATA;


/*
 * Name:  IOCTL_LJB_VMON_GET_POINTER_SHAPE
 *
 * details
 *  This IOCTL request kernel mode driver to return the current cursor shape.
 *  The cusor shape data is returned in the OutputBuffer field, which is a
 *  POINTER_SHAPE_DATA structure.
 *
 * parameters
 *    InputBuffer:        NULL
 *    InputBufferSize:    0
 *    OutputBuffer:       pointer to POINTER_SHAPE_DATA
 *    OutputBufferSize:   sizeof(POINTER_SHAPE_DATA)
 *
 */
#define IOCTL_LJB_VMON_GET_POINTER_SHAPE            \
    CTL_CODE(FILE_DEVICE_UNKNOWN,                   \
    LJB_VMON_IOCTL_BASE + 3,                        \
    METHOD_BUFFERED,                                \
    FILE_ANY_ACCESS)

/*
 * Name:  IOCTL_LJB_VMON_BLT_BITMAP
 *
 * details
 *  This request make kernel mode to return the frame buffer described by the
 *  BLT_DATA. User mode app should allocate large enough buffer for holding the
 *  the bitmap data. The size of the bitmap data is returned in TargetModeData
 *  during ModeChange Event. Upon input, the user app instructs the kernel driver to retrieve
 *  the frame buffer asociated with the given FrameId. Since the user app might
 *  be unresponsive and by the time the request reaches to kernel driver, the associated
 *  frame buffer associated with the FrameId might be already changed since last
 *  event report. If such a condition occurs, the kernel mode driver applys the
 *  latest frame, and gives the latest FrameId to user app.
 *
 *  User app should program the Width and Height correctly. The kernel driver
 *  checks the given Width and Height parameter against the current committed
 *  monitor resolution. If resolution mismatches, the kernel driver fails the
 *  request without returning bitmap data. The kernl driver also checks the
 *  given FrameBufferSize. If not large enough buffer is given, the kernel driver
 *  also gives up the request.
 *
 *  If the sanity checking pass, kernel driver locks down the user mode buffer pointed by
 *  FrameBuffer, and copy the desired bitmap data to FrameBuffer. The kernel
 *  driver also update FrameId field upon completing the request.
 *
 *  If 32bit app is running on 64bit OS, the upper 32bit of FrameBuffer is not
 *  used, and should be cleared to zero. Without properly zeroing out un-used
 *  field, the kernel driver might fail the request.
 *
 *  Since frame change event could occur frequently, the user mode buffer lock
 *  down and unlock operation could cause substantial CPU penalty. To miminize
 *  the page lock/unlock operation, user app could prelock the user buffer by
 *  IOCTL_LJB_VMON_LOCK_BUFFER. When the frame buffer is no longer in use,
 *  the user app should send IOCTL_LJB_VMON_LOCK_BUFFER to unlock previously
 *  locked buffer.
 *
 *  If kernel driver detects a user buffer is previously locked down, the kernel driver skips the
 *  user buffer lock down procedures and directly copy pixels to the user buffers.
 *
 * parameters
 *    InputBuffer:        pointer to BLT_DATA
 *    InputBufferSize:    sizeof (BLT_DATA)
 *    OutputBuffer:       pointer to BLT_DATA
 *    OutputBufferSize:   sizeof(BLT_DATA)
 *
 */
#define IOCTL_LJB_VMON_BLT_BITMAP                   \
    CTL_CODE(FILE_DEVICE_UNKNOWN,                   \
    LJB_VMON_IOCTL_BASE + 4,                        \
    METHOD_BUFFERED,                                \
    FILE_ANY_ACCESS)

typedef struct _BLT_DATA
{
    UINT        Width;
    UINT        Height;
    ULONG       FrameId;
    ULONG       FrameBufferSize;
    UINT64      FrameBuffer;
} BLT_DATA;

/*
 * Name:  IOCTL_LJB_VMON_LOCK_BUFFER
 *
 * details
 *
 * parameters
 *    InputBuffer:        pointer to LOCK_BUFFER_DATA
 *    InputBufferSize:    sizeof (LOCK_BUFFER_DATA)
 *    OutputBuffer:       NULL
 *    OutputBufferSize:   0
 */
#define IOCTL_LJB_VMON_LOCK_BUFFER                  \
    CTL_CODE(FILE_DEVICE_UNKNOWN,                   \
    LJB_VMON_IOCTL_BASE + 5,                        \
    METHOD_BUFFERED,                                \
    FILE_ANY_ACCESS)

typedef struct _LOCK_BUFFER_DATA
{
    UINT64      FrameBuffer;
    ULONG       FrameBufferSize;
} LOCK_BUFFER_DATA;

/*
 * Name:  IOCTL_LJB_VMON_UNLOCK_BUFFER
 *
 * details
 *
 * parameters
 *    InputBuffer:        pointer to LOCK_BUFFER_DATA
 *    InputBufferSize:    sizeof (LOCK_BUFFER_DATA)
 *    OutputBuffer:       NULL
 *    OutputBufferSize:   0
 */
#define IOCTL_LJB_VMON_UNLOCK_BUFFER                \
    CTL_CODE(FILE_DEVICE_UNKNOWN,                   \
    LJB_VMON_IOCTL_BASE + 6,                        \
    METHOD_BUFFERED,                                \
    FILE_ANY_ACCESS)

#endif
