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
    NULL, //&LJB_VIDPN_TOPOLOGY_GetPathSourceFromTarget,
    NULL, //&LJB_VIDPN_TOPOLOGY_AcquirePathInfo,
    NULL, //&LJB_VIDPN_TOPOLOGY_AcquireFirstPathInfo,
    NULL, //&LJB_VIDPN_TOPOLOGY_AcquireNextPathInfo,
    NULL, //&LJB_VIDPN_TOPOLOGY_UpdatePathSupportInfo,
    NULL, //&LJB_VIDPN_TOPOLOGY_ReleasePathInfo,
    NULL, //&LJB_VIDPN_TOPOLOGY_CreateNewPathInfo,
    NULL, //&LJB_VIDPN_TOPOLOGY_AddPath,
    NULL, //&LJB_VIDPN_TOPOLOGY_RemovePath
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
