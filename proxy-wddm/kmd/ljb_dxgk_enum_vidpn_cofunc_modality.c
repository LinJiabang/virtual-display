/*
 * ljb_dxgk_enum_vidpn_cofunc_modality.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_EnumVidPnCofuncModality)
#endif

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

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiEnumVidPnCofuncModality)(
        hAdapter,
        pEnumCofuncModality
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}