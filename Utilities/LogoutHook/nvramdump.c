//
//  main.c
//  NVRAMdump
//
//  Created by Rodion Shingarev on 27.12.20.
//  Copyright © 2020 Rodion Shingarev. All rights reserved.
//  based on the original NVRAM utility
//  Copyright (c) 2000-2016 Apple Inc. All rights reserved.
//  https://opensource.apple.com/source/system_cmds/system_cmds-854.40.2/nvram.tproj/nvram.c
//

#include <stdio.h>

#ifdef __APPLE__

#include <stdlib.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOKitKeys.h>
#include <CoreFoundation/CoreFoundation.h>
#include <err.h>
#include <mach/mach_error.h>
#include <string.h>

static io_registry_entry_t gOptionsRef;

// GetOFVariable(name, nameRef, valueRef)
//
//   Get the named firmware variable.
//   Return it and its symbol in valueRef and nameRef.
//
static kern_return_t GetOFVariable(char *name, CFStringRef *nameRef,
                                   CFTypeRef *valueRef)
{
    *nameRef = CFStringCreateWithCString(kCFAllocatorDefault, name,
                                         kCFStringEncodingUTF8);
    if (*nameRef == 0) {
        errx(1, "Error creating CFString for key %s", name);
    }

    *valueRef = IORegistryEntryCreateCFProperty(gOptionsRef, *nameRef, 0, 0);
    if (*valueRef == 0) return kIOReturnNotFound;

    return KERN_SUCCESS;
}

CFDictionaryRef CreateMyDictionary(void) {
    char *guid;

    // root
    CFMutableDictionaryRef dict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0,
                                                            &kCFTypeDictionaryKeyCallBacks,
                                                            &kCFTypeDictionaryValueCallBacks);
    //version
    int version = 1;
    CFNumberRef num = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &version);
    CFDictionarySetValue(dict, CFSTR("Version"), num);
    CFRelease(num);
    //Add
    CFMutableDictionaryRef dict0 = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks,
                                                             &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(dict, CFSTR("Add"), dict0);
    CFRelease(dict0);

    //   APPLE_BOOT_VARIABLE_GUID
    //   Print all of the firmware variables.
    CFMutableDictionaryRef dict1;
    kern_return_t          result;
    result = IORegistryEntryCreateCFProperties(gOptionsRef, &dict1, 0, 0);
    if (result != KERN_SUCCESS) {
        errx(1, "Error getting the firmware variables: %s", mach_error_string(result));
    }
    CFDictionarySetValue(dict0, CFSTR("7C436110-AB2A-4BBB-A880-FE41995C9F82"), dict1);
    CFRelease(dict1);

    CFMutableDictionaryRef dict2 = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks,
                                                             &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(dict0, CFSTR("8BE4DF61-93CA-11D2-AA0D-00E098032B8C"), dict2);

    //   EFI_GLOBAL_VARIABLE_GUID
    //   Print the given firmware variable.
    CFStringRef   nameRef;
    CFTypeRef     valueRef;

    const char *key[32];
    char name[64];
    CFStringRef   var;
    int i = 0;
    int n = 7; // num of keys in this GUID

    key[0] = "Boot0080";
    key[1] = "BootOrder";
    key[2] = "BootNext";
    key[3] = "Boot0081";
    key[4] = "Boot0082";
    key[5] = "Boot0083";
    key[6] = "BootCurrent";
    guid = "8BE4DF61-93CA-11D2-AA0D-00E098032B8C";

    for ( i = 0; i < n; i++ ) {
        snprintf(name, sizeof(name), "%s:%s", guid, key[i]);
        var = CFStringCreateWithCString(NULL, key[i], kCFStringEncodingUTF8);
        result = GetOFVariable(name, &nameRef, &valueRef);
        if (result == KERN_SUCCESS) {
            CFDictionaryAddValue (dict2, var, valueRef);
            CFRelease(valueRef);
        }
        CFRelease(nameRef);
    }
    CFRelease(dict2);

    //  APPLE_WIRELESS_NETWORK_VARIABLE_GUID
    CFMutableDictionaryRef dict3 = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks,
                                                             &kCFTypeDictionaryValueCallBacks);
    CFDictionarySetValue(dict0, CFSTR("36C28AB5-6566-4C50-9EBD-CBB920F83843"), dict3);
    n = 3; // num of keys in this GUID
    key[0] = "current-network";
    key[1] = "preferred-count";
    key[2] = "preferred-networks";
    guid = "36C28AB5-6566-4C50-9EBD-CBB920F83843";

    for ( i = 0; i < n; i++ ) {
        snprintf(name, sizeof(name), "%s:%s", guid, key[i]);
        var = CFStringCreateWithCString(NULL, key[i], kCFStringEncodingUTF8);
        result = GetOFVariable(name, &nameRef, &valueRef);
        if (result == KERN_SUCCESS) {
            CFDictionaryAddValue (dict3, var, valueRef);
            CFRelease(valueRef);
        }
        CFRelease(nameRef);
    }
    CFRelease(dict3);

    return dict;
}

int main(int argc, const char * argv[]) {
    CFDataRef data;
    FILE *fp;
    kern_return_t       result;
    mach_port_t         masterPort;

    result = IOMasterPort(bootstrap_port, &masterPort);
    if (result != KERN_SUCCESS) {
        errx(1, "Error getting the IOMaster port: %s",
             mach_error_string(result));
    }

    gOptionsRef = IORegistryEntryFromPath(masterPort, "IODeviceTree:/options");
    if (gOptionsRef == 0) {
        errx(1, "nvram is not supported on this system");
    }
    CFPropertyListRef propertyList = CreateMyDictionary();
    data = CFPropertyListCreateData( kCFAllocatorDefault, propertyList, kCFPropertyListXMLFormat_v1_0, 0, NULL );
    if (data == NULL) {
        errx(1, "Error converting variables to xml");
    }
    fp = fopen ("/tmp/nvram.plist", "w");
    if (fp == NULL) {
        errx(1, "Error opening /tmp/nvram.plist");
    }
    fwrite(CFDataGetBytePtr(data), sizeof(UInt8), CFDataGetLength(data), fp);
    fclose (fp);
    CFRelease(data);
    IOObjectRelease(gOptionsRef);
    return 0;
}

#else

int main() {
    fprintf(stderr, "This utility is only supported on macOS\n");
    return 1;
}

#endif
