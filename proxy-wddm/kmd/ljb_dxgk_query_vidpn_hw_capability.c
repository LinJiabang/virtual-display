/*
 * ljb_dxgk_query_vidpn_hw_capability.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_QueryVidPnHWCapability)
#endif

/*
 * Function: LJB_DXGK_QueryVidPnHWCapability
 *
 * Description:
 * The DxgkDdiQueryVidPnHWCapability function requests that the display miniport
 * driver report the capabilities of the hardware on a functional VidPn path.
 *
 * Return Value:
 * DxgkDdiQueryVidPnHwCapability returns STATUS_SUCCESS if it succeeds; otherwise,
 * it returns one of the error codes defined in Ntstatus.h.
 *
 * Remarks:
 * For more information on how to process this function, see Querying VidPN
 * Hardware Capabilities.
 *
 * DxgkDdiQueryVidPnHWCapability should be made pageable.
 */
NTSTATUS
LJB_DXGK_QueryVidPnHWCapability(
    _In_    const HANDLE                         hAdapter,
    _Inout_       DXGKARG_QUERYVIDPNHWCAPABILITY *pVidPnHWCaps
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    /*
     * check if pVidPnHWCaps->TargetId point to USB monitor
     */
    if (pVidPnHWCaps->TargetId >= Adapter->UsbTargetIdBase)
    {
        DBG_PRINT(Adapter, DBGLVL_FLOW,
            (__FUNCTION__ ": No DriverCapabilties for TargetId(0x%x)\n",
            pVidPnHWCaps->TargetId));
        RtlZeroMemory(&pVidPnHWCaps->VidPnHWCaps, sizeof(pVidPnHWCaps->VidPnHWCaps));
        return STATUS_SUCCESS;
    }
    ntStatus = (*DriverInitData->DxgkDdiQueryVidPnHWCapability)(
        hAdapter,
        pVidPnHWCaps
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}
