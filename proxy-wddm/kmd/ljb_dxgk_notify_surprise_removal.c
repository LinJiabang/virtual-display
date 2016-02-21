/*
 * ljb_dxgk_notify_surprise_removal.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

CONST CHAR * CONST RemovalTypeString[] =
{
    "DxgkRemovalHibernation",
};

/*
 * Function: LJB_DXGK_NotifySurpriseRemoval
 *
 * Description:
 * Called by the operating system after a user disconnected an external display
 * device without notifying the system.
 * Can optionally be implemented by Windows Display Driver Model (WDDM) 1.2 and
 * later display miniport drivers.
 *
 * Return Value:
 * Returns STATUS_SUCCESS if software resources were cleaned up for RemovalType
 * = DxgkRemovalHibernation. If the driver instead returns an error code, the
 * operating system will attempt to reboot the system, as described in the
 * following Remarks section.
 *
 * Remarks:
 * The operating system calls DxgkDdiNotifySurpriseRemoval only if the display
 * miniport driver indicates support by setting the SupportSurpriseRemovalInHibernation
 * member of the DXGK_DRIVERCAPS structure to 1.
 *
 * Note  This function is called only if the disconnected external display device
 * was in hibernation mode. This is indicated by RemovalType = DxgkRemovalHibernation,
 * the only available value.
 *
 * If the display miniport driver returns STATUS_SUCCESS, the DirectX graphics kernel
 * subsystem will continue to remove the external display adapter from the graphics
 * stack and will call other driver-implemented DxgkDdiXxx kernel-mode functions
 * to release all resources. In this case, the driver must complete its cleanup
 * of software resources in response to calls from the operating system but must
 * not touch or clean any hardware settings. If no other hardware is using the
 * driver, the operating system will unload the driver.
 *
 * If the driver returns an error code, does not set DXGK_DRIVERCAPS.SupportSurpriseRemovalInHibernation,
 * or does not implement this function, the DirectX graphics kernel subsystem will
 * not call any more driver-implemented DxgkDdiXxx functions and will attempt to
 * reboot the system. In this case, the resource that was allocated before the
 * external display device was disconnected will not be released.
 */
NTSTATUS
LJB_DXGK_NotifySurpriseRemoval(
    _In_ const HANDLE               hAdapter,
    _In_ DXGK_SURPRISE_REMOVAL_TYPE RemovalType
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    DBG_PRINT(Adapter, DBGLVL_PNP,
        (__FUNCTION__ ": RemovalType(0x%x:%s)\n",
        RemovalType,
        RemovalTypeString[RemovalType]
        ));

    ntStatus = (*DriverInitData->DxgkDdiNotifySurpriseRemoval)(
        hAdapter,
        RemovalType
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}
