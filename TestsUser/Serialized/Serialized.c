/** @file
  Copyright (C) 2018, vit9696. All rights reserved.

  All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/OcTemplateLib.h>
#include <Library/OcSerializeLib.h>
#include <Library/OcMiscLib.h>

#include <sys/time.h>

/*
 clang -g -fsanitize=undefined,address -I../Include -I../../Include -I../../../MdePkg/Include/ -include ../Include/Base.h Serialized.c ../../Library/OcXmlLib/OcXmlLib.c ../../Library/OcTemplateLib/OcTemplateLib.c ../../Library/OcSerializeLib/OcSerializeLib.c ../../Library/OcMiscLib/Base64Decode.c ../../Library/OcStringLib/OcAsciiLib.c  -o Serialized

 for fuzzing:
 clang-mp-7.0 -Dmain=__main -g -fsanitize=undefined,address,fuzzer -I../Include -I../../Include -I../../../MdePkg/Include/ -include ../Include/Base.h Serialized.c ../../Library/OcXmlLib/OcXmlLib.c ../../Library/OcTemplateLib/OcTemplateLib.c ../../Library/OcSerializeLib/OcSerializeLib.c ../../Library/OcMiscLib/Base64Decode.c ../../Library/OcStringLib/OcAsciiLib.c  -o Serialized
 rm -rf DICT fuzz*.log ; mkdir DICT ; cp Serialized.plist DICT ; ./Serialized -jobs=4 DICT

 rm -rf Serialized.dSYM DICT fuzz*.log Serialized
*/

VOID ConfigDestructCustom(VOID *Ptr, UINT32 Size) {
  printf("ConfigDestructCustom for %p with %u\n", Ptr, Size);
}

#define A_FIELDS(_, __) \
  _(int   , a, , 1   , ()             ) \
  _(int   , b, , 2   , ()             ) \
  _(char *, c, , NULL, OcFreePointer  )
  OC_DECLARE (A)

#define TEST_FIELDS(_, __) \
  OC_ARRAY (A, _, __)
  OC_DECLARE (TEST)

#define BLOB_FIELDS(_, __) \
  OC_BLOB (UINT8, [64], __({1, 2}), _, __)
  OC_DECLARE (BLOB)

#define STR_FIELDS(_, __) \
  OC_BLOB (CHAR8, [64], "Hello", _, __)
  OC_DECLARE (STR)

#define CONFIG_FIELDS(_, __) \
  _(bool    , val_b,     , true                   , ()                ) \
  _(uint32_t, val32,     , 123                    , ()                ) \
  _(char    , buf  , [16], "Hello"                , ()                ) \
  _(uint8_t , data , [16], __({1, 2, 3, 4})       , ()                ) \
  _(A       , str  ,     , OC_CONSTR(A, _, __)    , OC_DESTR (A)      ) \
  _(char *  , heap ,     , NULL                   , OcFreePointer     ) \
  _(TEST    , arr  ,     , OC_CONSTR(TEST, _, __) , OC_DESTR (TEST)   ) \
  _(BLOB    , blob ,     , OC_CONSTR(BLOB, _, __) , OC_DESTR (BLOB)   ) \
  _(STR     , dstr ,     , OC_CONSTR(STR, _, __)  , OC_DESTR (STR)    )
  OC_DECLARE (CONFIG)

//
// Structure implementation...
//

OC_STRUCTORS (A, ())
OC_STRUCTORS (TEST, ())
OC_STRUCTORS (BLOB, ())
OC_STRUCTORS (STR,  ())
OC_STRUCTORS (CONFIG, ConfigDestructCustom)

///////////////////////////////////////////////////////////////



#define KEXT_MODIFICATION_FIELDS(_, __)  \
  _(OC_STRING, Identifier , , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_STRING, Symbol     , , OC_STRING_CONSTR ("", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_DATA  , Find       , , OC_DATA_CONSTR ({0}, _, __) , OC_DESTR (OC_DATA)   ) \
  _(OC_DATA  , Mask       , , OC_DATA_CONSTR ({0}, _, __) , OC_DESTR (OC_DATA)   ) \
  _(OC_DATA  , Replace    , , OC_DATA_CONSTR ({0}, _, __) , OC_DESTR (OC_DATA)   ) \
  _(UINT32   , Count      , , 0                           , ()                   ) \
  _(UINT32   , Skip       , , 0                           , ()                   )
  OC_DECLARE (KEXT_MODIFICATION)

#define DEVICE_PROP_MAP_FIELDS(_, __) \
  OC_MAP (OC_STRING, OC_ASSOC, _, __)
  OC_DECLARE (DEVICE_PROP_MAP)


#define KEXT_MOD_ARRAY_FIELDS(_, __) \
  OC_ARRAY (KEXT_MODIFICATION, _, __)
  OC_DECLARE (KEXT_MOD_ARRAY)

//
// Target configuration layout
//

#define GLOBAL_CONFIGURATION_FIELDS(_, __) \
  _(BOOLEAN                  , CleanAcpiHeaders  ,     , FALSE, ()) \
  _(OC_STRING                , SmbiosProductName ,     , OC_STRING_CONSTR ("Computer1,1", _, __), OC_DESTR (OC_STRING) ) \
  _(OC_ASSOC                 , NvramVariables    ,     , OC_CONSTR (OC_ASSOC, _, __)            , OC_DESTR (OC_ASSOC) ) \
  _(KEXT_MOD_ARRAY           , KextMods          ,     , OC_CONSTR (KEXT_MOD_ARRAY, _, __)      , OC_DESTR (KEXT_MOD_ARRAY)) \
  _(DEVICE_PROP_MAP          , DeviceProperties  ,     , OC_CONSTR (DEVICE_PROP_MAP, _, __)     , OC_DESTR (DEVICE_PROP_MAP)) \
  _(UINT8                    , DataFixed         , [16], {0}                                    , ()) \
  _(UINT8                    , DataMeta          , [16], {0}                                    , ()) \
  _(OC_DATA                  , DataVar           ,     , OC_DATA_CONSTR ({0}, _, __)            , OC_DESTR (OC_DATA)   ) \
  _(OC_DATA                  , DataMetaVar       ,     , OC_DATA_CONSTR ({0}, _, __)            , OC_DESTR (OC_DATA)   ) \
  _(CHAR8                    , String            , [16], "Default"                              , ()) \
  _(INT32                    , Test32            ,     , 0                                      , ())
  OC_DECLARE (GLOBAL_CONFIGURATION)

OC_STRUCTORS (KEXT_MODIFICATION, ())
OC_ARRAY_STRUCTORS (KEXT_MOD_ARRAY)
OC_MAP_STRUCTORS (DEVICE_PROP_MAP)
OC_STRUCTORS (GLOBAL_CONFIGURATION, ())

STATIC
OC_SCHEMA
mNullConfigurationSchema[] = {};

//
// ACPI Support
//

STATIC
OC_SCHEMA
mAcpiConfigurationSchema[] = {
  OC_SCHEMA_BOOLEAN_IN ("CleanHeaders", GLOBAL_CONFIGURATION, CleanAcpiHeaders)
};

//
// Device Properties Support
//

STATIC
OC_SCHEMA
mDevicePropertiesEntrySchema = OC_SCHEMA_MDATA (NULL);


STATIC
OC_SCHEMA
mDevicePropertiesSchema = OC_SCHEMA_MAP (NULL, OC_ASSOC, &mDevicePropertiesEntrySchema);

//
// Miscellaneous Support
//

STATIC
OC_SCHEMA
mMiscConfigurationSchema[] = {
  OC_SCHEMA_DATAF_IN   ("DataFix", GLOBAL_CONFIGURATION, DataFixed),
  OC_SCHEMA_MDATAF_IN  ("DataMeta", GLOBAL_CONFIGURATION, DataMeta),
  OC_SCHEMA_DATA_IN    ("DataMetaVar", GLOBAL_CONFIGURATION, DataMetaVar),
  OC_SCHEMA_DATA_IN    ("DataVar", GLOBAL_CONFIGURATION, DataVar),
  OC_SCHEMA_STRINGF_IN ("String", GLOBAL_CONFIGURATION, String),
  OC_SCHEMA_INTEGER_IN ("Test32", GLOBAL_CONFIGURATION, Test32)
};

//
// Binary Modification Support
//

STATIC
OC_SCHEMA
mKextModConfigurationSchema[] = {
  OC_SCHEMA_INTEGER_IN   ("Count", KEXT_MODIFICATION, Count),
  OC_SCHEMA_DATA_IN      ("Find", KEXT_MODIFICATION, Find),
  OC_SCHEMA_STRING_IN    ("Identifier", KEXT_MODIFICATION, Identifier),
  OC_SCHEMA_DATA_IN      ("Mask", KEXT_MODIFICATION, Mask),
  OC_SCHEMA_DATA_IN      ("Replace", KEXT_MODIFICATION, Replace),
  OC_SCHEMA_STRING_IN    ("Symbol", KEXT_MODIFICATION, Symbol),
  OC_SCHEMA_INTEGER_IN   ("Skip", KEXT_MODIFICATION, Skip)
};

STATIC
OC_SCHEMA
mKextModSchema = OC_SCHEMA_DICT (NULL, mKextModConfigurationSchema);

STATIC
OC_SCHEMA
mModsConfigurationSchema[] = {
  OC_SCHEMA_ARRAY_IN ("Kext", GLOBAL_CONFIGURATION, KextMods, &mKextModSchema)
};

//
// NVRAM Support
//

STATIC
OC_SCHEMA
mNvramVariableSchema = OC_SCHEMA_MDATA (NULL);

//
// SMBIOS Support
//

STATIC
OC_SCHEMA
mSmbiosConfigurationSchema[] = {
  OC_SCHEMA_STRING_IN ("ProductName", GLOBAL_CONFIGURATION, SmbiosProductName)
};

//
// Root configuration
//

STATIC
OC_SCHEMA
mRootConfigurationNodes[] = {
  OC_SCHEMA_DICT   ("ACPI", mAcpiConfigurationSchema),
  OC_SCHEMA_MAP_IN ("DeviceProperties", GLOBAL_CONFIGURATION, DeviceProperties, &mDevicePropertiesSchema),
  OC_SCHEMA_DICT   ("Misc", mMiscConfigurationSchema),
  OC_SCHEMA_DICT   ("Mods", mModsConfigurationSchema),
  OC_SCHEMA_MAP_IN ("NVRAM", GLOBAL_CONFIGURATION, NvramVariables, &mNvramVariableSchema),
  OC_SCHEMA_DICT   ("SMBIOS", mSmbiosConfigurationSchema)
};

STATIC
OC_SCHEMA_INFO
mRootConfigurationInfo = {
  .Dict = {mRootConfigurationNodes, ARRAY_SIZE (mRootConfigurationNodes)}
};

STATIC
GLOBAL_CONFIGURATION
mGlobalConfiguration;


long long current_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

uint8_t *readFile(const char *str, uint32_t *size) {
  FILE *f = fopen(str, "rb");

  if (!f) return NULL;

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  uint8_t *string = malloc(fsize + 1);
  fread(string, fsize, 1, f);
  fclose(f);

  string[fsize] = 0;
  *size = fsize;

  return string;
}

int main(int argc, char** argv) {
  uint32_t f;
  uint8_t *b;
  if ((b = readFile(argc > 1 ? argv[1] : "Serialized.plist", &f)) == NULL) {
    printf("Read fail\n");
    return -1;
  }

  CONFIG cfg;
  CONFIG_CONSTRUCT (&cfg, sizeof (CONFIG));
  CONFIG_DESTRUCT (&cfg, sizeof (CONFIG));

  long long a = current_timestamp();

  GLOBAL_CONFIGURATION_CONSTRUCT(&mGlobalConfiguration, sizeof (mGlobalConfiguration));

  ParseSerialized (&mGlobalConfiguration, &mRootConfigurationInfo, b, f);

  DEBUG((EFI_D_ERROR, "Done in %llu ms\n", current_timestamp() - a));

  DEBUG((EFI_D_ERROR, "CleanAcpiHeaders %d\n", mGlobalConfiguration.CleanAcpiHeaders));
  DEBUG((EFI_D_ERROR, "SmbiosProductName %s\n", mGlobalConfiguration.SmbiosProductName.Value));
  UINT8  *Blob = mGlobalConfiguration.DataFixed;
  DEBUG((EFI_D_ERROR, "DataFixed %02X %02X %02X %02X\n", Blob[0], Blob[1], Blob[2], Blob[3]));
  Blob = mGlobalConfiguration.DataMeta;
  DEBUG((EFI_D_ERROR, "DataMeta %02X %02X %02X %02X\n", Blob[0], Blob[1], Blob[2], Blob[3]));
  Blob = OC_BLOB_GET (&mGlobalConfiguration.DataVar);
  DEBUG((EFI_D_ERROR, "DataVar (%u) %02X %02X %02X %02X\n", mGlobalConfiguration.DataVar.Size, Blob[0], Blob[1], Blob[2], Blob[3]));
  Blob = OC_BLOB_GET (&mGlobalConfiguration.DataMetaVar);
  DEBUG((EFI_D_ERROR, "DataMetaVar (%u) %02X %02X %02X %02X\n", mGlobalConfiguration.DataMetaVar.Size, Blob[0], Blob[1], Blob[2], Blob[3]));
  DEBUG((EFI_D_ERROR, "Test32 %u\n", mGlobalConfiguration.Test32));

  DEBUG((EFI_D_ERROR, "KextMod has %u entries\n", mGlobalConfiguration.KextMods.Count));
  for (UINT32 i = 0; i < mGlobalConfiguration.KextMods.Count; i++) {
    DEBUG((EFI_D_ERROR, "KextMod %u is for %s kext\n", i, OC_BLOB_GET (&mGlobalConfiguration.KextMods.Values[i]->Identifier)));
  }

  DEBUG((EFI_D_ERROR, "NVRAM has %u entries\n", mGlobalConfiguration.NvramVariables.Count));
  for (UINT32 i = 0; i < mGlobalConfiguration.NvramVariables.Count; i++) {
    CHAR8 *Str  = OC_BLOB_GET (mGlobalConfiguration.NvramVariables.Keys[i]);
    UINT8 *Blob = OC_BLOB_GET (mGlobalConfiguration.NvramVariables.Values[i]);
    DEBUG((EFI_D_ERROR, "NVRAM %u %s gives %.64s value\n", i, Str, (CHAR8 *) Blob));
  }

  DEBUG((EFI_D_ERROR, "DevProps have %u entries\n", mGlobalConfiguration.DeviceProperties.Count));
  for (UINT32 i = 0; i < mGlobalConfiguration.DeviceProperties.Count; i++) {
    CHAR8 *Str  = OC_BLOB_GET (mGlobalConfiguration.DeviceProperties.Keys[i]);
    DEBUG((EFI_D_ERROR, "DevProps device %u is called %.64s and has %u props\n", i, Str,
      mGlobalConfiguration.DeviceProperties.Values[i]->Count));

    for (UINT32 j = 0; j < mGlobalConfiguration.DeviceProperties.Values[i]->Count; j++) {
      CHAR8 *Str  = OC_BLOB_GET (mGlobalConfiguration.DeviceProperties.Values[i]->Keys[j]);
      UINT8 *Blob = OC_BLOB_GET (mGlobalConfiguration.DeviceProperties.Values[i]->Values[j]);
      DEBUG((EFI_D_ERROR, " Prop %u is called %.64s and has value %.64s\n", j,
        Str, Blob));
    }
  }

  GLOBAL_CONFIGURATION_DESTRUCT (&mGlobalConfiguration, sizeof (mGlobalConfiguration));
  free(b);

  return 0;
}

INT32 LLVMFuzzerTestOneInput(CONST UINT8 *Data, UINTN Size) {
  VOID *NewData = AllocatePool (Size);
  if (NewData) {
    CopyMem (NewData, Data, Size);
    GLOBAL_CONFIGURATION_CONSTRUCT (&mGlobalConfiguration, sizeof (mGlobalConfiguration));
    ParseSerialized (&mGlobalConfiguration, &mRootConfigurationInfo, NewData, Size);
    GLOBAL_CONFIGURATION_DESTRUCT (&mGlobalConfiguration, sizeof (mGlobalConfiguration));
    FreePool (NewData);
  }
  return 0;
}

