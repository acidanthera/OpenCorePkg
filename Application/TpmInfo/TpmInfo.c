/** @file
  Read Intel PTT TPM information.

  Copyright (c) 2021, vit9696. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/


#include <Uefi.h>
#include <Library/IoLib.h>
#include <Library/PrintLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/OcHeciLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <IndustryStandard/PttPtpRegs.h>
#include <IndustryStandard/MkhiMsgs.h>

#define PTT_HCI_TIMEOUT_A       500  ///< Timeout after 500 microseconds
#define PTT_HCI_POLLING_PERIOD  140  ///< Poll register every 140 microseconds

STATIC
EFI_STATUS
MmioRead32Timeout (
  IN      UINTN    Address,
  IN      UINT32   BitSet,
  IN      UINT32   BitClear,
  IN      UINT32   Period,
  IN      UINT32   Timeout,
  OUT     UINT32   *Value  OPTIONAL
  )
{
  UINT32   Tmp;
  UINT32   Waited;
  BOOLEAN  Missing;

  Missing = FALSE;

  for (Waited = 0; Waited < Timeout; Waited += Period){
    Tmp = MmioRead32 (Address);

    if (Tmp == 0xFFFFFFFF) {
      Missing = TRUE;
      continue;
    }

    if ((Tmp & BitSet) == BitSet && (Tmp & BitClear) == 0) {
      if (Value != NULL) {
        *Value = Tmp;
      }
      return EFI_SUCCESS;
    }

    MicroSecondDelay (Period);
  }

  if (Missing) {
    return EFI_NOT_FOUND;
  }

  return EFI_TIMEOUT;
}

STATIC
EFI_STATUS
CheckHeci (
  VOID
  )
{
  EFI_STATUS               Status;
  GEN_GET_FW_CAPSKU        MsgGenGetFwCapsSku;
  GEN_GET_FW_CAPS_SKU_ACK  MsgGenGetFwCapsSkuAck;
  UINT32                   Length;

  Status = HeciLocateProtocol ();

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "TPM: No HECI protocol %r\n", Status));
    return Status;
  }

  MsgGenGetFwCapsSku.MKHIHeader.Data               = 0;
  MsgGenGetFwCapsSku.MKHIHeader.Fields.GroupId     = MKHI_FWCAPS_GROUP_ID;
  MsgGenGetFwCapsSku.MKHIHeader.Fields.Command     = FWCAPS_GET_RULE_CMD;
  MsgGenGetFwCapsSku.MKHIHeader.Fields.IsResponse  = 0;
  MsgGenGetFwCapsSku.Data.RuleId                   = 0;
  Length = sizeof (GEN_GET_FW_CAPSKU);

  Status = HeciSendMessage (
    (UINT32 *) &MsgGenGetFwCapsSku,
    Length,
    BIOS_FIXED_HOST_ADDR,
    HECI_CORE_MESSAGE_ADDR
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "TPM: HECI failed to write %r\n", Status));
    return Status;
  }

  ZeroMem (&MsgGenGetFwCapsSkuAck, sizeof (MsgGenGetFwCapsSkuAck));

  Length = sizeof (MsgGenGetFwCapsSkuAck);
  Status = HeciReadMessage (
    BLOCKING,
    (UINT32 *) &MsgGenGetFwCapsSkuAck,
    &Length
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "TPM: HECI failed to read %r\n", Status));
    return Status;
  }

  if (MsgGenGetFwCapsSkuAck.MKHIHeader.Fields.Command == FWCAPS_GET_RULE_CMD
    && MsgGenGetFwCapsSkuAck.MKHIHeader.Fields.IsResponse == 1
    && MsgGenGetFwCapsSkuAck.MKHIHeader.Fields.Result == 0) {

    if (MsgGenGetFwCapsSkuAck.Data.FWCapSku.Fields.PTT) {
      DEBUG ((DEBUG_WARN, "TPM: HECI PCH supports TPM\n"));
      return EFI_SUCCESS;
    }

    DEBUG ((DEBUG_WARN, "TPM: HECI PCH does NOT support TPM\n"));
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_WARN, "TPM: HECI got invalid response\n"));
  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_WARN, "TPM: Will now verify fTPM (PTT) availability\n"));

  Status = CheckHeci ();

  DEBUG ((DEBUG_WARN, "TPM: HECI returned %r\n", Status));

  Status = MmioRead32Timeout (
    R_PTT_HCI_BASE_ADDRESS_A + R_CRB_CONTROL_STS,
    B_CRB_CONTROL_STS_TPM_STATUS,
    V_PTT_HCI_IGNORE_BITS,
    PTT_HCI_POLLING_PERIOD,
    PTT_HCI_TIMEOUT_A,
    NULL
    );

  DEBUG ((
    DEBUG_WARN,
    "TPM: Discovered status at 0x%08X is %r\n",
    R_PTT_HCI_BASE_ADDRESS_A,
    Status
    ));

  Status = MmioRead32Timeout (
    R_PTT_HCI_BASE_ADDRESS_B + R_CRB_CONTROL_STS,
    B_CRB_CONTROL_STS_TPM_STATUS,
    V_PTT_HCI_IGNORE_BITS,
    PTT_HCI_POLLING_PERIOD,
    PTT_HCI_TIMEOUT_A,
    NULL
    );

  DEBUG ((
    DEBUG_WARN,
    "TPM: Discovered status at 0x%08X is %r\n",
    R_PTT_HCI_BASE_ADDRESS_B,
    Status
    ));

  return EFI_SUCCESS;
}
