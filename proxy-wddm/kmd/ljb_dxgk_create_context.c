/*
 * ljb_dxgk_create_context.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_CreateContext)
#pragma alloc_text (PAGE, LJB_DXGK_DestroyContext)
#endif

/*
 * forward declaration
 */
static VOID
LJB_DXGK_CreateContextPostProcessing(
    __in LJB_ADAPTER *                  Adapter,
    __in LJB_CONTEXT *                  MyContext,
    _Inout_       DXGKARG_CREATECONTEXT *pCreateContext
    );

static VOID
LJB_DXGK_DestroyContextPostProcessing(
    __in LJB_CONTEXT *  MyContext
    );

/*
 * Function: LJB_DXGK_CreateContext
 *
 * Description:
 * The DxgkDdiCreateContext function creates a graphics processing unit (GPU) context.
 *
 * Return value
 * DxgkDdiCreateContext returns one of the following values:
 *
 *  STATUS_SUCCESS: DxgkDdiCreateContext successfully created the context.
 *
 *  STATUS_NO_MEMORY: DxgkDdiCreateContext could not allocate memory that was
 *  required for it to complete.
 *
 *  STATUS_GRAPHICS_DRIVER_MISMATCH: The display miniport driver is not compatible
 *  with the user-mode display driver that initiated the call to DxgkDdiCreateContext.
 *
 * Remarks
 * A driver uses a GPU context to hold a collection of rendering state.
 * A single process can create multiple contexts on a given device.
 * The driver must support an arbitrary number of contexts. The only valid reason
 * why a driver could not create a context is if system memory runs out.
 * Typically, each context can reference any resource that was previously created
 * for the device that owns that context.
 *
 * DxgkDdiCreateContext should be made pageable.
 */
NTSTATUS
LJB_DXGK_CreateContext(
    _In_    const HANDLE                hDevice,
    _Inout_       DXGKARG_CREATECONTEXT *pCreateContext
    )
{
    LJB_DEVICE * CONST                  MyDevice = LJB_DXGK_FindDevice(hDevice);
    LJB_ADAPTER * CONST                 Adapter = MyDevice->Adapter;
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    LJB_CONTEXT *                       MyContext;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    /*
     * create MyContext
     */
    MyContext = LJB_GetPoolZero(sizeof(LJB_CONTEXT));
    if (MyContext == NULL)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": unable to allocate MyContext\n"));
        return STATUS_NO_MEMORY;
    }

    InitializeListHead(&MyContext->ListEntry);
    MyContext->Adapter = Adapter;
    MyContext->MyDevice = MyDevice;
    MyContext->CreateContext = *pCreateContext;

    /*
     * pass the call to inbox driver
     */
    ntStatus = (*DriverInitData->DxgkDdiCreateContext)(hDevice, pCreateContext);
    if (!NT_SUCCESS(ntStatus))
    {
        LJB_FreePool(MyContext);
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        return ntStatus;
    }

    LJB_DXGK_CreateContextPostProcessing(Adapter, MyContext, pCreateContext);

    return ntStatus;
}

static VOID
LJB_DXGK_CreateContextPostProcessing(
    __in LJB_ADAPTER *              Adapter,
    __in LJB_CONTEXT *              MyContext,
    _Inout_ DXGKARG_CREATECONTEXT*  pCreateContext
    )
{
    KIRQL                               oldIrql;

    /*
     * track what the driver returns. Note that the size of pCreateContext->ContextInfo
     * is WDK dependent. Be careful, do not use structure assignment to track the
     * whole structure.
     */
    MyContext->hContext = pCreateContext->hContext;
    MyContext->ContextInfo.DmaBufferSize = pCreateContext->ContextInfo.DmaBufferSize;
    MyContext->ContextInfo.DmaBufferSegmentSet = pCreateContext->ContextInfo.DmaBufferSegmentSet;
    MyContext->ContextInfo.DmaBufferPrivateDataSize = pCreateContext->ContextInfo.DmaBufferPrivateDataSize;
    MyContext->ContextInfo.AllocationListSize = pCreateContext->ContextInfo.AllocationListSize;
    MyContext->ContextInfo.PatchLocationListSize = pCreateContext->ContextInfo.PatchLocationListSize;
    if (Adapter->DxgkInterface.Version >= DXGKDDI_INTERFACE_VERSION_WDDM2_0)
    {
        MyContext->ContextInfo.Caps = pCreateContext->ContextInfo.Caps;
        MyContext->ContextInfo.PagingCompanionNodeId = pCreateContext->ContextInfo.PagingCompanionNodeId;
    }

    KeAcquireSpinLock(&GlobalDriverData.ClientContextListLock, &oldIrql);
    InsertTailList(&GlobalDriverData.ClientContextListHead, &MyContext->ListEntry);
    KeReleaseSpinLock(&GlobalDriverData.ClientContextListLock, oldIrql);

    DBG_PRINT(Adapter, DBGLVL_FLOW,
        (__FUNCTION__ ": hContext(%p) tracked\n",
        MyContext->hContext
        ));
}

LJB_CONTEXT *
LJB_DXGK_FindContext(
    __in HANDLE     hContext
    )
{
    LIST_ENTRY * CONST  listHead = &GlobalDriverData.ClientContextListHead;
    LIST_ENTRY *        listEntry;
    LJB_CONTEXT *       MyContext;
    KIRQL               oldIrql;

    MyContext = NULL;
    KeAcquireSpinLock(&GlobalDriverData.ClientContextListLock, &oldIrql);
    for (listEntry = listHead->Flink;
         listEntry != listHead;
         listEntry = listEntry->Flink)
    {
        LJB_CONTEXT * thisContext;

        thisContext = CONTAINING_RECORD(listEntry, LJB_CONTEXT, ListEntry);
        if (thisContext->hContext == hContext)
        {
            MyContext = thisContext;
            break;
        }
    }
    KeReleaseSpinLock(&GlobalDriverData.ClientContextListLock, oldIrql);

    return MyContext;
}

/*
 * Function: LJB_DXGK_DestroyContext
 *
 * Description:
 * The DxgkDdiDestroyContext function destroys the specified graphics processing
 * unit (GPU) context.
 *
 * Return value
 * DxgkDdiDestroyContext returns STATUS_SUCCESS, or an appropriate error result
 * if the context is not successfully destroyed.
 *
 * Remarks
 * A driver should free all resources that it allocated for the context and clean
 * up any internal tracking data structures.
 *
 * DxgkDdiDestroyContext should be made pageable.
 */
NTSTATUS
LJB_DXGK_DestroyContext(
    _In_ const HANDLE hContext
    )
{
    LJB_CONTEXT * CONST                 MyContext = LJB_DXGK_FindContext(hContext);
    LJB_ADAPTER * CONST                 Adapter = MyContext->Adapter;
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;

    PAGED_CODE();

    /*
     * pass the call to inbox driver
     */
    ntStatus = (*DriverInitData->DxgkDdiDestroyContext)(hContext);
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
        return ntStatus;
    }

    LJB_DXGK_DestroyContextPostProcessing(MyContext);

    return ntStatus;
}

static VOID
LJB_DXGK_DestroyContextPostProcessing(
    __in LJB_CONTEXT *  MyContext
    )
{
    KIRQL                               oldIrql;

    KeAcquireSpinLock(&GlobalDriverData.ClientContextListLock, &oldIrql);
    RemoveEntryList(&MyContext->ListEntry);
    KeReleaseSpinLock(&GlobalDriverData.ClientContextListLock, oldIrql);
    LJB_FreePool(MyContext);
}
