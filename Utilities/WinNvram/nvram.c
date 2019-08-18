/** @file

Reads/writes to macOS NVRAM storage on Windows.

Copyright (c) 2018, vit9696

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <windows.h>

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

/* Boilerplate asking privileges to write to the nvram's efi store. */
void obtain_privilege() {
  HANDLE hToken;
  TOKEN_PRIVILEGES tkp;

  OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
  LookupPrivilegeValue(NULL, SE_SYSTEM_ENVIRONMENT_NAME, &tkp.Privileges[0].Luid);
  tkp.PrivilegeCount = 1;
  tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
  AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage:\n  nvram var\n  nvram var value\n");
    return -1;
  }

  const char *variable = argv[1];
  const char *guid = "{7C436110-AB2A-4BBB-A880-FE41995C9F82}";
  int code = EXIT_SUCCESS;

  obtain_privilege();

  if (argc == 2) {
    char readValue[256] = {0};
    uint32_t readValueSize = GetFirmwareEnvironmentVariableA(
      variable, guid, readValue, sizeof(readValue)-1);
    if (readValueSize > 0) {
      printf("%s = %s\n", variable, readValue);
    } else {
      printf("Failed to read %s, code %u\n",
        variable, (unsigned)GetLastError());
      code = EXIT_FAILURE;
    }
  } else {
    bool ok = SetFirmwareEnvironmentVariableA(
      variable, guid, argv[2], strlen(argv[2]));
    if (ok) {
      printf("%s was set to %s\n", variable, argv[2]);
    } else {
      printf("Failed to set %s to %s, code %u\n",
        variable, argv[2], (unsigned)GetLastError());
      code = EXIT_FAILURE;
    }
  }

  return code;
}
