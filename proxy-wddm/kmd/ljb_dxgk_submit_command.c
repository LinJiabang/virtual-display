/*
 * ljb_dxgk_submit_command.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

/*
 * Function: LJB_DXGK_SubmitCommand
 *
 * Description:
 * The DxgkDdiSubmitCommand function submits a direct memory access (DMA) buffer
 * to the hardware command execution unit.
 *
 * Return Value:
 * Returns STATUS_SUCCESS upon successful completion. If the driver instead
 * returns an error code, the operating system causes a system bugcheck to occur.
 * For more information, see the following Remarks section.
 *
 * Remarks:
 * Because paging operations are considered system operations, they are not
 * associated with a specific application context or graphics context. Therefore,
 * when the submission is for a paging operation, the DxgkDdiSubmitCommand function
 * is called with NULL specified in the hDevice member of the DXGKARG_SUBMITCOMMAND
 * structure that the pSubmitCommand parameter points to.
 *
 * However, if the architecture of a particular hardware and driver must have a
 * device internally, the driver must internally create the device during adapter
 * initialization and must keep the device internally as the system default device
 * for use in paging operations.
 *
 * The driver can write the value that is supplied in the SubmissionFenceId member
 * of DXGKARG_SUBMITCOMMAND into the fence command in the ring buffer. For more
 * information about fence commands, see Supplying Fence Identifiers.
 *
 * If the driver returns an error code, the Microsoft DirectX graphics kernel
 * subsystem causes a system bugcheck to occur. In a crash dump file, the error
 * is noted by the message BugCheck 0x119, which has the following four parameters.
 *
 * 1. 0x2
 * 2. The NTSTATUS error code returned from the failed driver call
 * 3. A pointer to the DXGKARG_SUBMITCOMMAND structure
 * 4. A pointer to an internal scheduler data structure
 *
 * DxgkDdiSubmitCommand should be made nonpageable because it runs at
 * IRQL = DISPATCH_LEVEL.
 */
NTSTATUS
LJB_DXGK_SubmitCommand(
    _In_ const HANDLE                hAdapter,
    _In_ const DXGKARG_SUBMITCOMMAND *pSubmitCommand
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    ntStatus = (*DriverInitData->DxgkDdiSubmitCommand)(
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