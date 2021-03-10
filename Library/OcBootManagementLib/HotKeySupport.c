/** @file
  Copyright (C) 2019, vit9696. All rights reserved.
  Additions (downkeys support) copyright (C) 2021 Mike Beaton. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#if defined(OC_TARGET_DEBUG) || defined(OC_TARGET_NOOPT)
//#define DEBUG_DETAIL
#endif
#define OC_SHOW_RUNNING_KEYS

#include "BootManagementInternal.h"

#include <Guid/AppleVariable.h>
#include <IndustryStandard/AppleCsrConfig.h>
#include <Protocol/AppleKeyMapAggregator.h>

#include <Library/BaseLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcTimerLib.h>
#include <Library/OcAppleKeyMapLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcConfigurationLib.h>
#include <Library/OcTemplateLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

STATIC BOOLEAN              mUseDownkeys;

OC_KEY_REPEAT_CONTEXT       mRepeatContext;
OC_HELD_KEY_INFO            mRepeatKeysHeld[OC_HELD_KEYS_DEFAULT_SIZE];

OC_KEY_REPEAT_CONTEXT       mDoNotRepeatContext;
OC_HELD_KEY_INFO            mDoNotRepeatKeysHeld[OC_HELD_KEYS_DEFAULT_SIZE];

//
// Get hotkeys pressed at load
//
APPLE_KEY_MAP_AGGREGATOR_PROTOCOL *
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
    DEBUG ((DEBUG_ERROR, "OCHK: Missing AppleKeyMapAggregator - %r\n", Status));
    return NULL;
  }

  NumKeys = ARRAY_SIZE (Keys);
  Status = KeyMap->GetKeyStrokes (
    KeyMap,
    &Modifiers,
    &NumKeys,
    Keys
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCHK: GetKeyStrokes - %r\n", Status));
    return KeyMap;
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
    DEBUG ((DEBUG_INFO, "OCHK: CMD+OPT+P+R causes NVRAM reset\n"));
    Context->PickerCommand = OcPickerResetNvram;
  } else if (HasCommand && HasKeyR) {
    DEBUG ((DEBUG_INFO, "OCHK: CMD+R causes recovery to boot\n"));
    Context->PickerCommand = OcPickerBootAppleRecovery;
  } else if (HasKeyX) {
    DEBUG ((DEBUG_INFO, "OCHK: X causes macOS to boot\n"));
    Context->PickerCommand = OcPickerBootApple;
  } else if (HasOption) {
    DEBUG ((DEBUG_INFO, "OCHK: OPT causes picker to show\n"));
    Context->PickerCommand = OcPickerShowPicker;
  } else if (HasEscape) {
    DEBUG ((DEBUG_INFO, "OCHK: ESC causes picker to show as OC extension\n"));
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

  return KeyMap;
}

VOID
OcInitDownkeys (
    IN OC_UEFI_INPUT                          *Input,
    IN APPLE_KEY_MAP_AGGREGATOR_PROTOCOL      *KeyMap
  )
{
  EFI_STATUS                 Status;
  CONST CHAR8                *DownkeysHandlerStr;

  //
  // NB initialise this whether downkeys support is used for kb loop or not
  //
  OcInitRunningKeys(GetTscFrequency());

  //
  // Failsafe and Auto = opposite of KeySupport, which is recommended for most people
  //
  mUseDownkeys = !Input->KeySupport;

  DownkeysHandlerStr = OC_BLOB_GET (&Input->DownkeysHandler);
  if (AsciiStrCmp (DownkeysHandlerStr, "Enabled") == 0) {
    mUseDownkeys = TRUE;
  } else if (AsciiStrCmp (DownkeysHandlerStr, "Disabled") == 0) {
    mUseDownkeys = FALSE;
  } else if (AsciiStrCmp (DownkeysHandlerStr, "Auto") != 0) {
    DEBUG ((DEBUG_WARN, "OCHK: Invalid downkeys handler mode %a\n", DownkeysHandlerStr));
  }

  if (mUseDownkeys) {
    DEBUG ((DEBUG_INFO, "OCHK: InitDownkeys\n"));

    //
    // Standard repeat rate
    //
    Status = OcInitKeyRepeatContext(
      &mRepeatContext,
      KeyMap,
      ARRAY_SIZE (mRepeatKeysHeld),
      mRepeatKeysHeld,
      OC_DOWNKEYS_DEFAULT_INITIAL_DELAY * 1000000ULL,
      OC_DOWNKEYS_DEFAULT_SUBSEQUENT_DELAY * 1000000ULL,
      TRUE
    );
      
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCHK: Init repeat context - %r\n", Status));
    }

    //
    // No repeat context, e.g. for toggle and special command keys
    //
    Status = OcInitKeyRepeatContext(
      &mDoNotRepeatContext,
      KeyMap,
      ARRAY_SIZE (mDoNotRepeatKeysHeld),
      mDoNotRepeatKeysHeld,
      0,
      0,
      TRUE
    );
      
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "OCHK: Init non-repeat context - %r\n", Status));
    }
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
  UINTN                              NumKeysUp;
  UINTN                              NumKeysDoNotRepeat;
  APPLE_KEY_CODE                     KeysDoNotRepeat[OC_KEY_MAP_DEFAULT_SIZE];
  BOOLEAN                            EnableFlush;

  BOOLEAN                            HasCommand;
  BOOLEAN                            HasShift;
  BOOLEAN                            HasKeyC;
  BOOLEAN                            HasKeyK;
  BOOLEAN                            HasKeyS;
  BOOLEAN                            HasKeyV;
  BOOLEAN                            HasKeyMinus;
  BOOLEAN                            WantsZeroSlide;
  UINT32                             CsrActiveConfig;
  UINTN                              CsrActiveConfigSize;
  UINT64                             CurrentTime;

  ASSERT (ARRAY_SIZE (KeysDoNotRepeat) >= ARRAY_SIZE (Keys)); // for CopyMem in legacy codepath

  if (SetDefault != NULL) {
    *SetDefault = 0;
  }

  if (mUseDownkeys) {
    // Downkeys support, helps with fluent animation in OpenCanopy (will not hang waiting for keys), but
    // does not include all fixes for legacy hardware present in previous approach (see esp. OcKeyMapFlush),
    // and may function unpredictably in terms of key repeat behaviour on systems which have EFI driver level
    // key repeat.
    //
    // TODO: Investigate using raw TSC timer instead
    //
    CurrentTime = GetTimeInNanoSecond (GetPerformanceCounter ());

    //
    // TO DO(?): We are using last collected modifiers for all keys, but should be correct or close enough not to matter
    //
    NumKeysUp           = 0;
    NumKeys             = ARRAY_SIZE (Keys);
    Status = OcGetUpDownKeys (
      &mRepeatContext,
      &Modifiers,
      &NumKeysUp, NULL,
      &NumKeys, Keys,
      CurrentTime
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OCHK: GetUpDownKeys for RepeatContext - %r\n", Status));
      return OC_INPUT_INVALID;
    }

#if defined(OC_SHOW_RUNNING_KEYS)
    OcShowRunningKeys (NumKeysUp, NumKeys, mRepeatContext.NumKeysHeld, Modifiers, L' ', TRUE); // normal
#endif

    NumKeysUp           = 0;
    NumKeysDoNotRepeat  = ARRAY_SIZE (KeysDoNotRepeat);
    Status = OcGetUpDownKeys (
      &mDoNotRepeatContext,
      &Modifiers,
      &NumKeysUp, NULL,
      &NumKeysDoNotRepeat, KeysDoNotRepeat,
      CurrentTime
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OCHK: GetUpDownKeys for DoNotRepeatContext - %r\n", Status));
      return OC_INPUT_INVALID;
    }

    EnableFlush = FALSE;
  } else {
    // Legacy support, tuned to work correctly on many older systems, but hangs in
    // call to OcKeyMapFlush below until all keys (including control keys) come up,
    // on newer and Apple hardware, preventing fluid animation in OpenCanopy.
    //
    NumKeys             = ARRAY_SIZE (Keys);
    DETAIL_DEBUG ((DEBUG_INFO, "OCHK: 1 %d\n", NumKeys));
    Status = KeyMap->GetKeyStrokes (
      KeyMap,
      &Modifiers,
      &NumKeys,
      Keys
      );

    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_WARN, "OCHK: GetKeyStrokes - %r\n", Status));
      return OC_INPUT_INVALID;
    }
    DETAIL_DEBUG ((DEBUG_INFO, "OCHK: 2 %d\n", NumKeys));

#if defined(OC_SHOW_RUNNING_KEYS)
    OcShowRunningKeys (0, 0, NumKeys, Modifiers, L' ', FALSE); // normal
#endif

    CopyMem (KeysDoNotRepeat, Keys, sizeof(Keys[0]) * NumKeys);
    NumKeysDoNotRepeat = NumKeys;

    EnableFlush = TRUE;
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
        DEBUG ((DEBUG_INFO, "OCHK: Shift means -x\n"));
        OcAppendArgumentToCmd (Context, Context->AppleBootArgs, "-x", L_STR_LEN ("-x"));
      }
      return OC_INPUT_INTERNAL;
    }

    //
    // CMD+V is always valid and enables Verbose Mode.
    //
    if (HasCommand && HasKeyV) {
      if (OcGetArgumentFromCmd (Context->AppleBootArgs, "-v", L_STR_LEN ("-v"), NULL) == NULL) {
        DEBUG ((DEBUG_INFO, "OCHK: CMD+V means -v\n"));
        OcAppendArgumentToCmd (Context, Context->AppleBootArgs, "-v", L_STR_LEN ("-v"));
      }
      return OC_INPUT_INTERNAL;
    }

    //
    // CMD+C+MINUS is always valid and disables compatibility check.
    //
    if (HasCommand && HasKeyC && HasKeyMinus) {
      if (OcGetArgumentFromCmd (Context->AppleBootArgs, "-no_compat_check", L_STR_LEN ("-no_compat_check"), NULL) == NULL) {
        DEBUG ((DEBUG_INFO, "OCHK: CMD+C+MINUS means -no_compat_check\n"));
        OcAppendArgumentToCmd (Context, Context->AppleBootArgs, "-no_compat_check", L_STR_LEN ("-no_compat_check"));
      }
      return OC_INPUT_INTERNAL;
    }

    //
    // CMD+K is always valid for new macOS and means force boot to release kernel.
    //
    if (HasCommand && HasKeyK) {
      if (AsciiStrStr (Context->AppleBootArgs, "kcsuffix=release") == NULL) {
        DEBUG ((DEBUG_INFO, "OCHK: CMD+K means kcsuffix=release\n"));
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
          DEBUG ((DEBUG_INFO, "OCHK: CMD+S+MINUS means slide=0\n"));
          OcAppendArgumentToCmd (Context, Context->AppleBootArgs, "slide=0", L_STR_LEN ("slide=0"));
        }
      } else if (OcGetArgumentFromCmd (Context->AppleBootArgs, "-s", L_STR_LEN ("-s"), NULL) == NULL) {
        DEBUG ((DEBUG_INFO, "OCHK: CMD+S means -s\n"));
        OcAppendArgumentToCmd (Context, Context->AppleBootArgs, "-s", L_STR_LEN ("-s"));
      }
      return OC_INPUT_INTERNAL;
    }
  }

  //
  // Handle VoiceOver - non-repeating.
  //
  if ((Modifiers & (APPLE_MODIFIER_LEFT_COMMAND | APPLE_MODIFIER_RIGHT_COMMAND)) != 0
    && OcKeyMapHasKey (KeysDoNotRepeat, NumKeysDoNotRepeat, AppleHidUsbKbUsageKeyF5)) {
    OcKeyMapFlush (KeyMap, 0, TRUE, EnableFlush, TRUE);
    return OC_INPUT_VOICE_OVER;
  }

  //
  // Handle reload menu - non-repeating.
  //
  if (OcKeyMapHasKey (KeysDoNotRepeat, NumKeysDoNotRepeat, AppleHidUsbKbUsageKeyEscape)
   || OcKeyMapHasKey (KeysDoNotRepeat, NumKeysDoNotRepeat, AppleHidUsbKbUsageKeyZero)) {
    OcKeyMapFlush (KeyMap, 0, TRUE, EnableFlush, TRUE);
    return OC_INPUT_ABORTED;
  }

  //
  // Handle show or toggle auxiliary - non-repeating.
  //
  if (OcKeyMapHasKey (KeysDoNotRepeat, NumKeysDoNotRepeat, AppleHidUsbKbUsageKeySpaceBar)) {
    OcKeyMapFlush (KeyMap, 0, TRUE, EnableFlush, TRUE);
    return OC_INPUT_MORE;
  }

  //
  // Default update is desired for Ctrl+Index and Ctrl+Enter.
  //
  if (SetDefault != NULL
    && Modifiers != 0
    && (Modifiers & ~(APPLE_MODIFIER_LEFT_CONTROL | APPLE_MODIFIER_RIGHT_CONTROL)) == 0) {
      *SetDefault = TRUE;
  }

  //
  // Check exact match on index strokes.
  //
  if ((Modifiers == 0 || (SetDefault != NULL && *SetDefault)) && NumKeys == 1) {
    if (Keys[0] == AppleHidUsbKbUsageKeyEnter
      || Keys[0] == AppleHidUsbKbUsageKeyReturn
      || Keys[0] == AppleHidUsbKbUsageKeyPadEnter) {
      OcKeyMapFlush (KeyMap, Keys[0], TRUE, EnableFlush, TRUE);
      return OC_INPUT_CONTINUE;
    }

    if (Keys[0] == AppleHidUsbKbUsageKeyUpArrow) {
      OcKeyMapFlush (KeyMap, Keys[0], TRUE, EnableFlush, TRUE);
      return OC_INPUT_UP;
    }

    if (Keys[0] == AppleHidUsbKbUsageKeyDownArrow) {
      OcKeyMapFlush (KeyMap, Keys[0], TRUE, EnableFlush, TRUE);
      return OC_INPUT_DOWN;
    }

    if (Keys[0] == AppleHidUsbKbUsageKeyLeftArrow) {
      OcKeyMapFlush (KeyMap, Keys[0], TRUE, EnableFlush, TRUE);
      return OC_INPUT_LEFT;
    }

    if (Keys[0] == AppleHidUsbKbUsageKeyRightArrow) {
      OcKeyMapFlush (KeyMap, Keys[0], TRUE, EnableFlush, TRUE);
      return OC_INPUT_RIGHT;
    }

    if (Keys[0] == AppleHidUsbKbUsageKeyPgUp
      || Keys[0] == AppleHidUsbKbUsageKeyHome) {
      OcKeyMapFlush (KeyMap, Keys[0], TRUE, EnableFlush, TRUE);
      return OC_INPUT_TOP;
    }

    if (Keys[0] == AppleHidUsbKbUsageKeyPgDn
      || Keys[0] == AppleHidUsbKbUsageKeyEnd) {
      OcKeyMapFlush (KeyMap, Keys[0], TRUE, EnableFlush, TRUE);
      return OC_INPUT_BOTTOM;
    }

    STATIC_ASSERT (AppleHidUsbKbUsageKeyF1 + 11 == AppleHidUsbKbUsageKeyF12, "Unexpected encoding");
    if (Keys[0] >= AppleHidUsbKbUsageKeyF1 && Keys[0] <= AppleHidUsbKbUsageKeyF12) {
      OcKeyMapFlush (KeyMap, Keys[0], TRUE, EnableFlush, TRUE);
      return OC_INPUT_FUNCTIONAL (Keys[0] - AppleHidUsbKbUsageKeyF1 + 1);
    }

    STATIC_ASSERT (AppleHidUsbKbUsageKeyF13 + 11 == AppleHidUsbKbUsageKeyF24, "Unexpected encoding");
    if (Keys[0] >= AppleHidUsbKbUsageKeyF13 && Keys[0] <= AppleHidUsbKbUsageKeyF24) {
      OcKeyMapFlush (KeyMap, Keys[0], TRUE, EnableFlush, TRUE);
      return OC_INPUT_FUNCTIONAL (Keys[0] - AppleHidUsbKbUsageKeyF13 + 13);
    }

    STATIC_ASSERT (AppleHidUsbKbUsageKeyOne + 8 == AppleHidUsbKbUsageKeyNine, "Unexpected encoding");
    for (KeyCode = AppleHidUsbKbUsageKeyOne; KeyCode <= AppleHidUsbKbUsageKeyNine; ++KeyCode) {
      if (OcKeyMapHasKey (Keys, NumKeys, KeyCode)) {
        OcKeyMapFlush (KeyMap, Keys[0], TRUE, EnableFlush, TRUE);
        return (INTN) (KeyCode - AppleHidUsbKbUsageKeyOne);
      }
    }

    STATIC_ASSERT (AppleHidUsbKbUsageKeyA + 25 == AppleHidUsbKbUsageKeyZ, "Unexpected encoding");
    for (KeyCode = AppleHidUsbKbUsageKeyA; KeyCode <= AppleHidUsbKbUsageKeyZ; ++KeyCode) {
      if (OcKeyMapHasKey (Keys, NumKeys, KeyCode)) {
        OcKeyMapFlush (KeyMap, Keys[0], TRUE, EnableFlush, TRUE);
        return (INTN) (KeyCode - AppleHidUsbKbUsageKeyA + 9);
      }
    }
  }

  if (NumKeys > 0) {
    return OC_INPUT_INVALID;
  }

  return OC_INPUT_TIMEOUT;
}

UINT64
OcWaitForAppleKeyIndexGetEndTime(
  IN UINTN    Timeout
  )
{
  if (Timeout == 0) {
    return 0ULL;
  }

  return GetTimeInNanoSecond (GetPerformanceCounter ()) + Timeout * 1000000u;
}

INTN
OcWaitForAppleKeyIndex (
  IN OUT OC_PICKER_CONTEXT                  *Context,
  IN     APPLE_KEY_MAP_AGGREGATOR_PROTOCOL  *KeyMap,
  IN     UINT64                             EndTime,
  IN OUT BOOLEAN                            *SetDefault  OPTIONAL
  )
{
  INTN                               ResultingKey;
  UINT64                             CurrTime;
  BOOLEAN                            OldSetDefault;

#if defined(OC_SHOW_RUNNING_KEYS)
  UINT64                             LoopDelayStart;
#endif

  //
  // These hotkeys are normally parsed by boot.efi, and they work just fine
  // when ShowPicker is disabled. On some BSPs, however, they may fail badly
  // when ShowPicker is enabled, and for this reason we support these hotkeys
  // within picker itself.
  //

  if (SetDefault != NULL) {
    OldSetDefault = *SetDefault;
    *SetDefault = 0;
  }

  while (TRUE) {
    ResultingKey = OcGetAppleKeyIndex (Context, KeyMap, SetDefault);

    CurrTime    = GetTimeInNanoSecond (GetPerformanceCounter ());  

    //
    // Requested for another iteration, handled Apple hotkey.
    //
    if (ResultingKey == OC_INPUT_INTERNAL) {
      continue;
    }

    //
    // Abort the timeout when unrecognised keys are pressed.
    //
    if (EndTime != 0 && ResultingKey == OC_INPUT_INVALID) {
      break;
    }

    //
    // Found key, return it.
    //
    if (ResultingKey != OC_INPUT_INVALID && ResultingKey != OC_INPUT_TIMEOUT) {
      break;
    }

    //
    // Return modifiers if they change, so we can optionally update UI
    //
    if (SetDefault != NULL && *SetDefault != OldSetDefault) {
      ResultingKey = OC_INPUT_MODIFIERS_ONLY;
      break;
    }

    if (EndTime != 0 && CurrTime != 0 && CurrTime >= EndTime) {
      ResultingKey = OC_INPUT_TIMEOUT;
      break;
    }

#if defined(OC_SHOW_RUNNING_KEYS)
    LoopDelayStart = AsmReadTsc();
#endif
    MicroSecondDelay (10);
#if defined(OC_SHOW_RUNNING_KEYS)
    OcInstrumentLoopDelay(LoopDelayStart, AsmReadTsc());
#endif
  }

  return ResultingKey;
}
