/*
 * ljb_dxgk_query_current_fence.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_QueryCurrentFence)
#endif

/*
 * Function: LJB_DXGK_QueryCurrentFence
 *
 * Description:
 * The DxgkDdiQueryCurrentFence function queries about the latest completed
 * submission fence identifier in the hardware command execution unit.
 *
 * Return Value:
 * DxgkDdiQueryCurrentFence returns STATUS_SUCCESS, or an appropriate error result
 * if the fence data is not successfully retrieved.
 *
 * Remarks:
 * A fence is an instruction that contains 64 bits of data and an address. The
 * display miniport driver can insert a fence in the direct memory access (DMA)
 * stream that is sent to the graphics processing unit (GPU). When the GPU reads
 * the fence, the GPU writes the fence data at the specified fence address.
 * However, before the GPU can write the fence data to memory, it must ensure that
 * all of the pixels from the primitives that precede the fence instruction are
 * retired and properly written to memory.
 *
 * Note  The GPU is not required to stall the entire pipeline while it waits for
 * the last pixel from the primitives that precede the fence instruction to retire;
 * the GPU can instead run the primitives that follow the fence instruction.
 *
 * Hardware that supports per-GPU-context virtual address space must support the
 * following types of fences:
 * Regular fences are fences that can be inserted in a DMA buffer that is created
 * in user mode. Because the content of a DMA buffer from user mode is not trusted,
 * fences within such a DMA buffer must refer to a virtual address in the GPU
 * context address space and not to a physical address. Access to such a virtual
 * address is bound by the same memory validation mechanism as any other virtual
 * address that the GPU accesses.
 *
 * Privileged fences are fences that can be inserted only in a DMA buffer that is
 * created (and only accessible) in kernel mode. Fences within such a DMA buffer
 * refer to a physical address in memory.
 *
 * Note that if the fence target address was accessible in user mode, malicious
 * software could perform a graphics operation over the memory location for the
 * fence and therefore override the content of what the kernel expected to receive.
 *
 * Note that a privileged DMA buffer can contain both regular and privileged
 * fences. However, if a privileged DMA buffer contains a regular fence, the
 * kernel component that generated such a DMA buffer is aware that the regular
 * fence inside might never be accessible.
 *
 * If the display miniport driver missed the last fence of a DMA buffer, the
 * driver's DxgkDdiQueryCurrentFence function might be called to report the
 * missed fence. For example, if the hardware generates a fence to memory, the
 * driver's DxgkDdiInterruptRoutine function is triggered to read the memory.
 * However, if the fence's data is not available when the driver attempts to read
 * the data (for example, if there is a defective chipset), the fence is typically
 * reported at the next interrupt, unless interrupts were stopped. If interrupts
 * were stopped and the DirectX graphics kernel subsystem waits too long for a
 * fence, the subsystem calls the driver's DxgkDdiQueryCurrentFence function to
 * verify the current fence and determine any pending fence that it might have
 * missed.
 *
 * Before the display miniport driver returns from a call to DxgkDdiQueryCurrentFence,
 * if the latest hardware-completed submission fence identifier has not yet been
 * reported, the driver must call the DxgkCbNotifyInterrupt function to report
 * the fence. To implement this functionality, the driver:
 * Tracks which fence was last reported to the operating system.
 * Raises IRQL to device interrupt. To raise IRQL to interrupt level, the driver
 * can call the DxgkCbSynchronizeExecution function to synchronize with its
 * DxgkDdiInterruptRoutine function.
 * At device interrupt IRQL, compares the last reported fence with the latest
 * hardware-completed fence.
 * At device interrupt IRQL, calls DxgkCbNotifyInterrupt only when the latest
 * hardware completed fence is newer than the last reported fence.
 *
 * DxgkDdiQueryCurrentFence should be made pageable.
 */
NTSTATUS
LJB_DXGK_QueryCurrentFence(
    _In_ const HANDLE                    hAdapter,
    _Out_      DXGKARG_QUERYCURRENTFENCE *pCurrentFence
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    ntStatus = (*DriverInitData->DxgkDdiQueryCurrentFence)(
        hAdapter,
        pCurrentFence
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    return ntStatus;
}