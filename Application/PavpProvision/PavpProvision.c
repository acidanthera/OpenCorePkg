/** @file
  Provision EPID key.

Copyright (c) 2019, vit9696. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Protocol/FirmwareVolume.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PciLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Guid/GlobalVariable.h>

#include <Library/OcDebugLogLib.h>
#include <Library/OcHeciLib.h>
#include <Library/OcMiscLib.h>

#include <Protocol/Heci.h>
#include <Protocol/Heci2.h>
#include <IndustryStandard/AppleProvisioning.h>
#include <IndustryStandard/HeciMsg.h>
#include <IndustryStandard/HeciClientMsg.h>

#define FORCE_PROVISIONING 1
#define SA_MC_BUS   0x00
#define R_SA_PAVPC (0x58)

#define MmPciAddress(Segment, Bus, Device, Function, Register) \
    ((UINTN) (PciRead32 (PCI_LIB_ADDRESS (0,0,0,0x60)) & 0xFC000000) + \
     (UINTN) (Bus << 20) + (UINTN) (Device << 15) + (UINTN) \
     (Function << 12) + (UINTN) (Register))

#define MmPci32(Segment, Bus, Device, Function, Register) \
   *((volatile UINT32 *) MmPciAddress (Segment, Bus, Device, Function, Register))

STATIC UINT8 mMeClientMap[HBM_ME_CLIENT_MAX];
STATIC UINT8 mMeClientActiveCount;

extern UINT8 gDefaultAppleEpidCertificate[];
extern UINTN gDefaultAppleEpidCertificateSize;

extern UINT8 gDefaultAppleGroupPublicKeys[];
extern UINTN gDefaultAppleGroupPublicKeysSize;

STATIC
EFI_STATUS
ReadProvisioningDataFile (
  IN  EFI_GUID         *FvNameGuid,
  OUT VOID             **Buffer,
  OUT UINTN            *BufferSize
  )
{
  UINTN                         Index;
  EFI_STATUS                    Status;
  UINT32                        AuthenticationStatus;
  EFI_FIRMWARE_VOLUME_PROTOCOL  *FirmwareVolumeInterface;
  UINTN                         NumOfHandles;
  EFI_HANDLE                    *HandleBuffer;

  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiFirmwareVolumeProtocolGuid,
    NULL,
    &NumOfHandles,
    &HandleBuffer
    );

  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < NumOfHandles; ++Index) {
      Status = gBS->HandleProtocol (
        HandleBuffer[Index],
        &gEfiFirmwareVolumeProtocolGuid,
        (VOID **) &FirmwareVolumeInterface
        );

      if (EFI_ERROR (Status)) {
        gBS->FreePool (HandleBuffer);
        return Status;
      }

      *Buffer = NULL;
      *BufferSize = 0;

      Status = FirmwareVolumeInterface->ReadSection (
        FirmwareVolumeInterface,
        FvNameGuid,
        EFI_SECTION_RAW,
        0,
        Buffer,
        BufferSize,
        &AuthenticationStatus
        );

      if (!EFI_ERROR (Status)) {
        gBS->FreePool (HandleBuffer);
        return Status;
      }
    }

    gBS->FreePool (HandleBuffer);
    Status = EFI_NOT_FOUND;
  }

  //
  // Implement fallback for our firmwares.
  //
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCPAVP: No %g in firmware, using default - %r\n", FvNameGuid, Status));

    if (CompareGuid (&gAppleEpidCertificateFileGuid, FvNameGuid)) {
      *Buffer = AllocateCopyPool (gDefaultAppleEpidCertificateSize, gDefaultAppleEpidCertificate);
      *BufferSize = gDefaultAppleEpidCertificateSize;
    } else if (CompareGuid (&gAppleEpidGroupPublicKeysFileGuid, FvNameGuid)) {
      *Buffer = AllocateCopyPool (gDefaultAppleGroupPublicKeysSize, gDefaultAppleGroupPublicKeys);
      *BufferSize = gDefaultAppleGroupPublicKeysSize;
    } else {
      *Buffer = NULL;
    }

    if (*Buffer != NULL) {
      Status = EFI_SUCCESS;
    } else {
      Status = EFI_NOT_FOUND;
    }
  }

  return Status;
}

STATIC
EFI_STATUS
ReadProvisioningData (
  OUT EPID_CERTIFICATE       **EpidCertificate,
  OUT EPID_GROUP_PUBLIC_KEY  **EpidGroupPublicKeys,
  OUT UINT32                 *EpidGroupPublicKeysCount
  )
{
  EFI_STATUS  Status;
  UINTN       EpidCertificateSize;
  UINTN       EpidGroupPublicKeysSize;

  Status = ReadProvisioningDataFile (
    &gAppleEpidCertificateFileGuid,
    (VOID **) EpidCertificate,
    &EpidCertificateSize
    );

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ReadProvisioningDataFile (
    &gAppleEpidGroupPublicKeysFileGuid,
    (VOID **) EpidGroupPublicKeys,
    &EpidGroupPublicKeysSize
    );

  if (EFI_ERROR (Status)) {
    gBS->FreePool (*EpidGroupPublicKeys);
    return Status;
  }

  if (EpidCertificateSize == EPID_CERTIFICATE_SIZE
    && EpidGroupPublicKeysSize % EPID_GROUP_PUBLIC_KEY_SIZE == 0) {
    *EpidGroupPublicKeysCount = (UINT32) (EpidGroupPublicKeysSize / EPID_GROUP_PUBLIC_KEY_SIZE);
    return EFI_SUCCESS;
  }

  gBS->FreePool (*EpidCertificate);
  gBS->FreePool (*EpidGroupPublicKeys);
  return EFI_VOLUME_CORRUPTED;
}

STATIC
VOID
SetProvisioningVariable (
  IN CHAR16  *Variable,
  IN UINT32  Value
  )
{
#ifdef FORCE_PROVISIONING
  (VOID) Variable;
  (VOID) Value;
#else
  gRT->SetVariable (
    Variable,
    &gEfiGlobalVariableGuid,
    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
    sizeof (Value),
    &Value
    );
#endif
}

STATIC
EFI_STATUS
GetGroupPublicKey (
  IN  EPID_GROUP_PUBLIC_KEY  *PublicKeys,
  IN  UINTN                  PublicKeyCount,
  IN  UINT32                 Key,
  OUT EPID_GROUP_PUBLIC_KEY  **ChosenPublicKey
  )
{
  EFI_STATUS  Status;
  UINT32      Index;

  Status = EFI_NOT_FOUND;

  for (Index = 0; Index < PublicKeyCount; ++Index) {
    if (SwapBytes32 (PublicKeys[Index].GroupId) == Key) {
      *ChosenPublicKey = &PublicKeys[Index];
      return EFI_SUCCESS;
    }
  }

  return Status;
}

STATIC
BOOLEAN
IsBuiltinGpuAvailable (
  VOID
  )
{
  EFI_STATUS                       Status;
  UINT32                           Value;
  EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL  *Interface;

  Status = gBS->LocateProtocol (
    &gEfiPciRootBridgeIoProtocolGuid,
    NULL,
    (VOID **) &Interface
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCPAVP: Failed to find PCI root protocol - %r\n", Status));
    return FALSE;
  }

  //
  // IGPU_DEVICE_ID       = 0x2
  // PCI_VENDOR_ID_OFFSET = 0x0
  // (IGPU_DEVICE_ID << 16U | PCI_VENDOR_ID_OFFSET)
  // See EFI_PCI_ADDRESS
  //
  Status = Interface->Pci.Read (
    Interface,
    EfiPciWidthUint32,
    0x20000,
    1,
    &Value
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "OCPAVP: Failed to read from IGPU device - %r\n", Status));
    return FALSE;
  }

  DEBUG ((DEBUG_INFO, "OCPAVP: IGPU is %X\n", Value));

  return Value != 0xFFFFFFFFU;
}

STATIC
EFI_STATUS
NeedsEpidProvisioning (
  VOID
  )
{
  EFI_STATUS  Status;
  UINT32      Data;
  UINTN       DataSize;

  if (IsBuiltinGpuAvailable()) {
    DataSize = sizeof (Data);

    Status = gRT->GetVariable (
      APPLE_EPID_PROVISIONED_VARIABLE_NAME,
      &gEfiGlobalVariableGuid,
      NULL,
      &DataSize,
      &Data
      );

#ifdef FORCE_PROVISIONING
    Data = 0;
#endif

    if (EFI_ERROR (Status) || Data != 1) {
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

STATIC
EFI_STATUS
NeedsFpfProvisioning (
  VOID
  )
{
  EFI_STATUS                   Status;
  UINT32                       Data;
  UINTN                        DataSize;
  APPLE_FPF_CONFIGURATION_HOB  *Hob;

#if 0
  Hob = GetFirstGuidHob (&gAppleFpfConfigurationHobGuid);
#else
  Hob = NULL;
#endif

  DEBUG ((DEBUG_INFO, "OCPAVP: HOB for FPF is %p\n", Hob));

  if (Hob == NULL || Hob->ShouldProvision) {
    DataSize = sizeof (Data);
    Status = gRT->GetVariable (
      APPLE_FPF_PROVISIONED_VARIABLE_NAME,
      &gEfiGlobalVariableGuid,
      NULL,
      &DataSize,
      &Data
      );

    if (EFI_ERROR (Status) || Data != 1) {
      return EFI_SUCCESS;
    }

    return EFI_NOT_FOUND;
  }

  return EFI_UNSUPPORTED;
}

EFI_STATUS
OcPerformEpidProvisioning (
  VOID
  )
{
  EFI_STATUS              Status;
  UINTN                   Index;
  HECI_CLIENT_PROPERTIES  Properties;
  EPID_GROUP_PUBLIC_KEY   *EpidGroupPublicKeys;
  EPID_CERTIFICATE        *EpidCertificate;
  UINT32                  EpidGroupPublicKeysCount;
  UINT32                  EpidStatus;
  UINT32                  EpidGroupId;
  EPID_GROUP_PUBLIC_KEY   *EpidCurrentGroupPublicKey;
  BOOLEAN                 SetVar;

  Status = NeedsEpidProvisioning ();
  DEBUG ((DEBUG_INFO, "OCPAVP: Needs provisioning EPID - %r\n", Status));
  if (EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  Status = HeciLocateProtocol ();
  DEBUG ((DEBUG_INFO, "OCPAVP: HECI protocol lookup - %r\n", Status));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ReadProvisioningData (
    &EpidCertificate,
    &EpidGroupPublicKeys,
    &EpidGroupPublicKeysCount
    );
  DEBUG ((DEBUG_INFO, "OCPAVP: Provisioning data - %r\n", Status));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get Management Engine proccesses namespace.
  // Each App like PAVP or FPF have unique identifier represented as GUID.
  //
  Status = HeciGetClientMap (mMeClientMap, &mMeClientActiveCount);
  DEBUG ((DEBUG_INFO, "OCPAVP: Got %d clients - %r\n", mMeClientActiveCount, Status));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EFI_NOT_FOUND;

  for (Index = 0; Index < mMeClientActiveCount; ++Index) {
    Status = HeciGetClientProperties (
      mMeClientMap[Index],
      &Properties
      );

    DEBUG ((
      DEBUG_INFO,
      "OCPAVP: Client %u has %g protocol - %r\n",
      (UINT32) Index,
      Properties.ProtocolName,
      Status
      ));

    if (EFI_ERROR (Status)) {
      break;
    }

    if (CompareGuid (&Properties.ProtocolName, &gMePavpProtocolGuid)) {
      break;
    }
  }

  if (!EFI_ERROR (Status) && Index != mMeClientActiveCount) {
    DEBUG ((DEBUG_INFO, "OCPAVP: Found application at %u\n", (UINT32) Index));

    Status = HeciConnectToClient (mMeClientMap[Index]);
    if (!EFI_ERROR (Status)) {

      EpidStatus = EpidGroupId = 0;
      Status = HeciPavpRequestProvisioning (&EpidStatus, &EpidGroupId);

      DEBUG ((DEBUG_INFO, "OCPAVP: Got EPID status %X and group id %x - %r\n", EpidStatus, EpidGroupId, Status));
    }

    if (!EFI_ERROR (Status)) {

      if (EpidStatus == EPID_STATUS_PROVISIONED) {
        SetProvisioningVariable (APPLE_EPID_PROVISIONED_VARIABLE_NAME, 1);
      } else if (EpidStatus == EPID_STATUS_CAN_PROVISION) {
        Status = GetGroupPublicKey (
          EpidGroupPublicKeys,
          EpidGroupPublicKeysCount,
          EpidGroupId,
          &EpidCurrentGroupPublicKey
          );

        DEBUG ((DEBUG_INFO, "OCPAVP: Got EPID group public key - %r\n", Status));

        if (!EFI_ERROR (Status)) {
          Status = HeciPavpPerformProvisioning (EpidCertificate, EpidCurrentGroupPublicKey, &SetVar);
          DEBUG ((DEBUG_INFO, "OCPAVP: Sent EPID certificate - %r / %d\n", Status, SetVar));
          if (!EFI_ERROR (Status) || SetVar) {
            SetProvisioningVariable (APPLE_EPID_PROVISIONED_VARIABLE_NAME, 1);
          }
        }
      }
    }

    HeciDisconnectFromClients ();
  } else {
    DEBUG ((DEBUG_INFO, "OCPAVP: No EPID application found\n"));

    if (Index == mMeClientActiveCount) {
      //
      // Do not retry provisioning on incompatible firmware.
      // TODO: Do we really need this?
      //
      SetProvisioningVariable (APPLE_EPID_PROVISIONED_VARIABLE_NAME, 1);
      Status = EFI_NOT_FOUND;
    }
  }

  gBS->FreePool (EpidCertificate);
  gBS->FreePool (EpidGroupPublicKeys);

  return Status;
}

EFI_STATUS
OcPerformFpfProvisioning (
  VOID
  )
{
  EFI_STATUS              Status;
  UINTN                   Index;
  HECI_CLIENT_PROPERTIES  Properties;
  UINT32                  FpfStatus;

  Status = NeedsFpfProvisioning ();
  DEBUG ((DEBUG_INFO, "OCPAVP: Needs provisioning FPF - %r\n", Status));
  if (EFI_ERROR (Status)) {
    return EFI_ALREADY_STARTED;
  }

  Status = HeciLocateProtocol ();
  DEBUG ((DEBUG_INFO, "OCPAVP: HECI protocol lookup - %r\n", Status));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get Management Engine proccesses namespace.
  // Each App like PAVP or FPF have unique identifier represented as GUID.
  //
  Status = HeciGetClientMap (mMeClientMap, &mMeClientActiveCount);
  DEBUG ((DEBUG_INFO, "OCPAVP: Got %d clients - %r\n", mMeClientActiveCount, Status));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EFI_NOT_FOUND;

  for (Index = 0; Index < mMeClientActiveCount; ++Index) {
    Status = HeciGetClientProperties (
      mMeClientMap[Index],
      &Properties
      );

    DEBUG ((
      DEBUG_INFO,
      "OCPAVP: Client %u has %g protocol - %r\n",
      (UINT32) Index,
      Properties.ProtocolName,
      Status
      ));

    if (EFI_ERROR (Status)) {
      break;
    }

    if (CompareGuid (&Properties.ProtocolName, &gMeFpfProtocolGuid)) {
      break;
    }
  }

  if (!EFI_ERROR (Status) && Index != mMeClientActiveCount) {
    DEBUG ((DEBUG_INFO, "OCPAVP: Found application at %u\n", (UINT32) Index));

    Status = HeciConnectToClient (mMeClientMap[Index]);

    //
    // I *think* FPF provisioning locks fuses from further update.
    // For this reason we do not want it.
    //
    if (!EFI_ERROR (Status)) {
      Status = HeciFpfGetStatus (&FpfStatus);
      DEBUG ((DEBUG_INFO, "OCPAVP: Got FPF status %u - %r\n", FpfStatus, Status));
      if (!EFI_ERROR (Status)) {
        if (FpfStatus == 250) {
          Status = HeciFpfProvision (&FpfStatus);
          DEBUG ((DEBUG_INFO, "OCPAVP: Got FPF provisioning %u - %r\n", FpfStatus, Status));
          if (!EFI_ERROR (Status) && FpfStatus == 0) {
            SetProvisioningVariable (APPLE_FPF_PROVISIONED_VARIABLE_NAME, 1);
          } else {
            Status = EFI_DEVICE_ERROR;
          }
        } else {
          Status = EFI_DEVICE_ERROR;
        }
      }

      HeciDisconnectFromClients ();
    }
  } else {
    DEBUG ((DEBUG_INFO, "OCPAVP: No FPF application found\n"));

    if (Index == mMeClientActiveCount) {
      //
      // Do not retry provisioning on incompatible firmware.
      // TODO: Do we really need this?
      //
      SetProvisioningVariable (APPLE_FPF_PROVISIONED_VARIABLE_NAME, 1);
      Status = EFI_NOT_FOUND;
    }
  }

  return Status;
}

STATIC
VOID
OcPerformProvisioning (
  VOID
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO, "OCPAVP: Checking PAVPC register...\n"));

  UINT32 PAVPC = MmPci32 (0, SA_MC_BUS, 0, 0, R_SA_PAVPC);

  DEBUG ((DEBUG_INFO, "OCPAVP: Current PAVPC is %X\n", PAVPC));

  if ((PAVPC & BIT2) == 0) {
    MmPci32 (0, SA_MC_BUS, 0, 0, R_SA_PAVPC) = (PAVPC & (~BIT2)) | BIT4;
    PAVPC = MmPci32 (0, SA_MC_BUS, 0, 0, R_SA_PAVPC);
    DEBUG ((DEBUG_INFO, "OCPAVP: New PAVPC is %X\n", PAVPC));
  }

  DEBUG ((DEBUG_INFO, "OCPAVP: Starting EPID provisioning\n"));

  Status = OcPerformEpidProvisioning ();

  DEBUG ((DEBUG_INFO, "OCPAVP: Done EPID provisioning - %r\n", Status));

#if 0
  DEBUG ((DEBUG_INFO, "OCPAVP: Starting FPF provisioning\n"));

  Status = OcPerformFpfProvisioning ();

  DEBUG ((DEBUG_INFO, "OCPAVP: Done FPF provisioning - %r\n", Status));
#endif
}


EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  OcPerformProvisioning ();

  gBS->Stall (SECONDS_TO_MICROSECONDS (3));

  return EFI_SUCCESS;
}
