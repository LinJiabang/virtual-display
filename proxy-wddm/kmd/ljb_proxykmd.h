
#ifndef _LJB_PROXYKMD_H_
#define _LJB_PROXYKMD_H_

#include <ntddk.h>
#include <dispmprt.h>

#pragma warning(disable:4201) /* allow nameless struct/union */


#define DXGKRNL_SYS_STR                 L"dxgkrnl.sys"
#define USER_MODE_DRIVER_NAME           L"UserModeDriverName"
#define USER_MODE_DRIVER_NAME_WOW       L"UserModeDriverNameWow"

#define MY_USER_MODE_DRIVER_NAME        L"ljb_umd.dll\0ljb_umd.dll\0ljb_umd.dll\0"
#define MY_USER_MODE_DRIVER_NAME_WOW    L"ljb_umd32.dll\0ljb_umd32.dll\0ljb_umd32.dll\0"

#endif