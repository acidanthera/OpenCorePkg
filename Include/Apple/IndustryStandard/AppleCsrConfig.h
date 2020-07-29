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

#define CSR_ALLOW_UNTRUSTED_KEXTS             BIT0
#define CSR_ALLOW_UNRESTRICTED_FS             BIT1
#define CSR_ALLOW_TASK_FOR_PID                BIT2
#define CSR_ALLOW_KERNEL_DEBUGGER             BIT3
#define CSR_ALLOW_APPLE_INTERNAL              BIT4
#define CSR_ALLOW_DESTRUCTIVE_DTRACE          BIT5 ///< Name deprecated
#define CSR_ALLOW_UNRESTRICTED_DTRACE         BIT5
#define CSR_ALLOW_UNRESTRICTED_NVRAM          BIT6
#define CSR_ALLOW_DEVICE_CONFIGURATION        BIT7
#define CSR_ALLOW_ANY_RECOVERY_OS             BIT8
#define CSR_ALLOW_UNAPPROVED_KEXTS            BIT9
#define CSR_ALLOW_EXECUTABLE_POLICY_OVERRIDE  BIT10
#define CSR_ALLOW_UNAUTHENTICATED_ROOT        BIT11 ///< Temporary name till 11.0 csr.h release

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
                         CSR_ALLOW_EXECUTABLE_POLICY_OVERRIDE | \
                         CSR_ALLOW_UNAUTHENTICATED_ROOT)

#define CSR_ALWAYS_ENFORCED_FLAGS (CSR_ALLOW_DEVICE_CONFIGURATION | CSR_ALLOW_ANY_RECOVERY_OS)

///
/// CSR capabilities that a booter can give to the system.
///
#define CSR_CAPABILITY_UNLIMITED        BIT0
#define CSR_CAPABILITY_CONFIG           BIT1
#define CSR_CAPABILITY_APPLE_INTERNAL   BIT2

#define CSR_VALID_CAPABILITIES (CSR_CAPABILITY_UNLIMITED | CSR_CAPABILITY_CONFIG | CSR_CAPABILITY_APPLE_INTERNAL)

#endif // APPLE_CSR_CONFIG_H
