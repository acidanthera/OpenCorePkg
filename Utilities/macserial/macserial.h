//
// Decode mac serial number
//
// Copyright (c) 2018-2020 vit9696
// Copyright (c) 2020 Matis Schotte
//

#ifndef GENSERIAL_H
#define GENSERIAL_H

#include <stdbool.h>
#include <stdint.h>

#define PROGRAM_VERSION "2.1.8"

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define SZUUID 16
#define PRIUUID "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X"
#define CASTUUID(uuid) (uuid)[0], (uuid)[1], (uuid)[2], (uuid)[3], (uuid)[4], (uuid)[5], (uuid)[6], \
  (uuid)[7], (uuid)[8], (uuid)[9], (uuid)[10], (uuid)[11], (uuid)[12], (uuid)[13], (uuid)[14], (uuid)[15]

#define SERIAL_WEEK_MIN 1
#define SERIAL_WEEK_MAX 53
#define SERIAL_YEAR_MIN 2000
#define SERIAL_YEAR_MAX 2030

#define SERIAL_YEAR_OLD_MIN 2003
#define SERIAL_YEAR_OLD_MAX 2012

#define SERIAL_YEAR_NEW_MIN 2010
#define SERIAL_YEAR_NEW_MID 2020
#define SERIAL_YEAR_NEW_MAX 2030

#define SERIAL_COPY_MIN 1
#define SERIAL_COPY_MAX 34

#define SERIAL_LINE_MIN 0
#define SERIAL_LINE_REPR_MAX 1155
#define SERIAL_LINE_MAX 3399 /* 68*33 + 33*34 + 33 */

#define SERIAL_OLD_LEN 11
#define SERIAL_NEW_LEN 12

#define MODEL_CODE_OLD_LEN 3
#define MODEL_CODE_NEW_LEN 4

#define COUNTRY_OLD_LEN 2
#define COUNTRY_NEW_LEN 3

#define MLB_MAX_SIZE 32

typedef struct {
  const char *productName;
  const char *serialNumber;
} PLATFORMDATA;

typedef struct {
  const char *code;
  const char *name;
} APPLE_MODEL_DESC;

typedef struct {
  const char *appleModel;
  char country[4];
  char year[3];
  char week[3];
  char line[4];
  char model[5];
  int32_t legacyCountryIdx;
  int32_t modernCountryIdx;
  int32_t modelIndex;
  int32_t decodedYear;
  int32_t decodedWeek;
  int32_t decodedCopy;
  int32_t decodedLine;
  bool valid;
} SERIALINFO;

typedef enum {
  MODE_SYSTEM_INFO,
  MODE_SERIAL_INFO,
  MODE_MLB_INFO,
  MODE_LIST_MODELS,
  MODE_LIST_PRODUCTS,
  MODE_GENERATE_MLB,
  MODE_GENERATE_CURRENT,
  MODE_GENERATE_ALL,
  MODE_GENERATE_DERIVATIVES
} PROGRAMMODE;

#endif // GENSERIAL_H
