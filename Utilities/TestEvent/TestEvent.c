/** @file
  Copyright (c) 2025, Pavel Naberezhnev. All rights reserved.
  SPDX-License-Identifier: BSD-3-Clause
**/

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <stdio.h>
#include <unistd.h>

// context for callback functions
struct NOTIFY_CONTEXT1 {
  EFI_EVENT    LinkedEvent;
  UINTN        Counter;
};

// callback-functions for signal
VOID
SignalNotify1 (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  struct NOTIFY_CONTEXT1  *ctx = (struct NOTIFY_CONTEXT1 *)Context;

  printf ("SignalNotify1()\n");
  (ctx->Counter)++;
}

VOID
SelfDestroyNotify1 (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  struct NOTIFY_CONTEXT1  *ctx = (struct NOTIFY_CONTEXT1 *)Context;

  printf ("@@@@ SelfDestroyNotify1()\n");
  (ctx->Counter)++;
  gBS->CloseEvent (Event);
}

VOID
SignalNotify2 (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  struct NOTIFY_CONTEXT1  *ctx = (struct NOTIFY_CONTEXT1 *)Context;
  EFI_STATUS              Status;

  // printf("SignalNotify2() %lu\n", ctx->Counter);
  Status = gBS->SignalEvent (ctx->LinkedEvent);

  // if (EFI_ERROR(Status)) {
  // printf("SignalNotify2() %lu Error emited %d\n", ctx->Counter, Status);
  // } else {
  // printf("SignalNotify2() %lu Success Emited\n", ctx->Counter);
  // }
  (ctx->Counter)++;
}

VOID
SignalNotify3 (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  struct NOTIFY_CONTEXT1  *ctx = (struct NOTIFY_CONTEXT1 *)Context;

  // printf("SignalNotify3() %lu\n", ctx->Counter);
  (ctx->Counter)++;
}

VOID
WaitNotify1 (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  struct NOTIFY_CONTEXT1  *ctx = (struct NOTIFY_CONTEXT1 *)Context;
  EFI_STATUS              Status;

  // printf("WaitNotify1() %lu\n", ctx->Counter);
  Status = gBS->SignalEvent (ctx->LinkedEvent);

  // if (EFI_ERROR(Status)) {
  //  printf("WaitNotify1() %lu Error emited %d\n", ctx->Counter, Status);
  // } else {
  //  printf("WaitNotify1() %lu Success Emited\n", ctx->Counter);
  // }
  (ctx->Counter)++;
}

int
main (
  )
{
  EFI_STATUS              Status;
  struct NOTIFY_CONTEXT1  SignalNotifyContext1 = { 0, };
  struct NOTIFY_CONTEXT1  SignalNotifyContext2 = { 0, };
  struct NOTIFY_CONTEXT1  SignalNotifyContext3 = { 0, };
  struct NOTIFY_CONTEXT1  WaitNotifyContext1   = { 0, };
  struct NOTIFY_CONTEXT1  SelfDestroyContext1  = { 0, };
  EFI_EVENT               Signal1;
  EFI_EVENT               Signal2;
  EFI_EVENT               Signal3;
  EFI_EVENT               Wait1;
  EFI_EVENT               Timer1;
  EFI_EVENT               SelfD1;
  UINTN                   TimerCounter1 = 0;
  UINTN                   Index;

  printf ("=== Test UEFI Event System  ===\n");

  // create events
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  SignalNotify1,
                  (VOID *)&SignalNotifyContext1,
                  &Signal1
                  );

  if (EFI_ERROR (Status) || (Signal1 == NULL)) {
    printf ("FAIL: Signal1 event creation (Status: %lu)\n", Status);
  } else {
    printf ("PASS: Signal1 event created\n");
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  SignalNotify2,
                  (VOID *)&SignalNotifyContext2,
                  &Signal2
                  );

  if (EFI_ERROR (Status) || (Signal2 == NULL)) {
    printf ("FAIL: Signal2 event creation (Status: %lu)\n", Status);
  } else {
    printf ("PASS: Signal2 event created\n");
  }

  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  SignalNotify3,
                  (VOID *)&SignalNotifyContext3,
                  &Signal3
                  );

  if (EFI_ERROR (Status) || (Signal3 == NULL)) {
    printf ("FAIL: Signal3 event creation (Status: %lu)\n", Status);
  } else {
    printf ("PASS: Signal3 event created\n");
  }

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_WAIT,
                  TPL_CALLBACK,
                  WaitNotify1,
                  (VOID *)&WaitNotifyContext1,
                  &Wait1
                  );

  if (EFI_ERROR (Status) || (Wait1 == NULL)) {
    printf ("FAIL: Wait1 event creation (Status: %lu)\n", Status);
  } else {
    printf ("PASS: Wait1 event created\n");
  }

  Status = gBS->CreateEvent (
                  EVT_TIMER,
                  TPL_CALLBACK,
                  NULL,
                  NULL,
                  &Timer1
                  );

  if (EFI_ERROR (Status) || (Timer1 == NULL)) {
    printf ("FAIL: Timer1 event creation (Status: %lu)\n", Status);
  } else {
    printf ("PASS: Timer1 event created\n");
  }

  Status = gBS->CreateEvent (
                  EVT_TIMER | EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  SelfDestroyNotify1,
                  (VOID *)&SelfDestroyContext1,
                  &SelfD1
                  );

  if (EFI_ERROR (Status) || (SelfD1 == NULL)) {
    printf ("FAIL: SelfD1 event creation (Status: %lu)\n", Status);
  } else {
    printf ("PASS: SelfD event created\n");
  }

  // links events
  // signal2 with signal3
  SignalNotifyContext2.LinkedEvent = Signal3;
  // wait1 with signal 2
  WaitNotifyContext1.LinkedEvent = Signal2;

  // try sets timers
  Status = gBS->SetTimer (Wait1, TimerPeriodic, 100*10000);

  if (EFI_ERROR (Status)) {
    printf ("FAIL: Set timer for Wait1 (Status: %lu)\n", Status);
  } else {
    printf ("PASS: Wait1 set to 100*10000 periodic\n");
  }

  Status = gBS->SetTimer (Timer1, TimerPeriodic, 10*10000);

  if (EFI_ERROR (Status)) {
    printf ("FAIL: Set timer for Timer1 (Status: %lu)\n", Status);
  } else {
    printf ("PASS: Timer1 set to 10*10000 periodic\n");
  }

  Status = gBS->SetTimer (SelfD1, TimerRelative, 30*10000);

  if (EFI_ERROR (Status)) {
    printf ("FAIL: Set timer for SelfD1 (Status: %lu)\n", Status);
  } else {
    printf ("PASS: SelfD1 set to 30*10000 relative\n");
  }

  // try emit signal1
  gBS->SignalEvent (Signal1);

  // try wait events
  EFI_EVENT  Events[] = { Timer1, Wait1 };

  for (int i = 0; i < 10; i++) {
    Status = gBS->WaitForEvent (2, Events, &Index);
    if (!EFI_ERROR (Status)) {
      printf ("Event %lu triggered\n", Index);
      if (Index == 0) {
        printf ("PASS: Timer1 event wait succeeded\n");
        TimerCounter1++;
      }
    } else {
      printf ("Event Error %lu (Status: %lu)\n", Index, Status);
    }
  }

  // try check event
  Status = gBS->CheckEvent (Signal1);
  if (Status == EFI_SUCCESS) {
    printf ("FAIL: Event check succeeded\n");
  } else {
    printf ("PASS: Event check error (Status: %lu)\n", Status);
  }

  // TPL Test
  EFI_TPL  OriginalTPL = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  printf ("TPL raised from %lu to %d\n", OriginalTPL, TPL_HIGH_LEVEL);

  gBS->SignalEvent (Signal1);
  printf ("EVENT SIGANL1 NEED PRINTF AFTER THIS LINE:\n");

  gBS->RestoreTPL (OriginalTPL);
  printf ("TPL restored to %lu\n", OriginalTPL);

  // test close event
  gBS->CloseEvent (Signal1);
  gBS->CloseEvent (Timer1);

  // try signal closed event
  // gBS->SignalEvent (Signal1);

  printf ("\n=== Test Summary ===\n");
  printf ("Signal1 callbacks: %ld\n", SignalNotifyContext1.Counter);
  printf ("Signal2 callbacks: %ld\n", SignalNotifyContext2.Counter);
  printf ("Signal3 callbacks: %ld\n", SignalNotifyContext3.Counter);
  printf ("Wait1 callbacks: %ld\n", WaitNotifyContext1.Counter);
  printf ("SelfD1 callbacks: %ld\n", SelfDestroyContext1.Counter);
  printf ("Timer1 callbacks: %ld\n", TimerCounter1);

  gBS->CloseEvent (Signal2);
  gBS->CloseEvent (Signal3);
  gBS->CloseEvent (Wait1);
}
