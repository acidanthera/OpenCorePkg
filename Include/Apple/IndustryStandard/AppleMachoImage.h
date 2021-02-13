/** @file
Copyright (c) 2016 - 2018, Download-Fritz.  All rights reserved.<BR>
This program and the accompanying materials are licensed and made available
under the terms and conditions of the BSD License which accompanies this
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef APPLE_MACHO_IMAGE_H
#define APPLE_MACHO_IMAGE_H

#define MACH_CPU_ARCH_ABI64     BIT24
#define MACH_CPU_ARCH_ABI64_32  BIT25

//
// Capability bits used in the definition of MACH_CPU_SUBTYPE.
//
#define MACH_CPU_SUBTYPE_MASK   0xFF000000U  ///< mask for feature flags
#define MACH_CPU_SUBTYPE_LIB64  BIT17        ///< 64 bit libraries

#define MACH_CPU_SUBTYPE_INTEL_MODEL_SHIFT  4U
#define MACH_CPU_SUBTYPE_INTEL_FAMILY_MAX   0x0FU
#define MACH_CPU_SUBTYPE_INTEL_MODEL_ALL    0U

#define MACH_CPU_SUBTYPE_INTEL(f, Model)  \
  ((f) + ((Model) << MACH_CPU_SUBTYPE_INTEL_MODEL_SHIFT))

#define MACH_CPU_SUBTYPE_INTEL_FAMILY(Identifier)  \
  ((Identifier) & MACH_CPU_SUBTYPE_INTEL_FAMILY_MAX)

#define MACH_CPU_SUBTYPE_INTEL_MODEL(Identifier)  \
  ((Identifier) >> MACH_CPU_SUBTYPE_INTEL_MODEL_SHIFT)

#define FORMALIZE_CPU_SUBTYPE(CpuSubtype) ((CpuSubtype) & ~CPU_SUBTYPE_MASK)

#define MACH_CPU_TYPE_64(CpuType) ((CpuType) | MACH_CPU_ARCH_ABI64)

#define MACH_CPU_TYPE_64_32(CpuType) ((CpuType) | MACH_CPU_ARCH_ABI64_32)

///
/// There is no standard way to detect endianness in EDK2 (yet).
/// Provide MACH_LITTLE_ENDIAN on IA32 and X64 platforms by default,
/// as they are strictly little endian.
///
#if !defined(MACH_LITTLE_ENDIAN) && !defined(MACH_BIG_ENDIAN)
#if defined (MDE_CPU_IA32) || defined(MDE_CPU_X64)
#define MACH_LITTLE_ENDIAN
#endif ///< defined (MDE_CPU_IA32) || defined(MDE_CPU_X64)
#endif ///< !defined(MACH_LITTLE_ENDIAN) && !defined(MACH_BIG_ENDIAN)

///
/// CPU Type definitions
///
enum {
  MachCpuTypeAny       = -1,
  MachCpuTypeVax       = 1,
  MachCpuTypeMc680x0   = 6,
  MachCpuTypeX86       = 7,
  MachCpuTypeI386      = MachCpuTypeX86,
  MachCpuTypeX8664     = MACH_CPU_TYPE_64 (MachCpuTypeX86),
  MachCpuTypeMc98000   = 10,
  MachCpuTypeHppa      = 11,
  MachCpuTypeArm       = 12,
  MachCpuTypeArm64     = MACH_CPU_TYPE_64 (MachCpuTypeArm),
  MachCpuTypeArm6432   = MACH_CPU_TYPE_64_32 (MachCpuTypeArm),

  MachCpuTypeMc88000   = 13,
  MachCpuTypeSparc     = 14,
  MachCpuTypeI860      = 15,
  MachCpuTypePowerPc   = 18,
  MachCpuTypePowerPc64 = MACH_CPU_TYPE_64 (MachCpuTypePowerPc),
  MachCpuTypeVeo       = 255
};

typedef INT32 MACH_CPU_TYPE;

enum {
  MachCpuSubtypeInvalid = -1,
  //
  // Any
  //
  MachCpuSubtypeMultiple     = -1,
  MachCpuSubtypeLittleEndian = 0,
  MachCpuSubtypeBigEndian    = 1,
  //
  // VAX Subtypes
  //
  MachCpuSubtypeVaxAll  = 0,
  MachCpuSubtypeVax780  = 1,
  MachCpuSubtypeVax785  = 2,
  MachCpuSubtypeVax750  = 3,
  MachCpuSubtypeVax730  = 4,
  MachCpuSubtypeUVax1   = 5,
  MachCpuSubtypeUVax2   = 6,
  MachCpuSubtypeVax8200 = 7,
  MachCpuSubtypeVax8500 = 8,
  MachCpuSubtypeVax8600 = 9,
  MachCpuSubtypeVax8650 = 10,
  MachCpuSubtypeVax8800 = 11,
  MachCpuSubtypeUVax3   = 12,
  //
  // MC680 Subtypes
  //
  MachCpuSubtypeMc680All    = 1,
  MachCpuSubtypeMc68030     = MachCpuSubtypeMc680All,
  MachCpuSubtypeMc68040     = 2,
  MachCpuSubtypeMc68030Only = 3,
  //
  // x86 Subtypes
  //
  MachCpuSubtypeX86All   = MACH_CPU_SUBTYPE_INTEL (3, 0),
  MachCpuSubtypeX86Arch1 = MACH_CPU_SUBTYPE_INTEL (4, 0),
  //
  // x86_64 Subtypes
  //
  MachCpuSubtypeX8664All = MachCpuSubtypeX86All,
  MachCpuSubtypeX8664H   = MACH_CPU_SUBTYPE_INTEL (8, 0),
  //
  // i386 Subytpes
  //
  MachCpuSubtypeI386All       = MachCpuSubtypeX86All,
  MachCpuSubtype386           = MachCpuSubtypeI386All,
  MachCpuSubtype486           = MachCpuSubtypeX86Arch1,
  MachCpuSubtype486Sx         = MACH_CPU_SUBTYPE_INTEL (4, 8),
  MachCpuSubtype586           = MACH_CPU_SUBTYPE_INTEL (5, 0),
  MachCpuSubtypePentium       = MachCpuSubtype586,
  MachCpuSubtypePentiumPro    = MACH_CPU_SUBTYPE_INTEL (6, 1),
  MachCpuSubtypePentium3M3    = MACH_CPU_SUBTYPE_INTEL (6, 3),
  MachCpuSubtypePentium3M5    = MACH_CPU_SUBTYPE_INTEL (6, 5),
  MachCpuSubtypeCeleron       = MACH_CPU_SUBTYPE_INTEL (7, 6),
  MachCpuSubtypeCeleronMobile = MACH_CPU_SUBTYPE_INTEL (7, 7),
  MachCpuSubtypePentium3      = MACH_CPU_SUBTYPE_INTEL (8, 0),
  MachCpuSubtypePentium3M     = MACH_CPU_SUBTYPE_INTEL (8, 1),
  MachCpuSubtypePentium3Xeon  = MACH_CPU_SUBTYPE_INTEL (8, 2),
  MachCpuSubtypePentiumM      = MACH_CPU_SUBTYPE_INTEL (9, 0),
  MachCpuSubtypePentium4      = MACH_CPU_SUBTYPE_INTEL (10, 0),
  MachCpuSubtypePentium4M     = MACH_CPU_SUBTYPE_INTEL (10, 1),
  MachCpuSubtypeItanium       = MACH_CPU_SUBTYPE_INTEL (11, 0),
  MachCpuSubtypeItanium2      = MACH_CPU_SUBTYPE_INTEL (11, 1),
  MachCpuSubtypeXeon          = MACH_CPU_SUBTYPE_INTEL (12, 0),
  MachCpuSubtypeXeonMp        = MACH_CPU_SUBTYPE_INTEL (12, 1),
  //
  // Mips subtypes
  //
  MachCpuSubtypeMipsAll    = 0,
  MachCpuSubtypeMipsR2300  = 1,
  MachCpuSubtypeMipsR2600  = 2,
  MachCpuSubtypeMipsR2800  = 3,
  MachCpuSubtypeMipsR2000a = 4,
  MachCpuSubtypeMipsR2000  = 5,
  MachCpuSubtypeMipsR3000a = 6,
  MachCpuSubtypeMipsR3000  = 7,
  //
  // MC98000 Subtypes
  //
  MachCpuSubtypeMc98000All = 0,
  MachCpuSubtypeMc98601    = 1,
  //
  // HPPA Subtypes
  //
  MachCpuSubtypeHppaAll    = 0,
  MachCpuSubtypeHppa7100   = MachCpuSubtypeHppaAll,
  MachCpuSubtypeHppa7100Lc = 1,
  //
  // ARM Subtypes
  //
  MachCpuSubtypeArmAll    = 0,
  MachCpuSubtypeArmV4T    = 5,
  MachCpuSubtypeArmV6     = 6,
  MachCpuSubtypeArmV5Tej  = 7,
  MachCpuSubtypeArmXscale = 8,
  MachCpuSubtypeArmV7     = 9,
  MachCpuSubtypeArmV7F    = 10,
  MachCpuSubtypeArmV7S    = 11,
  MachCpuSubtypeArmV7K    = 12,
  MachCpuSubtypeArmV8     = 13,
  MachCpuSubtypeArmV6M    = 14,
  MachCpuSubtypeArmV7M    = 15,
  MachCpuSubtypeArmV7Em   = 16,
  //
  // ARM64 Subytypes
  //
  MachCpuSubtypeArm64All = 0,
  MachCpuSubtypeArm64V8  = 1,
  //
  // ARM64_32 Subtypes
  //
  MachCpuSubtypeArm6432All = 0,
  MachCpuSubtypeArm6432V8  = 1,
  //
  // MC88000 Subytpes
  //
  MachCpuSubtypeMc88000All = 0,
  MachCpuSubtypeMc88100    = 1,
  MachCpuSubtypeMc88110    = 2,
  //
  // SPARC Subtypes
  //
  MachCpuSubtypeSparcAll = 0,
  //
  // i860 Subtypes
  //
  MachCpuSubtypeI860All = 0,
  MachCpuSubtypeI860860 = 1,
  //
  // PowerPC Subtypes
  //
  MachCpuSubtypePowerPcAll   = 0,
  MachCpuSubtypePowerPc601   = 1,
  MachCpuSubtypePowerPc602   = 2,
  MachCpuSubtypePowerPc603   = 3,
  MachCpuSubtypePowerPc603e  = 4,
  MachCpuSubtypePowerPc603ev = 5,
  MachCpuSubtypePowerPc604   = 6,
  MachCpuSubtypePowerPc604e  = 7,
  MachCpuSubtypePowerPc620   = 8,
  MachCpuSubtypePowerPc750   = 9,
  MachCpuSubtypePowerPc7400  = 10,
  MachCpuSubtypePowerPc7450  = 11,
  MachCpuSubtypePowerPc970   = 100,
  //
  // VEO Subtypes
  //
  MachCpuSubtypeVeo1   = 1,
  MachCpuSubtypeVeo2   = 2,
  MachCpuSubtypeVeo3   = 3,
  MachCpuSubtypeVeo4   = 4,
  MachCpuSubtypeVeoAll = MachCpuSubtypeVeo2
};

typedef INT32 MACH_CPU_SUBTYPE;

///
/// CPU Family definitions
///
enum {
  MachCpuFamilyUnknown = 0,
  //
  // PowerPC Family
  //
  MachCpuFamilyPowerPcG3 = 0xCEE41549,
  MachCpuFamilyPowerPcG4 = 0x77C184AE,
  MachCpuFamilyPowerPcG5 = 0xED76D8AA,
  //
  // Intel Family
  //
  MachCpuFamilyIntel613         = 0xAA33392B,
  MachCpuFamilyIntelYonah       = 0x73D67300,
  MachCpuFamilyIntelMerom       = 0x426F69EF,
  MachCpuFamilyIntelPenryn      = 0x78EA4FBC,
  MachCpuFamilyIntelNehalem     = 0x6B5A4CD2,
  MachCpuFamilyIntelWestmere    = 0x573B5EEC,
  MachCpuFamilyIntelSandyBridge = 0x5490B78C,
  MachCpuFamilyIntelIvyBridge   = 0x1F65E835,
  MachCpuFamilyIntelHaswell     = 0x10B282DC,
  MachCpuFamilyIntelBroadwell   = 0x582ED09C,
  MachCpuFamilyIntelSkylake     = 0x37FC219F,
  MachCpuFamilyIntelKabyLake    = 0x0F817246,
  MachCpuFamilyIntelCoffeeLake  = MachCpuFamilyIntelKabyLake,
  MachCpuFamilyIntelIceLake     = 0x38435547,
  MachCpuFamilyIntelCometLake   = 0x1CF8A03E,
  //
  // The following synonyms are deprecated:
  //
  MachCpuFamilyIntel614         = MachCpuFamilyIntelYonah,
  MachCpuFamilyIntel615         = MachCpuFamilyIntelMerom,
  MachCpuFamilyIntel623         = MachCpuFamilyIntelPenryn,
  MachCpuFamilyIntel626         = MachCpuFamilyIntelNehalem,
  MachCpuFamilyIntelCore        = MachCpuFamilyIntelYonah,
  MachCpuFamilyIntelCore2       = MachCpuFamilyIntelMerom,
  //
  // ARM Family
  //
  MachCpuFamilyArm9                = 0xE73283AE,
  MachCpuFamilyArm11               = 0x8FF620D8,
  MachCpuFamilyArmXscale           = 0x53B005F5,
  MachCpuFamilyArm12               = 0xBD1B0AE9,
  MachCpuFamilyArm13               = 0x0CC90E64,
  MachCpuFamilyArm14               = 0x96077EF1,
  MachCpuFamilyArm15               = 0xA8511BCA,
  MachCpuFamilyArmSwift            = 0x1E2D6381,
  MachCpuFamilyArmCyclone          = 0x37A09642,
  MachCpuFamilyArmTyphoon          = 0x2C91A47E,
  MachCpuFamilyArmTwister          = 0x92FB37C8,
  MachCpuFamilyArmHurricane        = 0x67CEEE93,
  MachCpuFamilyArmMonsoonMistral   = 0xE81E7EF6,
  MachCpuFamilyArmVortexTempest    = 0x07D34B9F,
  MachCpuFamilyArmLightningThunder = 0x462504D2,
};

typedef UINT32 MACH_CPU_FAMILY;
typedef INT32  MACH_CPU_THREADTYPE;
typedef INT32  MACH_VM_PROTECTION;

///
/// A variable length string in a load command is represented by an lc_str
/// union.  The strings are stored just after the load command structure and
/// the offset is from the start of the load command structure.  The size of
/// the string is reflected in the cmdsize field of the load command.  Once
/// again any padded bytes to bring the cmdsize field to a multiple of 4 bytes
/// must be zero.
///
typedef union {
  UINT32 Offset;       ///< offset to the string
  UINT32 Address32;    ///< pointer to the string
} MACH_LOAD_COMMAND_STRING;

///
/// Fixed virtual memory shared libraries are identified by two things.  The
/// target pathname (the name of the library as found for execution), and the
/// minor version number.  The address of where the headers are loaded is in
/// header_addr. (THIS IS OBSOLETE and no longer supported).
///
typedef struct {
  MACH_LOAD_COMMAND_STRING Name;           ///< library's target pathname
  UINT32                   MinorVersion;   ///< library's minor version number
  UINT32                   HeaderAddress;  ///< library's header address
} MACH_FIXED_VM_LIB;

///
/// Dynamicly linked shared libraries are identified by two things.  The
/// pathname (the name of the library as found for execution), and the
/// compatibility version number.  The pathname must match and the
/// compatibility number in the user of the library must be greater than or
/// equal to the library being used.  The time stamp is used to record the time
/// a library was built and copied into user so it can be use to determined if
/// the library used at runtime is exactly the same as used to built the
/// program.
///
typedef struct {
  ///
  /// library's path name
  ///
  MACH_LOAD_COMMAND_STRING Name;
  ///
  /// library's build time stamp
  ///
  UINT32                   Timestamp;
  ///
  /// library's current version number
  ///
  UINT32                   CurrentVersion;
  ///
  /// library's compatibility vers number
  ///
  UINT32                   CompatibilityVersion;
} MACH_DYLIB;

//
// Mach Load Commands
//

///
/// After MacOS X 10.1 when a new load command is added that is required to be
/// understood by the dynamic linker for the image to execute properly the
/// MACH_LC_REQUIRE_DYLD bit will be or 'ed into the load
/// command constant.  If the dynamic linker sees such a load command it it
/// does not understand will issue a "unknown load command required for
/// execution" error and refuse to use the image. Other load commands without
/// this bit that are not understood will simply be ignored.
///
#define MACH_LC_REQUIRE_DYLD  BIT31
///
/// segment of this file to be mapped
///
#define MACH_LOAD_COMMAND_SEGMENT                  1U
///
/// link-edit stab symbol table info
///
#define MACH_LOAD_COMMAND_SYMTAB                   2U
///
/// link-edit gdb symbol table info (obsolete)
///
#define MACH_LOAD_COMMAND_SYMSEG                   3U
///
/// thread
///
#define MACH_LOAD_COMMAND_THREAD                   4U
///
/// unix thread (includes a stack)
///
#define MACH_LOAD_COMMAND_UNIX_THREAD              5U
///
/// load a specified fixed VM shared library
///
#define MACH_LOAD_COMMAND_LOAD_FIXED_VM_LIB        6U
///
/// fixed VM shared library identification
///
#define MACH_LOAD_COMMAND_IDENTIFICATION_VM_LIB    7U
///
/// object identification info (obsolete)
///
#define MACH_LOAD_COMMAND_IDENTIFICATION           8U
///
/// fixed VM file inclusion (internal use)
///
#define MACH_LOAD_COMMAND_FIXED_VM_FILE            9U
///
/// prepage command (internal use)
///
#define MACH_LOAD_COMMAND_PRE_PAGE                 10U
///
/// dynamic link-edit symbol table info
///
#define MACH_LOAD_COMMAND_DYSYMTAB                 11U
///
/// load a dynamically linked shared library
///
#define MACH_LOAD_COMMAND_LOAD_DYLIB               12U
///
/// dynamically linked shared lib ident
///
#define MACH_LOAD_COMMAND_IDENTITY_DYLIB           13U
///
/// load a dynamic linker
///
#define MACH_LOAD_COMMAND_LOAD_DYLD                14U
///
/// dynamic linker identification
///
#define MACH_LOAD_COMMAND_IDENTIFICATION_DYLD      15U
///
/// modules prebound for a dynamically
///
#define MACH_LOAD_COMMAND_PREBOUNT_SYLIB           16U
///
/// image routines
///
#define MACH_LOAD_COMMAND_ROUTINES                 17U
///
/// sub framework
///
#define MACH_LOAD_COMMAND_SUB_FRAMEWORK            18U
///
/// sub umbrella
///
#define MACH_LOAD_COMMAND_SUB_UMBRELLA             19U
///
/// sub clien
///
#define MACH_LOAD_COMMAND_SUB_CLIENT               20U
///
/// sub library
///
#define MACH_LOAD_COMMAND_SUB_LIBRARY              21U
///
/// two-level namespace lookup hints
///
#define MACH_LOAD_COMMAND_TWO_LEVEL_HINTS          22U
///
/// prebind checksum
///
#define MACH_LOAD_COMMAND_PREBIND_CHECKSUM         23U
///
/// load a dynamically linked shared library that is allowed to be missing
/// (all symbols are weak imported).
///
#define MACH_LOAD_COMMAND_LLOAD_WEAK_DYLIB         (24U | MACH_LC_REQUIRE_DYLD)
///
/// 64-bit segment of this file to be mapped
///
#define MACH_LOAD_COMMAND_SEGMENT_64               25U
///
/// 64-bit image routines
///
#define MACH_LOAD_COMMAND_ROUTINES_64              26U
///
/// the uuid
///
#define MACH_LOAD_COMMAND_UUID                     27U
///
/// runpath additions
///
#define MACH_LOAD_COMMAND_RUN_PATH                 (28U | MACH_LC_REQUIRE_DYLD)
///
/// local of code signature
///
#define MACH_LOAD_COMMAND_CODE_SIGNATURE           29U
///
/// local of info to split segments
///
#define MACH_LOAD_COMMAND_SEGMENT_SPLIT_INFO       30U
///
/// load and re-export dylib
///
#define MACH_LOAD_COMMAND_REEXPORT_DYLIB           (31U | MACH_LC_REQUIRE_DYLD)
///
/// delay load of dylib until first use
///
#define MACH_LOAD_COMMAND_LAZY_LOAD_DYLIB          32U
///
/// encrypted segment information
///
#define MACH_LOAD_COMMAND_ENCRYPTION_INFO          33U
///
/// compressed dyld information
///
#define MACH_LOAD_COMMAND_DYLD_INFO                34U
///
/// compressed dyld information only
///
#define MACH_LOAD_COMMAND_DYLD_INFO_ONLY           (34U | MACH_LC_REQUIRE_DYLD)
///
/// load upward dylib
///
#define MACH_LOAD_COMMAND_LOAD_UPWARD_DYLIB        (35U | MACH_LC_REQUIRE_DYLD)
///
/// build for MacOSX min OS version
///
#define MACH_LOAD_COMMAND_VERSION_MIN_MAC_OS_X     36U
///
/// build for iPhoneOS min OS version
///
#define MACH_LOAD_COMMAND_VERSION_MIN_IPHONE_OS    37U
///
/// compressed table of function start addresses
///
#define MACH_LOAD_COMMAND_FUNCTION_STARTS          38U
///
/// string for dyld to treat like environment variable
///
#define MACH_LOAD_COMMAND_DYLD_ENVIRONMENT         39U
///
/// replacement for MachLoadCommandUnixThread
///
#define MACH_LOAD_COMMAND_MAIN                     (40U | MACH_LC_REQUIRE_DYLD)
///
/// table of non-in structions in __text
///
#define MACH_LOAD_COMMAND_DATA_IN_CODE             41U
///
/// source version used to build binary
///
#define MACH_LOAD_COMMAND_SOURCE_VERSION           42U
///
/// Code signing DRs copied from linked dylibs
///
#define MACH_LOAD_COMMAND_DYLIB_CODE_SIGN_DRS      43U
///
/// 64-bit encrypted segment information
///
#define MACH_LOAD_COMMAND_ENCRYPTION_INFO_64       44U
///
/// linker options in MH_OBJECT files
///
#define MACH_LOAD_COMMAND_LINKER_OPTION            45U
///
/// optimization hints in MH_OBJECT files
///
#define MACH_LOAD_COMMAND_LINKER_OPTIMIZATION_HINT 46U
///
/// build for AppleTV min OS version
///
#define MACH_LOAD_COMMAND_VERSION_MIN_TV_OS        47U
///
/// build for Watch min OS version
///
#define MACH_LOAD_COMMAND_VERSION_MIN_WATCH_OS     48U
///
/// arbitrary data included within a Mach-O file
///
#define MACH_LOAD_COMMAND_NOTE                     49U
///
/// build for platform min OS version
///
#define MACH_LOAD_COMMAND_BUILD_VERSION            50U
///
/// used with linkedit_data_command, payload is trie
///
#define MACH_LOAD_COMMAND_DYLD_EXPORTS_TRIE        (51U | MACH_LC_REQUIRE_DYLD)
///
/// used with linkedit_data_command
///
#define MACH_LOAD_COMMAND_DYLD_CHAINED_FIXUPS      (52U | MACH_LC_REQUIRE_DYLD)
///
/// used with fileset_entry_command
///
#define MACH_LOAD_COMMAND_FILESET_ENTRY            (53U  | MACH_LC_REQUIRE_DYLD)

typedef UINT32 MACH_LOAD_COMMAND_TYPE;

#define MACH_LOAD_COMMAND_HDR_                                    \
  MACH_LOAD_COMMAND_TYPE CommandType; /* type of load command */  \
  UINT32                 CommandSize; /* total size of command in bytes
                                         (includes sizeof section structs) */

#define NEXT_MACH_LOAD_COMMAND(Command)  \
  ((MACH_LOAD_COMMAND *)((UINTN)(Command) + (Command)->CommandSize))

typedef struct {
  MACH_LOAD_COMMAND_HDR_
} MACH_LOAD_COMMAND;

//
// Constants for the Flags field of the MACH_SEGMENT_COMMAND struct
//

///
/// the file contents for this segment is for the high part of the VM space,
/// the low part  is zero filled (for stacks in core files)
///
#define MACH_SEGMENT_FLAG_HIGH_VIRTUAL_MEMORY  BIT0
///
/// this segment is the VM that is allocated by a fixed VM library, for overlap
/// checking in the link editor
///
#define MACH_SEGMENT_FLAG_FIXED_VM_LIB         BIT1
///
/// this segment has nothing that was relocated in it and nothing relocated to
/// it, that is it maybe safely replaced without relocation
///
#define MACH_SEGMENT_FLAG_NO_RELOCATION        BIT2
///
/// This segment is protected.  If the segment starts at file offset 0, the
/// first page of the segment is not protected.  All other pages of the
/// segment are protected.
///
#define MACH_SEGMENT_FLAG_PROTECTED_VERSION_1  BIT3

#define MACH_SEGMENT_VM_PROT_NONE     0U

#define MACH_SEGMENT_VM_PROT_READ     BIT0
#define MACH_SEGMENT_VM_PROT_WRITE    BIT1
#define MACH_SEGMENT_VM_PROT_EXECUTE  BIT2

typedef UINT32 MACH_SEGMENT_FLAGS;

///
/// Constants for the type of a section
///
enum {
  ///
  /// regular section
  ///
  MachSectionTypeRegular                               = 0,
  ///
  /// zero fill on demand section
  ///
  MachSectionTypeZeroFill                              = 1,
  ///
  /// section with only literal C strings
  ///
  MachSectionTypeCStringLiterals                       = 2,
  ///
  /// section with only 4 byte literals
  ///
  MachSectionType4ByteLiterals                         = 3,
  ///
  /// section with only 8 byte literals
  ///
  MachSectionType8ByteLiterals                         = 4,
  ///
  /// section with only pointers to literals
  ///
  MachSectionTypeLiteralPointers                       = 5,

  //
  // For the two types of symbol pointers sections and the symbol stubs section
  // they have indirect symbol table entries.  For each of the entries in the
  // section the indirect symbol table entries, in corresponding order in the
  // indirect symbol table, start at the index stored in the reserved1 field
  // of the section structure.  Since the indirect symbol table entries
  // correspond to the entries in the section the number of indirect symbol
  // table entries is inferred from the size of the section divided by the size
  // of the entries in the section.  For symbol pointers sections the size of
  // the entries in the section is 4 bytes and for symbol stubs sections the
  // byte size of the stubs is stored in the reserved2 field of the section
  // structure.
  //

  ///
  /// section with only non-lazy symbol pointers
  ///
  MachSectionTypeNonLazySymbolPointers                 = 6,
  ///
  /// section with only lazy symbol pointers
  ///
  MachSectionTypeLazySymbolPointers                    = 7,
  ///
  /// section with only symbol stubs, byte size of stub in the reserved2 field
  ///
  MachSectionTypeSymbolStubs                           = 8,
  ///
  /// section with only function pointers for initialization
  ///
  MachSectionTypeModInitalizeFunctionPointers          = 9,
  ///
  /// section with only function pointers for termination
  ///
  MachSectionTypeModTerminateFunctionPointers          = 10,
  ///
  /// section contains symbols that are to be coalesced
  ///
  MachSectionTypeCoalesced                             = 11,
  ///
  /// zero fill on demand section (that can be larger than 4 gigabytes)
  ///
  MachSectionTypeGigabyteZeroFill                      = 12,
  ///
  /// section with only pairs of function pointers for interposing
  ///
  MachSectionTypeInterposing                           = 13,
  ///
  /// section with only 16 byte literals
  ///
  MachSectionType16ByteLiterals                        = 14,
  ///
  /// section contains DTrace Object Format
  ///
  MachSectionTypeDtraceObjectFormat                    = 15,
  ///
  /// section with only lazy symbol pointers to lazy loaded dylibs
  ///
  MachSectionTypeLazyDylibSymbolPointers               = 16,

  //
  // Section types to support thread local variables
  //

  ///
  /// template of initial values for TLVs
  ///
  MachSectionTypeThreadLocalRegular                    = 17,
  ///
  /// template of initial values for TLVs
  ///
  MachSectionTypeThreadLocalZeroFill                   = 18,
  ///
  /// TLV descriptors
  ///
  MachSectionTypeThreadLocalVariables                  = 19,
  ///
  /// pointers to TLV descriptors
  ///
  MachSectionTypeThreadVariablePointers                = 20,
  ///
  /// functions to call to initialize TLV values
  ///
  MachSectionTypeThreadLocalInitializeFunctionPointers = 21,
  ///
  /// 32-bit offsets to initializers
  ///
  MachSectionTypeThreadLocalInitializeFunctionOffsets  = 22
};

//
// Constants for the section attributes part of the Flags field of a section
// structure.
//

///
/// system setable attributes
///
#define MACH_SECTION_ATTRIBUTES_SYSYTEM            0x00FFFF00U
///
/// User setable attributes
///
#define MACH_SECTION_ATTRIBUTES_USER               0xFF000000U
///
/// section has local relocation entries
///
#define MACH_SECTION_ATTRIBUTE_LOCAL_RELOCATION     BIT8
///
/// section has external relocation entries
///
#define MACH_SECTION_ATTRIBUTE_EXTERNAL_RELOCATION  BIT9
///
/// section contains some machine instructions
///
#define MACH_SECTION_ATTRIBUTE_SOME_INSTRUCTIONS    BIT10

//
// The flags field of a section structure is separated into two parts a section
// type and section attributes.The section types are mutually exclusive (it
// can only have one type) but the section attributes are not (it may have more
// than one attribute).
//
#define MACH_SECTION_TYPE_MASK        0x000000FFU  ///< 256 section types
#define MACH_SECTION_ATTRIBUTES_MASK  0xFFFFFF00U  ///< 24 section attributes

///
/// If a segment contains any sections marked with
/// MACH_SECTION_ATTRIBUTE_DEBUG then all sections in that segment must have
/// this attribute.  No section other than a section marked with this attribute
/// may reference the contents of this section.  A section with this attribute
/// may contain no symbols and must have a section type S_REGULAR.  The static
/// linker will not copy section contents from sections with this attribute
/// into its output file.  These sections generally contain DWARF debugging
/// info.
///
#define MACH_SECTION_ATTRIBUTE_DEBUG                 BIT25
///
/// Used with i386 code stubs written on by dyld
///
#define MACH_SECTION_ATTRIBUTE_SELF_MODIFYING_CODE   BIT26
///
/// blocks are live if they reference live blocks
///
#define MACH_SECTION_ATTRIBUTE_LIVE_SUPPORT          BIT27
///
/// no dead stripping
///
#define MACH_SECTION_ATTRIBUTE_NO_DEAD_STRIP         BIT28
///
/// ok to strip static symbols in this section in files with the
/// MACH_HEADER_FLAG_DYNAMIC_LINKER_LINK flag
///
#define MACH_SECTION_ATTRIBUTE_STRIP_STATIC_SYMBOLS  BIT29
///
/// section contains coalesced symbols that are not to be in a ranlib table of
/// contents
///
#define MACH_SECTION_ATTRIBUTE_NO_TOC                BIT30
///
/// section contains only true machine instructions
///
#define MACH_SECTION_ATTRIBUTE_PURE_INSTRUCTIONS     BIT31

///
/// A segment is made up of zero or more sections.
/// Non-MachHeaderFileTypeObject files have all of their segments with the
/// proper sections in each, and padded to the Sectionecified segment alignment
/// when produced by the link editor.  The first segment of a
/// MachHeaderFileTypeExecute and MachHeaderFileTypeFixedVmLib format file
/// contains the MACH_HEADER and load commands of the object file before its
/// first section.  The zero fill sections are always last in their segment (in
/// all formats).  This allows the zeroed segment padding to be mapped into
/// memory where zero fill sections might be.  The gigabyte zero fill sections,
/// those with the section type MachSectionTypeGigabyteZeroFill, can only be
/// in a segment with sections of this type.  These segments are then placed
/// after all other segments.
///
/// The MachHeaderFileTypeObject format has all of its sections in one segment
/// for compactness.  There is no padding to a Sectionecified segment boundary
/// and the MACH_HEADER and load commands are not part of the segment.
///
/// Sections with the same section Name, SectionName, going into the same
/// segment, SegmentName, are combined by the link editor.  The resulting
/// section is aligned to the maximum alignment of the combined sections and is
/// the new section's  alignment.  The combined sections are aligned to their
/// original alignment in the combined section.  Any padded bytes to get the
/// Sectionecified alignment are zeroed.
///
/// The format of the relocation entries referenced by the reloff and nreloc
/// fields of the section structure for mach object files is described in the
/// header file <reloc.h>.
///
/// for 32-bit architectures
///
typedef struct {
  CHAR8  SectionName[16];          ///< Name of this section
  CHAR8  SegmentName[16];          ///< segment this section goes in
  UINT32 Address;                  ///< memory address of this section
  UINT32 Size;                     ///< size in bytes of this section
  UINT32 Offset;                   ///< file offset of this section
  UINT32 Alignment;                ///< section alignment (power of 2)
  UINT32 RelocationsOffset;        ///< file offset of relocation entries
  UINT32 NumRelocations;           ///< number of relocation entries
  UINT32 Flags;                    ///< flags (section type and attributes)
  UINT32 Reserved1;                ///< reserved (for offset or index)
  UINT32 Reserved2;                ///< reserved (for count or sizeof)
} MACH_SECTION;

///
/// for 64-bit architectures
///
typedef struct {
  CHAR8  SectionName[16];    ///< Name of this section
  CHAR8  SegmentName[16];    ///< segment this section goes in
  UINT64 Address;            ///< memory address of this section
  UINT64 Size;               ///< size in bytes of this section
  UINT32 Offset;             ///< file offset of this section
  UINT32 Alignment;          ///< section alignment (power of 2)
  UINT32 RelocationsOffset;  ///< file offset of relocation entries
  UINT32 NumRelocations;     ///< number of relocation entries
  UINT32 Flags;              ///< flags (section type and attributes)
  UINT32 Reserved1;          ///< reserved (for offset or index)
  UINT32 Reserved2;          ///< reserved (for count or sizeof)
  UINT32 Reserved3;          ///< reserved
} MACH_SECTION_64;

typedef union {
  MACH_SECTION    Section32;
  MACH_SECTION_64 Section64;
} MACH_SECTION_ANY;

#define NEXT_MACH_SEGMENT(Segment) \
  (MACH_SEGMENT_COMMAND *)((UINTN)(Segment) + (Segment)->Command.Size)

///
/// The segment load command indicates that a part of this file is to be
/// mapped into the task's address Sectionace.  The size of this segment in
/// memory, vmsize, maybe equal to or larger than the amount to map from this
/// file, filesize.  The file is mapped starting at fileoff to the beginning of
/// the segment in memory, vmaddr.  The rest of the memory of the segment,
/// if any, is allocated zero fill on demand.  The segment's maximum virtual
/// memory protection and initial virtual memory protection are Sectionecified
/// by the maxprot and initprot fields.  If the segment has sections then the
/// section structures directly follow the segment command and their size is
/// reflected in cmdsize.
///
/// for 32 - bit architectures
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  CHAR8              SegmentName[16];    ///< segment Name
  UINT32             VirtualAddress;     ///< memory address of this segment
  UINT32             Size;               ///< memory size of this segment
  UINT32             FileOffset;         ///< file offset of this segment
  UINT32             FileSize;           ///< amount to map from the file
  MACH_VM_PROTECTION MaximumProtection;  ///< maximum VM protection
  MACH_VM_PROTECTION InitialProtection;  ///< initial VM protection
  UINT32             NumSections;        ///< number of sections in segment
  MACH_SEGMENT_FLAGS Flags;              ///< flags
  MACH_SECTION       Sections[];
} MACH_SEGMENT_COMMAND;

#define NEXT_MACH_SEGMENT_64(Segment) \
  (MACH_SEGMENT_COMMAND_64 *)((UINTN)(Segment) + (Segment)->Hdr.Size)

///
/// The 64-bit segment load command indicates that a part of this file is to be
/// mapped into a 64-bit task's address Sectionace.  If the 64-bit segment has
/// sections then section_64 structures directly follow the 64-bit segment
/// command and their size is reflected in cmdsize.
///
/// for 64-bit architectures
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  CHAR8              SegmentName[16];    ///< segment Name
  UINT64             VirtualAddress;     ///< memory address of this segment
  UINT64             Size;               ///< memory size of this segment
  UINT64             FileOffset;         ///< file offset of this segment
  UINT64             FileSize;           ///< amount to map from the file
  MACH_VM_PROTECTION MaximumProtection;  ///< maximum VM protection
  MACH_VM_PROTECTION InitialProtection;  ///< initial VM protection
  UINT32             NumSections;   ///< number of sections in segment
  MACH_SEGMENT_FLAGS Flags;              ///< flags
  MACH_SECTION_64    Sections[];
} MACH_SEGMENT_COMMAND_64;

typedef union {
  MACH_SEGMENT_COMMAND    Segment32;
  MACH_SEGMENT_COMMAND_64 Segment64;
} MACH_SEGMENT_COMMAND_ANY;

///
/// A fixed virtual shared library (filetype == MH_FVMLIB in the mach header)
/// contains a fvmlib_command (cmd == LC_IDFVMLIB) to identify the library.
/// An object that uses a fixed virtual shared library also contains a
/// fvmlib_command (cmd == LC_LOADFVMLIB) for each library it uses.
/// (THIS IS OBSOLETE and no longer supported).
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  MACH_FIXED_VM_LIB FixedVmLib;  ///< the library identification
} MACH_FIXED_VM_LIB_COMMAND;

///
/// A dynamically linked shared library (filetype == MH_DYLIB in the mach
/// header) contains a dylib_command (cmd == LC_ID_DYLIB) to identify the
/// library.  An object that uses a dynamically linked shared library also
/// contains a dylib_command (cmd == LC_LOAD_DYLIB, LC_LOAD_WEAK_DYLIB, or
/// LC_REEXPORT_DYLIB) for each library it uses.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  MACH_DYLIB        Dylib;  ///< the library identification
} MACH_DYLIB_COMMAND;

///
/// A dynamically linked shared library may be a subframework of an umbrella
/// framework.  If so it will be linked with "-umbrella umbrella_name" where
/// Where "umbrella_name" is the name of the umbrella framework. A subframework
/// can only be linked against by its umbrella framework or other subframeworks
/// that are part of the same umbrella framework.  Otherwise the static link
/// editor produces an error and states to link against the umbrella framework.
/// The name of the umbrella framework for subframeworks is recorded in the
/// following structure.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  MACH_LOAD_COMMAND_STRING Umbrella;  ///< the umbrella framework name
} MACH_SUB_FRAMEWORK_COMMAND;

///
/// For dynamically linked shared libraries that are subframework of an
/// umbrella  framework they can allow clients other than the umbrella
/// framework or other subframeworks in the same umbrella framework.  To do
/// this the subframework is built with "-allowable_client client_name" and an
/// LC_SUB_CLIENT load command is created for each -allowable_client flag.
/// The client_name is usually a framework name.  It can also be a name used
/// for bundles clients where the bundle is built with
/// "-client_name client_name".
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  MACH_LOAD_COMMAND_STRING Client;  ///< the client name
} MACH_SUB_CLIENT_COMMAND;

///
/// A dynamically linked shared library may be a sub_umbrella of an umbrella
/// framework.  If so it will be linked with "-sub_umbrella umbrella_name"
/// where "umbrella_name" is the name of the sub_umbrella framework.  When
/// staticly linking when -twolevel_namespace is in effect a twolevel namespace
/// umbrella framework will only cause its subframeworks and those frameworks
/// listed as sub_umbrella frameworks to be implicited linked in.  Any other
/// dependent dynamic libraries will not be linked it when -twolevel_namespace
/// is in effect.  The primary library recorded by the static linker when
/// resolving a symbol in these libraries will be the umbrella framework.
/// Zero or more sub_umbrella frameworks may be use by an umbrella framework.
/// The name of a sub_umbrella framework is recorded in the following structure.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  MACH_LOAD_COMMAND_STRING SubUmbrella;  ///< the sub_umbrella framework name
} MACH_SUB_UMBRELLA_COMMAND;

///
/// A dynamically linked shared library may be a sub_library of another shared
/// library.  If so it will be linked with "-sub_library library_name" where
/// Where "library_name" is the name of the sub_library shared library.  When
/// staticly linking when -twolevel_namespace is in effect a twolevel namespace
/// shared library will only cause its subframeworks and those frameworks
/// listed as sub_umbrella frameworks and libraries listed as sub_libraries to
/// be implicited linked in.  Any other dependent dynamic libraries will not be
/// linked it when -twolevel_namespace is in effect.  The primary library
/// recorded by the static linker when resolving a symbol in these libraries
/// will be the umbrella framework (or dynamic library). Zero or more
/// sub_library shared libraries may be use by an umbrella framework or (or
/// dynamic library).  The name of a sub_library framework is recorded in the
/// following structure.  For example /usr/lib/libobjc_profile.A.dylib would be
/// recorded as "libobjc".
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  MACH_LOAD_COMMAND_STRING SubLibrary;  ///< the sub_library name
} MACH_SUB_LIBRARY_COMMAND;

///
/// A program (filetype == MH_EXECUTE) that is
/// prebound to its dynamic libraries has one of these for each library that
/// the static linker used in prebinding.  It contains a bit vector for the
/// modules in the library.  The bits indicate which modules are bound (1) and
/// which are not (0) from the library.  The bit for module 0 is the low bit
/// of the first byte.  So the bit for the Nth module is:
/// (linked_modules[N/8] >> N%8) & 1
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  MACH_LOAD_COMMAND_STRING Name;             ///< library's path name
  UINT32                   NumModules;       ///< number of modules in library
  MACH_LOAD_COMMAND_STRING LinkedModules[];  ///< bit vector of linked modules
} MACH_PREBOUND_DYLIB_COMMAND;

///
/// A program that uses a dynamic linker contains a dylinker_command to
/// identify the name of the dynamic linker (LC_LOAD_DYLINKER).  And a dynamic
/// linker contains a dylinker_command to identify the dynamic linker
/// (LC_ID_DYLINKER).  A file can have at most one of these.
/// This struct is also used for the LC_DYLD_ENVIRONMENT load command and
/// contains string for dyld to treat like environment variable.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  MACH_LOAD_COMMAND_STRING Name;  ///< dynamic linker's path name
} MACH_DYLINKER_COMMAND;

enum {
  MachX86ThreadState32    = 1,
  MachX86FloatState32     = 2,
  MachX86ExceptionState32 = 3,
  MachX86ThreadState64    = 4,
  MachX86FloatState64     = 5,
  MachX86ExceptionState64 = 6,
  MachX86ThreadState      = 7,
  MachX86FloatState       = 8,
  MachX86ExceptionState   = 9,
  MachX86DebugState32     = 10,
  MachX86DebugState64     = 11,
  MachX86DebugState       = 12,
  MachThreadStateNone     = 13,
  //
  // 14 and 15 are used for the internal x86SavedState flavours.
  //
  MachX86AvxState32       = 16,
  MachX86AvxState64       = 17,
  MachX86AvxState         = 18,
  MachX86Avx512State32    = 19,
  MachX86Avx512State64    = 20,
  MachX86Avx512State      = 21,
};

///
/// Size of maximum exported thread state in words
///
#define MACH_X86_THREAD_STATE_MAX 614

typedef struct {
  UINT32 eax;
  UINT32 ebx;
  UINT32 ecx;
  UINT32 edx;
  UINT32 edi;
  UINT32 esi;
  UINT32 ebp;
  UINT32 esp;
  UINT32 ss;
  UINT32 eflags;
  UINT32 eip;
  UINT32 cs;
  UINT32 ds;
  UINT32 es;
  UINT32 fs;
  UINT32 gs;
} MACH_X86_THREAD_STATE32;

typedef struct {
  UINT64 rax;
  UINT64 rbx;
  UINT64 rcx;
  UINT64 rdx;
  UINT64 rdi;
  UINT64 rsi;
  UINT64 rbp;
  UINT64 rsp;
  UINT64 r8;
  UINT64 r9;
  UINT64 r10;
  UINT64 r11;
  UINT64 r12;
  UINT64 r13;
  UINT64 r14;
  UINT64 r15;
  UINT64 rip;
  UINT64 rflags;
  UINT64 cs;
  UINT64 fs;
  UINT64 gs;
} MACH_X86_THREAD_STATE64;

typedef union {
  MACH_X86_THREAD_STATE32 State32;
  MACH_X86_THREAD_STATE64 State64;
} MACH_X86_THREAD_STATE;

///
/// MACH_VALID_THREAD_STATE_FLAVOR is a platform specific macro that when
/// passed an exception flavor will return if that is a defined flavor for that
/// platform. The macro must be manually updated to include all of the valid
/// exception flavors as defined above.
///
#define MACH_VALID_THREAD_STATE_FLAVOR(Flavor)  \
    (((Flavor) == MachX86ThreadState32)         \
  || ((Flavor) == MachX86FloatState32)          \
  || ((Flavor) == MachX86ExceptionState32)      \
  || ((Flavor) == MachX86DebugState32)          \
  || ((Flavor) == MachX86ThreadState64)         \
  || ((Flavor) == MachX86FloatState64)          \
  || ((Flavor) == MachX86ExceptionState64)      \
  || ((Flavor) == MachX86DebugState64)          \
  || ((Flavor) == MachX86ThreadState)           \
  || ((Flavor) == MachX86FloatState)            \
  || ((Flavor) == MachX86ExceptionState)        \
  || ((Flavor) == MachX86DebugState)            \
  || ((Flavor) == MachX86AvxState32)            \
  || ((Flavor) == MachX86AvxState64)            \
  || ((Flavor) == MachX86AvxState)              \
  || ((Flavor) == MachX86Avx512State32)         \
  || ((Flavor) == MachX86Avx512State64)         \
  || ((Flavor) == MachX86Avx512State)           \
  || ((Flavor) == MachThreadStateNone))

///
/// Thread commands contain machine-specific data structures suitable for
/// use in the thread state primitives.  The machine specific data structures
/// follow the  struct thread_command as follows.
/// Each flavor of machine specific data structure is preceded by an unsigned
/// long constant for the flavor of that data structure, an UINT32
/// that is the count of longs of the size of the state data  structure and
/// then the state data  structure follows.  This triple may be repeated for
/// many flavors.  The constants for the flavors, counts and state data
/// structure definitions are expected to be in the header file
/// <machine/thread_status.h>.
/// These machine specific data structures sizes must be multiples of
/// 4 bytes  The cmdsize reflects the total size of the thread_command
/// and all of the sizes of the constants for the flavors, counts and state
/// data structures.
///
/// For executable objects that are unix processes there will be one
/// thread_command (cmd == LC_UNIXTHREAD) created for it by the link-editor.
/// This is the same as a LC_THREAD, except that a stack is automatically
/// created (based on the shell's limit for the stack size).  Command arguments
/// and environment variables are copied onto that stack.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  UINT32            Flavor;           ///< flavor of thread state
  UINT32            NumThreadStates;  ///< count of UINT32s in thread state
  UINT32            ThreadState[];
} MACH_THREAD_COMMAND;

///
/// The routines command contains the address of the dynamic shared library
/// initialization routine and an index into the module table for the module
/// that defines the routine.  Before any modules are used from the library the
/// dynamic linker fully binds the module that defines the initialization routine
/// and then calls it.  This gets called before any module initialization
/// routines (used for C++ static constructors) in the library.
/// For 32-bit architectures.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  ///
  /// address of initialization routine
  ///
  UINT32            InitRoutineAddress;
  ///
  /// index into the module table that the init routine is defined in
  ///
  UINT32            InitModuleIndex;
  UINT32            Reserved1;
  UINT32            Reserved2;
  UINT32            Reserved3;
  UINT32            Reserved4;
  UINT32            Reserved5;
  UINT32            Reserved6;
} MACH_ROUTINES_COMMAND;

///
/// The 64-bit routines command.  Same use as above.  For 64-bit architectures.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  ///
  /// address of initialization routine
  ///
  UINT64            InitiRoutineAddress;
  ///
  /// index into the module table that the init routine is defined in
  ///
  UINT64            InitModuleIndex;
  UINT64            Reserved1;
  UINT64            Reserved2;
  UINT64            Reserved3;
  UINT64            Reserved4;
  UINT64            Reserved5;
  UINT64            Reserved6;
} MACH_ROUTINES_COMMAND_64;

///
/// The symtab_command contains the offsets and sizes of the link-edit 4.3BSD
/// "stab" style symbol table information as described in the header files
/// <nlist.h> and <stab.h>.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  UINT32            SymbolsOffset;  ///< symbol table offset
  UINT32            NumSymbols;     ///< number of symbol table entries
  UINT32            StringsOffset;  ///< string table offset
  UINT32            StringsSize;    ///< string table size in bytes
} MACH_SYMTAB_COMMAND;

///
/// This is the second set of the symbolic information which is used to support
/// the data structures for the dynamically link editor.
///
/// The original set of symbolic information in the symtab_command which
/// contains the symbol and string tables must also be present when this load
/// command is present.  When this load command is present the symbol table is
/// organized into three groups of symbols:
///  local symbols (static and debugging symbols) - grouped by module
///  defined external symbols - grouped by module (sorted by name if not lib)
///  undefined external symbols (sorted by name if MH_BINDATLOAD is not set,
///                 and in order the were seen by the static
///            linker if MH_BINDATLOAD is set)
/// In this load command there are offsets and counts to each of the three
/// groups of symbols.
///
/// This load command contains a the offsets and sizes of the following new
/// symbolic information tables:
///  table of contents
///  module table
///  reference symbol table
///  indirect symbol table
/// The first three tables above (the table of contents, module table and
/// reference symbol table) are only present if the file is a dynamically
/// linked shared library.  For executable and object modules, which are files
/// containing only one module, the information that would be in these three
/// tables is determined as follows:
///   table of contents - the defined external symbols are sorted by name
///  module table - the file contains only one module so everything in the
///           file is part of the module.
///  reference symbol table - is the defined and undefined external symbols
///
/// For dynamically linked shared library files this load command also contains
/// offsets and sizes to the pool of relocation entries for all sections
/// separated into two groups:
///  external relocation entries
///  local relocation entries
/// For executable and object modules the relocation entries continue to hang
/// off the section structures.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_

  //
  // The symbols indicated by symoff and nsyms of the LC_SYMTAB load command
  // are grouped into the following three groups:
  //  local symbols (further grouped by the module they are from)
  //  defined external symbols (further grouped by the module they are from)
  //  undefined symbols
  //
  // The local symbols are used only for debugging.  The dynamic binding
  // process may have to use them to indicate to the debugger the local
  // symbols for a module that is being bound.
  //
  // The last two groups are used by the dynamic binding process to do the
  // binding (indirectly through the module table and the reference symbol
  // table when this is a dynamically linked shared library file).
  //

  UINT32 LocalSymbolsIndex;  ///< index to local symbols
  UINT32 NumLocalSymbols;    ///< number of local symbols

  UINT32 ExternalSymbolsIndex;  ///< index to externally defined symbols
  UINT32 NumExternalSymbols;    ///< number of externally defined symbols

  UINT32 UndefinedSymbolsIndex;  ///< index to undefined symbols
  UINT32 NumUndefinedSymbols;    ///< number of undefined symbols

  //
  // For the for the dynamic binding process to find which module a symbol
  // is defined in the table of contents is used (analogous to the ranlib
  // structure in an archive) which maps defined external symbols to modules
  // they are defined in.  This exists only in a dynamically linked shared
  // library file.  For executable and object modules the defined external
  // symbols are sorted by name and is use as the table of contents.
  //

  ///
  /// file offset to table of contents
  ///
  UINT32 TableOfContentsFileOffset;
  ///
  /// number of entries in table of contents
  ///
  UINT32 TableOfContentsNumEntries;

  //
  // To support dynamic binding of "modules" (whole object files) the symbol
  // table must reflect the modules that the file was created from.  This is
  // done by having a module table that has indexes and counts into the merged
  // tables for each module.  The module structure that these two entries
  // refer to is described below.  This exists only in a dynamically linked
  // shared library file.  For executable and object modules the file only
  // contains one module so everything in the file belongs to the module.
  //

  UINT32 ModuleTableFileOffset;  ///< file offset to module table
  UINT32 ModuleTableNumEntries;  ///< number of module table entries

  //
  // To support dynamic module binding the module structure for each module
  // indicates the external references (defined and undefined) each module
  // makes.  For each module there is an offset and a count into the
  // reference symbol table for the symbols that the module references.
  // This exists only in a dynamically linked shared library file.  For
  // executable and object modules the defined external symbols and the
  // undefined external symbols indicates the external references.
  //

  ///
  /// offset to referenced symbol table
  ///
  UINT32 ReferencedSymbolTableFileOffset;
  ///
  /// number of referenced symbol table entries
  ///
  UINT32 ReferencedSymbolTableNumEntries;

  //
  // The sections that contain "symbol pointers" and "routine stubs" have
  // indexes and (implied counts based on the size of the section and fixed
  // size of the entry) into the "indirect symbol" table for each pointer
  // and stub.  For every section of these two types the index into the
  // indirect symbol table is stored in the section header in the field
  // reserved1.  An indirect symbol table entry is simply a 32bit index into
  // the symbol table to the symbol that the pointer or stub is referring to.
  // The indirect symbol table is ordered to match the entries in the section.
  //

  ///
  /// file offset to the indirect symbol table
  ///
  UINT32 IndirectSymbolsOffset;
  ///
  /// number of indirect symbol table entries
  ///
  UINT32 NumIndirectSymbols;

  //
  // To support relocating an individual module in a library file quickly the
  // external relocation entries for each module in the library need to be
  // accessed efficiently.  Since the relocation entries can't be accessed
  // through the section headers for a library file they are separated into
  // groups of local and external entries further grouped by module.  In this
  // case the presents of this load command who's extreloff, nextrel,
  // locreloff and nlocrel fields are non-zero indicates that the relocation
  // entries of non-merged sections are not referenced through the section
  // structures (and the reloff and nreloc fields in the section headers are
  // set to zero).
  //
  // Since the relocation entries are not accessed through the section headers
  // this requires the r_address field to be something other than a section
  // offset to identify the item to be relocated.  In this case r_address is
  // set to the offset from the vmaddr of the first LC_SEGMENT command.
  // For MH_SPLIT_SEGS images r_address is set to the the offset from the
  // vmaddr of the first read-write LC_SEGMENT command.
  //
  // The relocation entries are grouped by module and the module table
  // entries have indexes and counts into them for the group of external
  // relocation entries for that the module.
  //
  // For sections that are merged across modules there must not be any
  // remaining external relocation entries for them (for merged sections
  // remaining relocation entries must be local).
  //

  ///
  /// offset to external relocation entries
  ///
  UINT32 ExternalRelocationsOffset;
  ///
  /// number of external relocation entries
  ///
  UINT32 NumExternalRelocations;

  //
  // All the local relocation entries are grouped together (they are not
  // grouped by their module since they are only used if the object is moved
  // from it staticly link edited address).
  //

  UINT32 LocalRelocationsOffset;    ///< offset to local relocation entries
  UINT32 NumOfLocalRelocations;  ///< number of local relocation entries
} MACH_DYSYMTAB_COMMAND;

//
// An indirect symbol table entry is simply a 32bit index into the symbol table
// to the symbol that the pointer or stub is refering to.  Unless it is for a
// non-lazy symbol pointer section for a defined symbol which strip(1) as
// removed.  In which case it has the value INDIRECT_SYMBOL_LOCAL.  If the
// symbol was also absolute INDIRECT_SYMBOL_ABS is or'ed with that.
//
#define MACH_INDIRECT_SYMBOL_LOCAL  0x80000000U
#define MACH_INDIRECT_SYMBOL_ABS    0x40000000U

///
/// The entries in the two-level namespace lookup hints table are twolevel_hint
/// structs.  These provide hints to the dynamic link editor where to start
/// looking for an undefined symbol in a two-level namespace image.  The
/// isub_image field is an index into the sub-images (sub-frameworks and
/// sub-umbrellas list) that made up the two-level image that the undefined
/// symbol was found in when it was built by the static link editor.  If
/// isub-image is 0 the the symbol is expected to be defined in library and not
/// in the sub-images.  If isub-image is non-zero it is an index into the array
/// of sub-images for the umbrella with the first index in the sub-images being
/// 1. The array of sub-images is the ordered list of sub-images of the
/// umbrella that would be searched for a symbol that has the umbrella recorded
/// as its primary library.  The table of contents index is an index into the
/// library's table of contents.  This is used as the starting point of the
/// binary search or a directed linear search.
///
typedef struct {
  UINT32 SubImagesIndex       : 8;   ///< index into the sub images
  UINT32 TableOfContentsIndex : 24;  ///< index into the table of contents
} TWOLEVEL_HINT;

///
/// The twolevel_hints_command contains the offset and number of hints in the
/// two-level namespace lookup hints table.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  UINT32            Offset;         ///< offset to the hint table
  UINT32            NumHints;  ///< number of hints in the hint table
  TWOLEVEL_HINT     Hints[];
} MACH_TWO_LEVEL_HINTS_COMMAND;

///
/// The prebind_cksum_command contains the value of the original check sum for
/// prebound files or zero.  When a prebound file is first created or modified
/// for other than updating its prebinding information the value of the check
/// sum is set to zero.  When the file has it prebinding re-done and if the
/// value of the check sum is zero the original check sum is calculated and
/// stored in cksum field of this load command in the output file.  If when the
/// prebinding is re-done and the cksum field is non-zero it is left unchanged
/// from the input file.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  UINT32            Checksum;  ///< the check sum or zero
} MACH_PREBIND_CHECKSUM_COMMAND;

///
/// The uuid load command contains a single 128-bit unique random number that
/// identifies an object produced by the static link editor.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  UINT8 Uuid[16];  ///< the 128-bit uuid
} MACH_UUID_COMMAND;

///
/// The rpath_command contains a path which at runtime should be added to
/// the current run path used to find @rpath prefixed dylibs.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  MACH_LOAD_COMMAND_STRING Path;  ///< path to add to run path
} MACH_RUN_PATH_COMMAND;

///
/// The linkedit_data_command contains the offsets and sizes of a blob
/// of data in the __LINKEDIT segment.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  UINT32            DataOffset;  ///< file offset of data in __LINKEDIT segment
  UINT32            DataSize;    ///< file size of data in __LINKEDIT segment
} MACH_LINKEDIT_DATA_COMMAND;

///
/// The encryption_info_command contains the file offset and size of an
/// of an encrypted segment.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  ///
  /// file offset of encrypted range
  ///
  UINT32            CryptOffset;
  ///
  /// file size of encrypted range
  ///
  UINT32            CryptSize;
  ///
  /// which enryption system, 0 means not-encrypted yet
  ///
  UINT32            CryptId;
} MACH_ENCRYPTION_INFO_COMMAND;

///
/// The encryption_info_command_64 contains the file offset and size of an
/// of an encrypted segment (for use in x86_64 targets).
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  ///
  /// file offset of encrypted range
  ///
  UINT32            CryptOffset;
  ///
  /// file size of encrypted range
  ///
  UINT32            CryptSize;
  ///
  /// which enryption system, 0 means not-encrypted yet
  ///
  UINT32            CryptId;
  ///
  /// padding to make this struct's size a multiple of 8 bytes
  ///
  UINT32            Pad;
} MACH_ENCRYPTION_INFO_COMMAND_64;

///
/// The version_min_command contains the min OS version on which this
/// binary was built to run.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  UINT32            Version;     ///< X.Y.Z is encoded in nibbles xxxx.yy.zz
  UINT32            SdkVersion;  ///< X.Y.Z is encoded in nibbles xxxx.yy.zz
} MACH_VERSION_MIN_COMMAND;

///
/// Encoded tools
///
typedef struct {
  UINT32  Tool;     ///< enum for the tool
  UINT32  Version;  ///< version number of the tool
} MACH_BUILD_VERSION_TOOL;

///
/// The build_version_command contains the min OS version on which this
/// binary was built to run for its platform.  The list of known platforms and
/// tool values following it.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  UINT32                  Platform;    ///< platform
  UINT32                  MinOs;       ///< X.Y.Z is encoded in nibbles xxxx.yy.zz
  UINT32                  SdkVersion;  ///< X.Y.Z is encoded in nibbles xxxx.yy.zz
  UINT32                  NumTools;    ///< number of tool entries following this
  MACH_BUILD_VERSION_TOOL Tools[];
} MACH_BUILD_VERSION_COMMAND;

///
/// Known values for the platform field above.
///
#define MACH_PLATFORM_MACOS              1U
#define MACH_PLATFORM_IOS                2U
#define MACH_PLATFORM_TVOS               3U
#define MACH_PLATFORM_WATCHOS            4U
#define MACH_PLATFORM_BRIDGEOS           5U
#define MACH_PLATFORM_MACCATALYST        6U
#define MACH_PLATFORM_IOSSIMULATOR       7U
#define MACH_PLATFORM_TVOSSIMULATOR      8U
#define MACH_PLATFORM_WATCHOSSIMULATOR   9U
#define MACH_PLATFORM_DRIVERKIT          10U

///
/// Known values for the tool field above.
///
#define MACH_TOOL_CLANG                  1U
#define MACH_TOOL_SWIFT                  2U
#define MACH_TOOL_LD                     3U

///
/// The dyld_info_command contains the file offsets and sizes of
/// the new compressed form of the information dyld needs to
/// load the image.  This information is used by dyld on Mac OS X
/// 10.6 and later.  All information pointed to by this command
/// is encoded using byte streams, so no endian swapping is needed
/// to interpret it.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  //
  // Dyld rebases an image whenever dyld loads it at an address different
  // from its preferred address.  The rebase information is a stream
  // of byte sized opcodes whose symbolic names start with REBASE_OPCODE_.
  // Conceptually the rebase information is a table of tuples:
  //    <seg-index, seg-offset, type>
  // The opcodes are a compressed way to encode the table by only
  // encoding when a column changes.  In addition simple patterns
  // like "every n'th offset for m times" can be encoded in a few
  // bytes.
  //
  UINT32 RebaseOffset;  ///< file offset to rebase info
  UINT32 RebaseSize;    ///< size of rebase info
  //
  // Dyld binds an image during the loading process, if the image
  // requires any pointers to be initialized to symbols in other images.
  // The bind information is a stream of byte sized
  // opcodes whose symbolic names start with BIND_OPCODE_.
  // Conceptually the bind information is a table of tuples:
  //    <seg-index, seg-offset, type, symbol-library-ordinal, symbol-name,
  //     addend>
  // The opcodes are a compressed way to encode the table by only
  // encoding when a column changes.  In addition simple patterns
  // like for runs of pointers initialzed to the same value can be
  // encoded in a few bytes.
  //
  UINT32 BindingInfoOffset;  ///< file offset to binding info
  UINT32 BindingInfoSize;    ///< size of binding info
  //
  // Some C++ programs require dyld to unique symbols so that all
  // images in the process use the same copy of some code/data.
  // This step is done after binding. The content of the weak_bind
  // info is an opcode stream like the bind_info.  But it is sorted
  // alphabetically by symbol name.  This enable dyld to walk
  // all images with weak binding information in order and look
  // for collisions.  If there are no collisions, dyld does
  // no updating.  That means that some fixups are also encoded
  // in the bind_info.  For instance, all calls to "operator new"
  // are first bound to libstdc++.dylib using the information
  // in bind_info.  Then if some image overrides operator new
  // that is detected when the weak_bind information is processed
  // and the call to operator new is then rebound.
  //
  UINT32 WeakBindingInfoOffset;  ///< file offset to weak binding info
  UINT32 WeakBindingInfoSize;    ///< size of weak binding info
  //
  // Some uses of external symbols do not need to be bound immediately.
  // Instead they can be lazily bound on first use.  The lazy_bind
  // are contains a stream of BIND opcodes to bind all lazy symbols.
  // Normal use is that dyld ignores the lazy_bind section when
  // loading an image.  Instead the static linker arranged for the
  // lazy pointer to initially point to a helper function which
  // pushes the offset into the lazy_bind area for the symbol
  // needing to be bound, then jumps to dyld which simply adds
  // the offset to lazy_bind_off to get the information on what
  // to bind.
  //
  UINT32 LazyBindingInfoOffset;  ///< file offset to lazy binding info
  UINT32 LazyBindingInfoSize;    ///< size of lazy binding infs
  //
  // The symbols exported by a dylib are encoded in a trie.  This
  // is a compact representation that factors out common prefixes.
  // It also reduces LINKEDIT pages in RAM because it encodes all
  // information (name, address, flags) in one small, contiguous range.
  // The export area is a stream of nodes.  The first node sequentially
  // is the start node for the trie.
  //
  // Nodes for a symbol start with a uleb128 that is the length of
  // the exported symbol information for the string so far.
  // If there is no exported symbol, the node starts with a zero byte.
  // If there is exported info, it follows the length.
  //
  // First is a uleb128 containing flags. Normally, it is followed by
  // a uleb128 encoded offset which is location of the content named
  // by the symbol from the mach_header for the image.  If the flags
  // is EXPORT_SYMBOL_FLAGS_REEXPORT, then following the flags is
  // a uleb128 encoded library ordinal, then a zero terminated
  // UTF8 string.  If the string is zero length, then the symbol
  // is re-export from the specified dylib with the same name.
  // If the flags is EXPORT_SYMBOL_FLAGS_STUB_AND_RESOLVER, then following
  // the flags is two uleb128s: the stub offset and the resolver offset.
  // The stub is used by non-lazy pointers.  The resolver is used
  // by lazy pointers and must be called to get the actual address to use.
  //
  // After the optional exported symbol information is a byte of
  // how many edges (0-255) that this node has leaving it,
  // followed by each edge.
  // Each edge is a zero terminated UTF8 of the addition chars
  // in the symbol, followed by a uleb128 offset for the node that
  // edge points to.
  //
  UINT32 ExportSymbolsOffset;  ///< file offset to lazy binding info
  UINT32 ExportSymbolsSize;    ///< size of lazy binding infs
} MACH_DYLD_INFO_COMMAND;

///
/// The linker_option_command contains linker options embedded in object files.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  UINT32            NumStrings;  ///< number of strings
  ///
  /// concatenation of zero terminated UTF8 strings.  Zero filled at end to
  /// align.
  ///
  CHAR8             Strings[];
} MACH_LINKER_OPTION_COMMAND;

///
/// The symseg_command contains the offset and size of the GNU style
/// symbol table information as described in the header file <symseg.h>.
/// The symbol roots of the symbol segments must also be aligned properly
/// in the file.  So the requirement of keeping the offsets aligned to a
/// multiple of a 4 bytes translates to the length field of the symbol
/// roots also being a multiple of a long.  Also the padding must again be
/// zeroed. (THIS IS OBSOLETE and no longer supported).
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  UINT32            Offset;  ///< symbol segment offset
  UINT32            Size;    ///< symbol segment size in bytes
} MACH_SYMBOL_SEGMENT_COMMAND;

///
/// The ident_command contains a free format string table following the
/// ident_command structure.  The strings are null terminated and the size of
/// the command is padded out with zero bytes to a multiple of 4 bytes/
/// (THIS IS OBSOLETE and no longer supported).
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  CHAR8             Strings[];
} MACH_IDENTIFICATION_COMMAND;

///
/// The fvmfile_command contains a reference to a file to be loaded at the
/// specified virtual address.  (Presently, this command is reserved for
/// internal use.  The kernel ignores this command when loading a program into
/// memory).
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  MACH_LOAD_COMMAND_STRING Name;           ///< files pathname
  UINT32                   HeaderAddress;  ///< files virtual address
} MACH_FIXED_VM_FILE_COMMAND;

///
/// LC_FILESET_ENTRY commands describe constituent Mach-O files that are part
/// of a fileset. In one implementation, entries are dylibs with individual
/// mach headers and repositionable text and data segments. Each entry is
/// further described by its own mach header.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  UINT64                    VirtualAddress;  ///< memory address of the entry
  UINT64                    FileOffset;      ///< file offset of the entry
  MACH_LOAD_COMMAND_STRING  EntryId;         ///< contained entry id
  UINT32                    Reserved;        ///< reserved
  CHAR8                     Payload[];       ///< file information starting at entry id
} MACH_FILESET_ENTRY_COMMAND;

///
/// DYLD_CHAINED_PTR_64_KERNEL_CACHE, DYLD_CHAINED_PTR_X86_64_KERNEL_CACHE
///
typedef struct {
  UINT64 Target     : 30,  ///< basePointers[cacheLevel] + target
         CacheLevel :  2,  ///< what level of cache to bind to (indexes a mach_header array)
         Diversity  : 16,
         AddrDiv    :  1,
         Key        :  2,
         Next       : 12,  ///< 1 or 4-byte stide
         IsAuth     :  1;  ///< 0 -> not authenticated.  1 -> authenticated
} MACH_DYLD_CHAINED_PTR_64_KERNEL_CACHE_REBASE;

// header of the LC_DYLD_CHAINED_FIXUPS payload
typedef struct {
  UINT32 FixupsVersion;  ///< 0
  UINT32 StartsOffset;   ///< offset of dyld_chained_starts_in_image in chain_data
  UINT32 ImportsOffset;  ///< offset of imports table in chain_data
  UINT32 SymbolsOffset;  ///< offset of symbol strings in chain_data
  UINT32 ImportsCount;   ///< number of imported symbol names
  UINT32 ImportsFormat;  ///< DYLD_CHAINED_IMPORT*
  UINT32 SymbolsFormat;  ///< 0 => uncompressed, 1 => zlib compressed
} MACHO_DYLD_CHAINED_FIXUPS_HEADER;

///
/// This struct is embedded in LC_DYLD_CHAINED_FIXUPS payload
///
typedef struct {
  UINT32 NumSegments;
  UINT32 SegInfoOffset[];  ///< each entry is offset into this struct for that
                           ///< segment followed by pool of
                           ///< dyld_chain_starts_in_segment data
} MACH_DYLD_CHAINED_STARTS_IN_IMAGE;

typedef struct {
  UINT32 Size;             ///< size of this (amount kernel needs to copy)
  UINT16 PageSize;         ///< 0x1000 or 0x4000
  UINT16 PointerFormat;    ///< DYLD_CHAINED_PTR_*
  UINT64 SegmentOffset;    ///< offset in memory to start of segment
  UINT32 MaxValidPointer;  ///< for 32-bit OS, any value beyond this is not a pointer
  UINT16 PageCount;        ///< how many pages are in array
  UINT16 PageStart[];      ///< each entry is offset in each page of first element in chain
                           ///< or DYLD_CHAINED_PTR_START_NONE if no fixups on page
//UINT16 ChainStarts[];    ///< some 32-bit formats may require multiple starts per page.
                           ///< for those, if high bit is set in page_starts[], then it
                           ///< is index into chain_starts[] which is a list of starts
                           ///< the last of which has the high bit set
} MACH_DYLD_CHAINED_STARTS_IN_SEGMENT;

///
/// Values for MACH_DYLD_CHAINED_STARTS_IN_SEGMENT.PointerFormat.
///
enum {
  MACH_DYLD_CHAINED_PTR_ARM64E              =  1,  ///< stride 8, unauth target is vmaddr
  MACH_DYLD_CHAINED_PTR_64                  =  2,  ///< target is vmaddr
  MACH_DYLD_CHAINED_PTR_32                  =  3,
  MACH_DYLD_CHAINED_PTR_32_CACHE            =  4,
  MACH_DYLD_CHAINED_PTR_32_FIRMWARE         =  5,
  MACH_DYLD_CHAINED_PTR_64_OFFSET           =  6,  ///< target is vm offset
  MACH_DYLD_CHAINED_PTR_ARM64E_OFFSET       =  7,  ///< old name
  MACH_DYLD_CHAINED_PTR_ARM64E_KERNEL       =  7,  ///< stride 4, unauth target is vm offset
  MACH_DYLD_CHAINED_PTR_64_KERNEL_CACHE     =  8,
  MACH_DYLD_CHAINED_PTR_ARM64E_USERLAND     =  9,  ///< stride 8, unauth target is vm offset
  MACH_DYLD_CHAINED_PTR_ARM64E_FIRMWARE     = 10,  ///< stride 4, unauth target is vmaddr
  MACH_DYLD_CHAINED_PTR_X86_64_KERNEL_CACHE = 11,  ///< stride 1, x86_64 kernel caches
  MACH_DYLD_CHAINED_PTR_ARM64E_USERLAND24   = 12   ///< stride 8, unauth target is vm offset, 24-bit bind
};

enum {
  MACH_DYLD_CHAINED_PTR_START_NONE  = 0xFFFF,  ///< used in page_start[] to denote a page with no fixups
  MACH_DYLD_CHAINED_PTR_START_MULTI = 0x8000,  ///< used in page_start[] to denote a page which has multiple starts
  MACH_DYLD_CHAINED_PTR_START_LAST  = 0x8000,  ///< used in chain_starts[] to denote last start in list for page
};

///
/// The entry_point_command is a replacement for thread_command.
/// It is used for main executables to specify the location (file offset)
/// of main().  If -stack_size was used at link time, the stacksize
/// field will contain the stack size need for the main thread.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  UINT64            EntryOffset;  ///< file (__TEXT) offset of main()
  UINT64            StackSize;    ///< if not zero, initial stack size
} MACH_ENTRY_POINT_COMMAND;

#define MACH_SOURCE_VERSION_A_MASK  0xFFFFFF0000000000U
#define MACH_SOURCE_VERSION_B_MASK  0x000000FFC0000000U
#define MACH_SOURCE_VERSION_V_MASK  0x000000003FF00000U
#define MACH_SOURCE_VERSION_D_MASK  0x00000000000FFC00U
#define MACH_SOURCE_VERSION_E_MASK  0x00000000000003FFU

#define MACH_SOURCE_VERSION_A(Version)  (((Version) & MACH_SOURCE_VERSION_A_MASK) >> 40U)
#define MACH_SOURCE_VERSION_B(Version)  (((Version) & MACH_SOURCE_VERSION_B_MASK) >> 30U)
#define MACH_SOURCE_VERSION_C(Version)  (((Version) & MACH_SOURCE_VERSION_V_MASK) >> 20U)
#define MACH_SOURCE_VERSION_D(Version)  (((Version) & MACH_SOURCE_VERSION_D_MASK) >> 10U)
#define MACH_SOURCE_VERSION_E(Version)  ((Version)  & MACH_SOURCE_VERSION_E_MASK)

///
/// The source_version_command is an optional load command containing
/// the version of the sources used to build the binary.
///
typedef struct {
  MACH_LOAD_COMMAND_HDR_
  UINT64            Version;  ///< A.B.C.D.E as a24.b10.c10.d10.e10
} MACH_SOURCE_VERSION_COMMAND;

typedef union {
  MACH_LOAD_COMMAND                     *Hdr;
  MACH_SEGMENT_COMMAND                  *Segment;
  MACH_SEGMENT_COMMAND_64               *Segment64;
  MACH_FIXED_VM_LIB_COMMAND             *VmLib;
  MACH_DYLIB_COMMAND                    *Dylib;
  MACH_SUB_FRAMEWORK_COMMAND            *SubFramework;
  MACH_SUB_CLIENT_COMMAND               *SubClient;
  MACH_SUB_UMBRELLA_COMMAND             *SubUmbrella;
  MACH_SUB_LIBRARY_COMMAND              *SubLibrary;
  MACH_PREBOUND_DYLIB_COMMAND           *PreboundDyLib;
  MACH_DYLINKER_COMMAND                 *Dylinker;
  MACH_THREAD_COMMAND                   *Thread;
  MACH_ROUTINES_COMMAND                 *Routines;
  MACH_ROUTINES_COMMAND_64              *Routines64;
  MACH_SYMTAB_COMMAND                   *Symtab;
  MACH_DYSYMTAB_COMMAND                 *Dysymtab;
  MACH_TWO_LEVEL_HINTS_COMMAND          *TwoLevelHints;
  MACH_PREBIND_CHECKSUM_COMMAND         *PrebindChecksum;
  MACH_UUID_COMMAND                     *Uuid;
  MACH_RUN_PATH_COMMAND                 *RunPath;
  MACH_LINKEDIT_DATA_COMMAND            *LinkeditData;
  MACH_ENCRYPTION_INFO_COMMAND          *EncryptionInfo;
  MACH_ENCRYPTION_INFO_COMMAND_64       *EncryptionInfo64;
  MACH_VERSION_MIN_COMMAND              *VersionMin;
  MACH_BUILD_VERSION_COMMAND            *BuildVersion;
  MACH_DYLD_INFO_COMMAND                *DyldInfo;
  MACH_LINKER_OPTION_COMMAND            *LinkerOption;
  MACH_SYMBOL_SEGMENT_COMMAND           *SymbolSegment;
  MACH_IDENTIFICATION_COMMAND           *Identification;
  MACH_FIXED_VM_FILE_COMMAND            *FixedVmFile;
  MACH_LINKEDIT_DATA_COMMAND            *DyldExportsTrie;
  MACH_LINKEDIT_DATA_COMMAND            *DyldChainedFixups;
  MACH_FILESET_ENTRY_COMMAND            *FilesetEntry;
  VOID                                  *Pointer;
  UINTN                                 Address;
} MACH_LOAD_COMMAND_PTR;

#undef MACH_LOAD_COMMAND_HDR_

///
/// The layout of the file depends on the filetype.  For all but the MH_OBJECT
/// file type the segments are padded out and aligned on a segment alignment
/// boundary for efficient demand pageing.  The MH_EXECUTE, MH_FVMLIB,
/// MH_DYLIB, MH_DYLINKER and MH_BUNDLE file types also have the headers
/// included as part of their first segment.
///
/// The file type MH_OBJECT is a compact format intended as output of the
/// assembler and input (and possibly output) of the link editor (the .o
/// format).  All sections are in one unnamed segment with no segment padding.
/// This format is used as an executable format when the file is so small the
/// segment padding greatly increases its size.
///
/// The file type MH_PRELOAD is an executable format intended for things that
/// are not executed under the kernel (proms, stand alones, kernels, etc).  The
/// format can be executed under the kernel but may demand paged it and not
/// preload it before execution.
///
/// A core file is in MH_CORE format and can be any in an arbritray legal
/// Mach-O file.
///
/// Constants for the filetype field of the mach_header
///
enum {
  MachHeaderFileTypeObject            = 1,
  MachHeaderFileTypeExecute           = 2,
  MachHeaderFileTypeFixedVmLib        = 3,
  MachHeaderFileTypeCore              = 4,
  MachHeaderFileTypePreload           = 5,
  MachHeaderFileTypeDylib             = 6,
  MachHeaderFileTypeDynamicLinker     = 7,
  MachHeaderFileTypeBundle            = 8,
  MachHeaderFileTypeDynamicLinkerStub = 9,
  MachHeaderFileTypeDsym              = 10,
  MachHeaderFileTypeKextBundle        = 11,
  //
  // A file composed of other Mach-Os to be run in the same userspace sharing
  // a single linkedit. Used by 11.0 kernelcache.
  //
  MachHeaderFileTypeFileSet           = 12,
};

typedef UINT32 MACH_HEADER_FILE_TYPE;

//
// Constants for the Flags field of the MACH_HEADER struct
//
#define MACH_HEADER_FLAG_NO_UNDEFINED_REFERENCES          BIT0
#define MACH_HEADER_FLAG_INCREMENTAL_LINK                 BIT1
#define MACH_HEADER_FLAG_DYNAMIC_LINKER_LINK              BIT2
#define MACH_HEADER_FLAG_BINARY_DATA_LOAD                 BIT3
#define MACH_HEADER_FLAG_PREBOUND                         BIT4
#define MACH_HEADER_FLAG_SPLIT_SEGMENTS                   BIT5
#define MACH_HEADER_FLAG_LAZY_INITIALIZATION              BIT6
#define MACH_HEADER_FLAG_TWO_LEVEL                        BIT7
#define MACH_HEADER_FLAG_FORCE_FLAT                       BIT8
#define MACH_HEADER_FLAG_NO_MULTIPLE_DEFINITIONS          BIT9
#define MACH_HEADER_FLAG_NO_FIX_PREBINDING                BIT10
#define MACH_HEADER_FLAG_PREBINDABLE                      BIT11
#define MACH_HEADER_FLAG_ALL_MODULES_BOUND                BIT12
#define MACH_HEADER_FLAG_SUBSECTIONS_VIA_SYNBOLS          BIT13
#define MACH_HEADER_FLAG_CANONICAL                        BIT14
#define MACH_HEADER_FLAG_WEAK_DEFINES                     BIT15
#define MACH_HEADER_FLAG_BINDS_TO_WEAK                    BIT16
#define MACH_HEADER_FLAG_ALLOW_STACK_EXECUTION            BIT17
#define MACH_HEADER_FLAG_ROOT_SAFE                        BIT18
#define MACH_HEADER_FLAG_SET_UID_SAFE                     BIT19
#define MACH_HEADER_FLAG_NO_REEXPORTED_DYLIBS             BIT20
#define MACH_HEADER_FLAG_POSITION_INDEPENDENT_EXECUTABLE  BIT21
#define MACH_HEADER_FLAG_DEAD_STRIPPABLE_DYLIB            BIT22
#define MACH_HEADER_FLAG_HAS_TLV_DESCRIPTORS              BIT23
#define MACH_HEADER_FLAG_NO_HEAP_EXECUTION                BIT24
#define MACH_HEADER_FLAG_APP_EXTENSION_SAFE               BIT25
#define MACH_HEADER_FLAG_NLIST_OUTOFSYNC_WITH_DYLDINFO    BIT26
#define MACH_HEADER_FLAG_SIM_SUPPORT                      BIT27
#define MACH_HEADER_FLAG_DYLIB_IN_CACHE                   BIT31

typedef UINT32 MACH_HEADER_FLAGS;

//
// Constant for the magic field of the MACH_HEADER (32-bit architectures)
//
#define MACH_HEADER_SIGNATURE  0xFEEDFACE        ///< the mach magic number
#define MACH_HEADER_INVERT_SIGNATURE  0xCEFAEDFE ///< the mach magic number (byte swapped)

///
/// The 32-bit mach header appears at the very beginning of the object file for
/// 32-bit architectures.
///
typedef struct {
  UINT32                Signature;     ///< mach magic number identifier
  MACH_CPU_TYPE         CpuType;       ///< cpu Sectionecifier
  MACH_CPU_SUBTYPE      CpuSubtype;    ///< machine Sectionecifier
  MACH_HEADER_FILE_TYPE FileType;      ///< type of file
  UINT32                NumCommands;   ///< number of load commands
  UINT32                CommandsSize;  ///< the size of all load commands
  MACH_HEADER_FLAGS     Flags;         ///< flags
  MACH_LOAD_COMMAND     Commands[];
} MACH_HEADER;

//
// Constant for the magic field of the MACH_HEADER_64 (64-bit architectures)
//
#define MACH_HEADER_64_SIGNATURE  0xFEEDFACF        ///< the 64-bit mach magic number
#define MACH_HEADER_64_INVERT_SIGNATURE  0xCFFAEDFE ///< the 64-bit mach magic number (byte swapped)

///
/// The 64-bit mach header appears at the very beginning of object files for
/// 64-bit architectures.
///
typedef struct {
  UINT32                Signature;     ///< mach magic number identifier
  MACH_CPU_TYPE         CpuType;       ///< cpu Sectionecifier
  MACH_CPU_SUBTYPE      CpuSubtype;    ///< machine Sectionecifier
  MACH_HEADER_FILE_TYPE FileType;      ///< type of file
  UINT32                NumCommands;   ///< number of load commands
  UINT32                CommandsSize;  ///< the size of all load commands
  MACH_HEADER_FLAGS     Flags;         ///< flags
  UINT32                Reserved;      ///< reserved
  MACH_LOAD_COMMAND     Commands[];
} MACH_HEADER_64;

///
/// Allow selecting a correct header based on magic
///
typedef union {
  UINT32         Signature;
  MACH_HEADER    Header32;
  MACH_HEADER_64 Header64;
} MACH_HEADER_ANY;


//
// Symbols
//

///
/// Format of a symbol table entry of a Mach-O file for 32-bit architectures.
/// Modified from the BSD format.  The modifications from the original format
/// were changing n_other (an unused field) to n_sect and the addition of the
/// N_SECT type.  These modifications are required to support symbols in a
/// larger number of sections not just the three sections (text, data and bss)
/// in a BSD file.
///
typedef struct {
  union {
    UINT32 Address32;      ///< for use when in-core
    UINT32 StringIndex;    ///< index into the string table
  } UnifiedName;
  UINT8  Type;         ///< type flag, see below
  UINT8  Section;      ///< section number or NO_SECT
  INT16  Descriptor;   ///< see <mach-o/stab.h>
  UINT32 Value;        ///< value of this symbol (or stab offset)
} MACH_NLIST;

///
/// This is the symbol table entry structure for 64-bit architectures.
///
typedef struct {
  union {
    UINT32 StringIndex;  ///< index into the string table
  } UnifiedName;
  UINT8  Type;         ///< type flag, see below
  UINT8  Section;      ///< section number or NO_SECT
  UINT16 Descriptor;   ///< see <mach-o/stab.h>
  UINT64 Value;        ///< value of this symbol (or stab offset)
} MACH_NLIST_64;

typedef union {
  MACH_NLIST    Symbol32;
  MACH_NLIST_64 Symbol64;
} MACH_NLIST_ANY;

//
// Symbols with a index into the string table of zero (n_un.n_strx == 0) are
// defined to have a null, "", name.  Therefore all string indexes to non null
// names must not have a zero string index.  This is bit historical information
// that has never been well documented.
//

//
// The n_type field really contains four fields:
//  unsigned char N_STAB:3,
//          N_PEXT:1,
//          N_TYPE:3,
//          N_EXT:1;
// which are used via the following masks.
//
#define MACH_N_TYPE_STAB  0xE0U  ///< if any of these bits set, a symbolic
                                 ///< debugging entry
#define MACH_N_TYPE_PEXT  0x10U  ///< private external symbol bit
#define MACH_N_TYPE_TYPE  0x0EU  ///< mask for the type bit
#define MACH_N_TYPE_EXT   0x01U  ///< external symbol bit, set for external
                                 ///< symbols

//
// Only symbolic debugging entries have some of the N_STAB bits set and if any
// of these bits are set then it is a symbolic debugging entry (a stab).  In
// which case then the values of the n_type field (the entire field) are given
// in <mach-o/stab.h>
//

//
// Values for N_TYPE bits of the n_type field.
//
#define MACH_N_TYPE_UNDF  0x0U    ///< undefined, n_sect == NO_SECT
#define MACH_N_TYPE_ABS   0x2U    ///< absolute, n_sect == NO_SECT
#define MACH_N_TYPE_SECT  0xEU    ///< defined in section number n_sect
#define MACH_N_TYPE_PBUD  0xCU    ///< prebound undefined (defined in a dylib)
#define MACH_N_TYPE_INDR  0xAU    ///< indirect

//
// If the type is N_INDR then the symbol is defined to be the same as another
// symbol.  In this case the n_value field is an index into the string table
// of the other symbol's name.  When the other symbol is defined then they both
// take on the defined type and value.
//

//
// If the type is N_SECT then the n_sect field contains an ordinal of the
// section the symbol is defined in.  The sections are numbered from 1 and
// refer to sections in order they appear in the load commands for the file
// they are in.  This means the same ordinal may very well refer to different
// sections in different files.
//
// The n_value field for all symbol table entries (including N_STAB's) gets
// updated by the link editor based on the value of its n_sect field and where
// the section n_sect references gets relocated.  If the value of the n_sect
// field is NO_SECT then its n_value field is not changed by the link editor.
//
#define NO_SECT     0  ///< symbol is not in any section
#define MAX_SECT  255  ///< 1 thru 255 inclusive

//
// STAB
//

//
// Symbolic debugger symbols.  The comments give the conventional use for
//
//   .stabs "n_name", n_type, n_sect, n_desc, n_value
//
// where n_type is the defined constant and not listed in the comment.  Other
// fields not listed are zero. n_sect is the section ordinal the entry is
// refering to.
//
#define MACH_N_GSYM    0x20U  ///< global symbol: name,,NO_SECT,type,0
#define MACH_N_FNAME   0x22U  ///< procedure name (f77 kludge): name,,NO_SECT,0,0
#define MACH_N_FUN     0x24U  ///< procedure: name,,n_sect,linenumber,address
#define MACH_N_STSYM   0x26U  ///< static symbol: name,,n_sect,type,address
#define MACH_N_LCSYM   0x28U  ///< .lcomm symbol: name,,n_sect,type,address
#define MACH_N_BNSYM   0x2EU  ///< begin nsect sym: 0,,n_sect,0,address
#define MACH_N_AST     0x32U  ///< AST file path: name,,NO_SECT,0,0
#define MACH_N_OPT     0x3CU  ///< emitted with gcc2_compiled and in gcc source
#define MACH_N_RSYM    0x40U  ///< register sym: name,,NO_SECT,type,register
#define MACH_N_SLINE   0x44U  ///< src line: 0,,n_sect,linenumber,address
#define MACH_N_ENSYM   0x4EU  ///< end nsect sym: 0,,n_sect,0,address
#define MACH_N_SSYM    0x60U  ///< structure elt: name,,NO_SECT,type,struct_offset
#define MACH_N_SO      0x64U  ///< source file name: name,,n_sect,0,address
#define MACH_N_OSO     0x66U  ///< object file name: name,,0,0,st_mtime
#define MACH_N_LSYM    0x80U  ///< local sym: name,,NO_SECT,type,offset
#define MACH_N_BINCL   0x82U  ///< include file beginning: name,,NO_SECT,0,sum
#define MACH_N_SOL     0x84U  ///< #included file name: name,,n_sect,0,address
#define MACH_N_PARAMS  0x86U  ///< compiler parameters: name,,NO_SECT,0,0
#define MACH_N_VERSION 0x88U  ///< compiler version: name,,NO_SECT,0,0
#define MACH_N_OLEVEL  0x8AU  ///< compiler -O level: name,,NO_SECT,0,0
#define MACH_N_PSYM    0xA0U  ///< parameter: name,,NO_SECT,type,offset
#define MACH_N_EINCL   0xA2U  ///< include file end: name,,NO_SECT,0,0
#define MACH_N_ENTRY   0xA4U  ///< alternate entry: name,,n_sect,linenumber,address
#define MACH_N_LBRAC   0xC0U  ///< left bracket: 0,,NO_SECT,nesting level,address
#define MACH_N_EXCL    0xC2U  ///< deleted include file: name,,NO_SECT,0,sum
#define MACH_N_RBRAC   0xE0U  ///< right bracket: 0,,NO_SECT,nesting level,address
#define MACH_N_BCOMM   0xE2U  ///< begin common: name,,NO_SECT,0,0
#define MACH_N_ECOMM   0xE4U  ///< end common: name,,n_sect,0,0
#define MACH_N_ECOML   0xE8U  ///< end common (local name): 0,,n_sect,0,address
#define MACH_N_LENG    0xFEU  ///< second stab entry with length information

//
// The bit 0x0020 of the n_desc field is used for two non-overlapping purposes
// and has two different symbolic names, N_NO_DEAD_STRIP and N_DESC_DISCARDED.
//

///
/// The N_NO_DEAD_STRIP bit of the n_desc field only ever appears in a
/// relocatable .o file (MH_OBJECT filetype). And is used to indicate to the
/// static link editor it is never to dead strip the symbol.
///
#define MACH_N_NO_DEAD_STRIP 0x0020U ///< symbol is not to be dead stripped
///
/// The N_DESC_DISCARDED bit of the n_desc field never appears in linked image.
/// But is used in very rare cases by the dynamic link editor to mark an in
/// memory symbol as discared and longer used for linking.
///
#define MACH_N_DESC_DISCARDED 0x0020U
///
/// The N_WEAK_REF bit of the n_desc field indicates to the dynamic linker that
/// the undefined symbol is allowed to be missing and is to have the address of
/// zero when missing.
///
#define MACH_N_WEAK_REF  0x0040U
///
/// The N_WEAK_DEF bit of the n_desc field indicates to the static and dynamic
/// linkers that the symbol definition is weak, allowing a non-weak symbol to
/// also be used which causes the weak definition to be discared.  Currently
/// this is only supported for symbols in coalesed sections.
///
#define MACH_N_WEAK_DEF  0x0080U
///
/// The N_REF_TO_WEAK bit of the n_desc field indicates to the dynamic linker
/// that the undefined symbol should be resolved using flat namespace
/// searching.
///
#define MACH_N_REF_TO_WEAK  0x0080U
///
/// The N_ARM_THUMB_DEF bit of the n_desc field indicates that the symbol is
/// a definition of a Thumb function.
///
#define MACH_N_ARM_THUMB_DEF  0x0008U
///
/// The N_SYMBOL_RESOLVER bit of the n_desc field indicates that the
/// that the function is actually a resolver function and should
/// be called to get the address of the real function to use.
/// This bit is only available in .o files (MH_OBJECT filetype)
///
#define MACH_N_SYMBOL_RESOLVER  0x0100U
///
/// The N_ALT_ENTRY bit of the n_desc field indicates that the
/// symbol is pinned to the previous content.
///
#define MACH_N_ALT_ENTRY 0x0200U
///
/// for the berkeley pascal compiler, pc(1):
/// global pascal symbol: name,,NO_SECT,subtype,line
///
#define MACH_N_PC  0x30U

//
// Relocations
//

///
/// Format of a relocation entry of a Mach-O file.  Modified from the 4.3BSD
/// format.  The modifications from the original format were changing the value
/// of the r_symbolnum field for "local" (r_extern == 0) relocation entries.
/// This modification is required to support symbols in an arbitrary number of
/// sections not just the three sections (text, data and bss) in a 4.3BSD file.
/// Also the last 4 bits have had the r_type tag added to them.
///
typedef struct {
   INT32  Address;            ///< offset in the section to what is being
                              ///< relocated
   UINT32 SymbolNumber : 24;  ///< symbol index if r_extern == 1 or section
                              ///< ordinal if r_extern == 0
   UINT32 PcRelative   : 1;   ///< was relocated pc relative already
   UINT32 Size         : 2;   ///< 0=byte, 1=word, 2=long, 3=quad
   UINT32 Extern       : 1;   ///< does not include value of sym referenced
   UINT32 Type         : 4;   ///< if not 0, machine specific relocation type
} MACH_RELOCATION_INFO;

#define MACH_RELOC_ABSOLUTE  0U    ///< absolute relocation type for Mach-O files

//
// The r_address is not really the address as its name indicates but an
// offset.  In 4.3BSD a.out objects this offset is from the start of the
// "segment" for which relocation entry is for (text or data).  For Mach-O
// object files it is also an offset but from the start of the "section" for
// which the relocation entry is for.  See comments in <mach-o/loader.h> about
// the r_address field in images for used with the dynamic linker.
//
// In 4.3BSD a.out objects if r_extern is zero then r_symbolnum is an ordinal
// for the segment the symbol being relocated is in.  These ordinals are the
// symbol types N_TEXT, N_DATA, N_BSS or N_ABS.  In Mach-O object files these
// ordinals refer to the sections in the object file in the order their section
// structures appear in the headers of the object file they are in.  The first
// section has the ordinal 1, the second 2, and so on.  This means that the
// same ordinal in two different object files could refer to two different
// sections.  And further could have still different ordinals when combined
// by the link-editor.  The value MACH_RELOC_ABSOLUTE is used for relocation entries for
// absolute symbols which need no further relocation.
//

//
// For RISC machines some of the references are split across two instructions
// and the instruction does not contain the complete value of the reference.
// In these cases a second, or paired relocation entry, follows each of these
// relocation entries, using a PAIR r_type, which contains the other part of
// the reference not contained in the instruction.  This other part is stored
// in the pair's r_address field.  The exact number of bits of the other part
// of the reference store in the r_address field is dependent on the particular
// relocation type for the particular architecture.
//

///
/// mask to be applied to the r_address field of a relocation_info structure
/// to tell that is is really a scattered_relocation_info stucture
///
/// To make scattered loading by the link editor work correctly "local"
/// relocation entries can't be used when the item to be relocated is the value
/// of a symbol plus an offset (where the resulting expresion is outside the
/// block the link editor is moving, a blocks are divided at symbol addresses).
/// In this case. where the item is a symbol value plus offset, the link editor
/// needs to know more than just the section the symbol was defined.  What is
/// needed is the actual value of the symbol without the offset so it can do
/// the relocation correctly based on where the value of the symbol got
/// relocated to not the value of the expression (with the offset added to the
/// symbol value).  So for the NeXT 2.0 release no "local" relocation entries
/// are ever used when there is a non-zero offset added to a symbol.
/// The "external" and "local"  relocation entries remain unchanged.
///
/// The implemention is quite messy given the compatibility with the existing
/// relocation entry format.  The ASSUMPTION is that a section will never be
/// bigger than 2**24 - 1 (0x00ffffff or 16,777,215) bytes.  This assumption
/// allows the r_address (which is really an offset) to fit in 24 bits and high
/// bit of the r_address field in the relocation_info structure to indicate
/// it is really a scattered_relocation_info structure.  Since these are only
/// used in places where "local" relocation entries are used and not where
/// "external" relocation entries are used the r_extern field has been removed.
///
/// For scattered loading to work on a RISC machine where some of the
/// references  are split across two instructions the link editor needs to be
/// assured that each reference has a unique 32 bit reference (that more than
/// one reference is NOT sharing the same high 16 bits for example) so it move
/// each referenced item independent of each other.  Some compilers guarantees
/// this but the compilers don't so scattered loading can be done on those that
/// do guarantee this.
///
#define MACH_RELOC_SCATTERED  0x80000000U

typedef struct {
#if defined(MACH_LITTLE_ENDIAN)
  UINT32 Address    : 24;  ///< offset in the section to what is being
                           ///< relocated
  UINT32 Type       : 4;   ///< if not 0, machine specific relocation type
  UINT32 Size       : 2;   ///< 0=byte, 1=word, 2=long, 3=quad
  UINT32 PcRelative : 1;   ///< was relocated pc relative already
  UINT32 Scattered  : 1;   ///< 1=scattered, 0=non-scattered (see above)
  INT32  Value;            ///< the value the item to be relocated is refering
                           ///< to (without any offset added)
#elif defined(MACH_BIG_ENDIAN)
  UINT32 Scattered  : 1;  ///< 1=scattered, 0=non-scattered (see above)
  UINT32 PcRelative : 1;   ///< was relocated pc relative already
  UINT32 Size       : 2;   ///< 0=byte, 1=word, 2=long, 3=quad
  UINT32 Type       : 4;   ///< if not 0, machine specific relocation type
  UINT32 Address    : 24;  ///< offset in the section to what is being
                           ///< relocated
  INT32  Value;            ///< the value the item to be relocated is refering
                           ///< to (without any offset added)
#else
///
/// The reason for the ifdef's of __BIG_ENDIAN__ and __LITTLE_ENDIAN__ are that
/// when stattered relocation entries were added the mistake of using a mask
/// against a structure that is made up of bit fields was used.  To make this
/// design work this structure must be laid out in memory the same way so the
/// mask can be applied can check the same bit each time (r_scattered).
///
#error "No endianness information was provided!"
#endif
} MACH_SCATTERED_RELOCATION_INFO;

///
/// Relocation types used in a generic implementation.  Relocation entries for
/// normal things use the generic relocation as discribed above and their
/// r_type is GENERIC_RELOC_VANILLA (a value of zero).
///
/// Another type of generic relocation, GENERIC_RELOC_SECTDIFF, is to support
/// the difference of two symbols defined in different sections.  That is the
/// expression "symbol1 - symbol2 + constant" is a relocatable expression when
/// both symbols are defined in some section.  For this type of relocation the
/// both relocations entries are scattered relocation entries.  The value of
/// symbol1 is stored in the first relocation entry's r_value field and the
/// value of symbol2 is stored in the pair's r_value field.
///
/// A special case for a prebound lazy pointer is needed to beable to set the
/// value of the lazy pointer back to its non-prebound state.  This is done
/// using the GENERIC_RELOC_PB_LA_PTR r_type.  This is a scattered relocation
/// entry where the r_value feild is the value of the lazy pointer not
/// prebound.
///
enum {
 MachGenericRelocVanilla,              ///< generic relocation as discribed
                                       ///< above
 MachGenericRelocPair,                 ///< Only follows a
                                       ///< MachGenericRelocLocalSectDiff
 MachGenericRelocSectDiff,
 MachGenericRelocPreboundLazyPointer,  ///< prebound lazy pointer
 MachGenericRelocLocalSectDiff,
 MachGenericRelocThreadLocalVariables  ///< thread local variables
};

///
/// Relocations for x86_64 are a bit different than for other architectures in
/// Mach-O: Scattered relocations are not used.  Almost all relocations
/// produced by the compiler are external relocations.  An external relocation
/// has the r_extern bit set to 1 and the r_symbolnum field contains the symbol
/// table index of the target label.
///
/// When the assembler is generating relocations, if the target label is a
/// local label (begins with 'L'), then the previous non-local label in the
/// same section is used as the target of the external relocation.  An addend
/// is used with the distance from that non-local label to the target label.
/// Only when there is no previous non-local label in the section is an
/// internal relocation used.
///
/// The addend (i.e. the 4 in _foo+4) is encoded in the instruction (Mach-O
/// does not have RELA relocations).  For PC-relative relocations, the addend
/// is stored directly in the instruction.  This is different from other Mach-O
/// architectures, which encode the addend minus the current section offset.
///
/// The relocation types are:
///
///   X86_64_RELOC_UNSIGNED  // for absolute addresses
///   X86_64_RELOC_SIGNED    // for signed 32-bit displacement
///   X86_64_RELOC_BRANCH    // a CALL/JMP instruction with 32-bit displacement
///   X86_64_RELOC_GOT_LOAD  // a MOVQ load of a GOT entry
///   X86_64_RELOC_GOT    // other GOT references
///   X86_64_RELOC_SUBTRACTOR  // must be followed by a X86_64_RELOC_UNSIGNED
///
/// The following are sample assembly instructions, followed by the relocation
/// and section content they generate in an object file:
///
///   call _foo
///     r_type=X86_64_RELOC_BRANCH, r_length=2, r_extern=1, r_pcrel=1, r_symbolnum=_foo
///     E8 00 00 00 00
///
///   call _foo+4
///     r_type=X86_64_RELOC_BRANCH, r_length=2, r_extern=1, r_pcrel=1, r_symbolnum=_foo
///     E8 04 00 00 00
///
///   movq _foo@GOTPCREL(%rip), %rax
///     r_type=X86_64_RELOC_GOT_LOAD, r_length=2, r_extern=1, r_pcrel=1, r_symbolnum=_foo
///     48 8B 05 00 00 00 00
///
///   pushq _foo@GOTPCREL(%rip)
///     r_type=X86_64_RELOC_GOT, r_length=2, r_extern=1, r_pcrel=1, r_symbolnum=_foo
///     FF 35 00 00 00 00
///
///   movl _foo(%rip), %eax
///     r_type=X86_64_RELOC_SIGNED, r_length=2, r_extern=1, r_pcrel=1, r_symbolnum=_foo
///     8B 05 00 00 00 00
///
///   movl _foo+4(%rip), %eax
///     r_type=X86_64_RELOC_SIGNED, r_length=2, r_extern=1, r_pcrel=1, r_symbolnum=_foo
///     8B 05 04 00 00 00
///
///   movb  $0x12, _foo(%rip)
///     r_type=X86_64_RELOC_SIGNED, r_length=2, r_extern=1, r_pcrel=1, r_symbolnum=_foo
///     C6 05 FF FF FF FF 12
///
///   movl  $0x12345678, _foo(%rip)
///     r_type=X86_64_RELOC_SIGNED, r_length=2, r_extern=1, r_pcrel=1, r_symbolnum=_foo
///     C7 05 FC FF FF FF 78 56 34 12
///
///   .quad _foo
///     r_type=X86_64_RELOC_UNSIGNED, r_length=3, r_extern=1, r_pcrel=0, r_symbolnum=_foo
///     00 00 00 00 00 00 00 00
///
///   .quad _foo+4
///     r_type=X86_64_RELOC_UNSIGNED, r_length=3, r_extern=1, r_pcrel=0, r_symbolnum=_foo
///     04 00 00 00 00 00 00 00
///
///   .quad _foo - _bar
///     r_type=X86_64_RELOC_SUBTRACTOR, r_length=3, r_extern=1, r_pcrel=0, r_symbolnum=_bar
///     r_type=X86_64_RELOC_UNSIGNED, r_length=3, r_extern=1, r_pcrel=0, r_symbolnum=_foo
///     00 00 00 00 00 00 00 00
///
///   .quad _foo - _bar + 4
///     r_type=X86_64_RELOC_SUBTRACTOR, r_length=3, r_extern=1, r_pcrel=0, r_symbolnum=_bar
///     r_type=X86_64_RELOC_UNSIGNED, r_length=3, r_extern=1, r_pcrel=0, r_symbolnum=_foo
///     04 00 00 00 00 00 00 00
///
///   .long _foo - _bar
///     r_type=X86_64_RELOC_SUBTRACTOR, r_length=2, r_extern=1, r_pcrel=0, r_symbolnum=_bar
///     r_type=X86_64_RELOC_UNSIGNED, r_length=2, r_extern=1, r_pcrel=0, r_symbolnum=_foo
///     00 00 00 00
///
///   lea L1(%rip), %rax
///     r_type=X86_64_RELOC_SIGNED, r_length=2, r_extern=1, r_pcrel=1, r_symbolnum=_prev
///     48 8d 05 12 00 00 00
///     // assumes _prev is the first non-local label 0x12 bytes before L1
///
///   lea L0(%rip), %rax
///     r_type=X86_64_RELOC_SIGNED, r_length=2, r_extern=0, r_pcrel=1, r_symbolnum=3
///     48 8d 05 56 00 00 00
///    // assumes L0 is in third section and there is no previous non-local label.
///    // The rip-relative-offset of 0x00000056 is L0-address_of_next_instruction.
///    // address_of_next_instruction is the address of the relocation + 4.
///
///     add     $6,L0(%rip)
///             r_type=X86_64_RELOC_SIGNED_1, r_length=2, r_extern=0, r_pcrel=1, r_symbolnum=3
///    83 05 18 00 00 00 06
///    // assumes L0 is in third section and there is no previous non-local label.
///    // The rip-relative-offset of 0x00000018 is L0-address_of_next_instruction.
///    // address_of_next_instruction is the address of the relocation + 4 + 1.
///    // The +1 comes from SIGNED_1.  This is used because the relocation is not
///    // at the end of the instruction.
///
///   .quad L1
///     r_type=X86_64_RELOC_UNSIGNED, r_length=3, r_extern=1, r_pcrel=0, r_symbolnum=_prev
///     12 00 00 00 00 00 00 00
///     // assumes _prev is the first non-local label 0x12 bytes before L1
///
///   .quad L0
///     r_type=X86_64_RELOC_UNSIGNED, r_length=3, r_extern=0, r_pcrel=0, r_symbolnum=3
///     56 00 00 00 00 00 00 00
///     // assumes L0 is in third section, has an address of 0x00000056 in .o
///     // file, and there is no previous non-local label
///
///   .quad _foo - .
///     r_type=X86_64_RELOC_SUBTRACTOR, r_length=3, r_extern=1, r_pcrel=0, r_symbolnum=_prev
///     r_type=X86_64_RELOC_UNSIGNED, r_length=3, r_extern=1, r_pcrel=0, r_symbolnum=_foo
///     EE FF FF FF FF FF FF FF
///     // assumes _prev is the first non-local label 0x12 bytes before this
///     // .quad
///
///   .quad _foo - L1
///     r_type=X86_64_RELOC_SUBTRACTOR, r_length=3, r_extern=1, r_pcrel=0, r_symbolnum=_prev
///     r_type=X86_64_RELOC_UNSIGNED, r_length=3, r_extern=1, r_pcrel=0, r_symbolnum=_foo
///     EE FF FF FF FF FF FF FF
///     // assumes _prev is the first non-local label 0x12 bytes before L1
///
///   .quad L1 - _prev
///     // No relocations.  This is an assembly time constant.
///     12 00 00 00 00 00 00 00
///     // assumes _prev is the first non-local label 0x12 bytes before L1
///
///
///
/// In final linked images, there are only two valid relocation kinds:
///
///     r_type=X86_64_RELOC_UNSIGNED, r_length=3, r_pcrel=0, r_extern=1, r_symbolnum=sym_index
///  This tells dyld to add the address of a symbol to a pointer sized (8-byte)
///  piece of data (i.e on disk the 8-byte piece of data contains the addend). The
///  r_symbolnum contains the index into the symbol table of the target symbol.
///
///     r_type=X86_64_RELOC_UNSIGNED, r_length=3, r_pcrel=0, r_extern=0, r_symbolnum=0
/// This tells dyld to adjust the pointer sized (8-byte) piece of data by the amount
/// the containing image was loaded from its base address (e.g. slide).
///
enum {
  MachX8664RelocUnsigned,            ///< for absolute addresses
  MachX8664RelocSigned,              ///< for signed 32-bit displacement
  MachX8664RelocBranch,              ///< a CALL/JMP instruction with 32-bit
                                     ///< displacement
  MachX8664RelocGotLoad,             ///< a MOVQ load of a GOT entry
  MachX8664RelocGot,                 ///< other GOT references
  MachX8664RelocSubtractor,          ///< must be followed by a
                                     ///< MachX8664RelocUnsigned
  MachX8664RelocSigned1,             ///< for signed 32-bit displacement with
                                     ///< a -1 addend
  MachX8664RelocSigned2,             ///< for signed 32-bit displacement with
                                     ///< a -2 addend
  MachX8664RelocSigned4,             ///< for signed 32-bit displacement with
                                     ///< a -4 addend
  MachX8664RelocThreadLocalVariable  ///< for thread local variables
};

#endif // APPLE_MACHO_IMAGE_H
