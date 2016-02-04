/*
 * ljb_dxgk_start_device.c
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
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_StartDevice)
#endif

/*
 * Function: LJB_DXGK_AddDevice
 *
 * Description:
 * The DxgkDdiStartDevice function prepares a display adapter to receive I/O
 * requests.
 *
 * Return value
 * DxgkDdiStartDevice returns STATUS_SUCCESS if it succeeds; otherwise, it
 * returns one of the error codes defined in Ntstatus.h.
 *
 * Remarks
 * The DxgkDdiStartDevice function must perform the following actions:
 *
 * - Save the function pointers supplied by the DXGKRNL_INTERFACE structure passed
 * to the DxgkInterface parameter. Also save the DeviceHandle member of the
 * DXGKRNL_INTERFACE structure; you will need that handle to call back into the
 * DirectX graphics kernel subsystem.
 *
 * - Allocate a DXGK_DEVICE_INFO structure, and call DxgkCbGetDeviceInformation
 * to fill in the members of that structure, which include the registry path,
 * the PDO, and a list of translated resources for the display adapter represented
 * by MiniportDeviceContext. Save selected members (ones that the display miniport
 * driver will need later) of the DXGK_DEVICE_INFO structure in the context block
 * represented by MiniportDeviceContext.
 *
 * - Map memory resources into system space by calling the DxgkCbMapMemory function.
 *
 * - Initialize the context block represented by MiniportDeviceContext with any
 * state that is required to prepare the hardware to receive I/O requests.
 *
 * - Set NumberOfVideoPresentSources to the number of video present sources
 * supported by the display adapter that is represented by MiniportDeviceContext.
 *
 * - Set NumberOfChildren to the number of devices that are (or could become)
 * children of the display adapter represented by MiniportDeviceContext.
 *
 * - Enable interrupts for the display adapter represented by MiniportDeviceContext.
 *
 * Starting with Windows Display Driver Model (WDDM) 1.2, the display miniport
 * driver calls the DxgkCbAcquirePostDisplayOwnership function to obtain the
 * information about the display mode that had been previously set by the firmware
 * and system loader. If DxgkCbAcquirePostDisplayOwnership returns with
 * STATUS_SUCCESS, the driver determines whether it has to reinitialize the display
 * based on the display mode information that was returned through the DisplayInfo
 * parameter. Otherwise, the driver should not assume that any specific display
 * mode is currently enabled on the device, and it should initialize the display.
 *
 * The DxgkDdiStartDevice function should be made pageable.
 */
NTSTATUS
LJB_DXGK_StartDevice(
    _In_  const PVOID               MiniportDeviceContext,
    _In_        PDXGK_START_INFO    DxgkStartInfo,
    _In_        PDXGKRNL_INTERFACE  DxgkInterface,
    _Out_       PULONG              NumberOfVideoPresentSources,
    _Out_       PULONG              NumberOfChildren
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(MiniportDeviceContext);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;
    ULONG                               BytesRead;

    PAGED_CODE();

    /*
     * pass the call to inbox driver
     */
    ntStatus = (*DriverInitData->DxgkDdiStartDevice)(
        MiniportDeviceContext,
        DxgkStartInfo,
        DxgkInterface,
        NumberOfVideoPresentSources,
        NumberOfChildren
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        return ntStatus;
    }

    /*
     * save DxgkStartInfo, DxgkInterface, NumberOfVideoPresentSources, NumberOfChildren
     */
    Adapter->DxgkStartInfo = *DxgkStartInfo;
    Adapter->DxgkInterface = *DxgkInterface;
    Adapter->NumberOfVideoPresentSources = *NumberOfVideoPresentSources;
    Adapter->NumberOfChildren = *NumberOfChildren;

    (*DxgkInterface->DxgkCbReadDeviceSpace)(
        DxgkInterface->DeviceHandle,
        DXGK_WHICHSPACE_CONFIG,
        &Adapter->PciVendorId,
        0,
        sizeof(Adapter->PciVendorId),
        &BytesRead
        );
    *NumberOfVideoPresentSources += USB_MONITOR_MAX;
    *NumberOfChildren += USB_MONITOR_MAX;

    DBG_PRINT(Adapter, DBGLVL_INFO,
        (__FUNCTION__
        ": VendorId(0x%04X), Version(0x%8x), NumberOfVideoPresentSources(%u), NumberOfChildren(%u)\n",
        Adapter->PciVendorId,
        DxgkInterface->Version,
        Adapter->NumberOfVideoPresentSources,
        Adapter->NumberOfChildren
        ));
    return ntStatus;
}