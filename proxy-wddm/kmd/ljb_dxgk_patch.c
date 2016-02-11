/*
 * ljb_dxgk_patch.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_Patch)
#endif

/*
 * Function: LJB_DXGK_Patch
 *
 * Description:
 * The DxgkDdiPatch function assigns physical addresses to the given direct
 * memory access (DMA) buffer before the DMA buffer is submitted to the graphics
 * hardware.
 *
 * Return Value:
 * Returns STATUS_SUCCESS upon successful completion. If the driver instead
 * returns an error code, the operating system causes a system bugcheck to occur.
 * For more information, see the following Remarks section.
 *
 * Remarks:
 * The DxgkDdiPatch function must assign physical addresses to the DMA buffer in
 * place. Therefore, when the display miniport driver generates the DMA buffer,
 * the driver must ensure that space is available in the DMA buffer to insert
 * instructions that are required to handle physical addresses. Note that physical
 * addresses might correspond to video memory, AGP/PCI Express memory, or system
 * memory.
 *
 * The driver must examine the supplied patch-location list in the pPatchLocationList
 * member of the DXGKARG_PATCH structure that is pointed to by the pPatch parameter
 * to identify places in the DMA buffer that must be patched with physical addresses.
 * The supplied allocation list (which is specified by the pAllocationList member
 * of DXGKARG_PATCH) also contains the physical addresses that the video memory
 * manager generates. The call to the driver's DxgkDdiPatch function is the last
 * chance for the driver to modify the content of the DMA buffer before the DMA
 * buffer is submitted to the graphics processing unit (GPU). Note that the driver
 * can patch a DMA buffer multiple times in scenarios where the DMA buffer is
 * preempted.
 *
 * The driver can patch the value that is supplied in the SubmissionFenceId
 * member of DXGKARG_PATCH into the fence command at the end of the DMA buffer.
 * For more information about this member, see Supplying Fence Identifiers.
 *
 * If the driver returns an error code, the Microsoft DirectX graphics kernel
 * subsystem causes a system bugcheck to occur. In a crash dump file, the error
 * is noted by the message BugCheck 0x119, which has the following four
 * parameters.
 *
 * 1. 0x3
 * 2. A pointer to an internal scheduler data structure
 * 3. A pointer to an internal scheduler data structure
 * 4. A pointer to an internal scheduler data structure
 *
 * DxgkDdiPatch should be made pageable.
 */
NTSTATUS
LJB_DXGK_Patch(
    _In_ const HANDLE        hAdapter,
    _In_ const DXGKARG_PATCH *pPatch
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiPatch)(
        hAdapter,
        pPatch
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}