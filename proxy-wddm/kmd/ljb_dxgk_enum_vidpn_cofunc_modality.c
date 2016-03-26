/*
 * ljb_dxgk_enum_vidpn_cofunc_modality.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"
#include "ljb_dxgk_vidpn_interface.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_EnumVidPnCofuncModality)
#endif

static
NTSTATUS
LJB_DXGK_EnumVidPnCofuncModalityPostProcessing(
    __in LJB_ADAPTER *                              Adapter,
    __in LJB_VIDPN *                                MyVidPn,
    IN_CONST_PDXGKARG_ENUMVIDPNCOFUNCMODALITY_CONST pEnumCofuncModality
    );

/*
 * Function: LJB_DXGK_EnumVidPnCofuncModality
 *
 * Description:
 * The DxgkDdiEnumVidPnCofuncModality function makes the source and target modes
 * sets of a VidPN cofunctional with the VidPN's topology and the modes that have
 * already been pinned.
 *
 * Return Value:
 * DxgkDdiEnumVidPnCofuncModality returns STATUS_SUCCESS if it succeeds;
 * otherwise, it returns one of the error codes defined in Ntstatus.h.
 *
 * Remarks:
 * The hConstrainingVidPn member of pEnumCofuncModalityArg is a handle to a VidPN
 * object called the constraining VidPN. Other members of pEnumCofuncModalityArg
 * identify one video present source or target as the pivot of the enumeration
 * (or specify that there is no pivot).
 *
 * DxgkDdiEnumVidPnCofuncModality must perform the following tasks:
 * Examine the topology and mode sets of the constraining VidPN.
 * Update each mode set that is not the pivot and does not already have a pinned
 * mode. The updated mode sets must be cofunctional with the VidPN's topology and
 * with any modes that have already been pinned.
 *
 * Note that if a source or target is identified as the pivot of the enumeration,
 * the mode set for that source or target must not change. For more information
 * about how to update source and target mode sets, see Enumerating Cofunctional
 * VidPN Source and Target Modes.
 *
 * The DxgkDdiEnumVidPnCofuncModality function should be made pageable.
 */
NTSTATUS
LJB_DXGK_EnumVidPnCofuncModality(
    IN_CONST_HANDLE                                     hAdapter,
    IN_CONST_PDXGKARG_ENUMVIDPNCOFUNCMODALITY_CONST     pEnumCofuncModality
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;
    LJB_VIDPN *                         MyVidPn;
    DXGKARG_ENUMVIDPNCOFUNCMODALITY     MyEnumVidPnCoFuncModality;
    UINT                                NumOfInboxTarget;
    UINT                                NumOfUsbTarget;

    PAGED_CODE();

    MyVidPn = LJB_VIDPN_CreateVidPn(Adapter, pEnumCofuncModality->hConstrainingVidPn);

    if (MyVidPn == NULL)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?"__FUNCTION__": no MyVidPn allocated.\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ntStatus = STATUS_SUCCESS;
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
        MyEnumVidPnCoFuncModality = *pEnumCofuncModality;
        MyEnumVidPnCoFuncModality.hConstrainingVidPn = (D3DKMDT_HVIDPN) MyVidPn;
        ntStatus = (*DriverInitData->DxgkDdiEnumVidPnCofuncModality)(
            hAdapter,
            &MyEnumVidPnCoFuncModality
            );
        if (!NT_SUCCESS(ntStatus))
        {
            DBG_PRINT(Adapter, DBGLVL_ERROR,
                ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
            goto Exit;
        }
    }

    if (NumOfUsbTarget != 0)
    {
        ntStatus = LJB_DXGK_EnumVidPnCofuncModalityPostProcessing(
            Adapter, MyVidPn, pEnumCofuncModality);
        if (!NT_SUCCESS(ntStatus))
        {
            DBG_PRINT(Adapter, DBGLVL_ERROR,
                ("?" __FUNCTION__": LJB_DXGK_EnumVidPnCofuncModalityPostProcessing failed?\n"));
        }
    }

Exit:
    LJB_VIDPN_DestroyVidPn(MyVidPn);

    return ntStatus;
}

static
NTSTATUS
LJB_DXGK_EnumVidPnCofuncModalityPostProcessing(
    __in LJB_ADAPTER *                              Adapter,
    __in LJB_VIDPN *                                MyVidPn,
    IN_CONST_PDXGKARG_ENUMVIDPNCOFUNCMODALITY_CONST pEnumCofuncModality
    )
{
    NTSTATUS        ntStatus;

    // NOT YET Implemented
    UNREFERENCED_PARAMETER(Adapter);
    UNREFERENCED_PARAMETER(MyVidPn);
    UNREFERENCED_PARAMETER(pEnumCofuncModality);
    ntStatus = STATUS_SUCCESS;

    return ntStatus;
}