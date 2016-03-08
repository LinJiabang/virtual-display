/*
 * ljb_proxykmd_registry_callback.c.
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
#include <ntifs.h>
#include "ljb_proxykmd.h"

const char * const KeyValueInfoClassStr[]  =
    {
    "KeyValueBasicInformation",
    "KeyValueFullInformation",
    "KeyValuePartialInformation",
    "KeyValueFullInformationAlign64",
    "KeyValuePartialInformationAlign64",
    "MaxKeyValueInfoClass"
    };

static ULONG
LJB_PROXYKMD_CalculateRequiredLength(
    __in REG_QUERY_VALUE_KEY_INFORMATION *  RegQueryValueKeyInfo,
    __in BOOLEAN                            IsQueryUMD
    );

static void
LJB_PROXYKMD_OverrideUserModeDriverName(
    __in LJB_ADAPTER *                      Adapter,
    __in REG_QUERY_VALUE_KEY_INFORMATION *  RegQueryValueKeyInfo,
    __in UCHAR *                            pData
    );

static void
LJB_PROXYKMD_OverrideUserModeDriverNameWow(
    __in LJB_ADAPTER *                      Adapter,
    __in REG_QUERY_VALUE_KEY_INFORMATION *  RegQueryValueKeyInfo,
    __in UCHAR *                            pData
    );

static
BOOLEAN
IsCalledFromDwm(VOID);

/*
 * Name:  LJB_PROXYKMD_RegistryCallback
 *
 * Definition:
 *    EX_CALLBACK_FUNCTION    LJB_PROXYKMD_RegistryCallback;
 *
 * Description:
 *    A filter driver's RegistryCallback routine can monitor, block, or modify a
 *    registry operation.
 *
 *    The RegistryCallback routine is available on Microsoft Windows XP and later
 *    versions of Windows.
 *
 *    To be notified of registry operations, a kernel-mode component (such as the
 *    driver component of an anti-virus software package) can call CmRegisterCallback
 *    or CmRegisterCallbackEx to register a RegistryCallback routine.
 *
 *    You must enclose any access to an address that is referenced through the
 *    Argument2 parameter in a try/except block.
 *
 *    For more information about RegistryCallback routines and filtering registry
 *    operations, see Filtering Registry Calls.
 *
 *    RegisterCallback executes at IRQL = PASSIVE_LEVEL and in the context of the
 *    thread that is performing the registry operation.
 *
 * Return Value:
 *    Microsoft Windows XP and Windows Server 2003:
 *        If the RegistryCallback routine returns STATUS_SUCCESS, the configuration
 *        manager continues processing the registry operation.
 *        If the RegistryCallback routine returns a status value for which
 *        NT_SUCCESS(status) equals FALSE, the configuration manager stops processing
 *        the registry operation and returns the specified return value to the calling
 *        thread.
 *
 *    Windows Vista and later:
 *        If the RegistryCallback routine returns STATUS_SUCCESS, the configuration
 *        manager continues processing the registry operation.
 *        If the RegistryCallback routine returns STATUS_CALLBACK_BYPASS, the
 *        configuration manager stops processing the registry operation and returns
 *        STATUS_SUCCESS to the calling thread.
 *        If the RegistryCallback routine returns a status value for which
 *        NT_SUCCESS(status) equals FALSE (except for STATUS_CALLBACK_BYPASS), the
 *        configuration manager stops processing the registry operation and returns
 *        the specified return value to the calling thread.
 *
 *    For more information about when a RegistryCallback routine should return
 *    each of these status values, see Filtering Registry Calls.
 *
 */
NTSTATUS
LJB_PROXYKMD_RegistryCallback(
    __in PVOID CallbackContext,
    __in_opt PVOID Argument1,
    __in_opt PVOID Argument2
    )
{
    LJB_DEVICE_EXTENSION * CONST            DevExt = CallbackContext;
    REG_NOTIFY_CLASS CONST                  RegNotifyClass = (ULONG_PTR) Argument1;
    REG_POST_OPERATION_INFORMATION * CONST  RegPostOperationInfo = Argument2;
    DECLARE_CONST_UNICODE_STRING(UserModeDriverNameStr, USER_MODE_DRIVER_NAME);
    DECLARE_CONST_UNICODE_STRING(UserModeDriverNameWowStr, USER_MODE_DRIVER_NAME_WOW);
    REG_QUERY_VALUE_KEY_INFORMATION *       RegQueryValueKeyInfo;
    KEY_VALUE_FULL_INFORMATION *            KeyValueFullInfo;
    KEY_VALUE_PARTIAL_INFORMATION *         KeyValuePartialInfo;
    PCUNICODE_STRING                        ObjectName;
    LJB_ADAPTER *                           Adapter;
    LIST_ENTRY *                            ListHead;
    LIST_ENTRY *                            ListEntry;
    WCHAR                                   TmpName[MAX_PATH];
    ULONG                                   MinimumLength;
    ULONG                                   RequiredLength;
    NTSTATUS                                ntStatus;
    BOOLEAN                                 IsQueryUMD;

    /*
     * Only interested in RegNtPostQueryValueKey.
     */
    if (RegNotifyClass != RegNtPostQueryValueKey)
        return STATUS_SUCCESS;

    if (RegPostOperationInfo == NULL)
        return STATUS_SUCCESS;

    RegQueryValueKeyInfo = RegPostOperationInfo->PreInformation;
    if (RegQueryValueKeyInfo == NULL)
        return STATUS_SUCCESS;

    if (!RtlEqualUnicodeString(
            &UserModeDriverNameStr,
            RegQueryValueKeyInfo->ValueName,
            TRUE) &&
        !RtlEqualUnicodeString(
            &UserModeDriverNameWowStr,
            RegQueryValueKeyInfo->ValueName,
            TRUE)
        )
    {
        return STATUS_SUCCESS;
    }

    /*
     * when we reach here, the caller is either querying "UserModeDriverName" or "UserModeDriverNameWow"
     * We calculate the required length here!
     */
    IsQueryUMD = FALSE;
    MinimumLength = sizeof(*KeyValuePartialInfo);
    if (RegQueryValueKeyInfo->KeyValueInformationClass == KeyValueFullInformation)
        MinimumLength = sizeof(*KeyValueFullInfo);

    if (RtlEqualUnicodeString(
        &UserModeDriverNameStr,
        RegQueryValueKeyInfo->ValueName,
        TRUE))
    {
        IsQueryUMD = TRUE;
    }

    RequiredLength = LJB_PROXYKMD_CalculateRequiredLength(
        RegQueryValueKeyInfo,
        IsQueryUMD
        );

    /*
     * If the original query failed, we check the return status. It might be that
     * the RegQueryValueKeyInfo->Length is not large enough, and the returned
     * ResultLength contains the original size. In this case, we need to patch
     * ResultLength.
     */
    if (!NT_SUCCESS(RegPostOperationInfo->Status))
    {
        KdPrint(( __FUNCTION__ ": RegPostOperationInfo->Status(0x%x), IsQueryUMD(%u)\n",
            RegPostOperationInfo->Status,
            IsQueryUMD
            ));

        if (RegPostOperationInfo->Status != STATUS_BUFFER_TOO_SMALL && RegPostOperationInfo->Status != STATUS_BUFFER_OVERFLOW)
            return STATUS_SUCCESS;

        /*
         * over-ride RegPostOperationInfo->ResultLength!
         */
        if (*RegQueryValueKeyInfo->ResultLength < RequiredLength)
        {
            KdPrint((__FUNCTION__
                ": Override ResultLength(%u) to %u for KeyInformationClass(%u:%s)\n",
                *RegQueryValueKeyInfo->ResultLength,
                RequiredLength,
                RegQueryValueKeyInfo->KeyValueInformationClass,
                KeyValueInfoClassStr[RegQueryValueKeyInfo->KeyValueInformationClass]
                ));
            *RegQueryValueKeyInfo->ResultLength = RequiredLength;
        }
        return STATUS_SUCCESS;
    }

    ntStatus = CmCallbackGetKeyObjectID(
        &DevExt->RegistryCallbackCookie,
        RegQueryValueKeyInfo->Object,
        NULL,
        &ObjectName
        );
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?" __FUNCTION__
            ": CmCallbackGetKeyObjectID failed with ntStatus(0x%08x)\n",
            ntStatus
            ));
        return STATUS_SUCCESS;
    }

    RtlZeroMemory(TmpName, sizeof(TmpName));
    RtlCopyMemory(
        TmpName,
        RegQueryValueKeyInfo->ValueName->Buffer,
        RegQueryValueKeyInfo->ValueName->Length
        );
    KdPrint((__FUNCTION__ ":Object(%p), ValueName(%ws), KeyValueInformationClass(%u), Length(%u)\n",
        RegQueryValueKeyInfo->Object,
        TmpName,
        RegQueryValueKeyInfo->KeyValueInformationClass,
        RegQueryValueKeyInfo->Length
        ));

    if (RegQueryValueKeyInfo->KeyValueInformationClass != KeyValueFullInformation &&
        RegQueryValueKeyInfo->KeyValueInformationClass != KeyValuePartialInformation)
        return STATUS_SUCCESS;

    RtlZeroMemory(&TmpName, sizeof(TmpName));
    RtlCopyMemory(&TmpName, ObjectName->Buffer, ObjectName->Length);
    KdPrint((__FUNCTION__ ": ObjectName(%ws)\n",
        TmpName
        ));

    /*
     * for each Adapter, check if Adapter->pDriverKeyName is equal to ObjectName
     */
    Adapter = NULL;
    ListHead = &GlobalDriverData.ClientAdapterListHead;
    for (ListEntry = ListHead->Flink;
         ListEntry != ListHead;
         ListEntry = ListEntry->Flink)
    {
        LJB_ADAPTER *   ThisAdapter;
        UNICODE_STRING  DriverKeyName;

        ThisAdapter = CONTAINING_RECORD(
            ListEntry,
            LJB_ADAPTER,
            ListEntry
            );
        if (ThisAdapter->DriverKeyNameBuffer == NULL)
            continue;

        DriverKeyName.Length = (USHORT) ThisAdapter->DriverKeyNameInfo.NameLength;
        DriverKeyName.MaximumLength = MAX_PATH;
        DriverKeyName.Buffer = ThisAdapter->DriverKeyNameBuffer;
        if (RtlEqualUnicodeString(
            &DriverKeyName,
            ObjectName,
            TRUE            // CaseInSensitive
            ))
        {
            Adapter = ThisAdapter;
            break;
        }
    }

    if (Adapter == NULL)
    {
        KdPrint(("?" __FUNCTION__
            ": No Adapter found?\n"
            ));
        return STATUS_SUCCESS;
    }

    KeyValueFullInfo = RegQueryValueKeyInfo->KeyValueInformation;
    KeyValuePartialInfo = RegQueryValueKeyInfo->KeyValueInformation;

    /*
     * The real challenges are here! Handle REG_XXX_KEY_INFORMATION carefully!
     * We assume OS queries the UserModeDriverName or UserModeDriverName with
     * RegQueryValueKeyInfo->KeyValueInformationClass == KeyValueFullInformation!
     */
    ntStatus = STATUS_SUCCESS;

    /*
     * if the registry is intercepted once, we stop intercepting again
     */
    if ((IsQueryUMD && Adapter->UserModeDriverNameSize != 0) ||
        (!IsQueryUMD && Adapter->UserModeDriverNameWowSize != 0))
    {
        PUNICODE_STRING pImageFileName;
        WCHAR           ImageFileName[MAX_PATH];
        BOOLEAN         isDwm = IsCalledFromDwm();
        NTSTATUS        myStatus;

        myStatus = SeLocateProcessImageName(PsGetCurrentProcess(), &pImageFileName);
        if (NT_SUCCESS(myStatus))
        {
            RtlZeroMemory(ImageFileName, sizeof(ImageFileName));
            RtlCopyMemory(ImageFileName, pImageFileName->Buffer, pImageFileName->Length);
            KdPrint(( __FUNCTION__ ": called from (%ws)\n",
                ImageFileName
                ));
        }
        if (!isDwm)
        {
            KdPrint((__FUNCTION__": NOT called from DWM, ProcessId(%p).\n", PsGetCurrentProcessId()));
            return STATUS_SUCCESS;
        }
    }

    KdPrint((__FUNCTION__
        ": Length(%u), ResultLength(%u), MinimumLength(%u), RequiredLength(%u)\n",
        RegQueryValueKeyInfo->Length,
        *RegQueryValueKeyInfo->ResultLength,
        MinimumLength,
        RequiredLength
        ));

    if (*RegQueryValueKeyInfo->ResultLength < RequiredLength)
        *RegQueryValueKeyInfo->ResultLength = RequiredLength;

    if (RegQueryValueKeyInfo->Length < MinimumLength)
    {
        RegPostOperationInfo->ReturnStatus = STATUS_BUFFER_TOO_SMALL;
        ntStatus = STATUS_CALLBACK_BYPASS;
        KdPrint((__FUNCTION__
            ":Override ReturnStatus= STATUS_BUFFER_TOO_SMALL,"
            "ntStatus = STATUS_CALLBACK_BYPASS\n"
            ));
    }
    else if (RegQueryValueKeyInfo->Length < RequiredLength)
    {
        RegPostOperationInfo->ReturnStatus = STATUS_BUFFER_OVERFLOW;
        ntStatus = STATUS_CALLBACK_BYPASS;
        KdPrint((__FUNCTION__
            ":Override ReturnStatus= STATUS_BUFFER_OVERFLOW,"
            "ntStatus = STATUS_CALLBACK_BYPASS\n"
            ));
    }
    else
    {
        UCHAR   *   pData;

        /*
         * The caller provided large enough buffer (RegQueryValueKeyInfo->Length >= RequiredLength.
         * We are able to modify KeyValueFullInfo or KeyValuePartialInfo
         */
        if (RegQueryValueKeyInfo->KeyValueInformationClass == KeyValueFullInformation)
            pData = (UCHAR *) KeyValueFullInfo + KeyValueFullInfo->DataOffset;
        else
            pData = KeyValuePartialInfo->Data;

        if (IsQueryUMD)
        {
            ULONG       UserModeDriverNameSize;

            /*
             * Fixup Adapter->UserModeDriverName!
             */
            if (RegQueryValueKeyInfo->KeyValueInformationClass == KeyValueFullInformation)
                UserModeDriverNameSize = KeyValueFullInfo->DataLength;
            else
                UserModeDriverNameSize = KeyValuePartialInfo->DataLength;
            if (Adapter->UserModeDriverNameSize == 0)
            {
                KdPrint((__FUNCTION__ ": Fixup Adapter->UserModeDriverName (%ws), size(%u)\n",
                    pData,
                    UserModeDriverNameSize
                    ));
                RtlZeroMemory(
                    Adapter->UserModeDriverName,
                    sizeof(Adapter->UserModeDriverName)
                    );
                RtlCopyMemory(
                    Adapter->UserModeDriverName,
                    pData,
                    UserModeDriverNameSize
                    );
                Adapter->UserModeDriverNameSize = UserModeDriverNameSize;
            }

            LJB_PROXYKMD_OverrideUserModeDriverName(
                Adapter,
                RegQueryValueKeyInfo,
                pData
                );
        }
        else
        {
            ULONG       UserModeDriverNameWowSize;

            /*
             * Fixup Adapter->UserModeDriverNameWow!
             */
            if (RegQueryValueKeyInfo->KeyValueInformationClass == KeyValueFullInformation)
                UserModeDriverNameWowSize = KeyValueFullInfo->DataLength;
            else
                UserModeDriverNameWowSize = KeyValuePartialInfo->DataLength;
            if (Adapter->UserModeDriverNameWowSize == 0)
                {
                KdPrint((__FUNCTION__ ": Fixup Adapter->UserModeDriverNameWow from(%ws) to (%ws)\n",
                    Adapter->UserModeDriverNameWow,
                    pData
                    ));
                RtlZeroMemory(
                    Adapter->UserModeDriverNameWow,
                    sizeof(Adapter->UserModeDriverNameWow)
                    );
                RtlCopyMemory(
                    Adapter->UserModeDriverNameWow,
                    pData,
                    UserModeDriverNameWowSize
                    );
                Adapter->UserModeDriverNameWowSize = UserModeDriverNameWowSize;
                }

            LJB_PROXYKMD_OverrideUserModeDriverNameWow(
                Adapter,
                RegQueryValueKeyInfo,
                pData
                );
        }
    }

    return ntStatus;
}

static ULONG
LJB_PROXYKMD_CalculateRequiredLength(
    __in REG_QUERY_VALUE_KEY_INFORMATION *  RegQueryValueKeyInfo,
    __in BOOLEAN                            IsQueryUMD
    )
{
    ULONG                                   RequiredLength;

    RequiredLength = 0;
    if (IsQueryUMD)
    {
        switch (RegQueryValueKeyInfo->KeyValueInformationClass)
        {
        case KeyValueFullInformation:
            RequiredLength = sizeof(KEY_VALUE_FULL_INFORMATION) - sizeof(WCHAR) +
                sizeof(USER_MODE_DRIVER_NAME) + sizeof(MY_USER_MODE_DRIVER_NAME_FULL);
            break;

        case KeyValuePartialInformation:
            RequiredLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(MY_USER_MODE_DRIVER_NAME_FULL);
            break;

        default:
            break;
        }
    }
    else
    {
        switch (RegQueryValueKeyInfo->KeyValueInformationClass)
        {
        case KeyValueFullInformation:
            RequiredLength = sizeof(KEY_VALUE_FULL_INFORMATION) - sizeof(WCHAR) +
                sizeof(USER_MODE_DRIVER_NAME_WOW) + sizeof(MY_USER_MODE_DRIVER_NAME_WOW_FULL);
            break;

        case KeyValuePartialInformation:
            RequiredLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(MY_USER_MODE_DRIVER_NAME_WOW_FULL);
            break;

        default:
            break;
        }
    }

    return RequiredLength;
}

static void
LJB_PROXYKMD_OverrideUserModeDriverName(
    __in LJB_ADAPTER *                      Adapter,
    __in REG_QUERY_VALUE_KEY_INFORMATION *  RegQueryValueKeyInfo,
    __in UCHAR *                            pData
    )
    {
    WCHAR                                   UserModeDriverName[MAX_PATH];
    KEY_VALUE_FULL_INFORMATION *            KeyValueFullInfo;
    KEY_VALUE_PARTIAL_INFORMATION *         KeyValuePartialInfo;
    ULONG                                   i;
    ULONG                                   NumberOfEntries;

    KeyValueFullInfo = RegQueryValueKeyInfo->KeyValueInformation;
    KeyValuePartialInfo = RegQueryValueKeyInfo->KeyValueInformation;

    /*
     * check number of entries in UserModeDrivername
     */
    NumberOfEntries = 0;
    RtlCopyMemory(UserModeDriverName, Adapter->UserModeDriverName, sizeof(UserModeDriverName));
    for (i = 0; i < (Adapter->UserModeDriverNameSize / 2); i++)
    {
        if (i > 0 &&
            Adapter->UserModeDriverName[i] == L'\0' &&
            Adapter->UserModeDriverName[i - 1] == L'\0')
            break;

        if (i > 0 && Adapter->UserModeDriverName[i] == L'\0')
            {
            UserModeDriverName[i] = L' ';
            NumberOfEntries++;
            }
    }

    if (NumberOfEntries >= NUM_OF_UMD_ENTRIES)
    {
        RtlCopyMemory(
            pData,
            MY_USER_MODE_DRIVER_NAME_FULL,
            sizeof(MY_USER_MODE_DRIVER_NAME_FULL)
            );
        if (RegQueryValueKeyInfo->KeyValueInformationClass == KeyValueFullInformation)
            KeyValueFullInfo->DataLength = sizeof(MY_USER_MODE_DRIVER_NAME_FULL);
        else
            KeyValuePartialInfo->DataLength = sizeof(MY_USER_MODE_DRIVER_NAME_FULL);
    }
    else
    {
        ULONG   DataLength;
        UCHAR * pTmp;

        RtlZeroMemory(pData, sizeof(MY_USER_MODE_DRIVER_NAME_FULL));
        DataLength = 0;
        pTmp = pData;
        for (i = 0; i < NumberOfEntries; i++)
        {
            RtlCopyMemory(
                pTmp,
                MY_USER_MODE_DRIVER_NAME,
                sizeof(MY_USER_MODE_DRIVER_NAME)
                );
            DataLength += sizeof(MY_USER_MODE_DRIVER_NAME);
            pTmp += sizeof(MY_USER_MODE_DRIVER_NAME);
        }

        // Add trailing NULL
        DataLength += sizeof(WCHAR);
        if (RegQueryValueKeyInfo->KeyValueInformationClass == KeyValueFullInformation)
            KeyValueFullInfo->DataLength = DataLength;
        else
            KeyValuePartialInfo->DataLength = DataLength;
    }

    KdPrint((__FUNCTION__ ": Override UserModeDriverNameData from (%ws) to (%ws), NumberOfEntries(%u)\n",
        UserModeDriverName,
        pData,
        NumberOfEntries
        ));
}

static void
LJB_PROXYKMD_OverrideUserModeDriverNameWow(
    __in LJB_ADAPTER *                      Adapter,
    __in REG_QUERY_VALUE_KEY_INFORMATION *  RegQueryValueKeyInfo,
    __in UCHAR *                            pData
    )
{
    WCHAR                                   UserModeDriverNameWow[MAX_PATH];
    KEY_VALUE_FULL_INFORMATION *            KeyValueFullInfo;
    KEY_VALUE_PARTIAL_INFORMATION *         KeyValuePartialInfo;
    ULONG                                   i;
    ULONG                                   NumberOfEntries;

    KeyValueFullInfo = RegQueryValueKeyInfo->KeyValueInformation;
    KeyValuePartialInfo = RegQueryValueKeyInfo->KeyValueInformation;

    /*
     * check the number of entries in UserModeDriverNameWow.
     */
    NumberOfEntries = 0;
    RtlCopyMemory(UserModeDriverNameWow, Adapter->UserModeDriverNameWow, sizeof(UserModeDriverNameWow));
    for (i = 0; i < (Adapter->UserModeDriverNameWowSize / 2); i++)
    {
        if (i > 0 &&
            Adapter->UserModeDriverNameWow[i] == L'\0' &&
            Adapter->UserModeDriverNameWow[i - 1] == L'\0')
            break;

        if (i > 0 && Adapter->UserModeDriverNameWow[i] == L'\0')
        {
            UserModeDriverNameWow[i] = L' ';
            NumberOfEntries++;
        }
    }

    if (NumberOfEntries >= NUM_OF_UMD_ENTRIES)
    {
        RtlCopyMemory(
            pData,
            MY_USER_MODE_DRIVER_NAME_WOW_FULL,
            sizeof(MY_USER_MODE_DRIVER_NAME_WOW_FULL)
            );
        if (RegQueryValueKeyInfo->KeyValueInformationClass == KeyValueFullInformation)
            KeyValueFullInfo->DataLength = sizeof(MY_USER_MODE_DRIVER_NAME_WOW_FULL);
        else
            KeyValuePartialInfo->DataLength = sizeof(MY_USER_MODE_DRIVER_NAME_WOW_FULL);
    }
    else
    {
        ULONG   DataLength;
        UCHAR * pTmp;
        RtlZeroMemory(pData, sizeof(MY_USER_MODE_DRIVER_NAME_WOW_FULL));
        DataLength = 0;
        pTmp = pData;
        for (i = 0; i < NumberOfEntries; i++)
        {
            RtlCopyMemory(
                pTmp,
                MY_USER_MODE_DRIVER_NAME_WOW,
                sizeof(MY_USER_MODE_DRIVER_NAME_WOW)
                );
            DataLength += sizeof(MY_USER_MODE_DRIVER_NAME_WOW);
            pTmp += sizeof(MY_USER_MODE_DRIVER_NAME_WOW);
        }

        // Add trailing NULL
        DataLength += sizeof(WCHAR);
        if (RegQueryValueKeyInfo->KeyValueInformationClass == KeyValueFullInformation)
            KeyValueFullInfo->DataLength = DataLength;
        else
            KeyValuePartialInfo->DataLength = DataLength;
    }

    KdPrint((__FUNCTION__ ": Override UserModeDriverNameWow from (%ws) to (%ws), NumberOfEntries(%u)\n",
        UserModeDriverNameWow,
        pData,
        NumberOfEntries
        ));
}

static
BOOLEAN
IsCalledFromDwm(VOID)
{
    NTSTATUS            ntStatus;
    PUNICODE_STRING     pImageFileName;
    WCHAR               ImageFileName[MAX_PATH];
    WCHAR *             pTmp;
    UINT                FileNameLen;
    PEPROCESS           Process;
    HANDLE              ProcessId;

    ProcessId = PsGetCurrentProcessId();
    Process = PsGetCurrentProcess();

    ntStatus = SeLocateProcessImageName(Process, &pImageFileName);
    if (!NT_SUCCESS(ntStatus))
    {
        KdPrint(("?"__FUNCTION__ ": SeLocateProcessImageName failed with ntStatus(0x%08x)\n", ntStatus));
        return FALSE;
    }

    /*
     * The input is like this: \??\C:\Windows\system32\dwm.exe
     * compare the last 7 characters.
     */
    RtlZeroMemory(ImageFileName, sizeof(ImageFileName));
    RtlCopyMemory(
        ImageFileName,
        pImageFileName->Buffer,
        pImageFileName->Length
        );
    FileNameLen = 0;
    for (pTmp = ImageFileName; *pTmp != L'\0'; pTmp++)
        FileNameLen++;
    if (FileNameLen < 7)
        return FALSE;

    pTmp = &ImageFileName[0] + FileNameLen - 7;
    if (((pTmp[0] == L'd') || (pTmp[0] == L'D')) &&
        ((pTmp[1] == L'w') || (pTmp[1] == L'W')) &&
        ((pTmp[2] == L'm') || (pTmp[2] == L'M')) &&
         (pTmp[3] == L'.') &&
        ((pTmp[4] == L'e') || (pTmp[4] == L'E')) &&
        ((pTmp[5] == L'x') || (pTmp[5] == L'X')) &&
        ((pTmp[6] == L'e') || (pTmp[6] == L'E')))
    {
        KdPrint((__FUNCTION__ ": Found Dwm ProcessId(%p)\n", ProcessId));
        return TRUE;
    }

    return FALSE;
}
