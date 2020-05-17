/** @file
  Copyright (C) 2017, vit9696. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef APPLE_CSR_CONFIG_H
#define APPLE_CSR_CONFIG_H

///
/// System Integrity Proteciton codes.
/// Keep in sync with XNU bsd/sys/csr.h.
/// Last sync time: 4903.221.2.
///

///
/// Rootless configuration flags.
///

#define CSR_ALLOW_UNTRUSTED_KEXTS             (1U << 0U)
#define CSR_ALLOW_UNRESTRICTED_FS             (1U << 1U)
#define CSR_ALLOW_TASK_FOR_PID                (1U << 2U)
#define CSR_ALLOW_KERNEL_DEBUGGER             (1U << 3U)
#define CSR_ALLOW_APPLE_INTERNAL              (1U << 4U)
#define CSR_ALLOW_DESTRUCTIVE_DTRACE          (1U << 5U) /// < Name deprecated
#define CSR_ALLOW_UNRESTRICTED_DTRACE         (1U << 5U)
#define CSR_ALLOW_UNRESTRICTED_NVRAM          (1U << 6U)
#define CSR_ALLOW_DEVICE_CONFIGURATION        (1U << 7U)
#define CSR_ALLOW_ANY_RECOVERY_OS             (1U << 8U)
#define CSR_ALLOW_UNAPPROVED_KEXTS            (1U << 9U)
#define CSR_ALLOW_EXECUTABLE_POLICY_OVERRIDE  (1U << 10U)

#define CSR_VALID_FLAGS (CSR_ALLOW_UNTRUSTED_KEXTS | \
                         CSR_ALLOW_UNRESTRICTED_FS | \
                         CSR_ALLOW_TASK_FOR_PID | \
                         CSR_ALLOW_KERNEL_DEBUGGER | \
                         CSR_ALLOW_APPLE_INTERNAL | \
                         CSR_ALLOW_UNRESTRICTED_DTRACE | \
                         CSR_ALLOW_UNRESTRICTED_NVRAM | \
                         CSR_ALLOW_DEVICE_CONFIGURATION | \
                         CSR_ALLOW_ANY_RECOVERY_OS | \
                         CSR_ALLOW_UNAPPROVED_KEXTS | \
                         CSR_ALLOW_EXECUTABLE_POLICY_OVERRIDE)

#define CSR_ALWAYS_ENFORCED_FLAGS (CSR_ALLOW_DEVICE_CONFIGURATION | CSR_ALLOW_ANY_RECOVERY_OS)

///
/// CSR capabilities that a booter can give to the system.
///
#define CSR_CAPABILITY_UNLIMITED        (1U << 0U)
#define CSR_CAPABILITY_CONFIG           (1U << 1U)
#define CSR_CAPABILITY_APPLE_INTERNAL   (1U << 2U)

#define CSR_VALID_CAPABILITIES (CSR_CAPABILITY_UNLIMITED | CSR_CAPABILITY_CONFIG | CSR_CAPABILITY_APPLE_INTERNAL)

#endif // APPLE_CSR_CONFIG_H
