/*
 * ljb_dxgk_query_adapter_info.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_QueryAdapterInfo)
#endif

/*
 * Function: LJB_DXGK_QueryAdapterInfo
 *
 * Description:
 * The DxgkDdiQueryAdapterInfo function retrieves configuration information from
 * the graphics adapter.
 *
 * Return value
 * DxgkDdiQueryAdapterInfo returns one of the following values:
 *
 * STATUS_SUCCESS
 *  DxgkDdiQueryAdapterInfo successfully retrieved the configuration information.
 *
 * STATUS_INVALID_PARAMETER
 *  Parameters that were passed to DxgkDdiQueryAdapterInfo contained errors that
 *  prevented it from completing.
 *
 * STATUS_NO_MEMORY
 *  DxgkDdiQueryAdapterInfo could not allocate memory that was required for it
 *  to complete.
 *
 * STATUS_GRAPHICS_DRIVER_MISMATCH
 *  The display miniport driver is not compatible with the user-mode display
 *  driver that initiated the call to DxgkDdiQueryAdapterInfo (that is, supplied
 * private data for a query to the display miniport driver).
 *
 * Remarks
 * When the user-mode display driver calls the pfnQueryAdapterInfoCb function, a
 * call to the DxgkDdiQueryAdapterInfo function is initiated. DxgkDdiQueryAdapterInfo
 * receives the DXGKQAITYPE_UMDRIVERPRIVATE value in the Type member of the
 * DXGKARG_QUERYADAPTERINFO structure that the pQueryAdapterInfo parameter points
 * to. This function also receives a proprietary buffer in the pOutputData member
 * that it fills with the configuration information that is necessary for the
 * user-mode display driver to identify the adapter.
 *
 * If the DirectX graphics kernel subsystem (which is part of Dxgkrnl.sys) specifies
 * the DXGKQAITYPE_DRIVERCAPS value in the Type member of DXGKARG_QUERYADAPTERINFO
 * when the subsystem calls DxgkDdiQueryAdapterInfo, the display miniport driver
 * should populate the provided DXGK_DRIVERCAPS structure with information that
 * the subsystem can use.
 *
 * If the DirectX graphics kernel subsystem supplies the DXGKQAITYPE_QUERYSEGMENT
 * value in the Type member of DXGKARG_QUERYADAPTERINFO, the display miniport
 * driver should provide information about the memory segments that it supports.
 * For more information about memory segments, see Initializing Use of Memory
 * Segments.
 *
 * DxgkDdiQueryAdapterInfo should be made pageable.
 */
NTSTATUS
LJB_DXGK_QueryAdapterInfo(
    _In_ const HANDLE                   hAdapter,
    _In_ const DXGKARG_QUERYADAPTERINFO *pQueryAdapterInfo
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    DXGK_DRIVERCAPS * CONST             DriverCaps = pQueryAdapterInfo->pOutputData;
    DXGK_QUERYSEGMENTOUT3 *             SegmentOut3;
    DXGK_QUERYSEGMENTOUT4 *             SegmentOut4;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    /*
     * pass the call to inbox driver
     */
    ntStatus = (*DriverInitData->DxgkDdiQueryAdapterInfo)(hAdapter, pQueryAdapterInfo);
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        return ntStatus;
    }

    /*
     * post processing of DxgkDdiQueryAdapterInfo
     */
    switch (pQueryAdapterInfo->Type)
    {
    case DXGKQAITYPE_DRIVERCAPS:
        RtlCopyMemory(
            &Adapter->DriverCaps,
            pQueryAdapterInfo->pOutputData,
            pQueryAdapterInfo->OutputDataSize
            );
        // we don't support screen to screen blt YET.
        DriverCaps->PresentationCaps.NoScreenToScreenBlt = 1;

        // No SupportDirectFlip, SupportMultiPlaneOverlay yet.
        if (DriverInitData->Version >= DXGKDDI_INTERFACE_VERSION_WIN8)
        {
            DriverCaps->SupportDirectFlip = FALSE;
            DriverCaps->SupportMultiPlaneOverlay = FALSE;
            DriverCaps->SupportRuntimePowerManagement = FALSE;
        }
        break;

    case DXGKQAITYPE_QUERYSEGMENT:
        RtlCopyMemory(
            &Adapter->SegmentOut,
            pQueryAdapterInfo->pOutputData,
            pQueryAdapterInfo->OutputDataSize
            );
        if (Adapter->SegmentOut.NbSegment != 0 && Adapter->SegmentOut.pSegmentDescriptor != NULL)
        {
            UINT    i;

            for (i = 0; i < Adapter->SegmentOut.NbSegment; i++)
            {
                DXGK_SEGMENTDESCRIPTOR * CONST pSegmentDescriptor = Adapter->SegmentOut.pSegmentDescriptor + i;

                Adapter->SegmentDescriptors[i] = *pSegmentDescriptor;
                DBG_PRINT(Adapter, DBGLVL_INFO,
                    (__FUNCTION__":DXGKQAITYPE_QUERYSEGMENT: Segment[%u] =\n"
                    "\tBaseAddress(0x%x:0x%x)\n"
                    "\tCpuTranslatedAddress(0x%x:0x%x)\n"
                    "\tSize(0x%x)\n"
                    "\tNbOfBanks(0x%x)\n"
                    "\tCommitLimit(0x%x)\n"
                    "\tFlags(0x%x)\n",
                    i,
                    pSegmentDescriptor->BaseAddress.HighPart,
                    pSegmentDescriptor->BaseAddress.LowPart,
                    pSegmentDescriptor->CpuTranslatedAddress.HighPart,
                    pSegmentDescriptor->CpuTranslatedAddress.LowPart,
                    pSegmentDescriptor->Size,
                    pSegmentDescriptor->NbOfBanks,
                    pSegmentDescriptor->CommitLimit,
                    pSegmentDescriptor->Flags.Value
                    ));
            }
        }
        break;

    case DXGKQAITYPE_QUERYSEGMENT3:
        SegmentOut3 = pQueryAdapterInfo->pOutputData;
        Adapter->SegmentOut.NbSegment = SegmentOut3->NbSegment;
        Adapter->SegmentOut.PagingBufferSegmentId = SegmentOut3->PagingBufferSegmentId;
        Adapter->SegmentOut.PagingBufferSize = SegmentOut3->PagingBufferSize;
        Adapter->SegmentOut.PagingBufferPrivateDataSize = SegmentOut3->PagingBufferPrivateDataSize;
        if (SegmentOut3->NbSegment != 0 && SegmentOut3->pSegmentDescriptor != NULL)
        {
            UINT    i;

            for (i = 0; i < SegmentOut3->NbSegment; i++)
            {
                DXGK_SEGMENTDESCRIPTOR3 * CONST pSegmentDescriptor = SegmentOut3->pSegmentDescriptor + i;

                Adapter->SegmentDescriptors[i].Flags = pSegmentDescriptor->Flags;
                Adapter->SegmentDescriptors[i].BaseAddress = pSegmentDescriptor->BaseAddress;
                Adapter->SegmentDescriptors[i].CpuTranslatedAddress = pSegmentDescriptor->CpuTranslatedAddress;
                Adapter->SegmentDescriptors[i].Size = pSegmentDescriptor->Size;
                Adapter->SegmentDescriptors[i].NbOfBanks = pSegmentDescriptor->NbOfBanks;
                Adapter->SegmentDescriptors[i].CommitLimit = pSegmentDescriptor->CommitLimit;

                DBG_PRINT(Adapter, DBGLVL_INFO,
                    (__FUNCTION__":DXGKQAITYPE_QUERYSEGMENT3: Segment[%u] =\n"
                    "\tBaseAddress(0x%x:0x%x)\n"
                    "\tCpuTranslatedAddress(0x%x:0x%x)\n"
                    "\tSize(0x%x)\n"
                    "\tNbOfBanks(0x%x)\n"
                    "\tCommitLimit(0x%x)\n"
                    "\tFlags(0x%x)\n"
                    "\tSystemMemoryEndAddress(0x%x)\n",
                    i,
                    pSegmentDescriptor->BaseAddress.HighPart,
                    pSegmentDescriptor->BaseAddress.LowPart,
                    pSegmentDescriptor->CpuTranslatedAddress.HighPart,
                    pSegmentDescriptor->CpuTranslatedAddress.LowPart,
                    pSegmentDescriptor->Size,
                    pSegmentDescriptor->NbOfBanks,
                    pSegmentDescriptor->CommitLimit,
                    pSegmentDescriptor->Flags.Value,
                    pSegmentDescriptor->SystemMemoryEndAddress
                    ));
            }
        }
        break;

    case DXGKQAITYPE_QUERYSEGMENT4:
        SegmentOut4 = pQueryAdapterInfo->pOutputData;
        Adapter->SegmentOut.NbSegment = SegmentOut4->NbSegment;
        Adapter->SegmentOut.PagingBufferSegmentId = SegmentOut4->PagingBufferSegmentId;
        Adapter->SegmentOut.PagingBufferSize = SegmentOut4->PagingBufferSize;
        Adapter->SegmentOut.PagingBufferPrivateDataSize = SegmentOut4->PagingBufferPrivateDataSize;
        if (SegmentOut4->NbSegment != 0 && SegmentOut4->pSegmentDescriptor != NULL)
        {
            UINT    i;

            for (i = 0; i < SegmentOut4->NbSegment; i++)
            {
                DXGK_SEGMENTDESCRIPTOR4 *pSegmentDescriptor;

                pSegmentDescriptor = (DXGK_SEGMENTDESCRIPTOR4 *)(SegmentOut4->pSegmentDescriptor +
                i * SegmentOut4->SegmentDescriptorStride);

                Adapter->SegmentDescriptors[i].Flags = pSegmentDescriptor->Flags;
                Adapter->SegmentDescriptors[i].BaseAddress = pSegmentDescriptor->BaseAddress;
                Adapter->SegmentDescriptors[i].CpuTranslatedAddress = pSegmentDescriptor->CpuTranslatedAddress;
                Adapter->SegmentDescriptors[i].Size = pSegmentDescriptor->Size;
                Adapter->SegmentDescriptors[i].CommitLimit = pSegmentDescriptor->CommitLimit;

                DBG_PRINT(Adapter, DBGLVL_INFO,
                    (__FUNCTION__": DXGKQAITYPE_QUERYSEGMENT4:Segment[%u] =\n"
                    "\tBaseAddress(0x%x:0x%x)\n"
                    "\tCpuTranslatedAddress(0x%x:0x%x)\n"
                    "\tSize(0x%x)\n"
                    "\tCommitLimit(0x%x)\n"
                    "\tFlags(0x%x)\n"
                    "\tSystemMemoryEndAddress(0x%x)\n",
                    i,
                    pSegmentDescriptor->BaseAddress.HighPart,
                    pSegmentDescriptor->BaseAddress.LowPart,
                    pSegmentDescriptor->CpuTranslatedAddress.HighPart,
                    pSegmentDescriptor->CpuTranslatedAddress.LowPart,
                    pSegmentDescriptor->Size,
                    pSegmentDescriptor->CommitLimit,
                    pSegmentDescriptor->Flags.Value,
                    pSegmentDescriptor->SystemMemoryEndAddress
                    ));
            }
        }
        break;

    default:
        break;
    }

    return ntStatus;
}
