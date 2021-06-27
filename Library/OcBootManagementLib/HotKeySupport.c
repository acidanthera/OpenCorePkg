/** @file
  Copyright (C) 2019, vit9696. All rights reserved.<BR>
  Copyright (C) 2021, Mike Beaton. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-3-Clause
**/

#include "BootManagementInternal.h"

#include <Guid/AppleVariable.h>
#include <IndustryStandard/AppleCsrConfig.h>
#include <Protocol/AppleKeyMapAggregator.h>

#include <Library/BaseLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OcDebugLogLib.h>
#include <Library/OcTimerLib.h>
#include <Library/OcAppleKeyMapLib.h>
#include <Library/OcBootManagementLib.h>
#include <Library/OcConfigurationLib.h>
#include <Library/OcMiscLib.h>
#include <Library/OcTemplateLib.h>
#include <Library/OcTypingLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

//
// Get hotkeys pressed at load
//
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
  BOOLEAN                            HasZero;
  BOOLEAN                            HasOption;
  BOOLEAN                            HasKeyP;
  BOOLEAN                            HasKeyR;
  BOOLEAN                            HasKeyX;

  if (Context->TakeoffDelay > 0) {
    gBS->Stall (Context->TakeoffDelay);
  }

  KeyMap = OcGetProtocol (&gAppleKeyMapAggregatorProtocolGuid, DEBUG_ERROR, "OcLoadPickerHotKeys", "AppleKeyMapAggregator");

  NumKeys = ARRAY_SIZE (Keys);
  Status = KeyMap->GetKeyStrokes (
    KeyMap,
    &Modifiers,
    &NumKeys,
    Keys
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCHK: GetKeyStrokes - %r\n", Status));
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
  HasZero    = OcKeyMapHasKey (Keys, NumKeys, AppleHidUsbKbUsageKeyZero);
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
  } else if (HasEscape || HasZero) {
    DEBUG ((DEBUG_INFO, "OCHK: ESC/0 causes picker to show as OC extension\n"));
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

STATIC
VOID
EFIAPI
PickerFlushTypingBuffer (
  IN OUT OC_PICKER_CONTEXT                  *Context
  )
{
  OcFlushTypingBuffer(Context->HotKeyContext->TypingContext);
}

STATIC
VOID
EFIAPI
GetPickerKeyInfo (
  IN OUT OC_PICKER_CONTEXT                  *Context,
  IN     OC_PICKER_KEY_MAP                  KeyFilter,
     OUT OC_PICKER_KEY_INFO                 *PickerKeyInfo
  )
{
  EFI_STATUS                         Status;

  UINTN                              NumKeys;
  APPLE_KEY_CODE                     *Keys;
  APPLE_KEY_CODE                     Key;
  APPLE_MODIFIER_MAP                 Modifiers;
  CHAR16                             UnicodeChar;
  UINTN                              NumKeysUp;
  UINTN                              NumKeysDoNotRepeat;
  APPLE_KEY_CODE                     KeysDoNotRepeat[OC_KEY_MAP_DEFAULT_SIZE];
  APPLE_MODIFIER_MAP                 ModifiersDoNotRepeat;

  UINTN                              AkmaNumKeys;
  APPLE_MODIFIER_MAP                 AkmaModifiers;
  APPLE_KEY_CODE                     AkmaKeys[OC_KEY_MAP_DEFAULT_SIZE];

  APPLE_MODIFIER_MAP                 ValidBootModifiers;
  BOOLEAN                            TriggerBoot;
  BOOLEAN                            HasCommand;
  BOOLEAN                            HasKeyC;
  BOOLEAN                            HasKeyK;
  BOOLEAN                            HasKeyS;
  BOOLEAN                            HasKeyV;
  BOOLEAN                            HasKeyMinus;
  BOOLEAN                            WantsZeroSlide;
  UINT32                             CsrActiveConfig;
  UINTN                              CsrActiveConfigSize;

  ASSERT (Context->HotKeyContext->KeyMap             != NULL);
  ASSERT (Context->HotKeyContext->TypingContext      != NULL);
  ASSERT (Context->HotKeyContext->DoNotRepeatContext != NULL);
  ASSERT (PickerKeyInfo                              != NULL);

  PickerKeyInfo->OcKeyCode        = OC_INPUT_NO_ACTION;
  PickerKeyInfo->OcModifiers      = OC_MODIFIERS_NONE;
  PickerKeyInfo->UnicodeChar      = CHAR_NULL;

  //
  // AKMA hotkeys
  //
  AkmaNumKeys         = ARRAY_SIZE (AkmaKeys);
  Status = Context->HotKeyContext->KeyMap->GetKeyStrokes (
    Context->HotKeyContext->KeyMap,
    &AkmaModifiers,
    &AkmaNumKeys,
    AkmaKeys
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCHK: AKMA GetKeyStrokes - %r\n", Status));
    return;
  }

  //
  // Apple Event typing
  //
  Keys                = &Key;
  OcGetNextKeystroke(Context->HotKeyContext->TypingContext, &Modifiers, Keys, &UnicodeChar);
  if (*Keys == 0) {
    NumKeys = 0;
  }
  else {
    NumKeys = 1;
  }

  //
  // Non-repeating keys
  //
  NumKeysUp           = 0;
  NumKeysDoNotRepeat  = ARRAY_SIZE (KeysDoNotRepeat);
  Status = OcGetUpDownKeys (
    Context->HotKeyContext->DoNotRepeatContext,
    &ModifiersDoNotRepeat,
    &NumKeysUp, NULL,
    &NumKeysDoNotRepeat, KeysDoNotRepeat,
    0ULL // time not needed for non-repeat keys
    );

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "OCHK: GetUpDownKeys for DoNotRepeatContext - %r\n", Status));
    return;
  }


  DEBUG_CODE_BEGIN ();
  if (Context->KbDebug != NULL) {
    Context->KbDebug->Show (NumKeys, AkmaNumKeys, Modifiers);
  }
  DEBUG_CODE_END ();

  //
  // Set OcModifiers early, so they are correct even if - say - a hotkey or non-repeating key returns first.
  //
  ValidBootModifiers = APPLE_MODIFIERS_NONE;
  
  if (Context->AllowSetDefault) {
    ValidBootModifiers |= APPLE_MODIFIERS_CONTROL;
  }

  //
  // NB As historically SHIFT handling here is considered a 'hotkey':
  // it's original reason for being here is to fix difficulties in
  // detecting this and other hotkey modifiers during no-picker boot.
  //
  if ((KeyFilter & OC_PICKER_KEYS_HOTKEYS) != 0 && Context->PollAppleHotKeys) {
    ValidBootModifiers |= APPLE_MODIFIERS_SHIFT;
  }

  //
  // Default update is desired for Ctrl+Index and Ctrl+Enter.
  // Strictly apply only on CTRL or CTRL+SHIFT (when SHIFT allowed),
  // but no other modifiers.
  // Needs to be set/unset even if filtered for typing (otherwise can
  // get locked on if user tabs to typing context).
  //
  if ((Modifiers & ~ValidBootModifiers) == 0
    && (Modifiers & APPLE_MODIFIERS_CONTROL) != 0) {
    PickerKeyInfo->OcModifiers |= OC_MODIFIERS_SET_DEFAULT;
  }

  //
  // Alternative 'set default' key, if modifiers not working;
  // useful both for 'set default' and for tuning KeyForgetThreshold.
  //
  if (Context->AllowSetDefault
    && OcKeyMapHasKey (AkmaKeys, AkmaNumKeys, AppleHidUsbKbUsageKeyEquals)) {
    PickerKeyInfo->OcModifiers |= OC_MODIFIERS_SET_DEFAULT;
  }

  //
  // Loosely apply regardless of other modifiers.
  //
  if ((KeyFilter & OC_PICKER_KEYS_TAB_CONTROL) != 0
    && (Modifiers & APPLE_MODIFIERS_SHIFT) != 0) {
    PickerKeyInfo->OcModifiers |= OC_MODIFIERS_REVERSE_SWITCH_FOCUS;
  }

  //
  // Handle key combinations.
  //
  if ((KeyFilter & OC_PICKER_KEYS_HOTKEYS) != 0 && Context->PollAppleHotKeys) {
    HasCommand = (AkmaModifiers & APPLE_MODIFIERS_COMMAND) != 0;
    HasKeyC    = OcKeyMapHasKey (AkmaKeys, AkmaNumKeys, AppleHidUsbKbUsageKeyC);
    HasKeyK    = OcKeyMapHasKey (AkmaKeys, AkmaNumKeys, AppleHidUsbKbUsageKeyK);
    HasKeyS    = OcKeyMapHasKey (AkmaKeys, AkmaNumKeys, AppleHidUsbKbUsageKeyS);
    HasKeyV    = OcKeyMapHasKey (AkmaKeys, AkmaNumKeys, AppleHidUsbKbUsageKeyV);
    //
    // Checking for PAD minus is our extension to support more keyboards.
    //
    HasKeyMinus = OcKeyMapHasKey (AkmaKeys, AkmaNumKeys, AppleHidUsbKbUsageKeyMinus)
      || OcKeyMapHasKey (AkmaKeys, AkmaNumKeys, AppleHidUsbKbUsageKeyPadMinus);

    //
    // CMD+V is always valid and enables Verbose Mode.
    //
    if (HasCommand && HasKeyV) {
      if (OcGetArgumentFromCmd (Context->AppleBootArgs, "-v", L_STR_LEN ("-v"), NULL) == NULL) {
        DEBUG ((DEBUG_INFO, "OCHK: CMD+V means -v\n"));
        OcAppendArgumentToCmd (Context, Context->AppleBootArgs, "-v", L_STR_LEN ("-v"));
      }
      PickerKeyInfo->OcKeyCode = OC_INPUT_INTERNAL;
    }

    //
    // CMD+C+MINUS is always valid and disables compatibility check.
    //
    if (HasCommand && HasKeyC && HasKeyMinus) {
      if (OcGetArgumentFromCmd (Context->AppleBootArgs, "-no_compat_check", L_STR_LEN ("-no_compat_check"), NULL) == NULL) {
        DEBUG ((DEBUG_INFO, "OCHK: CMD+C+MINUS means -no_compat_check\n"));
        OcAppendArgumentToCmd (Context, Context->AppleBootArgs, "-no_compat_check", L_STR_LEN ("-no_compat_check"));
      }
      PickerKeyInfo->OcKeyCode = OC_INPUT_INTERNAL;
    }

    //
    // CMD+K is always valid for new macOS and means force boot to release kernel.
    //
    if (HasCommand && HasKeyK) {
      if (AsciiStrStr (Context->AppleBootArgs, "kcsuffix=release") == NULL) {
        DEBUG ((DEBUG_INFO, "OCHK: CMD+K means kcsuffix=release\n"));
        OcAppendArgumentToCmd (Context, Context->AppleBootArgs, "kcsuffix=release", L_STR_LEN ("kcsuffix=release"));
      }
      PickerKeyInfo->OcKeyCode = OC_INPUT_INTERNAL;
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
      PickerKeyInfo->OcKeyCode = OC_INPUT_INTERNAL;
    }
  }

  //
  // Handle typing chars.
  //
  if ((KeyFilter & OC_PICKER_KEYS_TYPING) != 0) {
    PickerKeyInfo->UnicodeChar = UnicodeChar;
  }

  //
  // Handle VoiceOver - non-repeating.
  //
  if ((KeyFilter & OC_PICKER_KEYS_VOICE_OVER) != 0
    && (Modifiers & APPLE_MODIFIERS_COMMAND) != 0
    && OcKeyMapHasKey (KeysDoNotRepeat, NumKeysDoNotRepeat, AppleHidUsbKbUsageKeyF5)) {
    PickerKeyInfo->OcKeyCode = OC_INPUT_VOICE_OVER;
    return;
  }

  if ((KeyFilter & OC_PICKER_KEYS_TYPING) == 0) {
    //
    // Handle reload menu - non-repeating.
    //
    if (OcKeyMapHasKey (KeysDoNotRepeat, NumKeysDoNotRepeat, AppleHidUsbKbUsageKeyEscape)
    || OcKeyMapHasKey (KeysDoNotRepeat, NumKeysDoNotRepeat, AppleHidUsbKbUsageKeyZero)
    ) {
      PickerKeyInfo->OcKeyCode = OC_INPUT_ABORTED;
      return;
    }

    //
    // Handle show or toggle auxiliary - non-repeating.
    //
    if (OcKeyMapHasKey (KeysDoNotRepeat, NumKeysDoNotRepeat, AppleHidUsbKbUsageKeySpaceBar)) {
      PickerKeyInfo->OcKeyCode = OC_INPUT_MORE;
      return;
    }
  }

  if (NumKeys == 1) {
    if ((KeyFilter & OC_PICKER_KEYS_TAB_CONTROL) != 0
      && Keys[0] == AppleHidUsbKbUsageKeyTab) {
      PickerKeyInfo->OcKeyCode = OC_INPUT_SWITCH_FOCUS;
      return;
    }

    if ((KeyFilter & OC_PICKER_KEYS_TYPING) != 0) {
      //
      // Typing index key strokes.
      //
      if (Keys[0] == AppleHidUsbKbUsageKeyLeftArrow) {
        PickerKeyInfo->OcKeyCode = OC_INPUT_TYPING_LEFT;
        return;
      }

      if (Keys[0] == AppleHidUsbKbUsageKeyRightArrow) {
        PickerKeyInfo->OcKeyCode = OC_INPUT_TYPING_RIGHT;
        return;
      }

      if (Keys[0] == AppleHidUsbKbUsageKeyEscape) {
        PickerKeyInfo->OcKeyCode = OC_INPUT_TYPING_CLEAR_ALL;
        return;
      }

      if (Keys[0] ==AppleHidUsbKbUsageKeyBackSpace) {
        PickerKeyInfo->OcKeyCode = OC_INPUT_TYPING_BACKSPACE;
        return;
      }

      if (Keys[0] == AppleHidUsbKbUsageKeyEnter
        || Keys[0] == AppleHidUsbKbUsageKeyReturn
        || Keys[0] == AppleHidUsbKbUsageKeyPadEnter) {
        PickerKeyInfo->OcKeyCode = OC_INPUT_TYPING_CONFIRM;
        return;
      }
    } else {
      //
      // Non-typing index key strokes.
      //

      //
      // Only select OS if valid modifiers are in place.
      //
      if ((Modifiers & ~ValidBootModifiers) == 0)
      {
        TriggerBoot = FALSE;

        if (Keys[0] == AppleHidUsbKbUsageKeyEnter
          || Keys[0] == AppleHidUsbKbUsageKeyReturn
          || Keys[0] == AppleHidUsbKbUsageKeyPadEnter) {

          PickerKeyInfo->OcKeyCode = OC_INPUT_CONTINUE;
          TriggerBoot = TRUE;
        }

        STATIC_ASSERT (AppleHidUsbKbUsageKeyOne + 8 == AppleHidUsbKbUsageKeyNine, "Unexpected encoding");
        if (Keys[0] >= AppleHidUsbKbUsageKeyOne && Keys[0] <= AppleHidUsbKbUsageKeyNine) {
          PickerKeyInfo->OcKeyCode = (INTN) (Keys[0] - AppleHidUsbKbUsageKeyOne);
          TriggerBoot = TRUE;
        }

        STATIC_ASSERT (AppleHidUsbKbUsageKeyA + 25 == AppleHidUsbKbUsageKeyZ, "Unexpected encoding");
        if (Keys[0] > AppleHidUsbKbUsageKeyA && Keys[0] <= AppleHidUsbKbUsageKeyZ) {
          PickerKeyInfo->OcKeyCode = (INTN) (Keys[0] - AppleHidUsbKbUsageKeyA + 9);
          TriggerBoot = TRUE;
        }

        if (TriggerBoot) {
          //
          // In order to allow shift+tab, shift only applies at time of hitting
          // enter or index, but if held then it enables Safe Mode.
          //
          if ((Modifiers & APPLE_MODIFIERS_SHIFT) != 0) {
            if (OcGetArgumentFromCmd (Context->AppleBootArgs, "-x", L_STR_LEN ("-x"), NULL) == NULL) {
              DEBUG ((DEBUG_INFO, "OCHK: Shift means -x\n"));
              OcAppendArgumentToCmd (Context, Context->AppleBootArgs, "-x", L_STR_LEN ("-x"));
            }
          }

          return;
        }
      }

      //
      // Apply navigation keys regardless of modifiers.
      // 
      if (Keys[0] == AppleHidUsbKbUsageKeyUpArrow) {
        PickerKeyInfo->OcKeyCode = OC_INPUT_UP;
        return;
      }

      if (Keys[0] == AppleHidUsbKbUsageKeyDownArrow) {
        PickerKeyInfo->OcKeyCode = OC_INPUT_DOWN;
        return;
      }

      if (Keys[0] == AppleHidUsbKbUsageKeyLeftArrow) {
        PickerKeyInfo->OcKeyCode = OC_INPUT_LEFT;
        return;
      }

      if (Keys[0] == AppleHidUsbKbUsageKeyRightArrow) {
        PickerKeyInfo->OcKeyCode = OC_INPUT_RIGHT;
        return;
      }

      if (Keys[0] == AppleHidUsbKbUsageKeyPgUp
        || Keys[0] == AppleHidUsbKbUsageKeyHome) {
        PickerKeyInfo->OcKeyCode = OC_INPUT_TOP;
        return;
      }

      if (Keys[0] == AppleHidUsbKbUsageKeyPgDn
        || Keys[0] == AppleHidUsbKbUsageKeyEnd) {
        PickerKeyInfo->OcKeyCode = OC_INPUT_BOTTOM;
        return;
      }

      //
      // Fn. keys should only apply if no modifiers held.
      //
      if (Modifiers == APPLE_MODIFIERS_NONE) {
        STATIC_ASSERT (AppleHidUsbKbUsageKeyF1 + 11 == AppleHidUsbKbUsageKeyF12, "Unexpected encoding");
        if (Keys[0] >= AppleHidUsbKbUsageKeyF1 && Keys[0] <= AppleHidUsbKbUsageKeyF12) {
          PickerKeyInfo->OcKeyCode = OC_INPUT_FUNCTIONAL (Keys[0] - AppleHidUsbKbUsageKeyF1 + 1);
          return;
        }

        STATIC_ASSERT (AppleHidUsbKbUsageKeyF13 + 11 == AppleHidUsbKbUsageKeyF24, "Unexpected encoding");
        if (Keys[0] >= AppleHidUsbKbUsageKeyF13 && Keys[0] <= AppleHidUsbKbUsageKeyF24) {
          PickerKeyInfo->OcKeyCode = OC_INPUT_FUNCTIONAL (Keys[0] - AppleHidUsbKbUsageKeyF13 + 13);
          return;
        }
      }
    }
  }

  //
  // Return NO_ACTION here, since all non-null actions now feedback
  // immediately to either picker, to allow UI response.
  //
  PickerKeyInfo->OcKeyCode    = OC_INPUT_NO_ACTION;
}

STATIC
UINT64
EFIAPI
WaitForPickerKeyInfoGetEndTime (
  IN UINT64     Timeout
  )
{
  if (Timeout == 0) {
    return 0ULL;
  }

  return GetTimeInNanoSecond (GetPerformanceCounter ()) + Timeout * 1000000ULL;
}

STATIC
BOOLEAN
EFIAPI
WaitForPickerKeyInfo (
  IN OUT OC_PICKER_CONTEXT                  *Context,
  IN     UINT64                             EndTime,
  IN     OC_PICKER_KEY_MAP                  KeyFilter,
  IN OUT OC_PICKER_KEY_INFO                 *PickerKeyInfo
  )
{
  OC_MODIFIER_MAP                    OldOcModifiers;
  UINT64                             CurrTime;
  UINT64                             LoopDelayStart;

  LoopDelayStart    = 0;
  OldOcModifiers    = PickerKeyInfo->OcModifiers;

  //
  // These hotkeys are normally parsed by boot.efi, and they work just fine
  // when ShowPicker is disabled. On some BSPs, however, they may fail badly
  // when ShowPicker is enabled, and for this reason we support these hotkeys
  // within picker itself.
  //

  while (TRUE) {
    GetPickerKeyInfo (Context, KeyFilter, PickerKeyInfo);

    //
    // All non-null actions (even internal) are now returned to picker for possible UI response.
    //
    if (PickerKeyInfo->OcKeyCode != OC_INPUT_NO_ACTION ||
      PickerKeyInfo->OcModifiers != OldOcModifiers     ||
      PickerKeyInfo->UnicodeChar != CHAR_NULL) {
      break;
    }

    if (EndTime != 0) {
      CurrTime    = GetTimeInNanoSecond (GetPerformanceCounter ());  
      if (CurrTime != 0 && CurrTime >= EndTime) {
        PickerKeyInfo->OcKeyCode = OC_INPUT_TIMEOUT;
        break;
      }
    }

    DEBUG_CODE_BEGIN ();
    LoopDelayStart = AsmReadTsc();
    DEBUG_CODE_END ();

    MicroSecondDelay (OC_MINIMAL_CPU_DELAY);

    DEBUG_CODE_BEGIN ();
    if (Context->KbDebug != NULL) {
      Context->KbDebug->InstrumentLoopDelay (LoopDelayStart, AsmReadTsc());
    }
    DEBUG_CODE_END ();
  }

  return PickerKeyInfo->OcModifiers != OldOcModifiers;
}

//
// Initialise picker keyboard handling.
//
EFI_STATUS
OcInitHotKeys (
  IN OUT OC_PICKER_CONTEXT  *Context
  )
{
  EFI_STATUS                         Status;

  DEBUG ((DEBUG_INFO, "OCHK: InitHotKeys\n"));

  ASSERT_EQUALS (Context->HotKeyContext, NULL);

  //
  // No kb debug unless initialiased on settings flag by a given picker itself.
  //
  Context->KbDebug = NULL;

  Context->HotKeyContext = AllocatePool (sizeof(*(Context->HotKeyContext)));
  if (Context->HotKeyContext == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Fn. ptrs.
  //
  Context->HotKeyContext->GetKeyInfo          = GetPickerKeyInfo;
  Context->HotKeyContext->GetKeyWaitEndTime   = WaitForPickerKeyInfoGetEndTime;
  Context->HotKeyContext->WaitForKeyInfo      = WaitForPickerKeyInfo;
  Context->HotKeyContext->FlushTypingBuffer   = PickerFlushTypingBuffer;

  Context->HotKeyContext->KeyMap = OcGetProtocol (&gAppleKeyMapAggregatorProtocolGuid, DEBUG_ERROR, "OcInitHotKeys", "AppleKeyMapAggregator");
  if (Context->HotKeyContext->KeyMap == NULL) {
    FreePool (Context->HotKeyContext);
    Context->HotKeyContext = NULL;
    return EFI_NOT_FOUND;
  }

  //
  // Non-repeating keys, e.g. ESC and SPACE.
  //
  Status = OcInitKeyRepeatContext (
    &Context->HotKeyContext->DoNotRepeatContext,
    Context->HotKeyContext->KeyMap,
    OC_HELD_KEYS_DEFAULT_SIZE,
    0,
    0,
    TRUE
  );
  
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCHK: Init non-repeating context - %r\n", Status));
    FreePool (Context->HotKeyContext);
    Context->HotKeyContext = NULL;
    return Status;
  }

  //
  // Typing handler, for most keys.
  //
  Status = OcRegisterTypingHandler(&Context->HotKeyContext->TypingContext);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "OCHK: Register typing handler - %r\n", Status));
    OcFreeKeyRepeatContext (&Context->HotKeyContext->DoNotRepeatContext);
    FreePool (Context->HotKeyContext);
    Context->HotKeyContext = NULL;
    return Status;
  }

  //
  // NB Raw AKMA is also still used for HotKeys, since we really do need
  // three different types of keys response for fluent UI behaviour.
  //

  return EFI_SUCCESS;
}

//
// Free picker keyboard handling resources.
//
VOID
OcFreeHotKeys (
  IN     OC_PICKER_CONTEXT  *Context
  )
{
  if (Context == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO, "OCHK: FreeHotKeys\n"));

  ASSERT (Context->HotKeyContext != NULL);

  OcUnregisterTypingHandler (&Context->HotKeyContext->TypingContext);
  OcFreeKeyRepeatContext (&Context->HotKeyContext->DoNotRepeatContext);

  FreePool (Context->HotKeyContext);
  Context->HotKeyContext = NULL;
}
