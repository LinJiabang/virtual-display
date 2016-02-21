/*
 * ljb_dxgk_submit_command_virtual.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

/*
 * Function: LJB_DXGK_SubmitCommandVirtual
 *
 * Description:
 * DxgkDdiSubmitCommandVirtual is used to submit a direct memory access (DMA) buffer
 * to a context that supports virtual addressing.
 *
 * The driver is responsible for making sure the correct address space is restored
 * ahead of submitting a particular DMA buffer.
 *
 * Return Value:
 * Returns one of the following values:
 *   STATUS_SUCCESS: The submitted command is well-formed.
 *   STATUS_INVALID_PARAMETER:The DMA or private data is determined to be malformed.
 *   In this case, the OS will put the calling device in an error state and all
 *   subsequent calls on it will fail. The SubmissionFenceId value passed to this
 *   call will be considered completed after all previous packets on the hardware
 *   finished and at that point the driver notion of the last completed fence ID
 *   should be updated to this value.
 *   Note  This behavior is different from DxgkDdiSubmitCommand call where no error
 *   is allowed to be returned due to the ability to validate the data in a prior
 *   DxgkDdiRender call.
 *
 * All other return values will lead to the OS bugcheck.
 *
 * Remarks:
 * None.
 */
NTSTATUS
LJB_DXGK_SubmitCommandVirtual(
    _In_ const HANDLE                           hAdapter,
    _In_ const DXGKARG_SUBMITCOMMANDVIRTUAL *   pSubmitCommand
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    /*
     * FIXME: check pSubmitCommand->VidPnSourceId for flip operation
     */
    ntStatus = (*DriverInitData->DxgkDdiSubmitCommandVirtual)(
        hAdapter,
        pSubmitCommand
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}
