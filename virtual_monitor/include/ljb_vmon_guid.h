#ifndef _LJB_VMON_GUID_H_
#define _LJB_VMON_GUID_H_

// {4059C453-0374-4124-82A2-9BB5795F1E26}
DEFINE_GUID(LJB_MONITOR_INTERFACE_GUID,
0x4059c453, 0x374, 0x4124, 0x82, 0xa2, 0x9b, 0xb5, 0x79, 0x5f, 0x1e, 0x26);

//
// Define an Interface Guid for bus enumerator class.
// This GUID is used to register (IoRegisterDeviceInterface)
// an instance of an interface so that enumerator application
// can send an ioctl to the bus driver.
//

DEFINE_GUID (GUID_DEVINTERFACE_VMON_BUS,
0xD35F7840, 0x6A0C, 0x11d2, 0xB8, 0x41, 0x00, 0xC0, 0x4F, 0xAD, 0x51, 0x71);
//  {D35F7840-6A0C-11d2-B841-00C04FAD5171}

//
// Define a Setup Class GUID for Toaster Class. This is same
// as the TOASTSER CLASS guid in the INF files.
//

DEFINE_GUID (GUID_DEVCLASS_VMON_BUS,
0xB85B7C50, 0x6A01, 0x11d2, 0xB8, 0x41, 0x00, 0xC0, 0x4F, 0xAD, 0x51, 0x71);
//{B85B7C50-6A01-11d2-B841-00C04FAD5171}

//
// Define a WMI GUID to get busenum info.
//

DEFINE_GUID (TOASTER_BUS_WMI_STD_DATA_GUID,
0x0006A660, 0x8F12, 0x11d2, 0xB8, 0x54, 0x00, 0xC0, 0x4F, 0xAD, 0x51, 0x71);
//{0006A660-8F12-11d2-B854-00C04FAD5171}

//
// Define a WMI GUID to get toaster device info.
//

DEFINE_GUID (TOASTER_WMI_STD_DATA_GUID,
0xBBA21300L, 0x6DD3, 0x11d2, 0xB8, 0x44, 0x00, 0xC0, 0x4F, 0xAD, 0x51, 0x71);

//
// Define a WMI GUID to represent device arrival notification WMIEvent class.
//

DEFINE_GUID (TOASTER_NOTIFY_DEVICE_ARRIVAL_EVENT,
0x1cdaff1, 0xc901, 0x45b4, 0xb3, 0x59, 0xb5, 0x54, 0x27, 0x25, 0xe2, 0x9c);
// {01CDAFF1-C901-45b4-B359-B5542725E29C}

//
// Define an Interface Guid to access the proprietary toaster interface.
// This guid is used to identify a specific interface in IRP_MN_QUERY_INTERFACE
// handler.
//

DEFINE_GUID(GUID_TOASTER_INTERFACE_STANDARD,
0xe0b27630, 0x5434, 0x11d3, 0xb8, 0x90, 0x0, 0xc0, 0x4f, 0xad, 0x51, 0x71);
// {E0B27630-5434-11d3-B890-00C04FAD5171}


//
// GUID definition are required to be outside of header inclusion pragma to avoid
// error during precompiled headers.
//

#endif