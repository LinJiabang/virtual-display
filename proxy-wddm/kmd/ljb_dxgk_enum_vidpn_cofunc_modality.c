/*
 * ljb_dxgk_enum_vidpn_cofunc_modality.c
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is NOT free software. Any unlicensed usage is prohbited.
 */
#include "ljb_proxykmd.h"
#include "ljb_dxgk_vidpn_interface.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_DXGK_EnumVidPnCofuncModality)
#endif

static
NTSTATUS
LJB_DXGK_EnumVidPnCofuncModalityPostProcessing(
    __in LJB_ADAPTER *                              Adapter,
    __in LJB_VIDPN *                                MyVidPn,
    IN_CONST_PDXGKARG_ENUMVIDPNCOFUNCMODALITY_CONST pEnumCofuncModality
    );

static
NTSTATUS
LJB_AcquireSourceModes(
    __in LJB_ADAPTER *                              Adapter,
    __in D3DKMDT_HVIDPNSOURCEMODESET                hVidPnSourceModeSet,
    __in CONST DXGK_VIDPNSOURCEMODESET_INTERFACE*   pVidPnSourceModeSetInterface,
    __out D3DKMDT_VIDPN_SOURCE_MODE **              ppSourceModes,
    __out SIZE_T *                                  pNumSourceModes
    );

static
VOID
LJB_PopulateSourceModeSetFromMonitor(
    __in LJB_VIDPN *                                MyVidPn,
    __in CONST DXGK_VIDPNSOURCEMODESET_INTERFACE*   pVidPnSourceModeSetInterface,
    __in D3DKMDT_HVIDPNSOURCEMODESET                hOldVidPnSourceModeSet,
    __in D3DKMDT_HVIDPNSOURCEMODESET                hNewVidPnSourceModeSet,
    __in D3DDDI_VIDEO_PRESENT_SOURCE_ID             VidPnSourceId,
    __in LJB_MONITOR_NODE *                         MonitorNode
    );

static
VOID
LJB_PopulateTargetModeSetFromMonitor(
    __in LJB_VIDPN *                                MyVidPn,
    __in CONST DXGK_VIDPNSOURCEMODESET_INTERFACE*   pVidPnSourceModeSetInterface,
    __in D3DKMDT_HVIDPNSOURCEMODESET                hOldVidPnSourceModeSet,
    __in CONST DXGK_VIDPNTARGETMODESET_INTERFACE*   pVidPnTargetModeSetInterface,
    __in D3DKMDT_HVIDPNTARGETMODESET                hVidPnTargetModeSet,
    __in D3DDDI_VIDEO_PRESENT_SOURCE_ID             VidPnSourceId,
    __in LJB_MONITOR_NODE *                         MonitorNode
    );

/*
 * Function: LJB_DXGK_EnumVidPnCofuncModality
 *
 * Description:
 * The DxgkDdiEnumVidPnCofuncModality function makes the source and target modes
 * sets of a VidPN cofunctional with the VidPN's topology and the modes that have
 * already been pinned.
 *
 * Return Value:
 * DxgkDdiEnumVidPnCofuncModality returns STATUS_SUCCESS if it succeeds;
 * otherwise, it returns one of the error codes defined in Ntstatus.h.
 *
 * Remarks:
 * The hConstrainingVidPn member of pEnumCofuncModalityArg is a handle to a VidPN
 * object called the constraining VidPN. Other members of pEnumCofuncModalityArg
 * identify one video present source or target as the pivot of the enumeration
 * (or specify that there is no pivot).
 *
 * DxgkDdiEnumVidPnCofuncModality must perform the following tasks:
 * Examine the topology and mode sets of the constraining VidPN.
 * Update each mode set that is not the pivot and does not already have a pinned
 * mode. The updated mode sets must be cofunctional with the VidPN's topology and
 * with any modes that have already been pinned.
 *
 * Note that if a source or target is identified as the pivot of the enumeration,
 * the mode set for that source or target must not change. For more information
 * about how to update source and target mode sets, see Enumerating Cofunctional
 * VidPN Source and Target Modes.
 *
 * The DxgkDdiEnumVidPnCofuncModality function should be made pageable.
 */
NTSTATUS
LJB_DXGK_EnumVidPnCofuncModality(
    IN_CONST_HANDLE                                     hAdapter,
    IN_CONST_PDXGKARG_ENUMVIDPNCOFUNCMODALITY_CONST     pEnumCofuncModality
    )
{
    LJB_ADAPTER * CONST                 Adapter = FIND_ADAPTER_BY_DRIVER_ADAPTER(hAdapter);
    LJB_CLIENT_DRIVER_DATA * CONST      ClientDriverData = Adapter->ClientDriverData;
    DRIVER_INITIALIZATION_DATA * CONST  DriverInitData = &ClientDriverData->DriverInitData;
    NTSTATUS                            ntStatus;
    LJB_VIDPN *                         MyVidPn;
    DXGKARG_ENUMVIDPNCOFUNCMODALITY     MyEnumVidPnCoFuncModality;
    UINT                                NumOfInboxTarget;
    UINT                                NumOfUsbTarget;

    PAGED_CODE();

    MyVidPn = LJB_VIDPN_CreateVidPn(Adapter, pEnumCofuncModality->hConstrainingVidPn);
    if (MyVidPn == NULL)
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?"__FUNCTION__": no MyVidPn allocated.\n"));
        return STATUS_NO_MEMORY;
    }

    ntStatus = STATUS_SUCCESS;

    /*
     * decide if we need to pass the call to inbox driver.
     * if there are inbox target attached, we need to pass the call to inbox driver.
     * If there are usb target attached, we need to process the call.
     * If this is NULL topology, let inbox driver handle it.
     */
    NumOfInboxTarget = LJB_VIDPN_GetNumberOfInboxTarget(MyVidPn);
    NumOfUsbTarget = LJB_VIDPN_GetNumberOfUsbTarget(MyVidPn);
    if (NumOfInboxTarget != 0 || NumOfUsbTarget == 0)
    {
        MyEnumVidPnCoFuncModality = *pEnumCofuncModality;
        MyEnumVidPnCoFuncModality.hConstrainingVidPn = (D3DKMDT_HVIDPN) MyVidPn;
        ntStatus = (*DriverInitData->DxgkDdiEnumVidPnCofuncModality)(
            hAdapter,
            &MyEnumVidPnCoFuncModality
            );
        if (!NT_SUCCESS(ntStatus))
        {
            DBG_PRINT(Adapter, DBGLVL_ERROR,
                ("?" __FUNCTION__ ": failed with 0x%08x\n", ntStatus));
            goto Exit;
        }
    }

    if (NumOfUsbTarget != 0)
    {
        ntStatus = LJB_DXGK_EnumVidPnCofuncModalityPostProcessing(
            Adapter, MyVidPn, pEnumCofuncModality);
        if (!NT_SUCCESS(ntStatus))
        {
            DBG_PRINT(Adapter, DBGLVL_ERROR,
                ("?" __FUNCTION__": LJB_DXGK_EnumVidPnCofuncModalityPostProcessing failed?\n"));
        }
    }

Exit:
    LJB_VIDPN_DestroyVidPn(MyVidPn);

    return ntStatus;
}

static
NTSTATUS
LJB_DXGK_EnumVidPnCofuncModalityPostProcessing(
    __in LJB_ADAPTER *                              Adapter,
    __in LJB_VIDPN *                                MyVidPn,
    IN_CONST_PDXGKARG_ENUMVIDPNCOFUNCMODALITY_CONST pEnumCofuncModality
    )
{
    DXGK_VIDPN_INTERFACE* CONST                 VidPnInterface = MyVidPn->VidPnInterface;
    D3DKMDT_HVIDPNTOPOLOGY CONST                hVidPnTopology = MyVidPn->Topology.hVidPnTopology;
    CONST DXGK_VIDPNTOPOLOGY_INTERFACE* CONST   VidPnTopologyInterface = MyVidPn->Topology.VidPnTopologyInterface;
    D3DKMDT_HVIDPNSOURCEMODESET                 hVidPnSourceModeSet = NULL;
    D3DKMDT_HVIDPNSOURCEMODESET                 hNewVidPnSourceModeSet = NULL;
    D3DKMDT_HVIDPNTARGETMODESET                 hVidPnTargetModeSet = NULL;
    CONST DXGK_VIDPNSOURCEMODESET_INTERFACE*    pVidPnSourceModeSetInterface = NULL;
    CONST DXGK_VIDPNTARGETMODESET_INTERFACE*    pVidPnTargetModeSetInterface = NULL;
    CONST D3DKMDT_VIDPN_SOURCE_MODE*            pVidPnPinnedSourceModeInfo = NULL;
    CONST D3DKMDT_VIDPN_TARGET_MODE*            pVidPnPinnedTargetModeInfo = NULL;
    LJB_MONITOR_NODE *                          MonitorNode;
    NTSTATUS                                    ntStatus;
    NTSTATUS                                    TempStatus;
    UINT                                        i;

    ntStatus = STATUS_SUCCESS;
    for (i = 0; i < MyVidPn->NumPaths; i++)
    {
        D3DKMDT_VIDPN_PRESENT_PATH * CONST ThisPath = MyVidPn->Paths + i;

        if (ThisPath->VidPnTargetId < Adapter->UsbTargetIdBase)
            continue;

        // Get the Source Mode Set interface so the pinned mode can be retrieved
        ntStatus = (*VidPnInterface->pfnAcquireSourceModeSet)(
            pEnumCofuncModality->hConstrainingVidPn,
            ThisPath->VidPnSourceId,
            &hVidPnSourceModeSet,
            &pVidPnSourceModeSetInterface
            );

        if (!NT_SUCCESS(ntStatus))
        {
            DBG_PRINT(Adapter, DBGLVL_ERROR,
                ("?"__FUNCTION__": pfnAcquireSourceModeSet failed with ntStatus(0x%08x)\n",
                ntStatus));
            break;
        }

        // Get the pinned mode, needed when VidPnSource isn't pivot, and when VidPnTarget isn't pivot
        ntStatus = (*pVidPnSourceModeSetInterface->pfnAcquirePinnedModeInfo)(
            hVidPnSourceModeSet,
            &pVidPnPinnedSourceModeInfo
            );

        // SOURCE MODES: If this source mode isn't the pivot point, do work on the source mode set
        if (!((pEnumCofuncModality->EnumPivotType == D3DKMDT_EPT_VIDPNSOURCE) &&
              (pEnumCofuncModality->EnumPivot.VidPnSourceId == ThisPath->VidPnSourceId)))
        {
            // If there's no pinned source add possible modes (otherwise they've already been added)
            if (pVidPnPinnedSourceModeInfo == NULL)
            {
                // Create a new source mode set which will be added to the constraining VidPn with all the possible modes
                ntStatus = (*VidPnInterface->pfnCreateNewSourceModeSet)(
                    pEnumCofuncModality->hConstrainingVidPn,
                    ThisPath->VidPnSourceId,
                    &hNewVidPnSourceModeSet,
                    &pVidPnSourceModeSetInterface
                    );
                if (!NT_SUCCESS(ntStatus))
                {
                    DBG_PRINT(Adapter, DBGLVL_ERROR,
                        ("?" __FUNCTION__
                        ":pfnCreateNewSourceModeSet failed with ntStatus(0x%08x)\n",
                        ntStatus
                        ));
                    break;
                }

                MonitorNode = LJB_GetMonitorNodeFromChildUid(Adapter, ThisPath->VidPnTargetId);
                LJB_PopulateSourceModeSetFromMonitor(
                    MyVidPn,
                    pVidPnSourceModeSetInterface,
                    hVidPnSourceModeSet,
                    hNewVidPnSourceModeSet,
                    ThisPath->VidPnSourceId,
                    MonitorNode
                    );
                LJB_DereferenceMonitorNode(MonitorNode);

                if (!NT_SUCCESS(ntStatus))
                {
                    break;
                }

                // Give DMM back the source modes just populated
                ntStatus = (*VidPnInterface->pfnAssignSourceModeSet)(
                    pEnumCofuncModality->hConstrainingVidPn,
                    ThisPath->VidPnSourceId,
                    hNewVidPnSourceModeSet
                    );
                if (!NT_SUCCESS(ntStatus))
                {
                    DBG_PRINT(Adapter, DBGLVL_ERROR,
                        ("?"__FUNCTION__":pfnAssignSourceModeSet failed with ntStatus(0x%08x)\n",
                        ntStatus));
                    break;
                }
            }
            else
            {
                // Release the pinned source mode as there's no other work to do
                ntStatus = (*pVidPnSourceModeSetInterface->pfnReleaseModeInfo)(hVidPnSourceModeSet, pVidPnPinnedSourceModeInfo);
                if (!NT_SUCCESS(ntStatus))
                {
                    DBG_PRINT(Adapter, DBGLVL_ERROR,
                        ("?"__FUNCTION__":pfnReleaseModeInfo failed with ntStatus(0x%08x)\n",
                        ntStatus
                        ));
                    break;
                }
                pVidPnPinnedSourceModeInfo = NULL; // Successfully released it
            }
        }

        // TARGET MODES: If this target mode isn't the pivot point, do work on the target mode set
        if (!((pEnumCofuncModality->EnumPivotType == D3DKMDT_EPT_VIDPNTARGET) &&
              (pEnumCofuncModality->EnumPivot.VidPnTargetId == ThisPath->VidPnTargetId)))
        {
            // Get the Target Mode Set interface so modes can be added if necessary
            ntStatus = (*VidPnInterface->pfnAcquireTargetModeSet)(
                pEnumCofuncModality->hConstrainingVidPn,
                ThisPath->VidPnTargetId,
                &hVidPnTargetModeSet,
                &pVidPnTargetModeSetInterface
                );
            if (!NT_SUCCESS(ntStatus))
            {
                DBG_PRINT(Adapter, DBGLVL_ERROR,
                    ("?"__FUNCTION__": pfnAcquireTargetModeSet failed with ntStatus(0x%08x)\n",
                    ntStatus
                    ));
                break;
            }

            ntStatus = (*pVidPnTargetModeSetInterface->pfnAcquirePinnedModeInfo)(
                hVidPnTargetModeSet,
                &pVidPnPinnedTargetModeInfo);
            if (!NT_SUCCESS(ntStatus))
            {
                DBG_PRINT(Adapter, DBGLVL_ERROR,
                    ("?"__FUNCTION__":pfnAcquirePinnedModeInfo failed with ntStatus(0x%08x)\n",
                    ntStatus
                    ));
                break;
            }

            // If there's no pinned target add possible modes (otherwise they've already been added)
            if (pVidPnPinnedTargetModeInfo == NULL)
            {
                // Release the acquired target mode set, since going to create a new one to put all modes in
                ntStatus = (*VidPnInterface->pfnReleaseTargetModeSet)(pEnumCofuncModality->hConstrainingVidPn, hVidPnTargetModeSet);
                if (!NT_SUCCESS(ntStatus))
                {
                    DBG_PRINT(Adapter, DBGLVL_ERROR,
                        ("?"__FUNCTION__":pfnReleaseTargetModeSet failed with ntStatus(0x%08x)\n",
                        ntStatus
                        ));
                    break;
                }
                hVidPnTargetModeSet = NULL; // Successfully released it

                // Create a new target mode set which will be added to the constraining VidPn with all the possible modes
                ntStatus = (*VidPnInterface->pfnCreateNewTargetModeSet)(
                    pEnumCofuncModality->hConstrainingVidPn,
                    ThisPath->VidPnTargetId,
                    &hVidPnTargetModeSet,
                    &pVidPnTargetModeSetInterface);
                if (!NT_SUCCESS(ntStatus))
                {
                    DBG_PRINT(Adapter, DBGLVL_ERROR,
                        ("?"__FUNCTION__": pfnCreateNewTargetModeSet failed with ntStatus(0x%08x)",
                        ntStatus
                        ));
                    break;
                }

                MonitorNode = LJB_GetMonitorNodeFromChildUid(Adapter, ThisPath->VidPnTargetId);
                LJB_PopulateTargetModeSetFromMonitor(
                    MyVidPn,
                    pVidPnSourceModeSetInterface,
                    hVidPnSourceModeSet,
                    pVidPnTargetModeSetInterface,
                    hVidPnTargetModeSet,
                    ThisPath->VidPnSourceId,
                    MonitorNode
                    );
                LJB_DereferenceMonitorNode(MonitorNode);

                if (!NT_SUCCESS(ntStatus))
                {
                    break;
                }

                // Give DMM back the source modes just populated
                ntStatus = (*VidPnInterface->pfnAssignTargetModeSet)(
                    pEnumCofuncModality->hConstrainingVidPn,
                    ThisPath->VidPnTargetId,
                    hVidPnTargetModeSet
                    );
                if (!NT_SUCCESS(ntStatus))
                {
                    DBG_PRINT(Adapter, DBGLVL_ERROR,
                        ("?"__FUNCTION__":pfnAssignTargetModeSet failed with ntStatus(0x%08x)\n",
                        ntStatus));
                    break;
                }
                hVidPnTargetModeSet = NULL; // Successfully assigned it (equivalent to releasing it)
            }
            else
            {
                // Release the pinned target as there's no other work to do
                ntStatus = (*pVidPnTargetModeSetInterface->pfnReleaseModeInfo)(hVidPnTargetModeSet, pVidPnPinnedTargetModeInfo);
                if (!NT_SUCCESS(ntStatus))
                {
                    DBG_PRINT(Adapter, DBGLVL_ERROR,
                        ("?"__FUNCTION__":pfnReleaseModeInfo failed with ntStatus(0x%08x)\n",
                        ntStatus));
                    break;
                }
                pVidPnPinnedTargetModeInfo = NULL; // Successfully released it

                // Release the acquired target mode set, since it is no longer needed
                ntStatus = (*VidPnInterface->pfnReleaseTargetModeSet)(pEnumCofuncModality->hConstrainingVidPn, hVidPnTargetModeSet);
                if (!NT_SUCCESS(ntStatus))
                {
                    DBG_PRINT(Adapter, DBGLVL_ERROR,
                        ("?"__FUNCTION__":pfnReleaseTargetModeSet failed with ntStatus(0x%08x)\n",
                        ntStatus));
                    break;
                }
                hVidPnTargetModeSet = 0; // Successfully released it
            }
        }// End: TARGET MODES

        /*
         * Update Rotation Support
         */
        ThisPath->ContentTransformation.RotationSupport.Identity   = 1;
        ThisPath->ContentTransformation.RotationSupport.Rotate90   = 1;
        ThisPath->ContentTransformation.RotationSupport.Rotate180  = 1;
        ThisPath->ContentTransformation.RotationSupport.Rotate270  = 1;
        ntStatus = (*VidPnTopologyInterface->pfnUpdatePathSupportInfo)(
            hVidPnTopology,
            ThisPath
            );
        if (!NT_SUCCESS(ntStatus))
        {
            DBG_PRINT(Adapter, DBGLVL_ERROR,
                ("?" __FUNCTION__ ": "
                "pfnUpdatePathSupportInfo failed with ntStatus(0x%08x)\n",
                ntStatus
                ));
        }

        /*
        * Release any resources
        */
        if ((pVidPnSourceModeSetInterface != NULL) &&
            (pVidPnPinnedSourceModeInfo != NULL))
        {
            TempStatus = (*pVidPnSourceModeSetInterface->pfnReleaseModeInfo)(
                hVidPnSourceModeSet,
                pVidPnPinnedSourceModeInfo
                );
            ASSERT(NT_SUCCESS(TempStatus));
        }

        if ((pVidPnTargetModeSetInterface != NULL) && (pVidPnPinnedTargetModeInfo != NULL))
        {
            TempStatus = (*pVidPnTargetModeSetInterface->pfnReleaseModeInfo)(
                hVidPnTargetModeSet,
                pVidPnPinnedTargetModeInfo
                );
            ASSERT(NT_SUCCESS(TempStatus));
        }

        if (hVidPnSourceModeSet != 0)
        {
            TempStatus = (*VidPnInterface->pfnReleaseSourceModeSet)(
                pEnumCofuncModality->hConstrainingVidPn,
                hVidPnSourceModeSet
                );
            ASSERT(NT_SUCCESS(TempStatus));
        }

        if (hVidPnTargetModeSet != 0)
        {
            TempStatus = (*VidPnInterface->pfnReleaseTargetModeSet)(
                pEnumCofuncModality->hConstrainingVidPn,
                hVidPnTargetModeSet
                );
            ASSERT(NT_SUCCESS(TempStatus));
        }
    }

    return ntStatus;
}

static
VOID
LJB_PopulateSourceModeSetFromMonitor(
    __in LJB_VIDPN *                                MyVidPn,
    __in CONST DXGK_VIDPNSOURCEMODESET_INTERFACE*   pVidPnSourceModeSetInterface,
    __in D3DKMDT_HVIDPNSOURCEMODESET                hOldVidPnSourceModeSet,
    __in D3DKMDT_HVIDPNSOURCEMODESET                hNewVidPnSourceModeSet,
    __in D3DDDI_VIDEO_PRESENT_SOURCE_ID             VidPnSourceId,
    __in LJB_MONITOR_NODE *                         MonitorNode
    )
{
    LJB_ADAPTER* CONST          Adapter = MyVidPn->Adapter;
    D3DKMDT_VIDPN_SOURCE_MODE * OldSourceModes;
    SIZE_T                      NumOldSourceModes;
    UINT                        NumInboxTargetFromSource;
    UINT                        i;
    NTSTATUS                    ntStatus;

    NumInboxTargetFromSource = LJB_VIDPN_GetNumberOfInboxTargetFromSource(MyVidPn, VidPnSourceId);
    ntStatus = LJB_AcquireSourceModes(
        Adapter,
        hOldVidPnSourceModeSet,
        pVidPnSourceModeSetInterface,
        &OldSourceModes,
        &NumOldSourceModes
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": "
            " LJB_AcquireSourceModes failed with ntStatus(0x%08x)?\n",
            ntStatus
            ));
        return;
    }

    for (i = 0; i < MonitorNode->NumModes; i++)
    {
        D3DKMDT_VIDPN_SOURCE_MODE *     SourceMode;
        D3DKMDT_MONITOR_SOURCE_MODE*    MonitorMode;

        MonitorMode = &MonitorNode->MonitorModes[i];

        /*
         * check for duplicate display case. If the VidPnSourceId is cloned to
         * both Inbox and USB target, make sure we enumerate modes that are previously
         * enumerated in the old source mode
         */
        if (NumInboxTargetFromSource != 0 && OldSourceModes != NULL && NumOldSourceModes != 0)
        {
            UINT    j;
            BOOLEAN SkipAddThisMode;

            SkipAddThisMode = TRUE;
            for (j = 0; j < NumOldSourceModes; j++)
            {
                if ((OldSourceModes[j].Format.Graphics.PrimSurfSize.cx ==
                     MonitorMode->VideoSignalInfo.ActiveSize.cx) &&
                    (OldSourceModes[j].Format.Graphics.PrimSurfSize.cy ==
                     MonitorMode->VideoSignalInfo.ActiveSize.cy))
                {
                    SkipAddThisMode = FALSE;
                    break;
                }
            }

            if (SkipAddThisMode)
            {
                DBG_PRINT(Adapter, DBGLVL_VIDPN,
                    (__FUNCTION__": Skipped adding this mode(%u,%u) to source mode set\n",
                    MonitorMode->VideoSignalInfo.ActiveSize.cx,
                    MonitorMode->VideoSignalInfo.ActiveSize.cy
                    ));
                break;
            }
        }

        ntStatus = (*pVidPnSourceModeSetInterface->pfnCreateNewModeInfo)(
            hNewVidPnSourceModeSet,
            &SourceMode
            );
        if (!NT_SUCCESS(ntStatus))
            break;

        SourceMode->Type = D3DKMDT_RMT_GRAPHICS;
        SourceMode->Format.Graphics.PrimSurfSize = MonitorMode->VideoSignalInfo.ActiveSize;
        SourceMode->Format.Graphics.VisibleRegionSize = MonitorMode->VideoSignalInfo.ActiveSize;
        SourceMode->Format.Graphics.Stride = MonitorMode->VideoSignalInfo.ActiveSize.cx << 2;
        SourceMode->Format.Graphics.PixelFormat = D3DDDIFMT_A8R8G8B8;
        SourceMode->Format.Graphics.ColorBasis = MonitorMode->ColorBasis;
        SourceMode->Format.Graphics.PixelValueAccessMode = D3DKMDT_PVAM_DIRECT;

        ntStatus = (*pVidPnSourceModeSetInterface->pfnAddMode)(
            hNewVidPnSourceModeSet,
            SourceMode
            );
        if (!NT_SUCCESS(ntStatus))
            break;
    }

    if (OldSourceModes != NULL)
    {
        LJB_FreePool(OldSourceModes);
    }
}

static
VOID
LJB_PopulateTargetModeSetFromMonitor(
    __in LJB_VIDPN *                                MyVidPn,
    __in CONST DXGK_VIDPNSOURCEMODESET_INTERFACE*   pVidPnSourceModeSetInterface,
    __in D3DKMDT_HVIDPNSOURCEMODESET                hOldVidPnSourceModeSet,
    __in CONST DXGK_VIDPNTARGETMODESET_INTERFACE*   pVidPnTargetModeSetInterface,
    __in D3DKMDT_HVIDPNTARGETMODESET                hVidPnTargetModeSet,
    __in D3DDDI_VIDEO_PRESENT_SOURCE_ID             VidPnSourceId,
    __in LJB_MONITOR_NODE *                         MonitorNode
    )
{
    LJB_ADAPTER* CONST          Adapter = MyVidPn->Adapter;
    D3DKMDT_VIDPN_SOURCE_MODE * OldSourceModes;
    SIZE_T                      NumOldSourceModes;
    UINT                        i;
    NTSTATUS                    ntStatus;

    UNREFERENCED_PARAMETER(VidPnSourceId);

    ntStatus = LJB_AcquireSourceModes(
        Adapter,
        hOldVidPnSourceModeSet,
        pVidPnSourceModeSetInterface,
        &OldSourceModes,
        &NumOldSourceModes
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": "
            " LJB_AcquireSourceModes failed with ntStatus(0x%08x)?\n",
            ntStatus
            ));
        return;
    }

    for (i = 0; i < MonitorNode->NumModes; i++)
    {
        D3DKMDT_VIDPN_TARGET_MODE *     TargetMode;
        D3DKMDT_MONITOR_SOURCE_MODE*    MonitorMode;

        MonitorMode = &MonitorNode->MonitorModes[i];

        /*
         * check if we are enumerating target modes for a clone view case.
         * For clone view, the inbox target has already populated source mode set.
         * We only populate target mode that are compatible with the old source mode
         * set
         */
        if (NumOldSourceModes != 0 && OldSourceModes != NULL)
        {
            BOOLEAN SkipAddThisMode;
            UINT    j;

            SkipAddThisMode = TRUE;
            for (j = 0; j < NumOldSourceModes; j++)
            {
                if ((OldSourceModes[j].Format.Graphics.PrimSurfSize.cx ==
                     MonitorMode->VideoSignalInfo.ActiveSize.cx) &&
                    (OldSourceModes[j].Format.Graphics.PrimSurfSize.cy ==
                     MonitorMode->VideoSignalInfo.ActiveSize.cy))
                {
                    SkipAddThisMode = FALSE;
                    break;
                }
            }

            if (SkipAddThisMode)
            {
                DBG_PRINT(Adapter, DBGLVL_VIDPN,
                    (__FUNCTION__": Skipped adding this mode(%u,%u) to source mode set\n",
                    MonitorMode->VideoSignalInfo.ActiveSize.cx,
                    MonitorMode->VideoSignalInfo.ActiveSize.cy
                    ));
                break;
            }
        }

        ntStatus = (*pVidPnTargetModeSetInterface->pfnCreateNewModeInfo)(
            hVidPnTargetModeSet,
            &TargetMode
            );
        if (!NT_SUCCESS(ntStatus))
            break;

        TargetMode->VideoSignalInfo = MonitorMode->VideoSignalInfo;
        TargetMode->Preference = MonitorMode->Preference;
        ntStatus = (*pVidPnTargetModeSetInterface->pfnAddMode)(
            hVidPnTargetModeSet,
            TargetMode
            );
        if (!NT_SUCCESS(ntStatus))
            break;
    }

    if (OldSourceModes != NULL)
    {
        LJB_FreePool(OldSourceModes);
    }
}

static
NTSTATUS
LJB_AcquireSourceModes(
    __in LJB_ADAPTER *                              Adapter,
    __in D3DKMDT_HVIDPNSOURCEMODESET                hVidPnSourceModeSet,
    __in CONST DXGK_VIDPNSOURCEMODESET_INTERFACE*   pVidPnSourceModeSetInterface,
    __out D3DKMDT_VIDPN_SOURCE_MODE **              ppSourceModes,
    __out SIZE_T *                                  pNumSourceModes
    )
{
    D3DKMDT_VIDPN_SOURCE_MODE *         pSourceModes;
    CONST D3DKMDT_VIDPN_SOURCE_MODE *   pThisSourceMode;
    CONST D3DKMDT_VIDPN_SOURCE_MODE *   pPrevSourceMode;
    NTSTATUS                            ntStatus;
    UINT                                i;

    UNREFERENCED_PARAMETER(Adapter);

    *ppSourceModes = NULL;
    *pNumSourceModes = 0;
    ntStatus =  (*pVidPnSourceModeSetInterface->pfnGetNumModes)(
        hVidPnSourceModeSet,
        pNumSourceModes
        );
    if (!NT_SUCCESS(ntStatus))
    {
        DBG_PRINT(Adapter, DBGLVL_ERROR,
            ("?" __FUNCTION__ ": "
            " pfnGetNumModes failed with ntStatus(0x%08x)\n",
            ntStatus
            ));
        return ntStatus;
    }

    pSourceModes = NULL;
    if (*pNumSourceModes != 0)
    {
        pSourceModes = LJB_GetPoolZero(
            sizeof(*pSourceModes) * (*pNumSourceModes)
            );
        if (pSourceModes == NULL)
        {
            DBG_PRINT(Adapter, DBGLVL_ERROR,
                ("?" __FUNCTION__ ": "
                "Unable to allocate pSourceModes\n"
                ));
            return STATUS_NO_MEMORY;
        }

        pThisSourceMode = NULL;
        pPrevSourceMode = NULL;
        for (i = 0; i < *pNumSourceModes; i++)
        {
            if (i == 0)
            {
                ntStatus = (*pVidPnSourceModeSetInterface->pfnAcquireFirstModeInfo)(
                    hVidPnSourceModeSet,
                    &pThisSourceMode
                    );
                if (!NT_SUCCESS(ntStatus))
                {
                    DBG_PRINT(Adapter, DBGLVL_ERROR,
                        ("?" __FUNCTION__ ": "
                        "pfnAcquireFirstModeInfo failed with ntStatus(0x%08x)?\n",
                        ntStatus
                        ));
                    LJB_FreePool(pSourceModes);
                    pSourceModes = NULL;
                    *pNumSourceModes = 0;
                    break;
                }
            }
            else
            {
                ntStatus = (*pVidPnSourceModeSetInterface->pfnAcquireNextModeInfo)(
                    hVidPnSourceModeSet,
                    pPrevSourceMode,
                    &pThisSourceMode
                    );
                if (!NT_SUCCESS(ntStatus))
                {
                    DBG_PRINT(Adapter, DBGLVL_ERROR,
                        ("?" __FUNCTION__ ": "
                        "pfnAcquireNextModeInfo failed with ntStatus(0x%08x)?\n",
                        ntStatus
                        ));
                    LJB_FreePool(pSourceModes);
                    pSourceModes = NULL;
                    *pNumSourceModes = 0;
                    break;
                }

                (VOID) (*pVidPnSourceModeSetInterface->pfnReleaseModeInfo)(
                    hVidPnSourceModeSet,
                    pPrevSourceMode
                    );
            }

            pPrevSourceMode = pThisSourceMode;
            pSourceModes[i] = *pThisSourceMode;
            DBG_PRINT(Adapter, DBGLVL_VIDPN,
                (__FUNCTION__ ": "
                "SrcMode[%u] : Type(%u), PrimSurfSize(%u, %u), "
                "VisibleRegionSize(%u, %u), PixelFormat(0x%x).\n",
                i,
                pThisSourceMode->Type,
                pThisSourceMode->Format.Graphics.PrimSurfSize.cx,
                pThisSourceMode->Format.Graphics.PrimSurfSize.cy,
                pThisSourceMode->Format.Graphics.VisibleRegionSize.cx,
                pThisSourceMode->Format.Graphics.VisibleRegionSize.cy,
                pThisSourceMode->Format.Graphics.PixelFormat
                ));
        } /* end of for loop */

        if (pPrevSourceMode != NULL)
        {
            (VOID) (*pVidPnSourceModeSetInterface->pfnReleaseModeInfo)(
                hVidPnSourceModeSet,
                pPrevSourceMode
                );
        }

        *ppSourceModes = pSourceModes;
    } /* end of if (pNumSourceModes != 0) */

    return ntStatus;
}

