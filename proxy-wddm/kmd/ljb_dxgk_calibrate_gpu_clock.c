/*
 * ljb_dxgk_calibrate_gpu_clock.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

/*
 * Function: LJB_DXGK_CalibrateGpuClock
 *
 * Description:
 * Called by the Microsoft DirectX graphics kernel subsystem to calibrate the GPU
 * time stamps in the DXGK_HISTORY_BUFFER history buffer with the CPU clock time.
 *
 * Return Value:
 * Returns STATUS_SUCCESS if it succeeds; otherwise, it returns one of the error
 * codes defined in Ntstatus.h.
 *
 * Remarks:
 * The DirectX graphics kernel subsystem uses the returned info in the pClockCalibration
 * parameter to estimate the drift between the GPU and CPU clocks.
 *
 * To minimize calibration inaccuracies, the driver should compute the values for
 * the GpuClockCounter and CpuClockCounter members of the DXGKARG_CALIBRATEGPUCLOCK
 * structure as nearly simultaneously as possible.
 *
 * The DirectX graphics kernel subsystem calls this function often enough, typically
 * at least once every 30ms, to minimize the accumulated drift between the GPU
 * and CPU clocks.
 */
NTSTATUS
LJB_DXGK_CalibrateGpuClock(
    _In_  const HANDLE                    hAdapter,
    _In_        UINT32                    NodeOrdinal,
    _In_        UINT32                    EngineOrdinal,
    _Out_       DXGKARG_CALIBRATEGPUCLOCK *pClockCalibration
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    LJB_ENGINE_INFO *                   EngineInfo = &Adapter->EngineInfo[NodeOrdinal][EngineOrdinal];
    NTSTATUS                            ntStatus;

    ntStatus = (*DriverInitData->DxgkDdiCalibrateGpuClock)(
        hAdapter,
        NodeOrdinal,
        EngineOrdinal,
        pClockCalibration
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
    }

    EngineInfo->CalibrateGpuClock = *pClockCalibration;
    return ntStatus;
}
