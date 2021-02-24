//
// OpenCore project
// Acidanthera version detection code
// Copyright (c) 2021 vit9696
// Public domain release
//

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <assert.h>

#if defined(__APPLE__)

#include <IOKit/graphics/IOGraphicsLib.h>
#include <CoreFoundation/CoreFoundation.h>

#define LILU_SERVICE_NAME    "Lilu"
#define ALC_SERVICE_NAME     "AppleALC"
#define ARPT_SERVICE_NAME    "AirportBrcmFixup"
#define BRKEYS_SERVICE_NAME  "BrightnessKeys"
#define CPUF_SERVICE_NAME    "CPUFriend"
#define CPUTSC_SERVICE_NAME  "CpuTscSync"
#define DBGEN_SERVICE_NAME   "DebugEnhancer"
#define HBFX_SERVICE_NAME    "HibernationFixup"
#define NVMEF_SERVICE_NAME   "NVMeFix"
#define RSTREV_SERVICE_NAME  "RestrictEvents"
#define RTCFX_SERVICE_NAME   "RTCMemoryFixup"
#define SMCBAT_SERVICE_NAME  "SMCBatteryManager"
#define SMCCPU_SERVICE_NAME  "SMCProcessor"
#define SMCDELL_SERVICE_NAME "SMCDellSensors"
#define SMCLIGH_SERVICE_NAME "SMCLightSensor"
#define SMCSSIO_SERVICE_NAME "SMCSuperIO"
#define VSMC_SERVICE_NAME    "VirtualSMC"
#define WEG_SERVICE_NAME     "WhateverGreen"

#define ACDT_VERSION_NAME    "VersionInfo"
#define ACDT_VERSION_LENGTH  32

typedef struct {
  char build_type[3];
  char delim1;
  char version_major;
  char version_minor;
  char version_patch;
  char delim2;
  char year[4];
  char delim3;
  char month[2];
  char delim4;
  char day[2];
} acdt_version_t;

static CFStringRef acdt_kext_list[] = {
  CFSTR(LILU_SERVICE_NAME),
  CFSTR(ALC_SERVICE_NAME),
  CFSTR(ARPT_SERVICE_NAME),
  CFSTR(BRKEYS_SERVICE_NAME),
  CFSTR(CPUF_SERVICE_NAME),
  CFSTR(CPUTSC_SERVICE_NAME),
  CFSTR(DBGEN_SERVICE_NAME),
  CFSTR(HBFX_SERVICE_NAME),
  CFSTR(NVMEF_SERVICE_NAME),
  CFSTR(RSTREV_SERVICE_NAME),
  CFSTR(RTCFX_SERVICE_NAME),
  CFSTR(SMCBAT_SERVICE_NAME),
  CFSTR(SMCCPU_SERVICE_NAME),
  CFSTR(SMCDELL_SERVICE_NAME),
  CFSTR(SMCLIGH_SERVICE_NAME),
  CFSTR(SMCSSIO_SERVICE_NAME),
  CFSTR(VSMC_SERVICE_NAME),
  CFSTR(WEG_SERVICE_NAME),
};

#define ACDT_KEXT_NUM (sizeof(acdt_kext_list)/sizeof(acdt_kext_list[0]))

io_object_t acdt_get_service_worker(CFStringRef service_name, io_iterator_t device_iterator) {
  io_object_t object;
  while ((object = IOIteratorNext(device_iterator))) {
    CFTypeRef class_name = IORegistryEntryCreateCFProperty(object, CFSTR("IOClass"), kCFAllocatorDefault, 0);
    if (class_name != NULL
      && CFGetTypeID(class_name) == CFStringGetTypeID()
      && CFEqual(class_name, service_name)) {
      break;
    }

    IOObjectRelease(object);

    kern_return_t result = IORegistryIteratorEnterEntry(device_iterator);
    if (result == kIOReturnSuccess) {
      object = acdt_get_service_worker(service_name, device_iterator);
      IORegistryIteratorExitEntry(device_iterator);
      if (object != 0) {
        break;
      }
    }  
  }

  return object;
}

io_object_t acdt_get_service(CFStringRef service_name) {
  io_iterator_t device_iterator;
  kern_return_t result = IORegistryCreateIterator(kIOMasterPortDefault,
    kIOServicePlane, 0, &device_iterator);
  if (result != kIOReturnSuccess) {
    fprintf(stderr, "ERROR: Unable to connect to access IOService - %d\n", result);
    return 0;
  }

  io_object_t object = acdt_get_service_worker(service_name, device_iterator);
  IOObjectRelease(device_iterator);
  return object;
}

bool acdt_detect_kext(CFStringRef service, char *version) {
  assert(service != NULL);
  assert(version != NULL);

  version[0] = '\0';

  io_object_t object = acdt_get_service(service);
  if (object == 0) {
    return false;
  }

  // Not every plugin is published as a service, thus do recursive property iteration.
  CFTypeRef version_name = IORegistryEntryCreateCFProperty(object,
    CFSTR(ACDT_VERSION_NAME), kCFAllocatorDefault, 0);
  if (version_name != NULL && CFGetTypeID(version_name) == CFStringGetTypeID()) {
    if (!CFStringGetCString(version_name, version, ACDT_VERSION_LENGTH, kCFStringEncodingUTF8)) {
      fprintf(stderr, "ERROR: Unable to decode version string\n");
    }
  } else {
    // This is non-fatal for older plugins that may not publish versions.
    fprintf(stderr, "ERROR: Kext has no version info, make sure you have the latest version.\n");
  }

  IOObjectRelease(object);

  return true;
}

const char *acdt_get_build_type(acdt_version_t *version) {
  if (strncmp(version->build_type, "DBG", 3) == 0) {
    return "DEBUG";
  }
  if (strncmp(version->build_type, "REL", 3) == 0) {
    return "RELEASE";
  }
  if (strncmp(version->build_type, "NPT", 3) == 0) {
    return "NOOPT";
  }
  return "Unknown";
}

int main() {
  char tmp_version[ACDT_VERSION_LENGTH];
  acdt_version_t acdt_version;
  printf("Installed kexts:\n");
  for (size_t i = 0; i < ACDT_KEXT_NUM; ++i) {
    bool has_kext = acdt_detect_kext(acdt_kext_list[i], tmp_version);
    printf("- %-32s: ", CFStringGetCStringPtr(acdt_kext_list[i], kCFStringEncodingUTF8));
    if (has_kext && tmp_version[0] != '\0') {
      memcpy(&acdt_version, tmp_version, sizeof(acdt_version));
      printf("Yes (%c.%c.%c, published %.2s.%.2s.%.4s, %s)\n", acdt_version.version_major,
        acdt_version.version_minor, acdt_version.version_patch,
        acdt_version.day, acdt_version.month, acdt_version.year,
        acdt_get_build_type(&acdt_version));
    } else if (has_kext) {
      printf("Yes\n");
    } else {
      puts("No");
    }
  }
}

#else

int main() {
  fprintf(stderr, "This utility requires macOS\n");
  return -1;
}

#endif
