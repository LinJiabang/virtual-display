/*
 * ljb_dxgk_set_power_component_fstate.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

/*
 * Function: LJB_DXGK_SetPowerComponentFState
 *
 * Description:
 * Called by the Microsoft DirectX graphics kernel subsystem to transition a power
 * component to an idle state (an F-state).
 *
 * Return Value:
 * Returns STATUS_SUCCESS if it succeeds; otherwise, it returns STATUS_INVALID_PARAMETER.
 *
 * Remarks:
 * The operating system calls DxgkDdiSetPowerComponentFState only if the display
 * miniport driver indicates support by setting DXGK_DRIVERCAPS.SupportRuntimePowerManagement
 * to TRUE.
 * Note  To avoid a possible deadlock, do not call the DxgkCbSetPowerComponentActive
 * function until this function has returned.
 *
 * When the display miniport driver transitions a power component from the F0 (fully on)
 * state to another F-state, it should save the context needed to later restore
 * the component back to the F0 state.
 *
 * The Power Management Framework only transitions a component to or from the F0
 * state.
 *
 * This function can be called simultaneously from multiple execution threads. However,
 * only one thread at a time can call this function to control a particular component.
 *
 * The operating system guarantees that this function follows the zero level synchronization
 * mode as defined in Threading and Synchronization Zero Level.
 */
NTSTATUS
LJB_DXGK_SetPowerComponentFState(
    _In_ const HANDLE   hAdapter,
    _In_ UINT           ComponentIndex,
    _In_ UINT           FState
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    DBG_PRINT(Adapter, DBGLVL_POWER,
        (__FUNCTION__ ": ComponentIndex(%u), FState(0x%x)\n",
        ComponentIndex,
        FState
        ));
    Adapter->FState[ComponentIndex] = FState;

    ntStatus = (*DriverInitData->DxgkDdiSetPowerComponentFState)(
        hAdapter,
        ComponentIndex,
        FState
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}
