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
    NULL, //&LJB_VIDPN_TOPOLOGY_GetNumPaths,
    NULL, //&LJB_VIDPN_TOPOLOGY_GetNumPathsFromSource,
    NULL, //&LJB_VIDPN_TOPOLOGY_EnumPathTargetsFromSource,
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
