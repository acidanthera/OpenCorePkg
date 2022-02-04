@vit9696 has provided this summary of the operation of KeySupport, as implemented by the files in this directory:

KeySupport sets up a polling timer every 10 ms (AIK_KEY_POLL_INTERVAL). This event is not guaranteed to arrive every 10 ms, last less than 10 ms, or even arrive with any constant period, but that's an ideal case. This is in AIK.c

When the event arrives (AIKPollKeyboardHandler), AIK does three things:

1. Clears out the keys pressed too long ago (AIKTargetRefresh) from AKMA. This is done based on event arrival count instead of timers since timers are unreliable. I.e. Target->Counter is incremented every event arrival and then compared against Target->KeyForgotThreshold to forget the key.

2. Checks for the key via the source protocol (AIKSourceGrabEfiKey, AIKSource.c) and applies basic filtering and conversion to ensure adequate input if it happened.

3. Then the key needs to go to two input targets. There is Data input target (UEFI key input, like V1, V2, AMI) and Target input target (AKMA). For Data is really just a ring-buffer based queue updated at the end. For AKMA it is similar but it does not append already present keys to the queue, instead it updates their counters to the current one. This way these keys will not be evicted by the subsequent refreshes (1).

Steps 2 and 3 happen in a cycle up to 6 (AIK_KEY_POLL_LIMIT) times to grab simultaneously pressed keys, e.g. P+R.
