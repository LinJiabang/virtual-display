/*
 * LuminonCore USBAV Driver IOCTL definitions
 */

#ifndef _LCI_USBAV_IOCTL_H_
#define _LCI_USBAV_IOCTL_H_

#define LCI_USBAV_IOCTL_BASE        0x0000

#define IOCTL_LCI_USBAV_GET_CONFIG_DESCRIPTOR       \
    CTL_CODE(FILE_DEVICE_UNKNOWN,                   \
    LCI_USBAV_IOCTL_BASE,                           \
    METHOD_BUFFERED,                                \
    FILE_ANY_ACCESS)

/*!
 Name:  IOCTL_LCI_USBAV_GENERIC_IO

 \details
    User space app sends provides the buffer for USB AV command message, as
    well as response message. The USB AV command message is placed in the
    lpInBuffer, and the size of Command message is specified by nInBufferSize.
    The Response message is specified by lpOutBuffer, and the response buffer
    size is specified by nOutBufferSize. For best efficiency, we use 
    METHOD_NEITHER.
    
    The Response Message buffer is required to be multiple of MaximumPacketSize.
    If the nOutBufferSize is not multiple of MaximumPacketSize (eg. 1024 on
    superspeed bulk endpoint), the driver rejects the request with
    STATUS_INVALID_PARAMETER.
 */
#define IOCTL_LCI_USBAV_GENERIC_IO                  \
    CTL_CODE(FILE_DEVICE_UNKNOWN,                   \
    LCI_USBAV_IOCTL_BASE + 1,                       \
    METHOD_NEITHER,                                 \
    FILE_ANY_ACCESS)

/*!
 Name:  IOCTL_LCI_USBAV_ACQUIRE_FRAME

 \details
    This IOCTL requests USB AV driver to return the latest frame buffer pointer,
    as well as the frame information. USB AV driver maps its internal frame
    buffer into user space, and fill in the frame information. The user mode
    app should release the frame by IOCTL_LCI_USBAV_RELEASE_FRAME when finishes
    using it.
    
    If the mapping operation is successful, USB AV driver output the user 
    frame address as well as the frame information into LCI_USBAV_FRAME_INFO
    structure specified in OutputBuffer.

 \parameters
    InputBuffer:        NULL
    InputBufferSize:    0
    OutputBuffer:       pointer to LCI_USBAV_FRAME_INFO
    OutputBufferSize:   sizeof(LCI_USBAV_FRAME_INFO)

 */
#define IOCTL_LCI_USBAV_ACQUIRE_FRAME               \
    CTL_CODE(FILE_DEVICE_UNKNOWN,                   \
    LCI_USBAV_IOCTL_BASE + 2,                       \
    METHOD_BUFFERED,                                \
    FILE_ANY_ACCESS)
    
typedef struct _LCI_USBAV_FRAME_INFO
    {
    ULONG64         KernelFrameHandle;
    ULONG64         UserFrameAddress;
    ULONG           FrameSize;
    UINT            Width;
    UINT            Height;
    UINT            Pitch;
    UINT            BytesPerPixel;
	ULONG			FrameIdAcquired;
    } LCI_USBAV_FRAME_INFO;
    
/*!
 Name:  IOCTL_LCI_USBAV_RELEASE_FRAME

 \details
    This IOCTL releases the frame buffer previously acquired by
    IOCTL_LCI_USBAV_ACQUIRE_FRAME
    

 \parameters
    InputBuffer:        pointer to LCI_USBAV_FRAME_INFO
    InputBufferSize:    sizeof(LCI_USBAV_FRAME_INFO)
    OutputBuffer:       NULL
    OutputBufferSize:   0

 */
#define IOCTL_LCI_USBAV_RELEASE_FRAME               \
    CTL_CODE(FILE_DEVICE_UNKNOWN,                   \
    LCI_USBAV_IOCTL_BASE + 3,                       \
    METHOD_BUFFERED,                                \
    FILE_ANY_ACCESS)

/*!
 Name:  IOCTL_LCI_WAIT_FOR_UPDATE

 \details
    This IOCTL requests USB AV driver to report the latest frame or cursor
    update status. The user app provides LCI_WAIT_FOR_UPDATE on how to wait.
    
    Upon input, if Flags.Frame is set, the driver would complete the IOCTL
    immediately if the current frame has FenceId larger than the FenceId provided.
    Otherwise, the IOCTL is not completed  until after any frame update event
    occurs. If Flags.Frame is not set, the driver ignores the frame update event,
    and would not block the IOCTL from completion. If  Flags.Cursor is set, the
    kernel driver would complete the IOCTL if any cursor update event occur 
    after the IOCTL trigger. If Flags.Cursor is not set, kernel driver ignores
    the cursor update event, and would not block the IOCTL.
    
    Upon output, the kernel reports the update status.

 \parameters
    InputBuffer:        pointer to LCI_WAIT_FOR_UPDATE
    InputBufferSize:    sizeof (LCI_WAIT_FOR_UPDATE)
    OutputBuffer:       pointer to LCI_WAIT_FOR_UPDATE
    OutputBufferSize:   sizeof(LCI_WAIT_FOR_UPDATE)

 */

#define IOCTL_LCI_WAIT_FOR_UPDATE                   \
    CTL_CODE(FILE_DEVICE_UNKNOWN,                   \
    LCI_USBAV_IOCTL_BASE + 4,                       \
    METHOD_BUFFERED,                                \
    FILE_ANY_ACCESS)
    
typedef struct _LCI_WAIT_FLAGS
    {
    UINT        Frame: 1;
    UINT        Cursor: 1;
    } LCI_WAIT_FLAGS;

typedef struct _LCI_WAIT_FOR_UPDATE
    {
    LCI_WAIT_FLAGS      Flags;
    UINT                FrameId;
    } LCI_WAIT_FOR_UPDATE;
#endif
