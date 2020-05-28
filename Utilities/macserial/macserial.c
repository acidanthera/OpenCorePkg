//
// Decode mac serial number
//
// Copyright (c) 2018 vit9696
//

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#endif

#include "macserial.h"
#include "modelinfo.h"

#ifdef __APPLE__
static CFTypeRef get_ioreg_entry(const char *path, CFStringRef name, CFTypeID type) {
  CFTypeRef value = NULL;
  io_registry_entry_t entry = IORegistryEntryFromPath(kIOMasterPortDefault, path);
  if (entry) {
    value = IORegistryEntryCreateCFProperty(entry, name, kCFAllocatorDefault, 0);
    if (value) {
      if (CFGetTypeID(value) != type) {
        CFRelease(value);
        value = NULL;
        printf("%s in %s has wrong type!\n", CFStringGetCStringPtr(name, kCFStringEncodingMacRoman), path);
      }
    } else {
      printf("Failed to find to %s in %s!\n", CFStringGetCStringPtr(name, kCFStringEncodingMacRoman), path);
    }
    IOObjectRelease(entry);
  } else {
    printf("Failed to connect to %s!\n", path);
  }
  return value;
}
#endif

// Apple uses various conversion tables (e.g. AppleBase34) for value encoding.
static int32_t alpha_to_value(char c, int32_t *conv, const char *blacklist) {
  if (c < 'A' || c > 'Z')
    return -1;

  while (blacklist && *blacklist != '\0')
    if (*blacklist++ == c)
      return -1;

  return conv[c - 'A'];
}

// This is modified base34 used by Apple with I and O excluded.
static int32_t base34_to_value(char c, int32_t mul) {
  if (c >= '0' && c <= '9')
    return (c - '0') * mul;
  if (c >= 'A' && c <= 'Z') {
      int32_t tmp = alpha_to_value(c, AppleTblBase34, AppleBase34Blacklist);
      if (tmp >= 0)
        return tmp * mul;
  }
  return -1;
}

static int32_t line_to_rmin(int32_t line) {
  // info->line[0] is raw decoded copy, but it is not the real first produced unit.
  // To get the real copy we need to find the minimal allowed raw decoded copy,
  // which allows to obtain info->decodedLine.
  int rmin = 0;
  if (line > SERIAL_LINE_REPR_MAX)
    rmin = (line - SERIAL_LINE_REPR_MAX + 67) / 68;
  return rmin;
}

// This one is modded to implement CCC algo for better generation.
// Changed base36 to base34, since that's what Apple uses.
// The algo is trash but is left for historical reasons.
static bool get_ascii7(uint32_t value, char *dst, size_t sz) {
  // This is CCC conversion.
  if (value < 1000000)
    return false;

  while (value > 10000000)
    value /= 10;

  // log(2**64) / log(34) = 12.57 => max 13 char + '\0'
  char buffer[14];
  size_t offset = sizeof(buffer);

  buffer[--offset] = '\0';
  do {
    buffer[--offset] = AppleBase34Reverse[value % 34];
  } while (value /= 34);

  strncpy(dst, &buffer[offset], sz-1);
  dst[sz-1] = 0;

  return true;
}

static bool verify_mlb_checksum(const char *mlb, size_t len) {
  const char alphabet[] = "0123456789ABCDEFGHJKLMNPQRSTUVWXYZ";
  size_t checksum = 0;
  for (size_t i = 0; i < len; ++i) {
    for (size_t j = 0; j <= sizeof (alphabet); ++j) {
      if (j == sizeof (alphabet))
        return false;
      if (mlb[i] == alphabet[j]) {
        checksum += (((i & 1) == (len & 1)) * 2 + 1) * j;
        break;
      }
    }
  }
  return checksum % (sizeof(alphabet) - 1) == 0;
}

// Taken from https://en.wikipedia.org/wiki/Xorshift#Example_implementation
// I am not positive what is better to use here (especially on Windows).
// Fortunately we only need something only looking random.
static uint32_t pseudo_random(void) {
 #ifdef __GNUC__
    if (arc4random)
      return arc4random();
#endif

  static uint32_t state;

  if (!state) {
    fprintf(stderr, "Warning: arc4random is not available!\n");
    state = (uint32_t)time(NULL);
  }

  uint32_t x = state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  state = x;
  return x;
}

// Taken from https://opensource.apple.com/source/Libc/Libc-1082.50.1/gen/FreeBSD/arc4random.c
// Mac OS X 10.6.8 and earlier do not have arc4random_uniform, so we implement one.
static uint32_t pseudo_random_between(uint32_t from, uint32_t to) {
  uint32_t upper_bound = to + 1 - from;

#ifdef __GNUC__
  // Prefer native implementation if available.
  if (arc4random_uniform)
    return from + arc4random_uniform(upper_bound);
#endif

  uint32_t r, min;

  if (upper_bound < 2)
    return from;

#if (ULONG_MAX > 0xffffffffUL)
  min = 0x100000000UL % upper_bound;
#else
  if (upper_bound > 0x80000000)
    min = 1 + ~upper_bound;
  else
    min = ((0xffffffff - (upper_bound * 2)) + 1) % upper_bound;
#endif

  for (;;) {
    r = pseudo_random();
    if (r >= min)
      break;
  }

  return from + r % upper_bound;
}

static int32_t get_current_model(void) {
#ifdef __APPLE__
  CFDataRef model = get_ioreg_entry("IODeviceTree:/", CFSTR("model"), CFDataGetTypeID());
  if (model) {
    const char *cptr = (const char *)CFDataGetBytePtr(model);
    size_t len = (size_t)CFDataGetLength(model);
    int32_t i;
    for (i = 0; i < APPLE_MODEL_MAX; i++) {
      if (!strncmp(ApplePlatformData[i].productName, cptr, len))
        break;
    }
    CFRelease(model);
    if (i < APPLE_MODEL_MAX)
      return i;
  }
#endif
  return -1;
}

static uint32_t get_production_year(AppleModel model, bool print) {
  uint32_t *years = &AppleModelYear[model][0];
  uint32_t num = 0;

  for (num = 0; num < APPLE_MODEL_YEAR_MAX && years[num]; num++) {
    if (print) {
      if (num+1 != APPLE_MODEL_YEAR_MAX && years[num+1])
        printf("%d, ", years[num]);
      else
        printf("%d\n", years[num]);
    }
  }

  if (ApplePreferredModelYear[model] > 0)
    return ApplePreferredModelYear[model];

  return years[pseudo_random() % num];
}

static const char *get_model_code(AppleModel model, bool print) {
  const char **codes = &AppleModelCode[model][0];

  if (print) {
    for (uint32_t i = 0; i < APPLE_MODEL_CODE_MAX && codes[i]; i++)
      if (i+1 != APPLE_MODEL_CODE_MAX && codes[i+1])
        printf("%s, ", codes[i]);
      else
        printf("%s\n", codes[i]);
  }

  // Always choose the first model for stability by default.
  return codes[0];
}

static const char *get_board_code(AppleModel model, bool print) {
  const char **codes = &AppleBoardCode[model][0];

  if (print) {
    for (uint32_t i = 0; i < APPLE_BOARD_CODE_MAX && codes[i]; i++)
      if (i+1 != APPLE_BOARD_CODE_MAX && codes[i+1])
        printf("%s, ", codes[i]);
      else
        printf("%s\n", codes[i]);
  }

  // Always choose the first model for stability by default.
  return codes[0];
}

static bool get_serial_info(const char *serial, SERIALINFO *info, bool print) {
  if (!info)
    return false;

  memset(info, 0, sizeof(SERIALINFO));

  // Verify length.
  size_t serial_len = strlen(serial);
  if (serial_len != SERIAL_OLD_LEN && serial_len != SERIAL_NEW_LEN) {
    printf("ERROR: Invalid serial length, must be %d or %d\n", SERIAL_NEW_LEN, SERIAL_OLD_LEN);
    return false;
  }

  // Assume every serial valid by default.
  info->valid = true;

  // Verify alphabet (base34 with I and O exclued).
  for (size_t i = 0; i < serial_len; i++) {
    if (!((serial[i] >= 'A' && serial[i] <= 'Z' && serial[i] != 'O' && serial[i] != 'I') ||
          (serial[i] >= '0' && serial[i] <= '9'))) {
      printf("WARN: Invalid symbol '%c' in serial!\n", serial[i]);
      info->valid = false;
    }
  }

  size_t model_len = 0;

  // Start with looking up the model.
  info->modelIndex = -1;
  for (uint32_t i = 0; i < ARRAY_SIZE(AppleModelCode); i++) {
    for (uint32_t j = 0; j < APPLE_MODEL_CODE_MAX; j++) {
      const char *code = AppleModelCode[i][j];
      if (!code)
        break;
      model_len = strlen(code);
      if (model_len == 0)
        break;
      assert(model_len == MODEL_CODE_OLD_LEN || model_len == MODEL_CODE_NEW_LEN);
      if (((serial_len == SERIAL_OLD_LEN && model_len == MODEL_CODE_OLD_LEN)
        || (serial_len == SERIAL_NEW_LEN && model_len == MODEL_CODE_NEW_LEN))
        && !strncmp(serial + serial_len - model_len, code, model_len)) {
        strncpy(info->model, code, sizeof(info->model));
        info->model[sizeof(info->model)-1] = '\0';
        info->modelIndex = (int32_t)i;
        break;
      }
    }
  }

  // Also lookup apple model.
  for (uint32_t i = 0; i < ARRAY_SIZE(AppleModelDesc); i++) {
    const char *code = AppleModelDesc[i].code;
    model_len = strlen(code);
    assert(model_len == MODEL_CODE_OLD_LEN || model_len == MODEL_CODE_NEW_LEN);
    if (((serial_len == SERIAL_OLD_LEN && model_len == MODEL_CODE_OLD_LEN)
      || (serial_len == SERIAL_NEW_LEN && model_len == MODEL_CODE_NEW_LEN))
      && !strncmp(serial + serial_len - model_len, code, model_len)) {
      info->appleModel = AppleModelDesc[i].name;
      break;
    }
  }

  // Fallback to possibly valid values if model is unknown.
  if (info->modelIndex == -1) {
    if (serial_len == SERIAL_NEW_LEN)
      model_len = MODEL_CODE_NEW_LEN;
    else
      model_len = MODEL_CODE_OLD_LEN;
    strncpy(info->model, serial + serial_len - model_len, model_len);
    info->model[model_len] = '\0';
  }

  // Lookup production location
  info->legacyCountryIdx = -1;
  info->modernCountryIdx = -1;

  if (serial_len == SERIAL_NEW_LEN) {
    strncpy(info->country, serial, COUNTRY_NEW_LEN);
    info->country[COUNTRY_NEW_LEN] = '\0';
    serial += COUNTRY_NEW_LEN;
    for (size_t i = 0; i < ARRAY_SIZE(AppleLocations); i++) {
      if (!strcmp(info->country, AppleLocations[i])) {
        info->modernCountryIdx = (int32_t)i;
        break;
      }
    }
  } else {
    strncpy(info->country, serial, COUNTRY_OLD_LEN);
    info->country[COUNTRY_OLD_LEN] = '\0';
    serial += COUNTRY_OLD_LEN;
    for (size_t i = 0; i < ARRAY_SIZE(AppleLegacyLocations); i++) {
      if (!strcmp(info->country, AppleLegacyLocations[i])) {
        info->legacyCountryIdx = (int32_t)i;
        break;
      }
    }
  }

  // Decode production year and week
  if (serial_len == SERIAL_NEW_LEN) {
    // These are not exactly year and week, lower year bit is used for week encoding.
    info->year[0] = *serial++;
    info->week[0] = *serial++;

    // New encoding started in 2010.
    info->decodedYear = alpha_to_value(info->year[0], AppleTblYear, AppleYearBlacklist);
    // Since year can be encoded ambiguously, check the model code for 2010/2020 difference.
    if (info->decodedYear == 0 && info->model[0] >= 'H') {
      info->decodedYear += 2020;
    } else if (info->decodedYear >= 0) {
      info->decodedYear += 2010;
    } else {
      printf("WARN: Invalid year symbol '%c'!\n", info->year[0]);
      info->valid = false;
    }

    if (info->week[0] > '0' && info->week[0] <= '9')
      info->decodedWeek = info->week[0] - '0';
    else
      info->decodedWeek = alpha_to_value(info->week[0], AppleTblWeek, AppleWeekBlacklist);
    if (info->decodedWeek > 0) {
      if (info->decodedYear > 0)
        info->decodedWeek += alpha_to_value(info->year[0], AppleTblWeekAdd, NULL);
    } else {
      printf("WARN: Invalid week symbol '%c'!\n", info->week[0]);
      info->valid = false;
    }
  } else {
    info->year[0] = *serial++;
    info->week[0] = *serial++;
    info->week[1] = *serial++;

    // This is proven by MacPro5,1 valid serials from 2011 and 2012.
    if (info->year[0] >= '0' && info->year[0] <= '2') {
      info->decodedYear = 2010 + info->year[0] - '0';
    } else if (info->year[0] >= '3' && info->year[0] <= '9') {
      info->decodedYear = 2000 + info->year[0] - '0';
    } else {
      info->decodedYear = -1;
      printf("WARN: Invalid year symbol '%c'!\n", info->year[0]);
      info->valid = false;
    }

    for (int32_t i = 0; i < 2; i++) {
      if (info->week[i] >= '0' && info->week[i] <= '9') {
        info->decodedWeek += (i == 0 ? 10 : 1) * (info->week[i] - '0');
      } else {
        info->decodedWeek = -1;
        printf("WARN: Invalid week symbol '%c'!\n", info->week[i]);
        info->valid = false;
        break;
      }
    }
  }

  if (info->decodedWeek < SERIAL_WEEK_MIN || info->decodedWeek > SERIAL_WEEK_MAX) {
    printf("WARN: Decoded week %d is out of valid range [%d, %d]!\n", info->decodedWeek, SERIAL_WEEK_MIN, SERIAL_WEEK_MAX);
    info->decodedWeek = -1;
  }

  if (info->decodedYear > 0 && info->modelIndex >= 0) {
    bool found = false;
    for (size_t i = 0; !found && i < APPLE_MODEL_YEAR_MAX && AppleModelYear[info->modelIndex][i]; i++)
      if ((int32_t)AppleModelYear[info->modelIndex][i] == info->decodedYear)
        found = true;
    if (!found) {
      printf("WARN: Invalid year %d for model %s\n", info->decodedYear, ApplePlatformData[info->modelIndex].productName);
      info->valid = false;
    }
  }

  // Decode production line and copy
  int32_t mul[] = {68, 34, 1};
  for (uint32_t i = 0; i < ARRAY_SIZE(mul); i++) {
    info->line[i] = *serial++;
    int32_t tmp = base34_to_value(info->line[i], mul[i]);
    if (tmp >= 0) {
      info->decodedLine += tmp;
    } else {
      printf("WARN: Invalid line symbol '%c'!\n", info->line[i]);
      info->valid = false;
      break;
    }
  }

  if (info->decodedLine >= 0)
    info->decodedCopy = base34_to_value(info->line[0], 1) - line_to_rmin(info->decodedLine);

  if (print) {
    printf("%14s: %4s - ", "Country", info->country);
    if (info->legacyCountryIdx >= 0)
      printf("%s\n", AppleLegacyLocationNames[info->legacyCountryIdx]);
    else if (info->modernCountryIdx >= 0)
      printf("%s\n", AppleLocationNames[info->modernCountryIdx]);
    else
      puts("Unknown, please report!");
    printf("%14s: %4s - %d\n", "Year", info->year, info->decodedYear);
    printf("%14s: %4s - %d", "Week", info->week, info->decodedWeek);
    if (info->decodedYear > 0 && info->decodedWeek > 0) {
      struct tm startd = {
        .tm_isdst = -1,
        .tm_year = info->decodedYear - 1900,
        .tm_mday = 1 + 7 * (info->decodedWeek-1),
        .tm_mon = 0
      };
      if (mktime(&startd) >= 0) {
        printf(" (%02d.%02d.%04d", startd.tm_mday, startd.tm_mon+1, startd.tm_year+1900);
        if (info->decodedWeek == 53 && startd.tm_mday != 31) {
          printf("-31.12.%04d", startd.tm_year+1900);
        } else if (info->decodedWeek < 53) {
          startd.tm_mday += 6;
          if (mktime(&startd))
            printf("-%02d.%02d.%04d", startd.tm_mday, startd.tm_mon+1, startd.tm_year+1900);
        }
        puts(")");
      }
    } else {
      puts("");
    }
    printf("%14s: %4s - %d (copy %d)\n", "Line", info->line, info->decodedLine,
      info->decodedCopy >= 0 ? info->decodedCopy + 1 : -1);
    printf("%14s: %4s - %s\n", "Model", info->model, info->modelIndex >= 0 ?
      ApplePlatformData[info->modelIndex].productName : "Unknown");
    printf("%14s: %s\n", "SystemModel", info->appleModel != NULL ? info->appleModel : "Unknown, please report!");
    printf("%14s: %s\n", "Valid", info->valid ? "Possibly" : "Unlikely");
  }

  return true;
}

static bool get_serial(SERIALINFO *info) {
  if (info->modelIndex < 0 && info->model[0] == '\0') {
    printf("ERROR: Unable to determine model!\n");
    return false;
  }

  if (info->model[0] == '\0') {
    strncpy(info->model, get_model_code((AppleModel)info->modelIndex, false), MODEL_CODE_NEW_LEN);
    info->model[MODEL_CODE_NEW_LEN] = '\0';
  }

  size_t country_len = strlen(info->country);
  if (country_len == 0) {
    // Random country choice strongly decreases key verification probability.
    country_len = strlen(info->model) == MODEL_CODE_NEW_LEN ? COUNTRY_NEW_LEN : COUNTRY_OLD_LEN;
    if (info->modelIndex < 0) {
      strncpy(info->country, country_len == COUNTRY_OLD_LEN ? AppleLegacyLocations[0] : AppleLocations[0], COUNTRY_NEW_LEN+1);
    } else {
      strncpy(info->country, &ApplePlatformData[info->modelIndex].serialNumber[0], country_len);
      info->country[country_len] = '\0';
    }
  }

  if (info->decodedYear < 0) {
    if (info->modelIndex < 0)
      info->decodedYear = country_len == COUNTRY_OLD_LEN ? SERIAL_YEAR_OLD_MAX : SERIAL_YEAR_NEW_MAX;
    else
      info->decodedYear = (int32_t)get_production_year((AppleModel)info->modelIndex, false);
  }

  // Last week is too rare to care
  if (info->decodedWeek < 0)
    info->decodedWeek = (int32_t)pseudo_random_between(SERIAL_WEEK_MIN, SERIAL_WEEK_MAX-1);

  if (country_len == COUNTRY_OLD_LEN) {
    if (info->decodedYear < SERIAL_YEAR_OLD_MIN || info->decodedYear > SERIAL_YEAR_OLD_MAX) {
      printf("ERROR: Year %d is out of valid legacy range [%d, %d]!\n", info->decodedYear, SERIAL_YEAR_OLD_MIN, SERIAL_YEAR_OLD_MAX);
      return false;
    }

    info->year[0] = '0' + (char)((info->decodedYear - 2000) % 10);
    info->week[0] = '0' + (char)((info->decodedWeek) / 10);
    info->week[1] = '0' + (info->decodedWeek) % 10;
  } else {
    if (info->decodedYear < SERIAL_YEAR_NEW_MIN || info->decodedYear > SERIAL_YEAR_NEW_MAX) {
      printf("ERROR: Year %d is out of valid modern range [%d, %d]!\n", info->decodedYear, SERIAL_YEAR_NEW_MIN, SERIAL_YEAR_NEW_MAX);
      return false;
    }

    size_t base_new_year = 2010;
    if (info->decodedYear == SERIAL_YEAR_NEW_MAX) {
      base_new_year = 2020;
    }

    info->year[0] = AppleYearReverse[(info->decodedYear - base_new_year) * 2 + (info->decodedWeek >= 27)];
    info->week[0] = AppleWeekReverse[info->decodedWeek];
  }

  if (info->decodedLine < 0)
    info->decodedLine = (int32_t)pseudo_random_between(SERIAL_LINE_MIN, SERIAL_LINE_MAX);

  int32_t rmin = line_to_rmin(info->decodedLine);

  // Verify and apply user supplied copy if any
  if (info->decodedCopy >= 0) {
    rmin += info->decodedCopy - 1;
    if (rmin * 68 > info->decodedLine) {
      printf("ERROR: Copy %d cannot represent line %d!\n", info->decodedCopy, info->decodedLine);
      return false;
    }
  }

  info->line[0] = AppleBase34Reverse[rmin];
  info->line[1] = AppleBase34Reverse[(info->decodedLine - rmin * 68) / 34];
  info->line[2] = AppleBase34Reverse[(info->decodedLine - rmin * 68) % 34];

  return true;
}

static void get_mlb(SERIALINFO *info, char *dst, size_t sz) {
  // This is a direct reverse from CCC, rework it later...
  do {
    uint32_t year = 0, week = 0;

    bool legacy = strlen(info->country) == COUNTRY_OLD_LEN;

    if (legacy) {
      year = (uint32_t)(info->year[0] - '0');
      week = (uint32_t)(info->week[0] - '0') * 10 + (uint32_t)(info->week[1] - '0');
    } else {
      char syear = info->year[0];
      char sweek = info->week[0];

      const char srcyear[] = "CDFGHJKLMNPQRSTVWXYZ";
      const char dstyear[] = "00112233445566778899";
      for (size_t i = 0; i < ARRAY_SIZE(srcyear)-1; i++) {
        if (syear == srcyear[i]) {
          year = (uint32_t)(dstyear[i] - '0');
          break;
        }
      }

      const char overrides[] = "DGJLNQSVXZ";
      for (size_t i = 0; i < ARRAY_SIZE(overrides)-1; i++) {
        if (syear == overrides[i]) {
          week = 27;
          break;
        }
      }

      const char srcweek[] = "123456789CDFGHJKLMNPQRSTVWXYZ";
      for (size_t i = 0; i < ARRAY_SIZE(srcweek)-1; i++) {
        if (sweek == srcweek[i]) {
          week += i + 1;
          break;
        }
      }

      // This is silently not handled, and it should not be needed for normal serials.
      // Bugged MacBookPro6,2 and MacBookPro7,1 will gladly hit it.
      if (week < SERIAL_WEEK_MIN) {
        snprintf(dst, sz, "FAIL-ZERO-%c", sweek);
        return;
      }
    }

    week--;

    if (week <= 9) {
      if (week == 0) {
        week = SERIAL_WEEK_MAX;
        if (year == 0)
          year = 9;
        else
          year--;
      }
    }

    if (legacy) {
      char code[4] = {0};
      // The loop is not present in CCC, but it throws an exception here,
      // and effectively generates nothing. The logic is crazy :/.
      // Also, it was likely meant to be written as pseudo_random() % 0x8000.
      while (!get_ascii7(pseudo_random_between(0, 0x7FFE) * 0x73BA1C, code, sizeof(code)));
      const char *board = get_board_code(info->modelIndex, false);
      char suffix = AppleBase34Reverse[pseudo_random() % 34];
      snprintf(dst, sz, "%s%d%02d0%s%s%c", info->country, year, week, code, board, suffix);
    } else {
      const char *part1 = MLBBlock1[pseudo_random() % ARRAY_SIZE(MLBBlock1)];
      const char *part2 = MLBBlock2[pseudo_random() % ARRAY_SIZE(MLBBlock2)];
      const char *board = get_board_code(info->modelIndex, false);
      const char *part3 = MLBBlock3[pseudo_random() % ARRAY_SIZE(MLBBlock3)];

      snprintf(dst, sz, "%s%d%02d%s%s%s%s", info->country, year, week, part1, part2, board, part3);
    }
  } while (!verify_mlb_checksum(dst, strlen(dst)));
}

static void get_system_info(void) {
#ifdef __APPLE__
  CFDataRef model    = get_ioreg_entry("IODeviceTree:/", CFSTR("model"), CFDataGetTypeID());
  CFDataRef board    = get_ioreg_entry("IODeviceTree:/", CFSTR("board-id"), CFDataGetTypeID());
  CFDataRef efiver   = get_ioreg_entry("IODeviceTree:/rom", CFSTR("version"), CFDataGetTypeID());
  CFStringRef serial = get_ioreg_entry("IODeviceTree:/", CFSTR("IOPlatformSerialNumber"), CFStringGetTypeID());
  CFStringRef hwuuid = get_ioreg_entry("IODeviceTree:/", CFSTR("IOPlatformUUID"), CFStringGetTypeID());
  CFDataRef smuuid   = get_ioreg_entry("IODeviceTree:/efi/platform", CFSTR("system-id"), CFDataGetTypeID());
  CFDataRef rom      = get_ioreg_entry("IODeviceTree:/options", CFSTR("4D1EDE05-38C7-4A6A-9CC6-4BCCA8B38C14:ROM"), CFDataGetTypeID());
  CFDataRef mlb      = get_ioreg_entry("IODeviceTree:/options", CFSTR("4D1EDE05-38C7-4A6A-9CC6-4BCCA8B38C14:MLB"), CFDataGetTypeID());

  CFDataRef   pwr[5] = {0};
  CFStringRef pwrname[5] = {
    CFSTR("Gq3489ugfi"),
    CFSTR("Fyp98tpgj"),
    CFSTR("kbjfrfpoJU"),
    CFSTR("oycqAZloTNDm"),
    CFSTR("abKPld1EcMni"),
  };

  for (size_t i = 0; i < ARRAY_SIZE(pwr); i++)
    pwr[i] = get_ioreg_entry("IOPower:/", pwrname[i], CFDataGetTypeID());

  if (model) {
    printf("%14s: %.*s\n", "Model", (int)CFDataGetLength(model), CFDataGetBytePtr(model));
    CFRelease(model);
  }

  if (board) {
    printf("%14s: %.*s\n", "Board ID", (int)CFDataGetLength(board), CFDataGetBytePtr(board));
    CFRelease(board);
  }

  if (efiver) {
    printf("%14s: %.*s\n", "FW Version", (int)CFDataGetLength(efiver), CFDataGetBytePtr(efiver));
    CFRelease(efiver);
  }

  if (hwuuid) {
    printf("%14s: %s\n", "Hardware UUID", CFStringGetCStringPtr(hwuuid, kCFStringEncodingMacRoman));
    CFRelease(hwuuid);
  }

  puts("");

  if (serial) {
    const char *cstr = CFStringGetCStringPtr(serial, kCFStringEncodingMacRoman);
    printf("%14s: %s\n", "Serial Number", cstr);
    SERIALINFO info;
    get_serial_info(cstr, &info, true);
    CFRelease(serial);
    puts("");
  }

  if (smuuid) {
    if (CFDataGetLength(smuuid) == SZUUID) {
      const uint8_t *p = CFDataGetBytePtr(smuuid);
      printf("%14s: " PRIUUID "\n", "System ID", CASTUUID(p));
    }
    CFRelease(smuuid);
  }

  if (rom) {
    if (CFDataGetLength(rom) == 6) {
      const uint8_t *p = CFDataGetBytePtr(rom);
      printf("%14s: %02X%02X%02X%02X%02X%02X\n", "ROM", p[0], p[1], p[2], p[3], p[4], p[5]);
    }
    CFRelease(rom);
  }

  if (mlb) {
    printf("%14s: %.*s\n", "MLB", (int)CFDataGetLength(mlb), CFDataGetBytePtr(mlb));
    if (!verify_mlb_checksum((const char *)CFDataGetBytePtr(mlb), CFDataGetLength(mlb)))
      printf("WARN: Invalid MLB checksum!\n");
    CFRelease(mlb);
  }

  puts("");

  for (size_t i = 0; i < ARRAY_SIZE(pwr); i++) {
    if (pwr[i]) {
      printf("%14s: ", CFStringGetCStringPtr(pwrname[i], kCFStringEncodingMacRoman));
      const uint8_t *p = CFDataGetBytePtr(pwr[i]);
      CFIndex sz = CFDataGetLength(pwr[i]);
      for (CFIndex j = 0; j < sz; j++)
        printf("%02X", p[j]);
      puts("");
      CFRelease(pwr[i]);
    }
  }

  puts("");
#endif

  printf("Version %s. Use -h argument to see usage options.\n", PROGRAM_VERSION);
}

static int usage(const char *app) {
  printf(
    "%s arguments:\n"
    " --help           (-h)  show this help\n"
    " --version        (-v)  show program version\n"
    " --deriv <serial> (-d)  generate all derivative serials\n"
    " --generate       (-g)  generate serial for current model\n"
    " --generate-all   (-a)  generate serial for all models\n"
    " --info <serial>  (-i)  decode serial information\n"
    " --verify <mlb>         verify MLB checksum\n"
    " --list           (-l)  list known mac models\n"
    " --list-products  (-lp) list known product codes\n"
    " --mlb <serial>         generate MLB based on serial\n"
    " --sys            (-s)  get system info\n\n"
    "Tuning options:\n"
    " --model <model>  (-m)  mac model used for generation\n"
    " --num <num>      (-n)  number of generated pairs\n"
    " --year <year>    (-y)  year used for generation\n"
    " --week <week>    (-w)  week used for generation\n"
    " --country <loc>  (-c)  country location used for generation\n"
    " --copy <copy>    (-o)  production copy index\n"
    " --line <line>    (-e)  production line\n"
    " --platform <ppp> (-p)  platform code used for generation\n\n", app);

  return EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
  PROGRAMMODE mode = MODE_SYSTEM_INFO;
  const char *passed_serial = NULL;
  SERIALINFO info = {
    .modelIndex  = -1,
    .decodedYear = -1,
    .decodedWeek = -1,
    .decodedCopy = -1,
    .decodedLine = -1
  };
  int32_t limit = 10;

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
      usage(argv[0]);
      return EXIT_SUCCESS;
    } else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--version")) {
      printf("ugrobator %s\n", PROGRAM_VERSION);
      return EXIT_SUCCESS;
    } else if (!strcmp(argv[i], "-g") || !strcmp(argv[i], "--generate")) {
      mode = MODE_GENERATE_CURRENT;
    } else if (!strcmp(argv[i], "-a") || !strcmp(argv[i], "--generate-all")) {
      mode = MODE_GENERATE_ALL;
    } else if (!strcmp(argv[i], "-i") || !strcmp(argv[i], "--info")) {
      if (++i == argc) return usage(argv[0]);
      mode = MODE_SERIAL_INFO;
      passed_serial = argv[i];
    } else if (!strcmp(argv[i], "--verify")) {
      if (++i == argc) return usage(argv[0]);
      mode = MODE_MLB_INFO;
      passed_serial = argv[i];
    } else if (!strcmp(argv[i], "-l") || !strcmp(argv[i], "--list")) {
      mode = MODE_LIST_MODELS;
    } else if (!strcmp(argv[i], "-lp") || !strcmp(argv[i], "--list-products")) {
      mode = MODE_LIST_PRODUCTS;
    } else if (!strcmp(argv[i], "-mlb") || !strcmp(argv[i], "--mlb")) {
      // -mlb is supported due to legacy versions.
      if (++i == argc) return usage(argv[0]);
      mode = MODE_GENERATE_MLB;
      passed_serial = argv[i];
    } else if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--deriv")) {
      if (++i == argc) return usage(argv[0]);
      mode = MODE_GENERATE_DERIVATIVES;
      passed_serial = argv[i];
    } else if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--sys")) {
      mode = MODE_SYSTEM_INFO;
    } else if (!strcmp(argv[i], "-m") || !strcmp(argv[i], "--model")) {
      if (mode == MODE_SYSTEM_INFO) mode = MODE_GENERATE_CURRENT;
      if (++i == argc) return usage(argv[0]);
      if (argv[i][0] >= '0' && argv[i][0] <= '9') {
        info.modelIndex = atoi(argv[i]);
      } else {
        for (int32_t j = 0; j < APPLE_MODEL_MAX; j++) {
          if (!strcmp(argv[i], ApplePlatformData[j].productName)) {
            info.modelIndex = j;
            break;
          }
        }
      }
      if (info.modelIndex < 0 || info.modelIndex > APPLE_MODEL_MAX) {
        printf("Model id (%d) or name (%s) is out of valid range [0, %d]!\n", info.modelIndex, argv[i], APPLE_MODEL_MAX-1);
        return EXIT_FAILURE;
      }
    } else if (!strcmp(argv[i], "-n") || !strcmp(argv[i], "--num")) {
      if (mode == MODE_SYSTEM_INFO) mode = MODE_GENERATE_CURRENT;
      if (++i == argc) return usage(argv[0]);
      limit = atoi(argv[i]);
      if (limit <= 0) {
        printf("Cannot generate %d pairs!\n", limit);
        return EXIT_FAILURE;
      }
    } else if (!strcmp(argv[i], "-y") || !strcmp(argv[i], "--year")) {
      if (mode == MODE_SYSTEM_INFO) mode = MODE_GENERATE_CURRENT;
      if (++i == argc) return usage(argv[0]);
      info.decodedYear = atoi(argv[i]);
      if (info.decodedYear < SERIAL_YEAR_MIN || info.decodedYear > SERIAL_YEAR_MAX) {
        printf("Year %d is out of valid range [%d, %d]!\n", info.decodedYear, SERIAL_YEAR_MIN, SERIAL_YEAR_MAX);
        return EXIT_FAILURE;
      }
    } else if (!strcmp(argv[i], "-w") || !strcmp(argv[i], "--week")) {
      if (mode == MODE_SYSTEM_INFO) mode = MODE_GENERATE_CURRENT;
      if (++i == argc) return usage(argv[0]);
      info.decodedWeek = atoi(argv[i]);
      if (info.decodedWeek < SERIAL_WEEK_MIN || info.decodedWeek > SERIAL_WEEK_MAX) {
        printf("Week %d is out of valid range [%d, %d]!\n", info.decodedWeek, SERIAL_WEEK_MIN, SERIAL_WEEK_MAX);
        return EXIT_FAILURE;
      }
    } else if (!strcmp(argv[i], "-c") || !strcmp(argv[i], "--country")) {
      if (mode == MODE_SYSTEM_INFO) mode = MODE_GENERATE_CURRENT;
      if (++i == argc) return usage(argv[0]);
      size_t len = strlen(argv[i]);
      if (len != COUNTRY_OLD_LEN && len != COUNTRY_NEW_LEN) {
        printf("Country location %s is neither %d nor %d symbols long!\n", argv[i], COUNTRY_OLD_LEN, COUNTRY_NEW_LEN);
        return EXIT_FAILURE;
      }
      strncpy(info.country, argv[i], COUNTRY_NEW_LEN+1);
    } else if (!strcmp(argv[i], "-p") || !strcmp(argv[i], "--platform")) {
      if (mode == MODE_SYSTEM_INFO) mode = MODE_GENERATE_CURRENT;
      if (++i == argc) return usage(argv[0]);
      size_t len = strlen(argv[i]);
      if (len != MODEL_CODE_OLD_LEN && len != MODEL_CODE_NEW_LEN) {
        printf("Platform code %s is neither %d nor %d symbols long!\n", argv[i], MODEL_CODE_OLD_LEN, MODEL_CODE_NEW_LEN);
        return EXIT_FAILURE;
      }
      strncpy(info.model, argv[i], MODEL_CODE_NEW_LEN+1);
    } else if (!strcmp(argv[i], "-o") || !strcmp(argv[i], "--copy")) {
      if (mode == MODE_SYSTEM_INFO) mode = MODE_GENERATE_CURRENT;
      if (++i == argc) return usage(argv[0]);
      info.decodedCopy = atoi(argv[i]);
      if (info.decodedCopy < SERIAL_COPY_MIN || info.decodedCopy > SERIAL_COPY_MAX) {
        printf("Copy %d is out of valid range [%d, %d]!\n", info.decodedCopy, SERIAL_COPY_MIN, SERIAL_COPY_MAX);
        return EXIT_FAILURE;
      }
    } else if (!strcmp(argv[i], "-e") || !strcmp(argv[i], "--line")) {
      if (mode == MODE_SYSTEM_INFO) mode = MODE_GENERATE_CURRENT;
      if (++i == argc) return usage(argv[0]);
      info.decodedLine = atoi(argv[i]);
      if (info.decodedLine < SERIAL_LINE_MIN || info.decodedLine > SERIAL_LINE_MAX) {
        printf("Line %d is out of valid range [%d, %d]!\n", info.decodedLine, SERIAL_LINE_MIN, SERIAL_LINE_MAX);
        return EXIT_FAILURE;
      }
    }
  }

  if (mode == MODE_SYSTEM_INFO) {
    get_system_info();
  } else if (mode == MODE_SERIAL_INFO) {
    get_serial_info(passed_serial, &info, true);
  } else if (mode == MODE_MLB_INFO) {
    size_t len = strlen(passed_serial);
    if (len == 13 || len == 17) {
      printf("Valid MLB length: %s\n", len == 13 ? "legacy" : "modern");
    } else {
      printf("WARN: Invalid MLB length: %u\n", (unsigned) len);
    }
    if (verify_mlb_checksum(passed_serial, strlen(passed_serial))) {
      printf("Valid MLB checksum.\n");
    } else {
      printf("WARN: Invalid MLB checksum!\n");
    }
  } else if (mode == MODE_LIST_MODELS) {
    printf("Available models:\n");
    for (int32_t j = 0; j < APPLE_MODEL_MAX; j++) {
      printf("%14s: %s\n", "Model", ApplePlatformData[j].productName);
      printf("%14s: ", "Prod years");
      get_production_year((AppleModel)j, true);
      printf("%14s: %s\n", "Base Serial", ApplePlatformData[j].serialNumber);
      printf("%14s: ", "Model codes");
      get_model_code((AppleModel)j, true);
      printf("%14s: ", "Board codes");
      get_board_code((AppleModel)j, true);
      puts("");
    }
    printf("Available legacy location codes:\n");
    for (size_t j = 0; j < ARRAY_SIZE(AppleLegacyLocations); j++)
      printf(" - %s, %s\n", AppleLegacyLocations[j], AppleLegacyLocationNames[j]);
    printf("\nAvailable new location codes:\n");
    for (size_t j = 0; j < ARRAY_SIZE(AppleLocations); j++)
      printf(" - %s, %s\n", AppleLocations[j], AppleLocationNames[j]);
    puts("");
  } else if (mode == MODE_LIST_PRODUCTS) {
    for (size_t j = 0; j < ARRAY_SIZE(AppleModelDesc); j++)
      printf("%4s - %s\n", AppleModelDesc[j].code, AppleModelDesc[j].name);
  } else if (mode == MODE_GENERATE_MLB) {
    if (get_serial_info(passed_serial, &info, false)) {
      char mlb[MLB_MAX_SIZE];
      get_mlb(&info, mlb, MLB_MAX_SIZE);
      printf("%s\n", mlb);
    }
  } else if (mode == MODE_GENERATE_CURRENT) {
    if (info.modelIndex < 0)
      info.modelIndex = get_current_model();
    for (int32_t i = 0; i < limit; i++) {
      SERIALINFO tmp = info;
      if (get_serial(&tmp)) {
        char mlb[MLB_MAX_SIZE];
        get_mlb(&tmp, mlb, MLB_MAX_SIZE);
        printf("%s%s%s%s%s | %s\n", tmp.country, tmp.year, tmp.week, tmp.line, tmp.model, mlb);
      }
    }
  } else if (mode == MODE_GENERATE_ALL) {
    for (int32_t i = 0; i < APPLE_MODEL_MAX; i++) {
      info.modelIndex = i;
      for (int32_t j = 0; j < limit; j++) {
        SERIALINFO tmp = info;
        if (get_serial(&tmp)) {
          char mlb[MLB_MAX_SIZE];
          get_mlb(&tmp, mlb, MLB_MAX_SIZE);
          printf("%14s | %s%s%s%s%s | %s\n", ApplePlatformData[info.modelIndex].productName,
            tmp.country, tmp.year, tmp.week, tmp.line, tmp.model, mlb);
        }
      }
    }
  } else if (mode == MODE_GENERATE_DERIVATIVES) {
    if (get_serial_info(passed_serial, &info, false)) {
      int rmin = line_to_rmin(info.decodedLine);
      for (int32_t k = 0; k < 34; k++) {
        int32_t start = k * 68;
        if (info.decodedLine > start && info.decodedLine - start <= SERIAL_LINE_REPR_MAX) {
          int32_t rem = info.decodedLine - start;
          printf("%s%s%s%c%c%c%s - copy %d\n", info.country, info.year, info.week, AppleBase34Reverse[k],
            AppleBase34Reverse[rem / 34], AppleBase34Reverse[rem % 34], info.model, k - rmin + 1);
        }
      }
    } else {
      return EXIT_FAILURE;
    }
  }
}
