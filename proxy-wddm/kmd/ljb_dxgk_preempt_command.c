/*
 * ljb_dxgk_preempt_command.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"


/*
 * Function: LJB_DXGK_PreemptCommand
 *
 * Description:
 * The DxgkDdiPreemptCommand function preempts a direct memory access (DMA)
 * buffer that was previously submitted to and currently queued in the hardware
 * command execution unit.
 *
 * Return Value:
 * Returns STATUS_SUCCESS upon successful completion. If the driver instead
 * returns an error code, the operating system causes a system bugcheck to occur.
 * For more information, see the following Remarks section.
 *
 * Remarks:
 * If the driver determines that the hardware is already finished processing all
 * of the submitted DMA buffers--and that the hardware informed the graphics
 * processing unit (GPU) scheduler about the completions--when its
 * DxgkDdiPreemptCommand function is called to preempt the DMA buffers, the driver
 * should perform the following operations instead of submitting the preempt fence
 * that is identified by the PreemptionFenceId member of DXGKARG_PREEMPTCOMMAND
 * to the hardware:
 * Raise IRQL to interrupt level. For example, the driver can call the
 * DxgkCbSynchronizeExecution function to synchronize with its DxgkDdiInterruptRoutine
 * function.
 *
 * Inform the GPU scheduler about the preemption information. The driver can either
 * call the DxgkCbNotifyInterrupt function directly or call its DxgkDdiInterruptRoutine
 * function (for example, if the driver must perform other updates as well).
 * Note that the GPU scheduler handles instances where the hardware has stopped
 * responding because of Timeout Detection and Recovery (TDR) work.
 *
 * If the driver returns an error code, the Microsoft DirectX graphics kernel
 * subsystem causes a system bugcheck to occur. In a crash dump file, the error
 * is noted by the message BugCheck 0x119, which has the following four parameters.
 *
 * 1. 0x2
 * 2. The NTSTATUS error code returned from the failed driver call
 * 3. A pointer to the DXGKARG_PREEMPTCOMMAND structure
 * 4. A pointer to an internal scheduler data structure
 *
 * DxgkDdiPreemptCommand should be made nonpageable because it runs at IRQL = DISPATCH_LEVEL
 */
NTSTATUS
LJB_DXGK_PreemptCommand(
    _In_ const HANDLE                   hAdapter,
    _In_ const DXGKARG_PREEMPTCOMMAND * pPreemptCommand
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    ntStatus = (*DriverInitData->DxgkDdiPreemptCommand)(
        hAdapter,
        pPreemptCommand
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}