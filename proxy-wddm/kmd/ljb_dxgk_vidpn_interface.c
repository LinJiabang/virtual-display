/*
 * ljb_dxgk_vidpn_interface.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"
#include "ljb_dxgk_vidpn_interface.h"

CONST DXGK_VIDPN_INTERFACE  MyVidPnInterface =
{
    DXGK_VIDPN_INTERFACE_VERSION_V1,            /* DXGK_VIDPN_INTERFACE_VERSION */
    &LJB_VIDPN_GetTopology,                     /* DXGKDDI_VIDPN_GETTOPOLOGY  */
    &LJB_VIDPN_AcquireSourceModeSet,            /* DXGKDDI_VIDPN_ACQUIRESOURCEMODESET */
    &LJB_VIDPN_ReleaseSourceModeSet,            /* DXGKDDI_VIDPN_RELEASESOURCEMODESET */
    &LJB_VIDPN_CreateNewSourceModeSet,          /* DXGKDDI_VIDPN_CREATENEWSOURCEMODESET */
    &LJB_VIDPN_AssignSourceModeSet,             /* DXGKDDI_VIDPN_ASSIGNSOURCEMODESET */
    &LJB_VIDPN_AssignMultiSampleSourceModeSet,  /* DXGKDDI_VIDPN_ASSIGNMULTISAMPLINGMETHODSET */
    &LJB_VIDPN_AcquireTargetModeSet,            /* DXGKDDI_VIDPN_ACQUIRETARGETMODESET */
    &LJB_VIDPN_ReleaseTargetModeSet,            /* DXGKDDI_VIDPN_RELEASETARGETMODESET */
    &LJB_VIDPN_CreateNewTargetModeSet,          /* DXGKDDI_VIDPN_CREATENEWTARGETMODESET */
    &LJB_VIDPN_AssignTargetModeSet              /* DXGKDDI_VIDPN_ASSIGNTARGETMODESET */
};

/*
 * Implementation
 */
LJB_VIDPN *
LJB_VIDPN_CreateVidPn(
    __in LJB_ADAPTER *  Adapter,
    __in D3DKMDT_HVIDPN hVidPn
    )
{
    LJB_VIDPN *     MyVidPn;
    NTSTATUS        ntStatus;

    MyVidPn = LJB_PROXYKMD_GetPoolZero(sizeof(LJB_VIDPN));
    if (MyVidPn != NULL)
    {
        MyVidPn->MagicBegin = LJB_VINPN_MAGIC;
        MyVidPn->Adapter = Adapter;
        MyVidPn->hVidPn = hVidPn;
    }

    ntStatus = LJB_VIDPN_PrefetchTopology(MyVidPn);
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__": unable te prefetch topology?\n"));
        LJB_VIDPN_DestroyVidPn(MyVidPn);
        MyVidPn = NULL;
    }
    return MyVidPn;
}

VOID
LJB_VIDPN_DestroyVidPn(
    __in LJB_VIDPN *    MyVidPn
    )
{
    if (MyVidPn->Topology.pPaths != NULL)
        LJB_PROXYKMD_FreePool(MyVidPn->Topology.pPaths);
    LJB_PROXYKMD_FreePool(MyVidPn);
}

NTSTATUS
LJB_VIDPN_PrefetchTopology(
    __in LJB_VIDPN *    MyVidPn
    )
{
    LJB_ADAPTER * CONST                 Adapter = MyVidPn->Adapter;
    DXGKRNL_INTERFACE * CONST           DxgkInterface = &Adapter->DxgkInterface;
    CONST DXGK_VIDPN_INTERFACE *        VidPnInterface;
    D3DKMDT_HVIDPNTOPOLOGY              hVidPnTopology;
    CONST DXGK_VIDPNTOPOLOGY_INTERFACE* VidPnTopologyInterface;
    CONST D3DKMDT_VIDPN_PRESENT_PATH *  PrevPath;
    CONST D3DKMDT_VIDPN_PRESENT_PATH *  ThisPath;
    NTSTATUS                            ntStatus;
    ULONG                               i;

    MyVidPn->NumPaths = 0;
    ntStatus = (*DxgkInterface->DxgkCbQueryVidPnInterface)(
        MyVidPn->hVidPn,
        DXGK_VIDPN_INTERFACE_VERSION_V1,
        &VidPnInterface
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?"__FUNCTION__": DxgkCbQueryVidPnInterface failed with ntStatus(0x%x)",
            ntStatus));
        return ntStatus;
    }

    /*
     * query topology
     */
    ntStatus = (*VidPnInterface->pfnGetTopology)(
        MyVidPn->hVidPn,
        &hVidPnTopology,
        &VidPnTopologyInterface
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?"__FUNCTION__": pfnGetTopology failed with ntStatus(0x%x)",
            ntStatus));
        return ntStatus;
    }

    ntStatus = (*VidPnTopologyInterface->pfnGetNumPaths)(
        hVidPnTopology,
        &MyVidPn->NumPaths
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?"__FUNCTION__": pfnGetNumPaths failed with ntStatus(0x%x)",
            ntStatus));
        return ntStatus;
    }

    /*
     * Query each paths.
     */
    PrevPath = NULL;
    ThisPath = NULL;
    for (i = 0; i < MyVidPn->NumPaths; i++)
    {
        if (i == 0)
        {
            ntStatus = (*VidPnTopologyInterface->pfnAcquireFirstPathInfo)(
                hVidPnTopology,
                &ThisPath
                );
            if (!NT_SUCCESS(ntStatus))
            {
                DBG_PRINT(Adapter, DBGLVL_ERROR,
                    ("?" __FUNCTION__ ": "
                    "pfnAcquireFirstPathInfo failed with ntStatus(0x%08x)?\n",
                    ntStatus
                    ));
                break;
            }
            if (ThisPath == NULL)
            {
                DBG_PRINT(Adapter, DBGLVL_ERROR,
                    ("?" __FUNCTION__ ": "
                    "pfnAcquireFirstPathInfo return NULL for ThisPath?\n"
                    ));
                break;
            }
        }
        else
        {
            ntStatus = (*VidPnTopologyInterface->pfnAcquireNextPathInfo)(
                hVidPnTopology,
                PrevPath,
                &ThisPath
                );
            if (!NT_SUCCESS(ntStatus))
            {
                DBG_PRINT(Adapter, DBGLVL_ERROR,
                    ("?" __FUNCTION__ ": "
                    "pfnAcquireNextPathInfo failed with ntStatus(0x%08x)?\n",
                    ntStatus
                    ));
                break;
            }

            if (ThisPath == NULL)
            {
                DBG_PRINT(Adapter, DBGLVL_ERROR,
                    ("?" __FUNCTION__ ": "
                    "pfnAcquireNextPathInfo return NULL for ThisPath?\n"
                    ));
                break;
            }

            ntStatus = (*VidPnTopologyInterface->pfnReleasePathInfo)(
                hVidPnTopology,
                PrevPath
                );
            if (!NT_SUCCESS(ntStatus))
            {
                DBG_PRINT(Adapter, DBGLVL_ERROR,
                    ("?" __FUNCTION__ ": "
                    "pfnReleasePathInfo failed with ntStatus(0x%08x)?\n",
                    ntStatus
                    ));
                break;
            }
        }
        PrevPath = ThisPath;
        MyVidPn->Paths[i] = *ThisPath;
    } /* end of for loop */

    /*
     * Release the last queried pPath
     */
    ntStatus =(*VidPnTopologyInterface->pfnReleasePathInfo)(
        hVidPnTopology,
        PrevPath
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": "
            "pfnReleasePathInfo failed with ntStatus(0x%08x)?\n",
            ntStatus
            ));
    }

    return ntStatus;
}

UINT
LJB_VIDPN_GetNumberOfUsbTarget(
    __in LJB_VIDPN *    MyVidPn
    )
{
    LJB_ADAPTER * CONST Adapter = MyVidPn->Adapter;
    UINT                i;
    UINT                NumOfUsbTarget;

    NumOfUsbTarget = 0;
    for (i = 0; i < MyVidPn->NumPaths; i++)
    {
        D3DKMDT_VIDPN_PRESENT_PATH * CONST Path = MyVidPn->Paths + i;

        if (Path->VidPnTargetId >= Adapter->UsbTargetIdBase)
            NumOfUsbTarget++;
    }
    return NumOfUsbTarget;
}

UINT
LJB_VIDPN_GetNumberOfInboxTarget(
    __in LJB_VIDPN *    MyVidPn
    )
{
    LJB_ADAPTER * CONST Adapter = MyVidPn->Adapter;
    UINT                i;
    UINT                NumOfInboxTarget;

    NumOfInboxTarget = 0;
    for (i = 0; i < MyVidPn->NumPaths; i++)
    {
        D3DKMDT_VIDPN_PRESENT_PATH * CONST Path = MyVidPn->Paths + i;

        if (Path->VidPnTargetId < Adapter->UsbTargetIdBase)
            NumOfInboxTarget++;
    }
    return NumOfInboxTarget;
}

/*
 * Name : LJB_DXGKCB_QueryVidPnInterface
 *
 * Description:
 * The DxgkCbQueryVidPnInterface function returns a pointer to a DXGK_VIDPN_INTERFACE
 * structure. The structure contains pointers to functions that the display miniport
 * driver can call to inspect and alter a VidPN object.
 *
 * The DXGK_VIDPN_INTERFACE structure contains pointers to functions that belong
 * to the VidPn interface, which is implemented by the video present network (VidPN)
 * manager.
 * The display miniport driver calls DxgkCbQueryVidPnInterface to obtain a pointer
 * to a DXGK_VIDPN_INTERFACE structure. The structure contains pointers to functions
 * that the display miniport driver can call to inspect and alter a VidPN object.
 *
 * For more information about the VidPN interface, see VidPN Objects and Interfaces.
 *
 * Return Value:
 * DxgkCbQueryVidPnInterface returns one of the following values:
 * STATUS_SUCCESS
 * The function succeeded.
 * STATUS_INVALID_PARAMETER
 * The value passed to ppVidPnInterface is not valid.
 * STATUS_GRAPHICS_INVALID_VIDPN
 * The handle passed to hVidPn is not valid.
 * STATUS_NOT_SUPPORTED
 * The interface version specified by VidPnInterfaceVersion is not supported.
 */
NTSTATUS
LJB_DXGKCB_QueryVidPnInterface(
    __in CONST D3DKMDT_HVIDPN               hVidPn,
    __in CONST DXGK_VIDPN_INTERFACE_VERSION VidPnInterfaceVersion,
    __out CONST DXGK_VIDPN_INTERFACE**      ppVidPnInterface
    )
{
    LJB_VIDPN * CONST   MyVidPn = (LJB_VIDPN *) hVidPn;
    LJB_ADAPTER *       Adapter;
    DXGKRNL_INTERFACE * DxgkInterface;
    NTSTATUS            ntStatus;

    DBG_UNREFERENCED_LOCAL_VARIABLE(Adapter);

    /*
     * sanity check. If Magic number doesn't match, don't try to hack.
     */
    if (MyVidPn->MagicBegin != LJB_VINPN_MAGIC)
    {
        KdPrint(("?" __FUNCTION__ ": not my vidpn object?\n"));
        return STATUS_GRAPHICS_INVALID_VIDPN;
    }

    /*
     * now MyVidPn is valid. Adapter, DxgkInterface, and call real DxgkCbQueryVidPnInterface
     */
    Adapter = MyVidPn->Adapter;
    DxgkInterface = &Adapter->DxgkInterface;
    MyVidPn->VidPnInterfaceVersion = VidPnInterfaceVersion;
    ntStatus = (*DxgkInterface->DxgkCbQueryVidPnInterface)(
        MyVidPn->hVidPn,
        VidPnInterfaceVersion,
        &MyVidPn->VidPnInterface
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR, ("?" __FUNCTION__": failed ntStatus(0x%08x)\n", ntStatus));
        return ntStatus;
    }

    *ppVidPnInterface = &MyVidPnInterface;
    DBG_PRINT(Adapter, DBGLVL_VIDPN, (__FUNCTION__": return MyVidPnInterface\n"));
    return ntStatus;
}


/*
 * Name : LJB_VIDPN_GetTopology
 *
 * Description:
 * The pfnGetTopology function returns a handle to the VidPN topology object
 * contained by a specified VidPN object.
 *
 * Return Value:
 * The pfnGetTopology function returns one of the following values:
 * STATUS_SUCCESS
 *  The function succeeded.
 * STATUS_GRAPHICS_INVALID_VIDPN
 *  The handle supplied in hVidPn was invalid.
 *
 * Comments
 * The display miniport driver does not need to release the handle that it receives
 * in phVidPnTopology.
 * The lifetime of the DXGK_VIDPNTOPOLOGY_INTERFACE structure returned in
 * ppVidPnTopologyInterface is owned by the operating system. Using this ownership
 * scheme, the operating system can migrate to newer implementations at run time
 * without breaking clients of the interface.
 *
 */
NTSTATUS
LJB_VIDPN_GetTopology(
    __in CONST D3DKMDT_HVIDPN                   hVidPn,
    __out D3DKMDT_HVIDPNTOPOLOGY*               phVidPnTopology,
    __out CONST DXGK_VIDPNTOPOLOGY_INTERFACE**  ppVidPnTopologyInterface
    )
{
    LJB_VIDPN * CONST       MyVidPn = (LJB_VIDPN *) hVidPn;
    LJB_ADAPTER * CONST     Adapter = MyVidPn->Adapter;
    LJB_VIDPN_TOPOLOGY *    MyTopology;
    NTSTATUS                ntStatus;

    /*
     * sanity check. If Magic number doesn't match, don't try to hack.
     */
    if (MyVidPn->MagicBegin != LJB_VINPN_MAGIC)
    {
        KdPrint(("?" __FUNCTION__ ": not my vidpn object?\n"));
        return STATUS_GRAPHICS_INVALID_VIDPN;
    }

    ntStatus = (*MyVidPn->VidPnInterface->pfnGetTopology)(
        MyVidPn->hVidPn,
        phVidPnTopology,
        ppVidPnTopologyInterface
        );
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__": failed ntStatus(0x%08x)\n", ntStatus));
        return ntStatus;
    }

    MyTopology = &MyVidPn->Topology;
    MyTopology->Adapter = Adapter;
    MyTopology->VidPnTopologyInterface = *ppVidPnTopologyInterface;
    MyTopology->hVidPnTopology = *phVidPnTopology;

    LJB_VIDPN_TOPOLOGY_Initialize(Adapter, MyTopology);
    *phVidPnTopology = (D3DKMDT_HVIDPNTOPOLOGY) MyTopology;
    *ppVidPnTopologyInterface = &MyTopologyInterface;

    DBG_PRINT(Adapter, DBGLVL_VIDPN,
        (__FUNCTION__": return MyTopology(%p)\n", MyTopology));
    return ntStatus;
}

/*
 * Name : LJB_VIDPN_AcquireSourceModeSet
 *
 * Description:
 * The pfnAcquireSourceModeSet function returns a handle to a particular source
 * mode set object that is contained by a specified VidPN object.
 *
 * Return Value:
 * The pfnAcquireSourceModeSet function returns one of the following values:
 * STATUS_SUCCESS
 *  The function succeeded.
 * STATUS_GRAPHICS_INVALID_VIDPN
 *  The handle supplied in hVidPn was invalid.
 * STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE
 *  The identifier supplied in VidPnSourceId was invalid.
 *
 * Comments
 * VidPN source identifiers are assigned by the operating system. DxgkDdiStartDevice,
 * implemented by the display miniport driver, returns the number N of video present
 * sources supported by the display adapter. Then the operating system assigns
 * identifiers 0, 1, 2, ¡K N - 1.
 *
 * When you have finished using the source mode set object handle, you must
 * release the handle by calling pfnReleaseSourceModeSet. Source mode set objects
 * are reference counted, so if you acquire a handle several times, you must
 * release it that same number of times.
 *
 * The lifetime of the DXGK_VIDPNSOURCEMODESET_INTERFACE structure returned in
 * ppVidPnSourceModeSetInterface is owned by the operating system. Using this
 * ownership scheme, the operating system can switch to newer implementations at
 * run time without breaking clients of the interface.
 *
 * The D3DDDI_VIDEO_PRESENT_SOURCE_ID data type is defined in D3dukmdt.h.
 *
 * The D3DKMDT_HVIDPN and D3DKMDT_HVIDPNSOURCEMODESET data types are defined in
 * D3dkmdt.h.
 *
 */
NTSTATUS
LJB_VIDPN_AcquireSourceModeSet(
    __in CONST D3DKMDT_HVIDPN                       hVidPn,
    __in CONST D3DDDI_VIDEO_PRESENT_SOURCE_ID       VidPnSourceId,
    __out D3DKMDT_HVIDPNSOURCEMODESET*              phVidPnSourceModeSet,
    __out CONST DXGK_VIDPNSOURCEMODESET_INTERFACE** ppVidPnSourceModeSetInterface
    )
{
    LJB_VIDPN * CONST   MyVidPn = (LJB_VIDPN *) hVidPn;
    LJB_ADAPTER * CONST Adapter = MyVidPn->Adapter;
    NTSTATUS            ntStatus;

    DBG_UNREFERENCED_LOCAL_VARIABLE(Adapter);
    /*
     * sanity check. If Magic number doesn't match, don't try to hack.
     */
    if (MyVidPn->MagicBegin != LJB_VINPN_MAGIC)
    {
        KdPrint(("?" __FUNCTION__ ": not my vidpn object?\n"));
        return STATUS_GRAPHICS_INVALID_VIDPN;
    }

    ntStatus = (*MyVidPn->VidPnInterface->pfnAcquireSourceModeSet)(
        MyVidPn->hVidPn,
        VidPnSourceId,
        phVidPnSourceModeSet,
        ppVidPnSourceModeSetInterface
        );
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__": failed ntStatus(0x%08x)\n", ntStatus));
    }
    return ntStatus;
}

/*
 * Name : LJB_VIDPN_ReleaseSourceModeSet
 *
 * Description:
 * The pfnReleaseSourceModeSet function releases a handle to a source mode set object.
 *
 * Return Value:
 * The pfnReleaseSourceModeSet  function returns one of the following values:
 * STATUS_SUCCESS
 *  The function succeeded.
 * STATUS_GRAPHICS_INVALID_VIDPN
 *  The handle supplied in hVidPn was invalid.
 * STATUS_GRAPHICS_INVALID_VIDPN_SOURCEMODESET
 *  The handle supplied in hVidPnSourceModeSet was invalid.
 * STATUS_GRAPHICS_RESOURCES_NOT_RELATED
 *  The VidPN identified by hVidPn does not contain the source mode set identified
 *  by hVidPnSourceModeSet.
 *
 * Comments
 * When you have finished using a handle that you obtained by calling pfnAcquireSourceModeSet,
 * you must release the handle by calling pfnReleaseSourceModeSet.
 *
 * If you obtain a handle by calling pfnCreateNewSourceModeSet and then pass that
 * handle to pfnAssignSourceModeSet, you do not need to release the handle.
 *
 * If you obtain a handle by calling pfnCreateNewSourceModeSet and then you decide
 * not to assign the new source mode set to a source, you must release the newly
 * obtained handle by calling pfnReleaseSourceModeSet.
 *
 * The D3DKMDT_HVIDPN and D3DKMDT_HVIDPNSOURCEMODESET data types are defined in
 * D3dkmdt.h.
 *
 */
NTSTATUS
LJB_VIDPN_ReleaseSourceModeSet(
    __in CONST D3DKMDT_HVIDPN               hVidPn,
    __in CONST D3DKMDT_HVIDPNSOURCEMODESET  hVidPnSourceModeSet
    )
{
    LJB_VIDPN * CONST   MyVidPn = (LJB_VIDPN *) hVidPn;
    LJB_ADAPTER * CONST Adapter = MyVidPn->Adapter;
    NTSTATUS            ntStatus;

    DBG_UNREFERENCED_LOCAL_VARIABLE(Adapter);

    /*
     * sanity check. If Magic number doesn't match, don't try to hack.
     */
    if (MyVidPn->MagicBegin != LJB_VINPN_MAGIC)
    {
        KdPrint(("?" __FUNCTION__ ": not my vidpn object?\n"));
        return STATUS_GRAPHICS_INVALID_VIDPN;
    }

    ntStatus = (*MyVidPn->VidPnInterface->pfnReleaseSourceModeSet)(
        MyVidPn->hVidPn,
        hVidPnSourceModeSet
        );
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__": failed ntStatus(0x%08x)\n", ntStatus));
    }
    return ntStatus;
}

/*
 * Name : LJB_VIDPN_CreateNewSourceModeSet
 *
 * Description:
 * The pfnCreateNewSourceModeSet function creates a new source mode set object
 * within a specified VidPN object.
 *
 * Return Value:
 * The pfnReleaseSourceModeSet  function returns one of the following values:
 * STATUS_SUCCESS
 *  The function succeeded.
 * STATUS_GRAPHICS_INVALID_VIDPN
 *  The handle supplied in hVidPn was invalid.
 * STATUS_NO_MEMORY
 *  The VidPN manager was unable to allocate the memory required to create the new
 *  source mode set object.
 *
 * Comments
 * To assign a new source mode set to a particular source in a VidPN, perform the
 * following steps:
 *
 * 1    Call pfnCreateNewSourceModeSet to get a handle to a new source mode set
 *      object. That source mode set object belongs to a particular VidPN object
 *      that you specify.
 * 2    Use the functions of the DXGK_VIDPNSOURCEMODESET_INTERFACE interface to
 *      add modes to the source mode set object.
 * 3    Call pfnAssignSourceModeSet to assign the new source mode set to a particular
 *      source.
 *
 * If you obtain a handle by calling pfnCreateNewSourceModeSet and then pass that
 * handle to pfnAssignSourceModeSet, you do not need to release the handle by calling
 * pfnReleaseSourceModeSet.
 *
 * If you obtain a handle by calling pfnCreateNewSourceModeSet and then you decide
 * not to assign the new source mode set to a source, you must release the newly
 * obtained handle by calling pfnReleaseSourceModeSet.
 *
 * The lifetime of the DXGK_VIDPNSOURCEMODESET_INTERFACE structure returned in
 * ppVidPnSourceModeSetInterface is owned by the operating system. Using this ownership
 * scheme, the operating system can switch to newer implementations at run time
 * without breaking clients of the interface.
 *
 * The D3DKMDT_HVIDPN and D3DKMDT_HVIDPNSOURCEMODESET data types are defined in
 * D3dkmdt.h.
 */
NTSTATUS
LJB_VIDPN_CreateNewSourceModeSet(
    __in CONST D3DKMDT_HVIDPN                       hVidPn,
    __in CONST D3DDDI_VIDEO_PRESENT_SOURCE_ID       VidPnSourceId,
    __out D3DKMDT_HVIDPNSOURCEMODESET*              phNewVidPnSourceModeSet,
    __out CONST DXGK_VIDPNSOURCEMODESET_INTERFACE** ppVidPnSourceModeSetInterface
    )
{
    LJB_VIDPN * CONST   MyVidPn = (LJB_VIDPN *) hVidPn;
    LJB_ADAPTER * CONST Adapter = MyVidPn->Adapter;
    NTSTATUS            ntStatus;

    DBG_UNREFERENCED_LOCAL_VARIABLE(Adapter);

    /*
     * sanity check. If Magic number doesn't match, don't try to hack.
     */
    if (MyVidPn->MagicBegin != LJB_VINPN_MAGIC)
    {
        KdPrint(("?" __FUNCTION__ ": not my vidpn object?\n"));
        return STATUS_GRAPHICS_INVALID_VIDPN;
    }

    ntStatus = (*MyVidPn->VidPnInterface->pfnCreateNewSourceModeSet)(
        MyVidPn->hVidPn,
        VidPnSourceId,
        phNewVidPnSourceModeSet,
        ppVidPnSourceModeSetInterface
        );
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__": failed ntStatus(0x%08x)\n", ntStatus));
    }
    return ntStatus;
}

/*
 * Name : LJB_VIDPN_AssignSourceModeSet
 *
 * Description:
 * The pfnAssignSourceModeSet function assigns a source mode set to a particular
 * source in a specified VidPN.
 *
 * Return Value:
 * The pfnAssignSourceModeSet  function returns one of the following values:
 * STATUS_SUCCESS
 *  The function succeeded.
 * STATUS_GRAPHICS_INVALID_VIDPN
 *  The handle supplied in hVidPn was invalid.
 * STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE
 *  The identifier supplied in VidPnSourceId was invalid.
 * STATUS_GRAPHICS_INVALID_VIDPN_SOURCEMODESET
 *  The handle supplied in hVidPnSourceModeSet was invalid.
 * STATUS_GRAPHICS_PINNED_MODE_MUST_REMAIN_IN_SET
 *  The source mode set you are attempting to assign does not contain the mode
 *  that was already pinned on the source.
 *
 * Comments
 * VidPN source identifiers are assigned by the operating system. DxgkDdiStartDevice,
 * implemented by the display miniport driver, returns the number N of video present
 * sources supported by the display adapter. Then the operating system assigns
 * identifiers 0, 1, 2, ¡K N - 1.
 *
 * If you obtain a handle by calling pfnCreateNewSourceModeSet and then pass that
 * handle to pfnAssignSourceModeSet, you do not need to release the handle by calling
 * pfnReleaseSourceModeSet.
 *
 * If you obtain a handle by calling pfnCreateNewSourceModeSet and then you decide
 * not to assign the new source mode set to a source, you must release the newly
 * obtained handle by calling pfnReleaseSourceModeSet.
 *
 *  Note  The pfnAssignSourceModeSet function releases or does not release the source
 *  mode set object that is identified by the hVidPnSourceModeSet parameter depending
 *  on the reason that caused pfnAssignSourceModeSet to fail.
 *
 *  pfnAssignSourceModeSet does not release the source mode set object if pfnAssignSourceModeSet
 *  fails with an invalid input parameter (that is, fails with the STATUS_GRAPHICS_INVALID_VIDPN,
 *  STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE, or STATUS_GRAPHICS_INVALID_VIDPN_SOURCEMODESET
 *  error code) because the parameters that were specified were not sufficient for
 *  the operating system to determine which mode set object to release. Such invalid
 *  parameter situations indicate a gross coding error in the driver. You can fix
 *  this error by specifying the correct VidPN handle, source identifier, or VidPN
 *  source mode set handle.
 *
 *  pfnAssignSourceModeSet will release the source mode set object after successfully
 *  validating all of the input parameters if pfnAssignSourceModeSet fails because
 *  of one of the following reasons:
 *
 *  The source mode set is empty.
 *  The source mode set does not contain a mode that is pinned in the previous mode
 *  set, if any.
 *  The source mode set was not created for the source that is identified by VidPnSourceId.
 *
 * The D3DDDI_VIDEO_PRESENT_SOURCE_ID data type is defined in D3dukmdt.h.
 * The D3DKMDT_HVIDPN and D3DKMDT_HVIDPNSOURCEMODESET data types are defined in
 * D3dkmdt.h.
 */
NTSTATUS
LJB_VIDPN_AssignSourceModeSet(
    __in D3DKMDT_HVIDPN                         hVidPn,
    __in CONST D3DDDI_VIDEO_PRESENT_SOURCE_ID   VidPnSourceId,
    __in CONST D3DKMDT_HVIDPNSOURCEMODESET      hVidPnSourceModeSet
    )
{
    LJB_VIDPN * CONST   MyVidPn = (LJB_VIDPN *) hVidPn;
    LJB_ADAPTER * CONST Adapter = MyVidPn->Adapter;
    NTSTATUS            ntStatus;

    DBG_UNREFERENCED_LOCAL_VARIABLE(Adapter);

    /*
     * sanity check. If Magic number doesn't match, don't try to hack.
     */
    if (MyVidPn->MagicBegin != LJB_VINPN_MAGIC)
    {
        KdPrint(("?" __FUNCTION__ ": not my vidpn object?\n"));
        return STATUS_GRAPHICS_INVALID_VIDPN;
    }

    ntStatus = (*MyVidPn->VidPnInterface->pfnAssignSourceModeSet)(
        MyVidPn->hVidPn,
        VidPnSourceId,
        hVidPnSourceModeSet
        );
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__": failed ntStatus(0x%08x)\n", ntStatus));
    }
    return ntStatus;
}

/*
 * Name : LJB_VIDPN_AssignMultiSampleSourceModeSet
 *
 * Description:
 * The pfnAssignMultisamplingMethodSet function assigns a set of multisampling
 * methods to a particular video present source in a specified VidPN.
 *
 *
 * Return Value:
 * The pfnAssignMultisamplingMethodSet function returns one of the following values:
 * STATUS_SUCCESS
 *  The function succeeded.
 * STATUS_GRAPHICS_INVALID_VIDPN
 *  The handle supplied in hVidPn was invalid.
 * STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE
 *  The identifier supplied in VidPnSourceId was invalid.
 * STATUS_NO_MEMORY
 *  The function failed because it was unable to allocate enough memory.
 *
 * Comments:
 * N/A
 */
NTSTATUS
LJB_VIDPN_AssignMultiSampleSourceModeSet(
    __in D3DKMDT_HVIDPN                         hVidPn,
    __in CONST D3DDDI_VIDEO_PRESENT_SOURCE_ID   VidPnSourceId,
    __in CONST SIZE_T                           NumMethods,
    __in CONST D3DDDI_MULTISAMPLINGMETHOD*      pSupportedMethodSet
    )
{
    LJB_VIDPN * CONST   MyVidPn = (LJB_VIDPN *) hVidPn;
    LJB_ADAPTER * CONST Adapter = MyVidPn->Adapter;
    NTSTATUS            ntStatus;

    DBG_UNREFERENCED_LOCAL_VARIABLE(Adapter);

    /*
     * sanity check. If Magic number doesn't match, don't try to hack.
     */
    if (MyVidPn->MagicBegin != LJB_VINPN_MAGIC)
    {
        KdPrint(("?" __FUNCTION__ ": not my vidpn object?\n"));
        return STATUS_GRAPHICS_INVALID_VIDPN;
    }

    ntStatus = (*MyVidPn->VidPnInterface->pfnAssignMultisamplingMethodSet)(
        MyVidPn->hVidPn,
        VidPnSourceId,
        NumMethods,
        pSupportedMethodSet
        );
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__": failed ntStatus(0x%08x)\n", ntStatus));
    }
    return ntStatus;
}

/*
 * Name : LJB_VIDPN_AcquireTargetModeSet
 *
 * Description:
 * The pfnAcquireTargetModeSet function returns a handle to a particular target
 * mode set object that is contained by a specified VidPN object.
 *
 * Return Value:
 * The pfnAcquireSourceModeSet function returns one of the following values:
 * STATUS_SUCCESS
 *  The function succeeded.
 * STATUS_GRAPHICS_INVALID_VIDPN
 *  The handle supplied in hVidPn was invalid.
 * STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE
 *  The identifier supplied in VidPnSourceId was invalid.
 *
 * Comments
 * VidPN target identifiers are assigned by the display miniport driver.
 * DxgkDdiQueryChildRelations, implemented by the display miniport driver, returns
 * an array of DXGK_CHILD_DESCRIPTOR structures, each of which contains an identifier.
 *
 * When you have finished using the target mode set object handle, you must release
 * the handle by calling pfnReleaseTargetModeSet. Target mode set objects are reference
 * counted, so if you acquire a handle several times, you must release it that same
 * number of times.
 *
 * The lifetime of the DXGK_VIDPNTARGETMODESET_INTERFACE structure returned in
 * ppVidPnTargetModeSetInterface is owned by the operating system. Using this
 * ownership scheme, the operating system can switch to newer implementations at
 * run time without breaking clients of the interface.
 *
 * The D3DDDI_VIDEO_PRESENT_TARGET_ID data type is defined in D3dukmdt.h.
 *
 * The D3DKMDT_HVIDPN and D3DKMDT_HVIDPNTARGETMODESET data types are defined in
 * D3dkmdt.h.
 */
NTSTATUS
LJB_VIDPN_AcquireTargetModeSet(
    __in CONST D3DKMDT_HVIDPN                       hVidPn,
    __in CONST D3DDDI_VIDEO_PRESENT_TARGET_ID       VidPnTargetId,
    __out D3DKMDT_HVIDPNTARGETMODESET*              phVidPnTargetModeSet,
    __out CONST DXGK_VIDPNTARGETMODESET_INTERFACE** ppVidPnTargetModeSetInterface
    )
{
    LJB_VIDPN * CONST   MyVidPn = (LJB_VIDPN *) hVidPn;
    LJB_ADAPTER * CONST Adapter = MyVidPn->Adapter;
    NTSTATUS            ntStatus;

    DBG_UNREFERENCED_LOCAL_VARIABLE(Adapter);

    /*
     * sanity check. If Magic number doesn't match, don't try to hack.
     */
    if (MyVidPn->MagicBegin != LJB_VINPN_MAGIC)
    {
        KdPrint(("?" __FUNCTION__ ": not my vidpn object?\n"));
        return STATUS_GRAPHICS_INVALID_VIDPN;
    }

    ntStatus = (*MyVidPn->VidPnInterface->pfnAcquireTargetModeSet)(
        MyVidPn->hVidPn,
        VidPnTargetId,
        phVidPnTargetModeSet,
        ppVidPnTargetModeSetInterface
        );
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__": failed ntStatus(0x%08x)\n", ntStatus));
    }
    return ntStatus;
}

/*
 * Name : LJB_VIDPN_ReleaseTargetModeSet
 *
 * Description:
 * The pfnReleaseTargetModeSet function releases a handle to a target mode set object.
 *
 * Return Value:
 * The pfnReleaseTargetModeSet function returns one of the following values:
 * STATUS_SUCCESS
 *  The function succeeded.
 * STATUS_GRAPHICS_INVALID_VIDPN
 *  The handle supplied in hVidPn was invalid.
 * STATUS_GRAPHICS_INVALID_VIDPN_TARGETMODESET
 *  The handle supplied in hVidPnTargetModeSet was invalid.
 * STATUS_GRAPHICS_RESOURCES_NOT_RELATED
 *  The VidPN identified by hVidPn does not contain the source mode set identified
 *  by hVidPnTargetModeSet.
 *
 * Comments
 * When you have finished using a handle that you obtained by calling pfnAcquireTargetModeSet,
 * you must release the handle by calling pfnReleaseTargetModeSet.
 *
 * If you obtain a handle by calling pfnCreateNewTargetModeSet and then pass that
 * handle to pfnAssignTargetModeSet, you do not need to release the handle.
 *
 * If you obtain a handle by calling pfnCreateNewTargetModeSet and then you decide
 * not to assign the new target mode set to a target, you must release the newly
 * obtained handle by calling pfnReleaseTargetModeSet.
 *
 * The D3DKMDT_HVIDPN and D3DKMDT_HVIDPNTARGETMODESET data types are defined in
 * D3dkmdt.h.
 *
 */
NTSTATUS
LJB_VIDPN_ReleaseTargetModeSet(
    __in CONST D3DKMDT_HVIDPN               hVidPn,
    __in CONST D3DKMDT_HVIDPNTARGETMODESET  hVidPnTargetModeSet
    )
{
    LJB_VIDPN * CONST   MyVidPn = (LJB_VIDPN *) hVidPn;
    LJB_ADAPTER * CONST Adapter = MyVidPn->Adapter;
    NTSTATUS            ntStatus;

    DBG_UNREFERENCED_LOCAL_VARIABLE(Adapter);

    /*
     * sanity check. If Magic number doesn't match, don't try to hack.
     */
    if (MyVidPn->MagicBegin != LJB_VINPN_MAGIC)
    {
        KdPrint(("?" __FUNCTION__ ": not my vidpn object?\n"));
        return STATUS_GRAPHICS_INVALID_VIDPN;
    }

    ntStatus = (*MyVidPn->VidPnInterface->pfnReleaseTargetModeSet)(
        MyVidPn->hVidPn,
        hVidPnTargetModeSet
        );
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__": failed ntStatus(0x%08x)\n", ntStatus));
    }
    return ntStatus;
}

/*
 * Name : LJB_VIDPN_CreateNewTargetModeSet
 *
 * Description:
 * The pfnCreateNewSourceModeSet function creates a new source mode set object
 * within a specified VidPN object.
 *
 * Return Value:
 * The pfnReleaseSourceModeSet  function returns one of the following values:
 * STATUS_SUCCESS
 *  The function succeeded.
 * STATUS_GRAPHICS_INVALID_VIDPN
 *  The handle supplied in hVidPn was invalid.
 * STATUS_NO_MEMORY
 *  The VidPN manager was unable to allocate the memory required to create the new
 *  target mode set object.
 *
 * Comments
 * To assign a new target mode set to a particular target in a VidPN implementation,
 * perform the following steps:
 *
 * 1    Call pfnCreateNewTargetModeSet to get a handle to a new target mode set
 *      object. That target mode set object belongs to a particular VidPN object
 *      that you specify.
 * 2    Use the functions of the DXGK_VIDPNTARGETMODESET_INTERFACE interface to
 *      add modes to the target mode set object.
 * 3    Call pfnAssignTargetModeSet to assign the new target mode set to a particular
 *      target.
 *
 * If you obtain a handle by calling pfnCreateNewTargetModeSet and then pass that
 * handle to pfnAssignTargetModeSet, you do not need to release the handle by calling
 * pfnReleaseTargetModeSet.
 *
 * If you obtain a handle by calling pfnCreateNewTargetModeSet and then you decide
 * not to assign the new target mode set to a target, you must release the newly
 * obtained handle by calling pfnReleaseTargetModeSet.
 *
 * The lifetime of the DXGK_VIDPNTARGETMODESET_INTERFACE structure returned in
 * ppVidPnTargetModeSetInterface is owned by the operating system. Using this ownership
 * scheme, the operating system can switch to newer implementations at run time
 * without breaking clients of the interface.
 *
 * The D3DKMDT_HVIDPN and D3DKMDT_HVIDPNSOURCEMODESET data types are defined in
 * D3dkmdt.h.
 */
NTSTATUS
LJB_VIDPN_CreateNewTargetModeSet(
    __in CONST D3DKMDT_HVIDPN                       hVidPn,
    __in CONST D3DDDI_VIDEO_PRESENT_TARGET_ID       VidPnTargetId,
    __out D3DKMDT_HVIDPNTARGETMODESET*              phNewVidPnTargetModeSet,
    __out CONST DXGK_VIDPNTARGETMODESET_INTERFACE** ppVidPnTargetModeSetInterface
    )
{
    LJB_VIDPN * CONST   MyVidPn = (LJB_VIDPN *) hVidPn;
    LJB_ADAPTER * CONST Adapter = MyVidPn->Adapter;
    NTSTATUS            ntStatus;

    DBG_UNREFERENCED_LOCAL_VARIABLE(Adapter);

    /*
     * sanity check. If Magic number doesn't match, don't try to hack.
     */
    if (MyVidPn->MagicBegin != LJB_VINPN_MAGIC)
    {
        KdPrint(("?" __FUNCTION__ ": not my vidpn object?\n"));
        return STATUS_GRAPHICS_INVALID_VIDPN;
    }

    ntStatus = (*MyVidPn->VidPnInterface->pfnCreateNewTargetModeSet)(
        MyVidPn->hVidPn,
        VidPnTargetId,
        phNewVidPnTargetModeSet,
        ppVidPnTargetModeSetInterface
        );
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__": failed ntStatus(0x%08x)\n", ntStatus));
    }
    return ntStatus;
}

/*
 * Name : LJB_VIDPN_AssignTargetModeSet
 *
 * Description:
 * The pfnAssignTargetModeSet function assigns a target mode set to a particular
 * target in a specified VidPN.
 *
 * Return Value:
 * The pfnAssignTargetModeSet function returns one of the following values:
 * STATUS_SUCCESS
 *  The function succeeded.
 * STATUS_GRAPHICS_INVALID_VIDPN
 *  The handle supplied in hVidPn was invalid.
 * STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_TARGET
 *  The identifier supplied in VidPnTargetId was invalid.
 * STATUS_GRAPHICS_INVALID_VIDPN_TARGETMODESET
 *  The handle supplied in hVidPnTargetModeSet was invalid.
 * STATUS_GRAPHICS_PINNED_MODE_MUST_REMAIN_IN_SET
 *  The target mode set you are attempting to assign does not contain the mode
 *  that was already pinned on the target.
 *
 * Comments
 * VidPN target identifiers are assigned by the display miniport driver.
 * DxgkDdiQueryChildRelations, implemented by the display miniport driver, returns
 * an array of DXGK_CHILD_DESCRIPTOR structures, each of which contains an identifier.
 *
 * If you obtain a handle by calling pfnCreateNewSourceModeSet and then pass that
 * handle to pfnAssignSourceModeSet, you do not need to release the handle by calling
 * pfnReleaseSourceModeSet.
 *
 * If you obtain a handle by calling pfnCreateNewTargetModeSet and then you decide
 * not to assign the new target mode set to a target, you must release the newly
 * obtained handle by calling pfnReleaseTargetModeSet.
 *
 *  Note  The pfnAssignTargetModeSet function releases or does not release the target
 *  mode set object that is identified by the hVidPnTargetModeSet parameter depending
 *  on the reason that caused pfnAssignTargetModeSet to fail.
 *
 *  pfnAssignTargetModeSet does not release the source mode set object if pfnAssignTargetModeSet
 *  fails with an invalid input parameter (that is, fails with the STATUS_GRAPHICS_INVALID_VIDPN,
 *  STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_TARGET, or STATUS_GRAPHICS_INVALID_VIDPN_TARGETMODESET
 *  error code) because the parameters that were specified were not sufficient for
 *  the operating system to determine which mode set object to release. Such invalid
 *  parameter situations indicate a gross coding error in the driver. You can fix
 *  this error by specifying the correct VidPN handle, source identifier, or VidPN
 *  target mode set handle.
 *
 *  pfnAssignTargetModeSet will release the target mode set object after successfully
 *  validating all of the input parameters if pfnAssignTargetModeSet fails because
 *  of one of the following reasons:
 *
 *  The target mode set is empty.
 *  The target mode set does not contain a mode that is pinned in the previous mode
 *  set, if any.
 *  The target mode set was not created for the target that is identified by VidPnTargetId.
 *
 * The D3DDDI_VIDEO_PRESENT_TARGET_ID data type is defined in D3dukmdt.h.
 * The D3DKMDT_HVIDPN and D3DKMDT_HVIDPNTARGETMODESET data types are defined in
 * D3dkmdt.h.
 */
NTSTATUS
LJB_VIDPN_AssignTargetModeSet(
    __in D3DKMDT_HVIDPN                         hVidPn,
    __in CONST D3DDDI_VIDEO_PRESENT_TARGET_ID   VidPnTargetId,
    __in CONST D3DKMDT_HVIDPNTARGETMODESET      hVidPnTargetModeSet
    )
{
    LJB_VIDPN * CONST   MyVidPn = (LJB_VIDPN *) hVidPn;
    LJB_ADAPTER * CONST Adapter = MyVidPn->Adapter;
    NTSTATUS            ntStatus;

    DBG_UNREFERENCED_LOCAL_VARIABLE(Adapter);

    /*
     * sanity check. If Magic number doesn't match, don't try to hack.
     */
    if (MyVidPn->MagicBegin != LJB_VINPN_MAGIC)
    {
        KdPrint(("?" __FUNCTION__ ": not my vidpn object?\n"));
        return STATUS_GRAPHICS_INVALID_VIDPN;
    }

    ntStatus = (*MyVidPn->VidPnInterface->pfnAssignTargetModeSet)(
        MyVidPn->hVidPn,
        VidPnTargetId,
        hVidPnTargetModeSet
        );
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__": failed ntStatus(0x%08x)\n", ntStatus));
    }
    return ntStatus;
}
