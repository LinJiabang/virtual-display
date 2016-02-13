/*
 * ljb_dxgk_control_interrupt.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_ControlInterrupt)
#endif

/*
 * Function: LJB_DXGK_ControlInterrupt
 *
 * Description:
 * The DxgkDdiControlInterrupt function enables or disables the given interrupt
 * type on the graphics hardware.
 *
 * Return Value:
 * DxgkDdiControlInterrupt returns one of the following values:
 *
 *  STATUS_SUCCESS: The interrupt type was successfully enabled or disabled on
 *  the graphics hardware.
 *
 *  STATUS_NOT_IMPLEMENTED: DxgkDdiControlInterrupt does not support enabling or
 *  disabling the specified interrupt type.
 *
 * Remarks:
 * The display miniport driver's DxgkDdiControlInterrupt function can enable or
 * disable the specified interrupt type. However, DxgkDdiControlInterrupt is not
 * required to disable the interrupt type if the driver requires the interrupt
 * type for an internal purpose. A call to DxgkDdiControlInterrupt to enable the
 * specified interrupt type indicates that the operating system requires that
 * the driver call the DxgkCbNotifyInterrupt function to report when the interrupt
 * type is triggered on the graphics hardware.
 *
 * Currently, the Microsoft DirectX graphics kernel subsystem specifies only the
 * DXGK_INTERRUPT_CRTC_VSYNC interrupt type in the InterruptType parameter. A
 * call to DxgkDdiControlInterrupt to enable the DXGK_INTERRUPT_CRTC_VSYNC
 * interrupt type indicates for the driver to control vertical retrace interrupt.
 * During every vertical retrace period and immediately after the primary surface
 * address specified in the DAC register is latched and scanned out, the interrupt
 * should be triggered and reported.
 *
 * The driver must return STATUS_NOT_IMPLEMENTED if an interrupt type other than
 * DXGK_INTERRUPT_CRTC_VSYNC is supplied.
 *
 * DxgkDdiControlInterrupt should be made pageable.
 */
NTSTATUS
LJB_DXGK_ControlInterrupt(
    _In_ const HANDLE              hAdapter,
    _In_ const DXGK_INTERRUPT_TYPE InterruptType,
    _In_       BOOLEAN             Enable
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiControlInterrupt)(
        hAdapter,
        InterruptType,
        Enable
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}
