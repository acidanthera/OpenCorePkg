/** @file
  Copyright (c) 2025, Pavel Naberezhnev. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <stdio.h>
#include <unistd.h>

/**
  The context for callback functions
**/
typedef struct {
  EFI_EVENT    LinkedEvent;
  UINTN        Counter;
} NOTIFY_CONTEXT1;

/**
  The callback function for a simple signal
**/
STATIC
VOID
SignalNotify1 (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  NOTIFY_CONTEXT1  *Ctx;

  DEBUG ((DEBUG_INFO, "SignalNotify1()\n"));
  Ctx = (NOTIFY_CONTEXT1 *)Context;
  ++(Ctx->Counter);
}

/**
  The callback function for a self-destroy signal
**/
STATIC
VOID
SelfDestroyNotify1 (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  NOTIFY_CONTEXT1  *Ctx;

  DEBUG ((DEBUG_INFO, "@@@@ SelfDestroyNotify1()\n"));
  Ctx = (NOTIFY_CONTEXT1 *)Context;
  ++(Ctx->Counter);
  gBS->CloseEvent (Event);
}

/**
  The callback function for linked signals
**/
STATIC
VOID
SignalNotify2 (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  NOTIFY_CONTEXT1  *Ctx;

  Ctx = (NOTIFY_CONTEXT1 *)Context;
  gBS->SignalEvent (Ctx->LinkedEvent);
  ++(Ctx->Counter);
}

/**
  The callback function for a last-linked signal
**/
STATIC
VOID
SignalNotify3 (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  NOTIFY_CONTEXT1  *Ctx;

  Ctx = (NOTIFY_CONTEXT1 *)Context;
  ++(Ctx->Counter);
}

/**
  The callback function for a linked wait signal
**/
STATIC
VOID
WaitNotify1 (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  NOTIFY_CONTEXT1  *Ctx;

  Ctx = (NOTIFY_CONTEXT1 *)Context;
  gBS->SignalEvent (Ctx->LinkedEvent);
  ++(Ctx->Counter);
}

int
ENTRY_POINT (
  VOID
  )
{
  EFI_STATUS       Status;
  NOTIFY_CONTEXT1  SignalNotifyContext1 = { 0, };
  NOTIFY_CONTEXT1  SignalNotifyContext2 = { 0, };
  NOTIFY_CONTEXT1  SignalNotifyContext3 = { 0, };
  NOTIFY_CONTEXT1  WaitNotifyContext1   = { 0, };
  NOTIFY_CONTEXT1  SelfDestroyContext1  = { 0, };
  EFI_EVENT        Signal1;
  EFI_EVENT        Signal2;
  EFI_EVENT        Signal3;
  EFI_EVENT        Wait1;
  EFI_EVENT        Timer1;
  EFI_EVENT        SelfD1;
  UINTN            TimerCounter1 = 0;
  UINTN            Index;
  INTN             Jndex;
  EFI_TPL          OriginalTPL;
  //
  // Array for waiting events
  //
  EFI_EVENT  Events[2];

  DEBUG ((DEBUG_INFO, "=== Test UEFI Event System  ===\n"));

  //
  // Create events
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  SignalNotify1,
                  (VOID *)&SignalNotifyContext1,
                  &Signal1
                  );

  if (EFI_ERROR (Status) || (Signal1 == NULL)) {
    DEBUG ((DEBUG_INFO, "FAIL: Signal1 event creation (Status: %r)\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "PASS: Signal1 event created\n"));
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  SignalNotify2,
                  (VOID *)&SignalNotifyContext2,
                  &Signal2
                  );

  if (EFI_ERROR (Status) || (Signal2 == NULL)) {
    DEBUG ((DEBUG_INFO, "FAIL: Signal2 event creation (Status: %r)\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "PASS: Signal2 event created\n"));
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  SignalNotify3,
                  (VOID *)&SignalNotifyContext3,
                  &Signal3
                  );

  if (EFI_ERROR (Status) || (Signal3 == NULL)) {
    DEBUG ((DEBUG_INFO, "FAIL: Signal3 event creation (Status: %r)\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "PASS: Signal3 event created\n"));
  }

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_WAIT,
                  TPL_CALLBACK,
                  WaitNotify1,
                  (VOID *)&WaitNotifyContext1,
                  &Wait1
                  );

  if (EFI_ERROR (Status) || (Wait1 == NULL)) {
    DEBUG ((DEBUG_INFO, "FAIL: Wait1 event creation (Status: %r)\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "PASS: Wait1 event created\n"));
  }

  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &Timer1
                  );

  if (EFI_ERROR (Status) || (Timer1 == NULL)) {
    DEBUG ((DEBUG_INFO, "FAIL: Timer1 event creation (Status: %r)\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "PASS: Timer1 event created\n"));
  }

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  SelfDestroyNotify1,
                  (VOID *)&SelfDestroyContext1,
                  &SelfD1
                  );

  if (EFI_ERROR (Status) || (SelfD1 == NULL)) {
    DEBUG ((DEBUG_INFO, "FAIL: SelfD1 event creation (Status: %r)\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "PASS: SelfD event created\n"));
  }

  //
  // Link events
  // Signal2 --> Signal3
  // Wait1 --> Signal2
  //
  SignalNotifyContext2.LinkedEvent = Signal3;
  WaitNotifyContext1.LinkedEvent   = Signal2;

  //
  // Try to set timers
  //
  Status = gBS->SetTimer (Wait1, TimerPeriodic, 100*10000);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "FAIL: Set timer for Wait1 (Status: %r)\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "PASS: Wait1 set to 100*10000 periodic\n"));
  }

  Status = gBS->SetTimer (Timer1, TimerPeriodic, 10*10000);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "FAIL: Set timer for Timer1 (Status: %r)\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "PASS: Timer1 set to 10*10000 periodic\n"));
  }

  Status = gBS->SetTimer (SelfD1, TimerRelative, 30*10000);

  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "FAIL: Set timer for SelfD1 (Status: %r)\n", Status));
  } else {
    DEBUG ((DEBUG_INFO, "PASS: SelfD1 set to 30*10000 relative\n"));
  }

  //
  // Try to emit Signal1
  //
  gBS->SignalEvent (Signal1);

  //
  // Try to wait events
  //
  Events[0] = Timer1;
  Events[1] = Wait1;

  for (Jndex = 0; Jndex < 10; Jndex++) {
    Status = gBS->WaitForEvent (2, Events, &Index);
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "Event %u triggered\n", Index));
      if (Index == 0) {
        DEBUG ((DEBUG_INFO, "PASS: Timer1 event wait succeeded\n"));
        TimerCounter1++;
      }
    } else {
      DEBUG ((DEBUG_INFO, "Event Error %u (Status: %r)\n", Index, Status));
    }
  }

  //
  // Try to check event
  //
  Status = gBS->CheckEvent (Signal1);
  if (Status == EFI_SUCCESS) {
    DEBUG ((DEBUG_INFO, "FAIL: Event check succeeded\n"));
  } else {
    DEBUG ((DEBUG_INFO, "PASS: Event check error (Status: %r)\n", Status));
  }

  //
  // The TPL test
  //
  OriginalTPL = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  DEBUG ((DEBUG_INFO, "TPL raised from %u to %d\n", OriginalTPL, TPL_HIGH_LEVEL));

  gBS->SignalEvent (Signal1);
  DEBUG ((DEBUG_INFO, "EVENT SIGANL1 NEED PRINTF AFTER THIS LINE:\n"));

  gBS->RestoreTPL (OriginalTPL);
  DEBUG ((DEBUG_INFO, "TPL restored to %u\n", OriginalTPL));

  //
  // Try to close event
  //
  gBS->CloseEvent (Signal1);
  gBS->CloseEvent (Timer1);

  //
  // Try to signal a closed event. Need to trigger ASSERT()
  //
  // gBS->SignalEvent (Signal1);

  DEBUG ((DEBUG_INFO, "\n=== Test Summary ===\n"));
  DEBUG ((DEBUG_INFO, "Signal1 callbacks: %u\n", SignalNotifyContext1.Counter));
  DEBUG ((DEBUG_INFO, "Signal2 callbacks: %u\n", SignalNotifyContext2.Counter));
  DEBUG ((DEBUG_INFO, "Signal3 callbacks: %u\n", SignalNotifyContext3.Counter));
  DEBUG ((DEBUG_INFO, "Wait1 callbacks: %u\n", WaitNotifyContext1.Counter));
  DEBUG ((DEBUG_INFO, "SelfD1 callbacks: %u\n", SelfDestroyContext1.Counter));
  DEBUG ((DEBUG_INFO, "Timer1 callbacks: %u\n", TimerCounter1));

  gBS->CloseEvent (Signal2);
  gBS->CloseEvent (Signal3);
  gBS->CloseEvent (Wait1);
}
