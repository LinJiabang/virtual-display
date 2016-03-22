/*
 * ljb_dxgk_vidpn_topology.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"
#include "ljb_dxgk_vidpn_interface.h"

CONST DXGK_VIDPNTOPOLOGY_INTERFACE  MyTopologyInterface =
{
    &LJB_VIDPN_TOPOLOGY_GetNumPaths,
    &LJB_VIDPN_TOPOLOGY_GetNumPathsFromSource,
    &LJB_VIDPN_TOPOLOGY_EnumPathTargetsFromSource,
    &LJB_VIDPN_TOPOLOGY_GetPathSourceFromTarget,
    &LJB_VIDPN_TOPOLOGY_AcquirePathInfo,
    &LJB_VIDPN_TOPOLOGY_AcquireFirstPathInfo,
    &LJB_VIDPN_TOPOLOGY_AcquireNextPathInfo,
    &LJB_VIDPN_TOPOLOGY_UpdatePathSupportInfo,
    &LJB_VIDPN_TOPOLOGY_ReleasePathInfo,
    &LJB_VIDPN_TOPOLOGY_CreateNewPathInfo,
    &LJB_VIDPN_TOPOLOGY_AddPath,
    &LJB_VIDPN_TOPOLOGY_RemovePath
};

/*
 * Name: LJB_VIDPN_TOPOLOGY_Initialize
 *
 * Description:
 *  Populate all Paths from the given topology.
 *
 * Return Value:
 *  None
 */
VOID
LJB_VIDPN_TOPOLOGY_Initialize(
    __in LJB_ADAPTER *          Adapter,
    __in LJB_VIDPN_TOPOLOGY *   MyTopology
    )
{
    CONST DXGK_VIDPNTOPOLOGY_INTERFACE * CONST VidPnTopologyInterface =
        MyTopology->VidPnTopologyInterface;
    CONST D3DKMDT_VIDPN_PRESENT_PATH *  PrevPath;
    CONST D3DKMDT_VIDPN_PRESENT_PATH *  ThisPath;
    NTSTATUS                            ntStatus;
    ULONG                               i;

    MyTopology->Adapter = Adapter;
    MyTopology->NumPaths = 0;
    ntStatus = (*VidPnTopologyInterface->pfnGetNumPaths)(
        MyTopology->hVidPnTopology,
        &MyTopology->NumPaths
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": "
            " pfnGetNumPaths failed with ntStatus(0x%08x)?\n",
            ntStatus
            ));
        return;
    }

    if (MyTopology->NumPaths == 0)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": "
            "Null topology?\n"
            ));
        return;
    }

    if (MyTopology->pPaths != NULL)
        LJB_PROXYKMD_FreePool(MyTopology->pPaths);
    MyTopology->pPaths = LJB_PROXYKMD_GetPoolZero(
        MyTopology->NumPaths * sizeof(D3DKMDT_VIDPN_PRESENT_PATH)
        );
    if (MyTopology->pPaths == NULL)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": "
            "no pPaths allocated, pretending we have null topology.\n"
            ));
        MyTopology->NumPaths = 0;
        return;
    }

    /*
     * Query each paths.
     */
    PrevPath = NULL;
    ThisPath = NULL;
    for (i = 0; i < MyTopology->NumPaths; i++)
    {
        if (i == 0)
        {
            ntStatus = (*VidPnTopologyInterface->pfnAcquireFirstPathInfo)(
                MyTopology->hVidPnTopology,
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
                MyTopology->hVidPnTopology,
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
                MyTopology->hVidPnTopology,
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
        MyTopology->pPaths[i] = *ThisPath;
    } /* end of for loop */

    /*
     * Release the last queried pPath
     */
    ntStatus =(*VidPnTopologyInterface->pfnReleasePathInfo)(
        MyTopology->hVidPnTopology,
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
}

/*
 * Name: LJB_VIDPN_TOPOLOGY_GetNumPaths
 *
 * Description:
 * The pfnGetNumPaths function returns the number of video present paths in a
 * specified VidPN topology.
 *
 * Return Value
 * The pfnGetNumPaths function returns one of the following values:
 *  STATUS_SUCCESS
 *      The function succeeded.
 *  STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY
 *      The handle supplied in hVidPnTopology was invalid.
 */
NTSTATUS
LJB_VIDPN_TOPOLOGY_GetNumPaths(
	__in CONST D3DKMDT_HVIDPNTOPOLOGY   hVidPnTopology,
    __out SIZE_T*                       pNumPaths
    )
{
    LJB_VIDPN_TOPOLOGY * CONST  MyTopology = (LJB_VIDPN_TOPOLOGY*) hVidPnTopology;
    LJB_ADAPTER * CONST         Adapter = MyTopology->Adapter;
    D3DKMDT_VIDPN_PRESENT_PATH *Path;
    SIZE_T                      NumPaths;
    UINT                        i;

    /*
     * scan MyTopology->pPaths, and filter out paths that connects to USB target
     */
    NumPaths = 0;
    for (i = 0; i < MyTopology->NumPaths; i++)
    {
        Path = MyTopology->pPaths + i;
        if (Path->VidPnTargetId < Adapter->UsbTargetIdBase)
        {
            NumPaths++;
        }
    }
    ASSERT(NumPaths != 0);
    if (NumPaths == 0)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": Check caller stack, avoid calling inbox driver if possible\n"));
    }
    *pNumPaths = NumPaths;
    return STATUS_SUCCESS;
}

/*
 * Name: LJB_VIDPN_TOPOLOGY_GetNumPathsFromSource
 *
 * Description:
 * The pfnGetNumPathsFromSource function returns the number of video present paths
 * that contain a specified video present source.
 *
 * A topology is a collection paths, each of which contains a (source, target) pair.
 * It is possible for a particular source to appear in more than one path. For example,
 * one source can be paired with two distinct targets in the case of a clone view.
 *
 * VidPN source identifiers are assigned by the operating system. DxgkDdiStartDevice,
 * implemented by the display miniport driver, returns the number N of video present
 * sources supported by the display adapter. Then the operating system assigns identifiers 0, 1, 2, ¡K N - 1.
 *
 * The D3DKMDT_HVIDPNTOPOLOGY data type is defined in D3dkmdt.h.
 *
 * The D3DDDI_VIDEO_PRESENT_SOURCE_ID data type is defined in D3dukmdt.h.
 *
 * Return Value
 * The pfnGetNumPathsFromSource function returns one of the following values:
 *  STATUS_SUCCESS
 *  The function succeeded.
 *  STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY
 *  The handle supplied in hVidPnTopology was invalid.
 *  STATUS_INVALID_PARAMETER
 *  The pointer supplied in pNumPathsFromSource was in valid.
 */
NTSTATUS
LJB_VIDPN_TOPOLOGY_GetNumPathsFromSource(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY           hVidPnTopology,
    __in CONST D3DDDI_VIDEO_PRESENT_SOURCE_ID   VidPnSourceId,
    __out SIZE_T*                               pNumPathsFromSource
    )
{
    LJB_VIDPN_TOPOLOGY * CONST  MyTopology = (LJB_VIDPN_TOPOLOGY*) hVidPnTopology;
    LJB_ADAPTER * CONST         Adapter = MyTopology->Adapter;
    D3DKMDT_VIDPN_PRESENT_PATH *Path;
    SIZE_T                      NumPaths;
    UINT                        i;

    if (pNumPathsFromSource == NULL)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__": invalid pNumPathsFromSource\n"));
        return STATUS_INVALID_PARAMETER;
    }

    /*
     * scan MyTopology->pPaths, and filter out paths that connects to USB target
     */
    NumPaths = 0;
    for (i = 0; i < MyTopology->NumPaths; i++)
    {
        Path = MyTopology->pPaths + i;
        if (Path->VidPnSourceId != VidPnSourceId)
            continue;

        if (Path->VidPnTargetId < Adapter->UsbTargetIdBase)
        {
            NumPaths++;
        }
    }
    ASSERT(NumPaths != 0);
    if (NumPaths == 0)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": Check caller stack, avoid calling inbox driver if possible\n"));
    }
    *pNumPathsFromSource = NumPaths;
    return STATUS_SUCCESS;
}

/*
 * Name: LJB_VIDPN_TOPOLOGY_EnumPathTargetsFromSource
 *
 * Description:
 * The pfnEnumPathTargetsFromSource function returns the identifier of one of the
 * video present targets associated with a specified video present source.
 *
 * VidPnPresentPathIndex is not an index into the set of all paths in the topology
 * identified by hVidPnTopology. It is an index into a subset of all the paths
 * in the topology: specifically, the subset of all paths that contain the source
 * identified by VidPnSourceId.
 *
 * To enumerate (in a given topology) all the targets associated with a particular
 * source, perform the following steps.
 *
 *  Call pfnGetNumPathsFromSource to determine the number N of paths that contain
 *  the source of interest. Think of those paths as an indexed set with indices
 *  0, 1, ¡K N - 1.
 *
 *  For each index 0 though N - 1, pass the source identifier and the index to
 *  pfnEnumPathTargetsFromSource.
 *
 * A topology is a collection paths, each of which contains a (source, target)
 * pair. It is possible for a particular source to appear in more than one path.
 * For example, one source can be paired with two distinct targets in the case
 * of a clone view.
 *
 * VidPN source identifiers are assigned by the operating system. DxgkDdiStartDevice,
 * implemented by the display miniport driver, returns the number N of video present
 * sources supported by the display adapter. Then the operating system assigns
 * identifiers 0, 1, 2, ¡K N - 1.
 *
 * VidPN target identifiers are assigned by the display miniport driver.
 * DxgkDdiQueryChildRelations, implemented by the display miniport driver, returns
 * an array of DXGK_CHILD_DESCRIPTOR structures, each of which contains an identifier.
 *
 * Return Value
 * The pfnEnumPathTargetsFromSource function returns one of the following values:
 *
 *  STATUS_SUCCESS
 *  The function succeeded.
 *  STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY
 *  The handle supplied in hVidPnTopology was invalid.
 *  STATUS_INVALID_PARAMETER
 *  The pointer supplied in pVidPnTargetId was in valid.
 */
NTSTATUS
LJB_VIDPN_TOPOLOGY_EnumPathTargetsFromSource(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY           hVidPnTopology,
    __in CONST D3DDDI_VIDEO_PRESENT_SOURCE_ID   VidPnSourceId,
    __in CONST D3DKMDT_VIDPN_PRESENT_PATH_INDEX VidPnPresentPathIndex,
    __out D3DDDI_VIDEO_PRESENT_TARGET_ID*       pVidPnTargetId
    )
{
    LJB_VIDPN_TOPOLOGY * CONST  MyTopology = (LJB_VIDPN_TOPOLOGY*) hVidPnTopology;
    LJB_ADAPTER * CONST         Adapter = MyTopology->Adapter;
    D3DKMDT_VIDPN_PRESENT_PATH *Path;
    SIZE_T                      NumPaths;
    UINT                        i;

    if (pVidPnTargetId == NULL)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,("?" __FUNCTION__": bad pVidPnTargetId\n"));
        return STATUS_INVALID_PARAMETER;
    }

    /*
     * scan MyTopology->pPaths, and filter out paths that connects to USB target
     */
    NumPaths = 0;
    for (i = 0; i < MyTopology->NumPaths; i++)
    {
        Path = MyTopology->pPaths + i;
        if (Path->VidPnSourceId != VidPnSourceId)
            continue;

        if (NumPaths != VidPnPresentPathIndex)
        {
            NumPaths++;
            continue;
        }

        if (Path->VidPnTargetId >= Adapter->UsbTargetIdBase)
        {
            DBG_PRINT(Adapter, DBGLVL_ERROR,
                ("?" __FUNCTION__": invalid path for VidPnSourceId(0x%x)/VidPnPresentPathIndex(0x%x)\n",
                VidPnSourceId,
                VidPnPresentPathIndex
                ));
            return STATUS_GRAPHICS_PATH_NOT_IN_TOPOLOGY;
        }
        *pVidPnTargetId = Path->VidPnTargetId;
        break;
    }
    return STATUS_SUCCESS;
}

/*
 * Name: LJB_VIDPN_TOPOLOGY_GetPathSourceFromTarget
 *
 * Description:
 * The pfnGetPathSourceFromTarget function returns the identifier of the video
 * present source that is associated with a specified video present target.
 *
 * A topology is a collection paths, each of which contains a (source, target)
 * pair. A particular target belongs to at most one path, so given a target ID,
 * there is at most one source associated with that target.
 *
 * VidPN source identifiers are assigned by the operating system. DxgkDdiStartDevice,
 * implemented by the display miniport driver, returns the number N of video present
 * sources supported by the display adapter. Then the operating system assigns
 * identifiers 0, 1, 2, ¡K N - 1.
 *
 * VidPN target identifiers are assigned by the display miniport driver. DxgkDdiQueryChildRelations,
 * implemented by the display miniport driver, returns an array of DXGK_CHILD_DESCRIPTOR
 * structures, each of which contains an identifier.
 *
 * Return Value
 * The pfnEnumPathTargetsFromSource function returns one of the following values:
 *
 *  STATUS_SUCCESS
 *  The function succeeded.
 *  STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY
 *  The handle supplied in hVidPnTopology was invalid.
 *  STATUS_INVALID_PARAMETER
 *  The pointer supplied in pVidPnSourceId  was in valid.
 */
NTSTATUS
LJB_VIDPN_TOPOLOGY_GetPathSourceFromTarget(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY           hVidPnTopology,
    __in CONST D3DDDI_VIDEO_PRESENT_TARGET_ID   VidPnTargetId,
    __out D3DDDI_VIDEO_PRESENT_SOURCE_ID*       pVidPnSourceId
    )
{
    LJB_VIDPN_TOPOLOGY * CONST  MyTopology = (LJB_VIDPN_TOPOLOGY*) hVidPnTopology;
    LJB_ADAPTER * CONST         Adapter = MyTopology->Adapter;
    D3DKMDT_VIDPN_PRESENT_PATH *Path;
    UINT                        i;

    if (pVidPnSourceId == NULL)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,("?" __FUNCTION__": bad pVidPnSourceId\n"));
        return STATUS_INVALID_PARAMETER;
    }

    if (VidPnTargetId >= Adapter->UsbTargetIdBase)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR, ("?" __FUNCTION__": bad VidPnTargetId?\n"));
        return STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY;
    }

    /*
     * scan MyTopology->pPaths, and filter out paths that connects to USB target
     */
    for (i = 0; i < MyTopology->NumPaths; i++)
    {
        Path = MyTopology->pPaths + i;
        if (Path->VidPnTargetId != VidPnTargetId)
            continue;

        *pVidPnSourceId = Path->VidPnSourceId;
        break;
    }
    return STATUS_SUCCESS;
}

/*
 * Name: LJB_VIDPN_TOPOLOGY_AcquirePathInfo
 *
 * Description:
 * The pfnAcquirePathInfo function returns a descriptor of the video present path
 * specified by a video present source and a video present target within a particular
 * VidPN topology.
 *
 * When you have finished using the D3DKMDT_VIDPN_PRESENT_PATH structure, you must
 * release the structure by calling pfnReleasePathInfo.
 *
 * A path contains a (source, target) pair, and a topology is a collection of paths.
 * This function returns a descriptor for the path, in a specified topology, that
 * contains a specified (source, target) pair.
 *
 * You can enumerate all the paths that belong to a VidPN topology object by calling
 * pfnAcquireFirstPathInfo and then making a sequence of calls to pfnAcquireNextPathInfo.
 *
 * VidPN source identifiers are assigned by the operating system. DxgkDdiStartDevice,
 * implemented by the display miniport driver, returns the number N of video present
 * sources supported by the display adapter. Then the operating system assigns
 * identifiers 0, 1, 2, ¡K N - 1.
 *
 * VidPN target identifiers are assigned by the display miniport driver. DxgkDdiQueryChildRelations,
 * implemented by the display miniport driver, returns an array of DXGK_CHILD_DESCRIPTOR
 * structures, each of which contains an identifier.
 *
 * Return Value
 * The pfnEnumPathTargetsFromSource function returns one of the following values:
 *
 *  STATUS_SUCCESS
 *  The function succeeded.
 *  STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY
 *  The handle supplied in hVidPnTopology was invalid.
 */
NTSTATUS
LJB_VIDPN_TOPOLOGY_AcquirePathInfo(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY           hVidPnTopology,
    __in CONST D3DDDI_VIDEO_PRESENT_SOURCE_ID   VidPnSourceId,
    __in CONST D3DDDI_VIDEO_PRESENT_TARGET_ID   VidPnTargetId,
    __out CONST D3DKMDT_VIDPN_PRESENT_PATH**    pVidPnPresentPathInfo
    )
{
    LJB_VIDPN_TOPOLOGY * CONST  MyTopology = (LJB_VIDPN_TOPOLOGY*) hVidPnTopology;
    LJB_ADAPTER * CONST         Adapter = MyTopology->Adapter;
    CONST DXGK_VIDPNTOPOLOGY_INTERFACE * CONST VidPnTopologyInterface = MyTopology->VidPnTopologyInterface;
    NTSTATUS                    ntStatus;

    if (VidPnTargetId >= Adapter->UsbTargetIdBase)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR, ("?" __FUNCTION__": bad VidPnTargetId?\n"));
        return STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY;
    }

    ntStatus = (VidPnTopologyInterface->pfnAcquirePathInfo)(
        MyTopology->hVidPnTopology,
        VidPnSourceId,
        VidPnTargetId,
        pVidPnPresentPathInfo
        );
    return ntStatus;
}

/*
 * Name: LJB_VIDPN_TOPOLOGY_AcquireFirstPathInfo
 *
 * Description:
 * The pfnAcquireFirstPathInfo structure returns a descriptor of the first path
 * in a specified VidPN topology object.
 *
 * When you have finished using the D3DKMDT_VIDPN_PRESENT_PATH structure, you must
 * release the structure by calling pfnReleasePathInfo.
 *
 * You can enumerate all the paths that belong to a VidPN topology object by calling
 * pfnAcquireFirstPathInfo and then making a sequence of calls to pfnAcquireNextPathInfo.
 *
 * Return Value
 * The pfnAcquireFirstPathInfo  function returns one of the following values:
 *
 *  STATUS_SUCCESS
 *  The function succeeded.
 *  STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY
 *  The handle supplied in hVidPnTopology was invalid.
 */
NTSTATUS
LJB_VIDPN_TOPOLOGY_AcquireFirstPathInfo(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY           hVidPnTopology,
    __out CONST D3DKMDT_VIDPN_PRESENT_PATH**    ppFirstVidPnPresentPathInfo
    )
{
    LJB_VIDPN_TOPOLOGY * CONST  MyTopology = (LJB_VIDPN_TOPOLOGY*) hVidPnTopology;
    LJB_ADAPTER * CONST         Adapter = MyTopology->Adapter;
    CONST DXGK_VIDPNTOPOLOGY_INTERFACE * CONST VidPnTopologyInterface = MyTopology->VidPnTopologyInterface;
    D3DKMDT_VIDPN_PRESENT_PATH *Path;
    UINT                        i;
    NTSTATUS                    ntStatus;

    ntStatus = STATUS_SUCCESS;
    for (i = 0; i < MyTopology->NumPaths; i++)
    {
        Path = MyTopology->pPaths + i;
        if (Path->VidPnTargetId < Adapter->UsbTargetIdBase)
        {
            ntStatus = (*VidPnTopologyInterface->pfnAcquirePathInfo)(
                MyTopology->hVidPnTopology,
                Path->VidPnSourceId,
                Path->VidPnTargetId,
                ppFirstVidPnPresentPathInfo
                );
            break;
        }
    }
    ASSERT(i != MyTopology->NumPaths);
    return ntStatus;
}
/*
 * Name: LJB_VIDPN_TOPOLOGY_AcquireNextPathInfo
 *
 * Description:
 * The pfnAcquireNextPathInfo function returns a descriptor of the next video
 * present path in a specified VidPN topology, given the current path.
 *
 * When you have finished using the D3DKMDT_VIDPN_PRESENT_PATH structure, you must
 * release the structure by calling pfnReleasePathInfo.
 *
 * You can enumerate all the paths that belong to a VidPN topology object by calling
 * pfnAcquireFirstPathInfo and then making a sequence of calls to pfnAcquireNextPathInfo.
 *
 * Return Value
 * The pfnAcquireNextPathInfo function returns one of the following values:
 *
 *  STATUS_SUCCESS
 *  The function succeeded.
 *  STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY
 *  The handle supplied in hVidPnTopology was invalid.
 */
NTSTATUS
LJB_VIDPN_TOPOLOGY_AcquireNextPathInfo(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY               hVidPnTopology,
    __in CONST D3DKMDT_VIDPN_PRESENT_PATH* CONST    pVidPnPresentPathInfo,
    __out CONST D3DKMDT_VIDPN_PRESENT_PATH**        ppNextVidPnPresentPathInfo
    )
{
    LJB_VIDPN_TOPOLOGY * CONST      MyTopology = (LJB_VIDPN_TOPOLOGY*) hVidPnTopology;
    LJB_ADAPTER * CONST             Adapter = MyTopology->Adapter;
    CONST DXGK_VIDPNTOPOLOGY_INTERFACE * CONST VidPnTopologyInterface = MyTopology->VidPnTopologyInterface;
    D3DKMDT_VIDPN_PRESENT_PATH *    pPresentPath;
    D3DKMDT_VIDPN_PRESENT_PATH *    pNextPath;
    ULONG                           i;
    NTSTATUS                        ntStatus;

    pPresentPath = NULL;
    pNextPath = NULL;
    for (i = 0; i < MyTopology->NumPaths; i++)
    {
        D3DKMDT_VIDPN_PRESENT_PATH * CONST pPath = MyTopology->pPaths + i;

        if (pPath->VidPnSourceId == pVidPnPresentPathInfo->VidPnSourceId &&
            pPath->VidPnTargetId == pVidPnPresentPathInfo->VidPnTargetId)
        {
            pPresentPath = pPath;
            continue;
        }

        if (pPresentPath == NULL)
            continue;

        if (pPath->VidPnTargetId < Adapter->UsbTargetIdBase)
        {
            pNextPath = pPath;
            break;
        }
    }

    if (pNextPath != NULL)
    {
        ntStatus = (*VidPnTopologyInterface->pfnAcquirePathInfo)(
                MyTopology->hVidPnTopology,
                pNextPath->VidPnSourceId,
                pNextPath->VidPnTargetId,
                ppNextVidPnPresentPathInfo
                );
    }
    else
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": "
            "oops, no pNextPath?\n"
            ));
        ntStatus = STATUS_GRAPHICS_NO_MORE_ELEMENTS_IN_DATASET;
    }
    return ntStatus;
}

/*
 * Name: LJB_VIDPN_TOPOLOGY_UpdatePathSupportInfo
 *
 * Description:
 * The pfnUpdatePathSupportInfo function updates the transformation and copy
 * protection support of a particular path in a specified VidPN topology.
 *
 * The display miniport driver's DxgkDdiEnumVidPnCofuncModality function calls
 * pnfUpdatePathSupportInfo to report rotation, scaling, and copy protection
 * support for each of the paths in a topology.
 *
 * Return Value
 * The pfnUpdatePathSupportInfo function returns one of the following values:
 *
 *  STATUS_SUCCESS
 *  The function succeeded.
 *  STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY
 *  The handle supplied in hVidPnTopology was invalid.
 *  STATUS_INVALID_PARAMETER
 *  An invalid parameter was supplied.
 *  STATUS_ACCESS_DENIED
 *  The path cannot be removed in the context of the current DDI call.
 */
NTSTATUS
LJB_VIDPN_TOPOLOGY_UpdatePathSupportInfo(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY               hVidPnTopology,
    __in CONST D3DKMDT_VIDPN_PRESENT_PATH* CONST    pVidPnPresentPathInfo
    )
{
    LJB_VIDPN_TOPOLOGY * CONST      MyTopology = (LJB_VIDPN_TOPOLOGY*) hVidPnTopology;
    LJB_ADAPTER * CONST             Adapter = MyTopology->Adapter;
    CONST DXGK_VIDPNTOPOLOGY_INTERFACE * CONST VidPnTopologyInterface = MyTopology->VidPnTopologyInterface;
    NTSTATUS                        ntStatus;

    DBG_UNREFERENCED_LOCAL_VARIABLE(Adapter);

    ASSERT(pVidPnPresentPathInfo->VidPnTargetId < Adapter->UsbTargetIdBase);
    ntStatus = (VidPnTopologyInterface->pfnUpdatePathSupportInfo)(
        MyTopology->hVidPnTopology,
        pVidPnPresentPathInfo
        );
    return ntStatus;
}

/*
 * Name: LJB_VIDPN_TOPOLOGY_ReleasePathInfo
 *
 * Description:
 * The pfnReleasePathInfo function releases a D3DKMDT_VIDPN_PRESENT_PATH structure
 * that the VidPN manager previously provided to the display miniport driver.
 *
 * When you have finished using a D3DKMDT_VIDPN_PRESENT_PATH structure that you
 * obtained by calling any of the following functions, you must release the structure
 * by calling pfnReleasePathInfo.
 *
 * pfnAcquireFirstPathInfo
 * pfnAcquireNextPathInfo
 * pfnAcqirePathInfo
 *
 * If you obtain a D3DKMDT_VIDPN_PRESENT_PATH structure by calling pfnCreateNewPathInfo
 * and then pass that structure to pfnAddPath, you do not need to release the
 * structure.
 *
 * If you obtain a handle by calling pfnCreateNewPathInfo and then you decide not
 * to add the new path to a topology, you must release the newly created structire
 * by calling pfnReleasePathInfo.
 *
 * Return Value
 * The pfnReleasePathInfo function returns one of the following values:
 *
 *  STATUS_SUCCESS
 *  The function succeeded.
 *  STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY
 *  The handle supplied in hVidPnTopology was invalid.
 *  STATUS_GRAPHICS_INVALID_VIDPN_PRESENT_PATH
 *  The pointer supplied in pVidPnPresentPathInfo was invalid.
 */
NTSTATUS
LJB_VIDPN_TOPOLOGY_ReleasePathInfo(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY               hVidPnTopology,
    __in CONST D3DKMDT_VIDPN_PRESENT_PATH* CONST    pVidPnPresentPathInfo
    )
{
    LJB_VIDPN_TOPOLOGY * CONST      MyTopology = (LJB_VIDPN_TOPOLOGY*) hVidPnTopology;
    LJB_ADAPTER * CONST             Adapter = MyTopology->Adapter;
    CONST DXGK_VIDPNTOPOLOGY_INTERFACE * CONST VidPnTopologyInterface = MyTopology->VidPnTopologyInterface;
    NTSTATUS                        ntStatus;

    DBG_UNREFERENCED_LOCAL_VARIABLE(Adapter);

    ASSERT(pVidPnPresentPathInfo->VidPnTargetId < Adapter->UsbTargetIdBase);
    ntStatus = (VidPnTopologyInterface->pfnReleasePathInfo)(
        MyTopology->hVidPnTopology,
        pVidPnPresentPathInfo
        );
    return ntStatus;
}

/*
 * Name: LJB_VIDPN_TOPOLOGY_CreateNewPathInfo
 *
 * Description:
 * The pfnCreateNewPathInfo function returns a pointer to a D3DKMDT_VIDPN_PRESENT_PATH
 * structure that the display miniport driver populates before calling pfnAddPath.
 *
 * After you call pfnCreateNewPathInfo to obtain a D3DKMDT_VIDPN_PRESENT_PATH
 * structure, you must do one, but not both, of the following:
 *
 *  Populate the structure and pass it to pfnAddPath.
 *  Release the structure by calling pfnReleasePathInfo.
 *
 * Return Value
 * The pfnReleasePathInfo function returns one of the following values:
 *
 *  STATUS_SUCCESS
 *  The function succeeded.
 *  STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY
 *  The handle supplied in hVidPnTopology was invalid.
 */
NTSTATUS
LJB_VIDPN_TOPOLOGY_CreateNewPathInfo(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY   hVidPnTopology,
    __out D3DKMDT_VIDPN_PRESENT_PATH**  ppNewVidPnPresentPathInfo
    )
{
    LJB_VIDPN_TOPOLOGY * CONST      MyTopology = (LJB_VIDPN_TOPOLOGY*) hVidPnTopology;
    LJB_ADAPTER * CONST             Adapter = MyTopology->Adapter;
    CONST DXGK_VIDPNTOPOLOGY_INTERFACE * CONST VidPnTopologyInterface = MyTopology->VidPnTopologyInterface;
    NTSTATUS                        ntStatus;

    DBG_UNREFERENCED_LOCAL_VARIABLE(Adapter);

    ntStatus = (VidPnTopologyInterface->pfnCreateNewPathInfo)(
        MyTopology->hVidPnTopology,
        ppNewVidPnPresentPathInfo
        );
    return ntStatus;
}

/*
 * Name: LJB_VIDPN_TOPOLOGY_AddPath
 *
 * Description:
 * The pfnAddPath function adds a video present path to a specified VidPN topology
 * object.
 *
 * To add a path to a topology, the display miniport driver performs the following
 * steps.
 *
 *  Call pfnCreateNewPathInfo to obtain a pointer to a D3DKMDT_VIDPN_PRESENT_PATH
 *  structure allocated by the VidPN manager.
 *
 *  Populate the D3DKMDT_VIDPN_PRESENT_PATH structure with information about the
 *  path, including video present source and target identifiers.
 *
 *  Call pfnAddPath to add the path to a topology.
 *
 * The VidPN manager allocates a D3DKMDT_VIDPN_PRESENT_PATH structure when you call
 * pfnCreateNewPathInfo. If you add the path described by that structure to a
 * topology, then you do not need to explicitly release the structure; pfnAddPath
 * releases it.
 *
 * If you obtain a a D3DKMDT_VIDPN_PRESENT_PATH structure by calling pfnCreateNewPathInfo
 * and then decide not to add that path to a topology, then you must explicity
 * release the structure by calling pfnReleasePathInfo.
 *
 * Return Value
 * The pfnReleasePathInfo function returns one of the following values:
 *
 *  STATUS_SUCCESS
 *  The function succeeded.
 *  STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY
 *  The handle supplied in hVidPnTopology was invalid.
 *  STATUS_ACCESS_DENIED
 *  The path cannot be removed in the context of the current DDI call.
 */
NTSTATUS
LJB_VIDPN_TOPOLOGY_AddPath(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY       hVidPnTopology,
    __in D3DKMDT_VIDPN_PRESENT_PATH* CONST  pVidPnPresentPath
    )
{
    LJB_VIDPN_TOPOLOGY * CONST      MyTopology = (LJB_VIDPN_TOPOLOGY*) hVidPnTopology;
    LJB_ADAPTER * CONST             Adapter = MyTopology->Adapter;
    CONST DXGK_VIDPNTOPOLOGY_INTERFACE * CONST VidPnTopologyInterface = MyTopology->VidPnTopologyInterface;
    NTSTATUS                        ntStatus;

    DBG_UNREFERENCED_LOCAL_VARIABLE(Adapter);

    ntStatus = (VidPnTopologyInterface->pfnAddPath)(
        MyTopology->hVidPnTopology,
        pVidPnPresentPath
        );
    return ntStatus;
}

/*
 * Name: LJB_VIDPN_TOPOLOGY_RemovePath
 *
 * Description:
 * The pfnRemovePath function removes a video present path to a specified VidPN
 * topology object
 *
 * Return Value
 * The pfnRemovePath function returns one of the following values.
 *
 *  STATUS_SUCCESS
 *  The specified video present path has been successfully removed from this VidPN
 *  topology object.
 *  STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_SOURCE
 *  The VidPN source identifier supplied in VidPnSourceId is invalid.
 *  STATUS_GRAPHICS_INVALID_VIDEO_PRESENT_TARGET
 *  The VidPN target identifier supplied in VidPnTargetId is invalid.
 *  STATUS_GRAPHICS_INVALID_VIDPN_TOPOLOGY
 *  The handle supplied in hVidPnTopology is invalid.
 *  STATUS_ACCESS_DENIED
 *  The path cannot be removed in the context of the current DDI call.
 */
NTSTATUS
LJB_VIDPN_TOPOLOGY_RemovePath(
    __in CONST D3DKMDT_HVIDPNTOPOLOGY           hVidPnTopology,
    __in CONST D3DDDI_VIDEO_PRESENT_SOURCE_ID   VidPnSourceId,
    __in CONST D3DDDI_VIDEO_PRESENT_TARGET_ID   VidPnTargetId
    )
{
    LJB_VIDPN_TOPOLOGY * CONST      MyTopology = (LJB_VIDPN_TOPOLOGY*) hVidPnTopology;
    LJB_ADAPTER * CONST             Adapter = MyTopology->Adapter;
    CONST DXGK_VIDPNTOPOLOGY_INTERFACE * CONST VidPnTopologyInterface = MyTopology->VidPnTopologyInterface;
    NTSTATUS                        ntStatus;

    DBG_UNREFERENCED_LOCAL_VARIABLE(Adapter);
    ASSERT(VidPnTargetId < Adapter->UsbTargetIdBase);

    ntStatus = (VidPnTopologyInterface->pfnRemovePath)(
        MyTopology->hVidPnTopology,
        VidPnSourceId,
        VidPnTargetId
        );
    return ntStatus;
}
