/*
 * ljb_proxykmd_dispatch_add_device.c.
 *
 * Author: Lin Jiabang (lin.jiabang@gmail.com)
 *     Copyright (C) 2016  Lin Jiabang
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "ljb_proxykmd.h"
#include "ljb_proxykmd_guid.h"
#include <aux_klib.h>

#pragma warning(disable:28175) /* allow DriverObject->DriverName access */

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, LJB_PROXYKMD_AddDevice)
#endif

/*
 * undocumented API exported from WDK10 displib
 */
NTSTATUS DlpLoadDxgkrnl(
    __out PFILE_OBJECT *     DxgkFileObject,
    __out PDEVICE_OBJECT *   DxgkDeviceObject
    );
VOID DlpUnloadDxgkrnl(VOID);

/*
 * This routine tries to load dxgkrnl.sys, and obtains the DxgkDeviceObject.
 * If theDxgkDeviceObject is successfully retrieved, create an filter device
 * object and attach on top of it.
 */
static
NTSTATUS
LJB_PROXYKMD_CreateAndAttachDxgkFilter(
    __in PDRIVER_OBJECT     DriverObject,
    __in DEVICE_OBJECT *    DeviceObject
    )
{
    LJB_DEVICE_EXTENSION *  CONST   DeviceExtension = DeviceObject->DeviceExtension;
    LJB_DEVICE_EXTENSION *          FilterDeviceExtension;
    DEVICE_OBJECT *                 DxgkDeviceObject;
    FILE_OBJECT *                   DxgkFileObject;
    DEVICE_OBJECT *                 FilterDeviceObject;
    //UNICODE_STRING                  DxgkDevName;
    //UNICODE_STRING                  DxgkSvcName;
    NTSTATUS                        ntStatus;

    ntStatus = DlpLoadDxgkrnl(&DxgkFileObject, &DxgkDeviceObject);

    //RtlInitUnicodeString(&DxgkDevName, DXGK_DEV_NAME);
    //RtlInitUnicodeString(&DxgkSvcName, DXGK_SVC_NAME);
    //ntStatus = ZwLoadDriver(&DxgkSvcName);
    //if ((!NT_SUCCESS(ntStatus) && ntStatus != STATUS_IMAGE_ALREADY_LOADED))
    //{
    //    KdPrint(("?" __FUNCTION__ ": "
    //        "ZwLoadDriver failed with ntStatus(%08x)\n",
    //        ntStatus
    //        ));
    //    return ntStatus;
    //}
    //
    //ntStatus = IoGetDeviceObjectPointer(
    //    &DxgkDevName,
    //    FILE_ALL_ACCESS,
    //    &DxgkFileObject,
    //    &DxgkDeviceObject
    //    );
    //if (!NT_SUCCESS(ntStatus))
    //{
    //    KdPrint(("?" __FUNCTION__ ": "
    //        "IoGetDeviceObjectPointer failed with ntStatus(%08x)\n",
    //        ntStatus
    //        ));
    //    (VOID) ZwUnloadDriver(&DxgkSvcName);
    //    return ntStatus;
    //}

    KdPrint(( __FUNCTION__ ": DxgkDeviceObject(%p), DxgkFileObject(%p)\n",
        DxgkDeviceObject,
        DxgkFileObject
        ));

    ntStatus = IoCreateDevice(
        DriverObject,
        sizeof(LJB_DEVICE_EXTENSION),
        NULL,
        FILE_DEVICE_UNKNOWN,
        0,
        TRUE,
        &FilterDeviceObject
        );
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__ ": "
            " Not able to create FilterDeviceObject, ntStatus(0x%08x)?\n",
            ntStatus
            ));
        ObDereferenceObject(DxgkFileObject);
        //(VOID) ZwUnloadDriver(&DxgkSvcName);
        DlpUnloadDxgkrnl();
        return ntStatus;
    }

    FilterDeviceExtension = FilterDeviceObject->DeviceExtension;
    FilterDeviceExtension->DeviceType = DEVICE_TYPE_FILTER;
    FilterDeviceExtension->DeviceObject = FilterDeviceObject;
    FilterDeviceExtension->PhysicalDeviceObject = DxgkDeviceObject;
    FilterDeviceExtension->DebugMask = DeviceExtension->DebugMask;
    FilterDeviceExtension->NextLowerDriver = IoAttachDeviceToDeviceStack(
        FilterDeviceObject,
        DxgkDeviceObject
        );
    if (FilterDeviceExtension->NextLowerDriver == NULL)
    {
        KdPrint(("?" __FUNCTION__
            "IoAttachDeviceToDeviceStack(%p, %p) failed?\n",
            FilterDeviceObject,
            DxgkDeviceObject
            ));
        IoDeleteDevice(FilterDeviceObject);
        ObDereferenceObject(DxgkFileObject);
        //(VOID) ZwUnloadDriver(&DxgkSvcName);
        DlpUnloadDxgkrnl();
        return STATUS_UNSUCCESSFUL;
    }

    /*
     * after we attach a filter to the dxgk device object, we no longer
     * need a reference to dxgk device object. Dereference it here
     */
    ObDereferenceObject(DxgkFileObject);

    KdPrint((__FUNCTION__ ": "
        "Successfully attach to lower device(%p).\n",
        FilterDeviceExtension->NextLowerDriver
        ));

    DeviceExtension->FilterDeviceObject    = FilterDeviceObject;

    IoInitializeRemoveLock(
        &FilterDeviceExtension->RemoveLock,
        LJB_POOL_TAG,
        1,
        100
        );

    FilterDeviceObject->Flags |= FilterDeviceExtension->NextLowerDriver->Flags &
        (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE);
    FilterDeviceObject->DeviceType = FilterDeviceExtension->NextLowerDriver->DeviceType;
    FilterDeviceObject->Characteristics = FilterDeviceExtension->NextLowerDriver->Characteristics;
    FilterDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    return ntStatus;
}

static
BOOLEAN
LJB_PROXYKMD_IsDxgknlLoaded(
    __in DEVICE_OBJECT *    DeviceObject
    )
{
    NTSTATUS                        ntStatus;
    BOOLEAN                         DxgkIsLoaded;
    ULONG                           ModulesSize;
    AUX_MODULE_EXTENDED_INFO*       ModExtInfos;
    ULONG                           NumOfModules;
    UINT                            i;

    UNREFERENCED_PARAMETER(DeviceObject);

    DxgkIsLoaded = FALSE;
    ntStatus = AuxKlibInitialize();
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__
            ": AuxKlibInitialize failed with ntStatus(0x%08x)?\n",
            ntStatus
            ));
        return FALSE;
    }

    ntStatus = AuxKlibQueryModuleInformation(
        &ModulesSize,
        sizeof(AUX_MODULE_EXTENDED_INFO),
        NULL
        );
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__
            ": AuxKlibQueryModuleInformation failed with ntStatus(0x%08x)?\n",
            ntStatus
            ));
        return FALSE;
    }

    if (ModulesSize == 0)
    {
        KdPrint(("?" __FUNCTION__
            ": ModulesSize is 0?\n"
            ));
        return FALSE;
    }

    NumOfModules = ModulesSize / sizeof(AUX_MODULE_EXTENDED_INFO);
    ModExtInfos = LJB_PROXYKMD_GetPoolZero(ModulesSize);
    if (ModExtInfos == NULL)
    {
        KdPrint(("?" __FUNCTION__
            ": unable to allocate ModExtInfos?\n"
            ));
        return FALSE;
    }

    ntStatus = AuxKlibQueryModuleInformation(
        &ModulesSize,
        sizeof(AUX_MODULE_EXTENDED_INFO),
        ModExtInfos
        );
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__
            ": AuxKlibQueryModuleInformation failed with ntStatus(0x%08x)?\n",
            ntStatus
            ));
        LJB_PROXYKMD_FreePool(ModExtInfos);
        return FALSE;
    }

    for (i = 0; i < NumOfModules; i++)
    {
        AUX_MODULE_EXTENDED_INFO* CONST ModExtInfo = ModExtInfos + i;
        UCHAR * FileName;

        FileName = ModExtInfo->FullPathName + ModExtInfo->FileNameOffset;
        if ((FileName[0] == 'd' || FileName[0] == 'D') &&
            (FileName[1] == 'x' || FileName[1] == 'X') &&
            (FileName[2] == 'g' || FileName[2] == 'G') &&
            (FileName[3] == 'k' || FileName[3] == 'K') &&
            (FileName[4] == 'r' || FileName[4] == 'R') &&
            (FileName[5] == 'n' || FileName[5] == 'N') &&
            (FileName[6] == 'l' || FileName[6] == 'L') &&
            (FileName[7] == '.') &&
            (FileName[8] == 's' || FileName[8] == 'S') &&
            (FileName[9] == 'y' || FileName[9] == 'Y') &&
            (FileName[10] == 's' || FileName[10] == 'S'))
        {
            KdPrint((__FUNCTION__ ": Found %s\n", ModExtInfo->FullPathName));
            DxgkIsLoaded = TRUE;
            break;
        }
    }

    LJB_PROXYKMD_FreePool(ModExtInfos);
    return DxgkIsLoaded;
}

NTSTATUS
LJB_PROXYKMD_AddDevice(
    __in PDRIVER_OBJECT DriverObject,
    __in PDEVICE_OBJECT PhysicalDeviceObject
    )
{
    PDEVICE_OBJECT          DeviceObject;
    LJB_DEVICE_EXTENSION *  DeviceExtension;
    RTL_OSVERSIONINFOW      RtlOsVersion;
    NTSTATUS                ntStatus;

    PAGED_CODE();

    ntStatus = IoCreateDevice(
        DriverObject,
        sizeof (LJB_DEVICE_EXTENSION),
        NULL,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &DeviceObject
        );
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__ ": IoCreateDevice failed (0x%08x)\n", ntStatus));
        return ntStatus;
    }

    DeviceExtension = DeviceObject->DeviceExtension;
    INITIALIZE_PNP_STATE(DeviceExtension);

    RtlZeroMemory(&RtlOsVersion, sizeof(RtlOsVersion));
    RtlOsVersion.dwOSVersionInfoSize = sizeof(RtlOsVersion);
    ntStatus = RtlGetVersion(&RtlOsVersion);
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__ ": RtlGetVersion failed with (0x%08x)?\n",
            ntStatus
            ));
        IoDeleteDevice(DeviceObject);
        return ntStatus;
    }

    DeviceExtension->RtlOsVersion           = RtlOsVersion;
    DeviceExtension->DeviceType             = DEVICE_TYPE_FDO;
    DeviceExtension->PhysicalDeviceObject   = PhysicalDeviceObject;
    DeviceExtension->DeviceObject           = DeviceObject;

    DeviceExtension->NextLowerDriver = IoAttachDeviceToDeviceStack(
        DeviceObject,
        PhysicalDeviceObject
        );
    if (DeviceExtension->NextLowerDriver == NULL)
    {
        KdPrint(("?" __FUNCTION__ ": "
            "IoAttachDeviceToDeviceStack failed?\n"
            ));
        IoDeleteDevice(DeviceObject);
        return STATUS_NO_SUCH_DEVICE;
    }

    ntStatus = IoRegisterDeviceInterface(
        PhysicalDeviceObject,
        (LPGUID) &LJB_PROXYKMD_INTERFACE_GUID,
        NULL,
        &DeviceExtension->InterfaceName
        );
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__ ": "
            "IoRegisterDeviceInterface failed(0x%08x)?\n"
            ));
        IoDetachDevice (DeviceExtension->NextLowerDriver);
        IoDeleteDevice(DeviceObject);
        return ntStatus;
    }

    GlobalDriverData.DebugMask      = DeviceExtension->DebugMask;
    GlobalDriverData.DriverObject    = DriverObject;
    GlobalDriverData.DeviceObject    = DeviceObject;

    /*
     * We might get loaded during boot or during installation.
     * If Dxgkrnl.sys isn't yet loaded, we wait until it is loaded before
     * attaching a filter object to it. If it is already loaded, attach our
     * filter object
     */
    if (!LJB_PROXYKMD_IsDxgknlLoaded(DeviceObject))
    {
    }

    /*
     * now that the Dxgkrnl is loaded, create and attach our filter
     */
    ntStatus = LJB_PROXYKMD_CreateAndAttachDxgkFilter(
        DriverObject,
        DeviceObject
        );
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__ ": "
            "LJB_PROXYKMD_CreateAndAttachDxgkFilter failed with ntStatus(0x%08x)?\n",
            ntStatus
            ));
        RtlFreeUnicodeString(&DeviceExtension->InterfaceName);
        IoDetachDevice (DeviceExtension->NextLowerDriver);
        IoDeleteDevice(DeviceObject);
        return ntStatus;
    }

    IoInitializeRemoveLock (
        &DeviceExtension->RemoveLock ,
        LJB_POOL_TAG,
        1,
        100
        );

    DeviceObject->Flags |= DeviceExtension->NextLowerDriver->Flags &
        (DO_BUFFERED_IO | DO_DIRECT_IO | DO_POWER_PAGABLE );
    DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

   return ntStatus;
}