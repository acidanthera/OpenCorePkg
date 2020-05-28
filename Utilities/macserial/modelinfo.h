//
// Decode mac serial number
//
// Copyright (c) 2018 vit9696
//

#ifndef GENSERIAL_MODELINFO_H
#define GENSERIAL_MODELINFO_H

#include <assert.h>

#include "macserial.h"
#include "modelinfo_autogen.h"

#ifdef static_assert
static_assert(ARRAY_SIZE(AppleModelCode) == APPLE_MODEL_MAX &&
  ARRAY_SIZE(AppleBoardCode) == APPLE_MODEL_MAX &&
  ARRAY_SIZE(ApplePlatformData) == APPLE_MODEL_MAX &&
  ARRAY_SIZE(AppleModelYear) == APPLE_MODEL_MAX &&
  ARRAY_SIZE(ApplePreferredModelYear) == APPLE_MODEL_MAX,
  "Inconsistent model data");
#endif

static const char *AppleLegacyLocations[] = {
  "CK",
  "CY",
  "FC",
  "G8",
  "QP",
  "XA",
  "XB",
  "PT",
  "QT",
  "UV",
  "RN",
  "RM",
  "SG",
  "W8",
  "YM",
  // New
  "H0",
  "C0",
  "C3",
  "C7",
  "MB",
  "EE",
  "VM",
  "1C",
  "4H",
  "MQ",
  "WQ",
  "7J",
  "FK",
  "F1",
  "F2",
  "F7",
  "DL",
  "DM",
  "73",
};

#define APPLE_LEGACY_LOCATION_OLDMAX 15

static const char *AppleLegacyLocationNames[] = {
  "Ireland (Cork)",
  "Korea",
  "USA (Fountain, Colorado)",
  "USA",
  "USA",
  "USA (ElkGrove/Sacramento, California)",
  "USA (ElkGrove/Sacramento, California)",
  "Korea",
  "Taiwan (Quanta Computer)",
  "Taiwan",
  "Mexico",
  "Refurbished Model",
  "Singapore",
  "China (Shanghai)",
  "China",
  // New
  "Unknown",
  "China (Quanta Computer, Tech-Com)",
  "China (Shenzhen, Foxconn)",
  "China (Shanghai, Pegatron)",
  "Malaysia",
  "Taiwan",
  "Czech Republic (Pardubice, Foxconn)",
  "China",
  "China",
  "China",
  "China",
  "China (Hon Hai/Foxconn)",
  "China (Zhengzhou, Foxconn)",
  "China (Zhengzhou, Foxconn)",
  "China (Zhengzhou, Foxconn)",
  "China",
  "China (Foxconn)",
  "China (Foxconn)",
  "Unknown",
};

static const char *AppleLocations[] = {
  "C02",
  "C07",
  "C17",
  "C1M",
  "C2V",
  "CK2",
  "D25",
  "F5K",
  "W80",
  "W88",
  "W89",
  "CMV",
  "YM0",
  "DGK",
};

static const char *AppleLocationNames[] = {
  "China (Quanta Computer)",
  "China (Quanta Computer)",
  "China",
  "China",
  "China",
  "Ireland (Cork)",
  "Unknown",
  "USA (Flextronics)",
  "Unknown",
  "Unknown",
  "Unknown",
  "Unknown",
  "China (Hon Hai/Foxconn)",
  "Unknown"
};

#ifdef static_assert
static_assert(ARRAY_SIZE(AppleLegacyLocations) == ARRAY_SIZE(AppleLegacyLocationNames),
  "Inconsistent legacy location data");
static_assert(ARRAY_SIZE(AppleLocations) == ARRAY_SIZE(AppleLocationNames),
  "Inconsistent legacy location data");
#endif

static const char *MLBBlock1[] = {
  "200", "600", "403", "404", "405", "303", "108",
  "207", "609", "501", "306", "102", "701", "301",
  "501", "101", "300", "130", "100", "270", "310",
  "902", "104", "401", "902", "500", "700", "802"
};

static const char *MLBBlock2[] = {
  "GU", "4N", "J9", "QX", "OP", "CD", "GU"
};

static const char *MLBBlock3[] = {
  "1H", "1M", "AD", "1F", "A8", "UE", "JA", "JC", "8C", "CB", "FB"
};

// Used for line/copy:               A   B   C   D   E   F   G   H   I   J   K   L   M   N   O   P   Q   R   S   T   U   V   W   X   Y   Z
static int32_t AppleTblBase34[] =  {10, 11, 12, 13, 14, 15, 16, 17,  0, 18, 19, 20, 21, 22,  0, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33};
static const char *AppleBase34Blacklist = "IO";
static const char *AppleBase34Reverse = "0123456789ABCDEFGHJKLMNPQRSTUVWXYZ";

// Used for year:                    A   B   C   D   E   F   G   H   I   J   K   L   M   N   O   P   Q   R   S   T   U   V   W   X   Y   Z
static int32_t AppleTblYear[] =    { 0,  0,  0,  0,  0,  1,  1,  2,  0,  2,  3,  3,  4,  4,  0,  5,  5,  6,  6,  7,  0,  7,  8,  8,  9,  9};
static const char *AppleYearBlacklist = "ABEIOU";
static const char *AppleYearReverse   = "CDFGHJKLMNPQRSTVWXYZ";

// Used for week to year add:        A   B   C   D   E   F   G   H   I   J   K   L   M   N   O   P   Q   R   S   T   U   V   W   X   Y   Z
static int32_t AppleTblWeekAdd[] = { 0,  0,  0, 26,  0,  0, 26,  0,  0, 26,  0, 26,  0, 26,  0,  0, 26,  0, 26,  0,  0, 26,  0, 26,  0, 26};

// Used for week                     A   B   C   D   E   F   G   H   I   J   K   L   M   N   O   P   Q   R   S   T   U   V   W   X   Y   Z
static int32_t AppleTblWeek[] =    { 0,  0, 10, 11,  0, 12, 13, 14,  0, 15, 16, 17, 18, 19,  0, 20, 21, 22,  0, 23,  0, 24, 25, 26, 27,  0};
static const char *AppleWeekBlacklist = "ABEIOSUZ";
static const char *AppleWeekReverse   = "0123456789CDFGHJKLMNPQRTVWX123456789CDFGHJKLMNPQRTVWXY";

#ifdef static_assert
static_assert(ARRAY_SIZE(AppleTblBase34) == 26 && ARRAY_SIZE(AppleTblYear) == 26 &&
  ARRAY_SIZE(AppleTblWeekAdd) == 26 && ARRAY_SIZE(AppleTblWeek) == 26,
  "Conversion table must cover latin alphabet");
#endif

#endif // GENSERIAL_MODELINFO_H
