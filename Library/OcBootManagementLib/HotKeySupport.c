/** @file
  Copyright (C) 2019, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include "BootManagementInternal.h"

#include <Guid/AppleVariable.h>
#include <IndustryStandard/AppleCsrConfig.h>
#include <Protocol/AppleKeyMapAggregator.h>

#include <Library/BaseLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcTimerLib.h>
#include <Library/OcAppleKeyMapLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

VOID
OcLoadPickerHotKeys (
  IN OUT OC_PICKER_CONTEXT  *Context
  )
{
  EFI_STATUS                         Status;
  APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *KeyMap;

  UINTN                              NumKeys;
  APPLE_MODIFIER_MAP                 Modifiers;
  APPLE_KEY_CODE                     Keys[OC_KEY_MAP_DEFAULT_SIZE];

  BOOLEAN                            HasCommand;
  BOOLEAN                            HasEscape;
  BOOLEAN                            HasOption;
  BOOLEAN                            HasKeyP;
  BOOLEAN                            HasKeyR;
  BOOLEAN                            HasKeyX;

  if (Context->TakeoffDelay > 0) {
    gBS->Stall (Context->TakeoffDelay);
  }

  Status = gBS->LocateProtocol (
    &gAppleKeyMapAggregatorProtocolGuid,
    NULL,
    (VOID **) &KeyMap
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCB: Missing AppleKeyMapAggregator - %r\n", Status));
    return;
  }

  NumKeys = ARRAY_SIZE (Keys);
  Status = KeyMap->GetKeyStrokes (
    KeyMap,
    &Modifiers,
    &NumKeys,
    Keys
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCB: GetKeyStrokes - %r\n", Status));
    return;
  }

  //
  // I do not like this code a little, as it is prone to race conditions during key presses.
  // For the good false positives are not too critical here, and in reality users are not that fast.
  //
  // Reference key list:
  // https://support.apple.com/HT201255
  // https://support.apple.com/HT204904
  //
  // We are slightly more permissive than AppleBds, as we permit combining keys.
  //

  HasCommand = (Modifiers & (APPLE_MODIFIER_LEFT_COMMAND | APPLE_MODIFIER_RIGHT_COMMAND)) != 0;
  HasOption  = (Modifiers & (APPLE_MODIFIER_LEFT_OPTION  | APPLE_MODIFIER_RIGHT_OPTION)) != 0;
  HasEscape  = OcKeyMapHasKey (Keys, NumKeys, AppleHidUsbKbUsageKeyEscape);
  HasKeyP    = OcKeyMapHasKey (Keys, NumKeys, AppleHidUsbKbUsageKeyP);
  HasKeyR    = OcKeyMapHasKey (Keys, NumKeys, AppleHidUsbKbUsageKeyR);
  HasKeyX    = OcKeyMapHasKey (Keys, NumKeys, AppleHidUsbKbUsageKeyX);

  if (HasOption && HasCommand && HasKeyP && HasKeyR) {
    DEBUG ((DEBUG_INFO, "OCB: CMD+OPT+P+R causes NVRAM reset\n"));
    Context->PickerCommand = OcPickerResetNvram;
  } else if (HasCommand && HasKeyR) {
    DEBUG ((DEBUG_INFO, "OCB: CMD+R causes recovery to boot\n"));
    Context->PickerCommand = OcPickerBootAppleRecovery;
  } else if (HasKeyX) {
    DEBUG ((DEBUG_INFO, "OCB: X causes macOS to boot\n"));
    Context->PickerCommand = OcPickerBootApple;
  } else if (HasOption) {
    DEBUG ((DEBUG_INFO, "OCB: OPT causes picker to show\n"));
    Context->PickerCommand = OcPickerShowPicker;
  } else if (HasEscape) {
    DEBUG ((DEBUG_INFO, "OCB: ESC causes picker to show as OC extension\n"));
    Context->PickerCommand = OcPickerShowPicker;
  } else {
    //
    // In addition to these overrides we always have ShowPicker = YES in config.
    // The following keys are not implemented:
    // C - CD/DVD boot, legacy that is gone now.
    // D - Diagnostics, could implement dumping stuff here in some future,
    //     but we will need to store the data before handling the key.
    //     Should also be DEBUG only for security reasons.
    // N - Network boot, simply not supported (and bad for security).
    // T - Target disk mode, simply not supported (and bad for security).
    //
  }
}

INTN
EFIAPI
OcGetAppleKeyIndex (
  IN OUT OC_PICKER_CONTEXT                  *Context,
  IN     APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *KeyMap,
     OUT BOOLEAN                            *SetDefault  OPTIONAL
  )
{
  EFI_STATUS                         Status;
  APPLE_KEY_CODE                     KeyCode;

  UINTN                              NumKeys;
  APPLE_MODIFIER_MAP                 Modifiers;
  APPLE_KEY_CODE                     Keys[OC_KEY_MAP_DEFAULT_SIZE];

  BOOLEAN                            HasCommand;
  BOOLEAN                            HasShift;
  BOOLEAN                            HasKeyC;
  BOOLEAN                            HasKeyK;
  BOOLEAN                            HasKeyS;
  BOOLEAN                            HasKeyV;
  BOOLEAN                            HasKeyMinus;
  BOOLEAN                            WantsZeroSlide;
  BOOLEAN                            WantsDefault;
  UINT32                             CsrActiveConfig;
  UINTN                              CsrActiveConfigSize;

  NumKeys = ARRAY_SIZE (Keys);
  Status = KeyMap->GetKeyStrokes (
    KeyMap,
    &Modifiers,
    &NumKeys,
    Keys
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCB: GetKeyStrokes - %r\n", Status));
    return OC_INPUT_INVALID;
  }

  //
  // Handle key combinations.
  //
  if (Context->PollAppleHotKeys) {
    HasCommand = (Modifiers & (APPLE_MODIFIER_LEFT_COMMAND | APPLE_MODIFIER_RIGHT_COMMAND)) != 0;
    HasShift   = (Modifiers & (APPLE_MODIFIER_LEFT_SHIFT | APPLE_MODIFIER_RIGHT_SHIFT)) != 0;
    HasKeyC    = OcKeyMapHasKey (Keys, NumKeys, AppleHidUsbKbUsageKeyC);
    HasKeyK    = OcKeyMapHasKey (Keys, NumKeys, AppleHidUsbKbUsageKeyK);
    HasKeyS    = OcKeyMapHasKey (Keys, NumKeys, AppleHidUsbKbUsageKeyS);
    HasKeyV    = OcKeyMapHasKey (Keys, NumKeys, AppleHidUsbKbUsageKeyV);
    //
    // Checking for PAD minus is our extension to support more keyboards.
    //
    HasKeyMinus = OcKeyMapHasKey (Keys, NumKeys, AppleHidUsbKbUsageKeyMinus)
      || OcKeyMapHasKey (Keys, NumKeys, AppleHidUsbKbUsageKeyPadMinus);

    //
    // Shift is always valid and enables Safe Mode.
    //
    if (HasShift) {
      if (OcGetArgumentFromCmd (Context->AppleBootArgs, "-x", L_STR_LEN ("-x"), NULL) == NULL) {
        DEBUG ((DEBUG_INFO, "OCB: Shift means -x\n"));
        OcAppendArgumentToCmd (Context, Context->AppleBootArgs, "-x", L_STR_LEN ("-x"));
      }
      return OC_INPUT_INTERNAL;
    }

    //
    // CMD+V is always valid and enables Verbose Mode.
    //
    if (HasCommand && HasKeyV) {
      if (OcGetArgumentFromCmd (Context->AppleBootArgs, "-v", L_STR_LEN ("-v"), NULL) == NULL) {
        DEBUG ((DEBUG_INFO, "OCB: CMD+V means -v\n"));
        OcAppendArgumentToCmd (Context, Context->AppleBootArgs, "-v", L_STR_LEN ("-v"));
      }
      return OC_INPUT_INTERNAL;
    }

    //
    // CMD+C+MINUS is always valid and disables compatibility check.
    //
    if (HasCommand && HasKeyC && HasKeyMinus) {
      if (OcGetArgumentFromCmd (Context->AppleBootArgs, "-no_compat_check", L_STR_LEN ("-no_compat_check"), NULL) == NULL) {
        DEBUG ((DEBUG_INFO, "OCB: CMD+C+MINUS means -no_compat_check\n"));
        OcAppendArgumentToCmd (Context, Context->AppleBootArgs, "-no_compat_check", L_STR_LEN ("-no_compat_check"));
      }
      return OC_INPUT_INTERNAL;
    }

    //
    // CMD+K is always valid for new macOS and means force boot to release kernel.
    //
    if (HasCommand && HasKeyK) {
      if (AsciiStrStr (Context->AppleBootArgs, "kcsuffix=release") == NULL) {
        DEBUG ((DEBUG_INFO, "OCB: CMD+K means kcsuffix=release\n"));
        OcAppendArgumentToCmd (Context, Context->AppleBootArgs, "kcsuffix=release", L_STR_LEN ("kcsuffix=release"));
      }
      return OC_INPUT_INTERNAL;
    }

    //
    // boot.efi also checks for CMD+X, but I have no idea what it is for.
    //

    //
    // boot.efi requires unrestricted NVRAM just for CMD+S+MINUS, and CMD+S
    // does not work at all on T2 macs. For CMD+S we simulate T2 behaviour with
    // DisableSingleUser Booter quirk if necessary.
    // Ref: https://support.apple.com/HT201573
    //
    if (HasCommand && HasKeyS) {
      WantsZeroSlide = HasKeyMinus;

      if (WantsZeroSlide) {
        CsrActiveConfig     = 0;
        CsrActiveConfigSize = sizeof (CsrActiveConfig);
        Status = gRT->GetVariable (
          L"csr-active-config",
          &gAppleBootVariableGuid,
          NULL,
          &CsrActiveConfigSize,
          &CsrActiveConfig
          );
        //
        // FIXME: CMD+S+Minus behaves as CMD+S when "slide=0" is not supported
        //        by the SIP configuration. This might be an oversight, but is
        //        consistent with the boot.efi implementation.
        //
        WantsZeroSlide = !EFI_ERROR (Status) && (CsrActiveConfig & CSR_ALLOW_UNRESTRICTED_NVRAM) != 0;
      }

      if (WantsZeroSlide) {
        if (AsciiStrStr (Context->AppleBootArgs, "slide=0") == NULL) {
          DEBUG ((DEBUG_INFO, "OCB: CMD+S+MINUS means slide=0\n"));
          OcAppendArgumentToCmd (Context, Context->AppleBootArgs, "slide=0", L_STR_LEN ("slide=0"));
        }
      } else if (OcGetArgumentFromCmd (Context->AppleBootArgs, "-s", L_STR_LEN ("-s"), NULL) == NULL) {
        DEBUG ((DEBUG_INFO, "OCB: CMD+S means -s\n"));
        OcAppendArgumentToCmd (Context, Context->AppleBootArgs, "-s", L_STR_LEN ("-s"));
      }
      return OC_INPUT_INTERNAL;
    }
  }

  //
  // Handle VoiceOver.
  //
  if ((Modifiers & (APPLE_MODIFIER_LEFT_COMMAND | APPLE_MODIFIER_RIGHT_COMMAND)) != 0
    && OcKeyMapHasKey (Keys, NumKeys, AppleHidUsbKbUsageKeyF5)) {
    OcKeyMapFlush (KeyMap, 0, TRUE);
    return OC_INPUT_VOICE_OVER;
  }

  //
  // Handle reload menu.
  //
  if (OcKeyMapHasKey (Keys, NumKeys, AppleHidUsbKbUsageKeyEscape)
   || OcKeyMapHasKey (Keys, NumKeys, AppleHidUsbKbUsageKeyZero)) {
    OcKeyMapFlush (KeyMap, 0, TRUE);
    return OC_INPUT_ABORTED;
  }

  if (OcKeyMapHasKey (Keys, NumKeys, AppleHidUsbKbUsageKeySpaceBar)) {
    OcKeyMapFlush (KeyMap, 0, TRUE);
    return OC_INPUT_MORE;
  }

  //
  // Default update is desired for Ctrl+Index and Ctrl+Enter.
  //
  WantsDefault = Modifiers != 0 && (Modifiers & ~(APPLE_MODIFIER_LEFT_CONTROL | APPLE_MODIFIER_RIGHT_CONTROL)) == 0;

  //
  // Check exact match on index strokes.
  //
  if ((Modifiers == 0 || WantsDefault) && NumKeys == 1) {
    if (Keys[0] == AppleHidUsbKbUsageKeyEnter
      || Keys[0] == AppleHidUsbKbUsageKeyReturn
      || Keys[0] == AppleHidUsbKbUsageKeyPadEnter) {
      if (WantsDefault && SetDefault != NULL) {
        *SetDefault = TRUE;
      }
      OcKeyMapFlush (KeyMap, Keys[0], TRUE);
      return OC_INPUT_CONTINUE;
    }

    if (Keys[0] == AppleHidUsbKbUsageKeyUpArrow) {
      OcKeyMapFlush (KeyMap, Keys[0], TRUE);
      return OC_INPUT_UP;
    }

    if (Keys[0] == AppleHidUsbKbUsageKeyDownArrow) {
      OcKeyMapFlush (KeyMap, Keys[0], TRUE);
      return OC_INPUT_DOWN;
    }

    if (Keys[0] == AppleHidUsbKbUsageKeyLeftArrow) {
      OcKeyMapFlush (KeyMap, Keys[0], TRUE);
      return OC_INPUT_LEFT;
    }

    if (Keys[0] == AppleHidUsbKbUsageKeyRightArrow) {
      OcKeyMapFlush (KeyMap, Keys[0], TRUE);
      return OC_INPUT_RIGHT;
    }

    if (Keys[0] == AppleHidUsbKbUsageKeyPgUp
      || Keys[0] == AppleHidUsbKbUsageKeyHome) {
      OcKeyMapFlush (KeyMap, Keys[0], TRUE);
      return OC_INPUT_TOP;
    }

    if (Keys[0] == AppleHidUsbKbUsageKeyPgDn
      || Keys[0] == AppleHidUsbKbUsageKeyEnd) {
      OcKeyMapFlush (KeyMap, Keys[0], TRUE);
      return OC_INPUT_BOTTOM;
    }

    STATIC_ASSERT (AppleHidUsbKbUsageKeyF1 + 11 == AppleHidUsbKbUsageKeyF12, "Unexpected encoding");
    if (Keys[0] >= AppleHidUsbKbUsageKeyF1 && Keys[0] <= AppleHidUsbKbUsageKeyF12) {
      OcKeyMapFlush (KeyMap, Keys[0], TRUE);
      return OC_INPUT_FUNCTIONAL (Keys[0] - AppleHidUsbKbUsageKeyF1 + 1);
    }

    STATIC_ASSERT (AppleHidUsbKbUsageKeyF13 + 11 == AppleHidUsbKbUsageKeyF24, "Unexpected encoding");
    if (Keys[0] >= AppleHidUsbKbUsageKeyF13 && Keys[0] <= AppleHidUsbKbUsageKeyF24) {
      OcKeyMapFlush (KeyMap, Keys[0], TRUE);
      return OC_INPUT_FUNCTIONAL (Keys[0] - AppleHidUsbKbUsageKeyF13 + 13);
    }

    STATIC_ASSERT (AppleHidUsbKbUsageKeyOne + 8 == AppleHidUsbKbUsageKeyNine, "Unexpected encoding");
    for (KeyCode = AppleHidUsbKbUsageKeyOne; KeyCode <= AppleHidUsbKbUsageKeyNine; ++KeyCode) {
      if (OcKeyMapHasKey (Keys, NumKeys, KeyCode)) {
        if (WantsDefault && SetDefault != NULL) {
          *SetDefault = TRUE;
        }
        OcKeyMapFlush (KeyMap, Keys[0], TRUE);
        return (INTN) (KeyCode - AppleHidUsbKbUsageKeyOne);
      }
    }

    STATIC_ASSERT (AppleHidUsbKbUsageKeyA + 25 == AppleHidUsbKbUsageKeyZ, "Unexpected encoding");
    for (KeyCode = AppleHidUsbKbUsageKeyA; KeyCode <= AppleHidUsbKbUsageKeyZ; ++KeyCode) {
      if (OcKeyMapHasKey (Keys, NumKeys, KeyCode)) {
        if (WantsDefault && SetDefault != NULL) {
          *SetDefault = TRUE;
        }
        OcKeyMapFlush (KeyMap, Keys[0], TRUE);
        return (INTN) (KeyCode - AppleHidUsbKbUsageKeyA + 9);
      }
    }
  }

  if (NumKeys > 0) {
    return OC_INPUT_INVALID;
  }

  return OC_INPUT_TIMEOUT;
}

INTN
OcWaitForAppleKeyIndex (
  IN OUT OC_PICKER_CONTEXT                  *Context,
  IN     APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *KeyMap,
  IN     UINTN                              Timeout,
     OUT BOOLEAN                            *SetDefault  OPTIONAL
  )
{
  INTN                               ResultingKey;
  UINT64                             CurrTime;
  UINT64                             EndTime;

  //
  // These hotkeys are normally parsed by boot.efi, and they work just fine
  // when ShowPicker is disabled. On some BSPs, however, they may fail badly
  // when ShowPicker is enabled, and for this reason we support these hotkeys
  // within picker itself.
  //

  CurrTime  = GetTimeInNanoSecond (GetPerformanceCounter ());
  EndTime   = CurrTime + Timeout * 1000000ULL;

  if (SetDefault != NULL) {
    *SetDefault = FALSE;
  }

  while (Timeout == 0 || CurrTime == 0 || CurrTime < EndTime) {
    CurrTime    = GetTimeInNanoSecond (GetPerformanceCounter ());  

    ResultingKey = OcGetAppleKeyIndex (Context, KeyMap, SetDefault);

    //
    // Requested for another iteration, handled Apple hotkey.
    //
    if (ResultingKey == OC_INPUT_INTERNAL) {
      continue;
    }

    //
    // Abort the timeout when unrecognised keys are pressed.
    //
    if (Timeout != 0 && ResultingKey == OC_INPUT_INVALID) {
      return OC_INPUT_INVALID;
    }

    //
    // Found key, return it.
    //
    if (ResultingKey != OC_INPUT_INVALID && ResultingKey != OC_INPUT_TIMEOUT) {
      return ResultingKey;
    }

    MicroSecondDelay (10);
  }

  return OC_INPUT_TIMEOUT;
}
