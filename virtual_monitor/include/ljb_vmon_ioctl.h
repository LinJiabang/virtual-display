#ifndef _LJB_VMON_IOCTL_H_
#define _LJB_VMON_IOCTL_H_

#define LJB_VMON_IOCTL_BASE        0x0000

/*
 * Name:  IOCTL_LJB_VMON_ACQUIRE_FRAME
 *
 * details
 *    This IOCTL requests VMON func driver to return the latest frame buffer pointer,
 *    as well as the frame information. VMON driver maps its internal frame
 *    buffer into user space, and fill in the frame information. The user mode
 *    app should release the frame by IOCTL_LJB_VMON_RELEASE_FRAME when finishes
 *    using it.
 *
 *    If the mapping operation is successful, VMON driver output the user
 *    frame address as well as the frame information into LJB_VMON_FRAME_INFO
 *    structure specified in OutputBuffer.
 *
 * parameters
 *    InputBuffer:        NULL
 *    InputBufferSize:    0
 *    OutputBuffer:       pointer to LJB_VMON_FRAME_INFO
 *    OutputBufferSize:   sizeof(LJB_VMON_FRAME_INFO)
 *
 */
#define IOCTL_LJB_VMON_ACQUIRE_FRAME                \
    CTL_CODE(FILE_DEVICE_UNKNOWN,                   \
    LJB_VMON_IOCTL_BASE + 2,                        \
    METHOD_BUFFERED,                                \
    FILE_ANY_ACCESS)

typedef struct _LJB_VMON_FRAME_INFO
    {
    ULONG64         KernelFrameHandle;
    ULONG64         UserFrameAddress;
    ULONG           FrameSize;
    UINT32          Width;
    UINT32          Height;
    UINT32          Pitch;
    UINT32          BytesPerPixel;
    ULONG           FrameIdAcquired;
    } LJB_VMON_FRAME_INFO;

/*
 * Name:  IOCTL_LJB_VMON_RELEASE_FRAME
 * 
 * details
 *    This IOCTL releases the frame buffer previously acquired by
 *    IOCTL_LJB_VMON_ACQUIRE_FRAME
 * 
 * 
 * parameters
 *    InputBuffer:        pointer to LJB_VMON_FRAME_INFO
 *    InputBufferSize:    sizeof(LJB_VMON_FRAME_INFO)
 *    OutputBuffer:       NULL
 *    OutputBufferSize:   0
 * 
 */
#define IOCTL_LJB_VMON_RELEASE_FRAME                \
    CTL_CODE(FILE_DEVICE_UNKNOWN,                   \
    LJB_VMON_IOCTL_BASE + 3,                        \
    METHOD_BUFFERED,                                \
    FILE_ANY_ACCESS)

/*
 * Name:  IOCTL_LJB_VMON_WAIT_FOR_UPDATE
 * 
 * details
 *    This IOCTL requests VMON driver to report the latest frame or cursor
 *    update status. The user app provides LJB_VMON_WAIT_FOR_UPDATE_DATA to indicate to
 *    the kernel driver whether the user app is waiting for frame update or cursor
 *    update data.
 * 
 *    Upon input, if Flags.Frame is set, the driver would complete the IOCTL
 *    immediately if the current frame has FenceId larger than the FenceId provided.
 *    Otherwise, the IOCTL is not completed  until after any frame update event
 *    occurs. If Flags.Frame is not set, the driver ignores the frame update event,
 *    and would not block the IOCTL from completion. If  Flags.Cursor is set, the
 *    kernel driver would complete the IOCTL if any cursor update event occur
 *    after the IOCTL trigger. If Flags.Cursor is not set, kernel driver ignores
 *    the cursor update event, and would not block the IOCTL.
 * 
 *    Upon output, the kernel reports the update status.
 * 
 * parameters
 *    InputBuffer:        pointer to LJB_VMON_WAIT_FOR_UPDATE_DATA
 *    InputBufferSize:    sizeof (LJB_VMON_WAIT_FOR_UPDATE_DATA)
 *    OutputBuffer:       pointer to LJB_VMON_WAIT_FOR_UPDATE_DATA
 *    OutputBufferSize:   sizeof(LJB_VMON_WAIT_FOR_UPDATE_DATA)
 * 
 */

#define IOCTL_LJB_VMON_WAIT_FOR_UPDATE              \
    CTL_CODE(FILE_DEVICE_UNKNOWN,                   \
    LJB_VMON_IOCTL_BASE + 4,                        \
    METHOD_BUFFERED,                                \
    FILE_ANY_ACCESS)

typedef struct _LJB_WAIT_FLAGS
    {
    UINT32        Frame: 1;
    UINT32        Cursor: 1;
    } LJB_WAIT_FLAGS;

typedef struct _LJB_VMON_WAIT_FOR_UPDATE_DATA
    {
    LJB_WAIT_FLAGS      Flags;
    UINT32              FrameId;
    } LJB_VMON_WAIT_FOR_UPDATE_DATA;

#endif /* _LJB_VMON_IOCTL_H_ */
