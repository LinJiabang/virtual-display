#include "ljb_vmon.h"

DWORD
LJB_VMON_Main(
    __in LPVOID         lpThreadParameter
    )
{
    HANDLE CONST        hDefaultHeap = GetProcessHeap();
    PDEVICE_INFO        pDeviceInfo;
    LJB_VMON_DEV_CTX *  dev_ctx;
    BOOL                bRet;

    pDeviceInfo = lpThreadParameter;
    dev_ctx = HeapAlloc(hDefaultHeap, HEAP_ZERO_MEMORY, sizeof(*dev_ctx));

    if (dev_ctx == NULL)
    {
        DBG_PRINT(("unable to get dev_ctx?\n"));
        return 0;
    }

    /*
     * Check the existence of the device
     */
    bRet = LJB_VMON_GetDeviceHandle(dev_ctx);
    if (bRet)
    {
        dev_ctx->pDeviceInfo = pDeviceInfo;
        pDeviceInfo->dev_ctx = dev_ctx;
        dev_ctx->exit_vmon_thread = FALSE;
        LJB_VMON_PixelMain(dev_ctx);

        // stop
        LJB_VMON_CloseDeviceHandle(dev_ctx);
    }
    HeapFree(hDefaultHeap, 0, dev_ctx);

    return 1;
}
