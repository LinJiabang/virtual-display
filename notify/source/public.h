/*++
Copyright (c) 1990-2000    Microsoft Corporation All Rights Reserved

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

#ifndef __PUBLIC_H
#define __PUBLIC_H

#define BUS_HARDWARE_IDS L"vmonbus\\{B85B7C50-6A01-11d2-B841-00C04FAD5171}\0"
#define BUS_HARDWARE_IDS_LENGTH sizeof (BUS_HARDWARE_IDS)

#define BUSENUM_COMPATIBLE_IDS BUS_HARDWARE_IDS
#define BUSENUM_COMPATIBLE_IDS_LENGTH sizeof(BUSENUM_COMPATIBLE_IDS)


#define FILE_DEVICE_BUSENUM         FILE_DEVICE_BUS_EXTENDER

#define BUSENUM_IOCTL(_index_) \
    CTL_CODE (FILE_DEVICE_BUSENUM, _index_, METHOD_BUFFERED, FILE_READ_DATA)

#define IOCTL_BUSENUM_PLUGIN_HARDWARE               BUSENUM_IOCTL (0x0)
#define IOCTL_BUSENUM_UNPLUG_HARDWARE               BUSENUM_IOCTL (0x1)
#define IOCTL_BUSENUM_EJECT_HARDWARE                BUSENUM_IOCTL (0x2)
#define IOCTL_TOASTER_DONT_DISPLAY_IN_UI_DEVICE     BUSENUM_IOCTL (0x3)

//
//  Data structure used in PlugIn and UnPlug ioctls
//

typedef struct _BUSENUM_PLUGIN_HARDWARE
{
    //
    // sizeof (struct _BUSENUM_HARDWARE)
    //
    IN ULONG Size;

    //
    // Unique serial number of the device to be enumerated.
    // Enumeration will be failed if another device on the
    // bus has the same serail number.
    //

    IN ULONG SerialNo;

    //
    // An array of (zero terminated wide character strings). The array itself
    //  also null terminated (ie, MULTI_SZ)
    //
    #pragma warning(disable:4200)  // nonstandard extension used

    IN  WCHAR   HardwareIDs[];

    #pragma warning(default:4200)

} BUSENUM_PLUGIN_HARDWARE, *PBUSENUM_PLUGIN_HARDWARE;

typedef struct _BUSENUM_UNPLUG_HARDWARE
{
    //
    // sizeof (struct _REMOVE_HARDWARE)
    //

    IN ULONG Size;

    //
    // Serial number of the device to be plugged out
    //

    ULONG   SerialNo;

    ULONG Reserved[2];

} BUSENUM_UNPLUG_HARDWARE, *PBUSENUM_UNPLUG_HARDWARE;

typedef struct _BUSENUM_EJECT_HARDWARE
{
    //
    // sizeof (struct _EJECT_HARDWARE)
    //

    IN ULONG Size;

    //
    // Serial number of the device to be ejected
    //

    ULONG   SerialNo;

    ULONG Reserved[2];

} BUSENUM_EJECT_HARDWARE, *PBUSENUM_EJECT_HARDWARE;

#endif

