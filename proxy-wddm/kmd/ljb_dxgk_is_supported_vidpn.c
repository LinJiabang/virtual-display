/*
 * ljb_dxgk_is_supported_vidpn.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"
#include "ljb_dxgk_vidpn_interface.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_IsSupportedVidPn)
#endif

static
BOOLEAN
LJB_DXGK_IsMyVidPnSupported(
    __in LJB_VIDPN *    MyVidPn
    );

/*
 * Function: LJB_DXGK_IsSupportedVidPn
 *
 * Description:
 * The DxgkDdiIsSupportedVidPn function determines whether a specified VidPN is
 * supported on a display adapter.
 *
 * Return Value:
 *  DxgkDdiIsSupportedVidPn returns one of the following values:
 *
 *  STATUS_SUCCESS: The topology of the VidPN is valid. The IsVidPnSupported
 *  structure member is set to either TRUE or FALSE.
 *
 *  STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY: The topology of the VidPN implementation
 *  is invalid. The IsVidPnSupported structure member is set to FALSE.
 *
 *  STATUS_NO_MEMORY: The function failed because it was unable to allocate memory.
 *
 * Remarks:
 * For more information about the analysis that this function must perform, see
 * Determining Whether a VidPN is Supported on a Display Adapter.
 *
 * If pIsSupportedVidPnArg->hDesiredVidPn is zero, DxgkDdiIsSupportedVidPn must
 * set pIsSupportedVidPnArg->IsVidPnSupported to TRUE, the idea being that the
 * display adapter can always be configured to display nothing.
 *
 * DxgkDdiIsSupportedVidPn should be made pageable.
 */
NTSTATUS
LJB_DXGK_IsSupportedVidPn(
    _In_    const HANDLE                   hAdapter,
    _Inout_       DXGKARG_ISSUPPORTEDVIDPN *pIsSupportedVidPnArg
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    LJB_VIDPN *                         MyVidPn;
    NTSTATUS                            ntStatus;
    UINT                                NumOfInboxTarget;
    UINT                                NumOfUsbTarget;
    BOOLEAN                             IsVidPnSupported;

    PAGED_CODE();

    /*
     * if NULL vidpn, let inbox driver handle it
     */
    if (pIsSupportedVidPnArg->hDesiredVidPn == NULL)
    {
        return (*DriverInitData->DxgkDdiIsSupportedVidPn)(
            hAdapter,
            pIsSupportedVidPnArg
            );
    }

    MyVidPn = LJB_VIDPN_CreateVidPn(Adapter, pIsSupportedVidPnArg->hDesiredVidPn);
    if (MyVidPn == NULL)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?"__FUNCTION__": no MyVidPn allocated.\n"));
        return STATUS_NO_MEMORY;
    }

    ntStatus = STATUS_SUCCESS;
    IsVidPnSupported = TRUE;

    /*
     * decide if we need to pass the call to inbox driver.
     * if there are inbox target attached, we need to pass the call to inbox driver.
     * If there are usb target attached, we need to process the call.
     * If this is NULL topology, let inbox driver handle it.
     */
    NumOfInboxTarget = LJB_VIDPN_GetNumberOfInboxTarget(MyVidPn);
    NumOfUsbTarget = LJB_VIDPN_GetNumberOfUsbTarget(MyVidPn);
    if (NumOfInboxTarget != 0 || NumOfUsbTarget == 0)
    {
        pIsSupportedVidPnArg->hDesiredVidPn = (D3DKMDT_HVIDPN) MyVidPn;

        ntStatus = (*DriverInitData->DxgkDdiIsSupportedVidPn)(
            hAdapter,
            pIsSupportedVidPnArg
            );
        if (!NT_SUCCESS(ntStatus))
        {
            DBG_PRINT(Adapter, DBGLVL_ERROR,
                ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
            goto Exit;
        }

        IsVidPnSupported = pIsSupportedVidPnArg->IsVidPnSupported;
        DBG_PRINT(Adapter, DBGLVL_VIDPN,
            (__FUNCTION__": inbox driver set IsVidPnSupported(%u)\n",
            IsVidPnSupported));
    }

    /*
     * If inbox driver says no, we don't need to handle it.
     * If inbox driver says yes, we need to verify if we support the vidpn on
     * our paths
     */
    if (NumOfUsbTarget != 0 && IsVidPnSupported)
    {
        IsVidPnSupported = LJB_DXGK_IsMyVidPnSupported(MyVidPn);
    }
    pIsSupportedVidPnArg->IsVidPnSupported = IsVidPnSupported;

Exit:
    LJB_VIDPN_DestroyVidPn(MyVidPn);

    return ntStatus;
}

static
BOOLEAN
LJB_DXGK_IsMyVidPnSupported(
    __in LJB_VIDPN *    MyVidPn
    )
{
    LJB_ADAPTER * CONST             Adapter = MyVidPn->Adapter;
    D3DKMDT_VIDPN_PRESENT_PATH *    MyPath;
    BOOLEAN                         IsSupported;
    UINT                            i;

    DBG_UNREFERENCED_LOCAL_VARIABLE(Adapter);
    IsSupported = TRUE;

    for (i = 0; i < MyVidPn->NumPaths; i++)
    {
        MyPath = MyVidPn->Paths;
        if (MyPath->VidPnTargetId < Adapter->UsbTargetIdBase)
            continue;

        /*
         * Check ContentTransformation.Scaling
         */
        if ((MyPath->ContentTransformation.Scaling != D3DKMDT_VPPS_UNPINNED) &&
            (MyPath->ContentTransformation.Scaling != D3DKMDT_VPPS_IDENTITY) )
        {
            DBG_PRINT(Adapter, DBGLVL_VIDPN,
                (__FUNCTION__
                ": Unsupported Scaling(%u).\n",
                MyPath->ContentTransformation.Scaling
                ));
            return FALSE;
        }

        /*
         * Check ContentTransformation.Scaling
         */
        if (MyPath->ContentTransformation.ScalingSupport.Centered ||
            MyPath->ContentTransformation.ScalingSupport.Stretched ||
            MyPath->ContentTransformation.ScalingSupport.AspectRatioCenteredMax ||
            MyPath->ContentTransformation.ScalingSupport.Custom)
        {
            DBG_PRINT(Adapter, DBGLVL_VIDPN,
                (__FUNCTION__
                ": Unsupported ScalingSupport(%u).\n",
                MyPath->ContentTransformation.ScalingSupport
                ));
            return FALSE;
        }

        /*
         * Check CopyProtection.CopyProtectionType
         */
        if (MyPath->CopyProtection.CopyProtectionType != D3DKMDT_VPPMT_NOPROTECTION)
        {
            DBG_PRINT(Adapter, DBGLVL_VIDPN,
                (__FUNCTION__
                ": Unsupported CopyProtectionType(%u).\n",
                MyPath->CopyProtection.CopyProtectionType
                ));
            return FALSE;
        }

        /*
         * check CopyProtection.CopyProtectionSupport.MacroVisionApsTrigger
         */
        if (MyPath->CopyProtection.CopyProtectionSupport.MacroVisionApsTrigger ||
            MyPath->CopyProtection.CopyProtectionSupport.MacroVisionFull)
        {
            DBG_PRINT(Adapter, DBGLVL_VIDPN,
                (__FUNCTION__
                ": Unsupported MacroVisionApsTrigger(%u)/MacroVisionFull(%u).\n",
                MyPath->CopyProtection.CopyProtectionSupport.MacroVisionApsTrigger,
                MyPath->CopyProtection.CopyProtectionSupport.MacroVisionFull
                ));
            return FALSE;
        }
    }

    return IsSupported;
}