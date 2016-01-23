/*
 * ljb_proxykmd.h
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

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

typedef enum _DEVICE_TYPE
    {
    DEVICE_TYPE_INVALID,
    DEVICE_TYPE_FDO,
    DEVICE_TYPE_FILTER
    } DEVICE_TYPE;

typedef enum _PNP_STATE
    {
    PNP_NOT_STARTED,
    PNP_STARTED,
    PNP_STOP_PENDING,
    PNP_STOPPED,
    PNP_REMOVE_PENDING,
    PNP_SURPRISE_REMOVE_PENDING,
    PNP_DELETED
    } PNP_STATE;

#define INITIALIZE_PNP_STATE(_Data_)    \
        (_Data_)->DevicePnPState =  PNP_NOT_STARTED;\
        (_Data_)->PreviousPnPState = PNP_NOT_STARTED;

#define SET_NEW_PNP_STATE(_Data_, _state_) \
        (_Data_)->PreviousPnPState =  (_Data_)->DevicePnPState;\
        (_Data_)->DevicePnPState = (_state_);

#define RESTORE_PREVIOUS_PNP_STATE(_Data_)   \
        (_Data_)->DevicePnPState =   (_Data_)->PreviousPnPState;\


typedef struct _LJB_DEVICE_EXTENSION
{
    DEVICE_OBJECT *                     DeviceObject;
    DEVICE_OBJECT *                     NextLowerDriver;
    DEVICE_OBJECT *                     PhysicalDeviceObject;
    DEVICE_OBJECT *                     FilterDeviceObject;
    RTL_OSVERSIONINFOW                  RtlOsVersion;

    DEVICE_TYPE                         DeviceType;
    PNP_STATE                           DevicePnPState;
    PNP_STATE                           PreviousPnpState;
    IO_REMOVE_LOCK                      RemoveLock;

    PFN_DXGK_INITIALIZE                 pfnDxgkInialize;
    PFILE_OBJECT                        DxgkFileObject;

    ULONG                               DebugLevel;
    } LJB_DEVICE_EXTENSION;


#endif