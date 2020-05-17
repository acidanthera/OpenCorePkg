/* $Id: fsw_hfs.h $ */
/** @file
 * fsw_hfs.h - HFS file system driver header.
 */

/*
 * Copyright (C) 2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef _FSW_HFS_H_
#define _FSW_HFS_H_

#define VOLSTRUCTNAME fsw_hfs_volume
#define DNODESTRUCTNAME fsw_hfs_dnode

#include "fsw_core.h"

#ifdef _MSC_VER
# define inline __inline
#endif

#ifdef _MSC_VER
/*
 * vasily: disable warning for non-standard anonymous struct/union
 * declarations
 */
# pragma warning (disable:4201)
#endif

/*
 * The HFS Plus volume format is described in detail in Apple Technote 1150.
 *
 * http://developer.apple.com/library/archive/technotes/tn/tn1150.html
 */

//! Block size for HFS volumes.
#define HFS_BLOCKSIZE            512

//! Block number where the HFS superblock resides.
#define HFS_SUPERBLOCK_BLOCKNO   2

/* Make world look Applish enough for the system header describing HFS layout  */
#define __APPLE_API_PRIVATE
#define __APPLE_API_UNSTABLE

#define u_int8_t  fsw_u8
#define u_int16_t fsw_u16
#define u_int32_t fsw_u32
#define u_int64_t fsw_u64
#define int8_t    fsw_s8
#define int16_t   fsw_s16
#define int32_t   fsw_s32
#define int64_t   fsw_s64

#ifdef _MSC_VER
# define __attribute__(xx)
# pragma pack(push,2)
#endif

#if defined(__APPLE__) && !defined(EFIAPI)
#include <hfs/hfs_format.h>
#else
#include "hfs_format.h"
#endif

#ifdef _MSC_VER
# pragma pack(pop)
#endif

#undef u_int8_t
#undef u_int16_t
#undef u_int32_t
#undef u_int64_t
#undef int8_t
#undef int16_t
#undef int32_t
#undef int64_t

struct hfs_dirrec {
    fsw_u8      _dummy;
};

#pragma pack(push,2)

struct fsw_hfs_key
{
  union
  {
    struct HFSPlusExtentKey  ext_key;
    struct HFSPlusCatalogKey cat_key;
    fsw_u16                  key_len; /* Length is at the beginning of all keys */
  };
};

#pragma pack(pop)

/**
 * HFS: Dnode structure with HFS-specific data.
 */

struct fsw_hfs_dnode
{
  struct fsw_dnode          g;          //!< Generic dnode structure
  HFSPlusExtentRecord       extents;
  fsw_u32                   ctime;
  fsw_u32                   mtime;
  fsw_u64                   used_bytes;
  /* link file stuff */
  fsw_u32 creator;
  fsw_u32 crtype;
  /* hardlinks stuff */
  fsw_u32 ilink;
};

/**
 * HFS: In-memory B-tree structure.
 */
struct fsw_hfs_btree
{
    fsw_u32                  btroot_node;
    fsw_u32                  btnode_size;
    struct fsw_hfs_dnode*    btfile;
};

/**
 * HFS: In-memory volume structure with HFS-specific data.
 */

struct fsw_hfs_volume
{
    struct fsw_volume            g;            //!< Generic volume structure

    struct HFSPlusVolumeHeader   *primary_voldesc;  //!< Volume Descriptor
    struct fsw_hfs_btree          catalog_tree;     // Catalog tree
    struct fsw_hfs_btree          extents_tree;     // Extents overflow tree
    struct fsw_hfs_dnode          root_file;
    int                           (*btkey_compare)(BTreeKey *fskey, BTreeKey *hostkey);
    fsw_u32                       block_size_shift;
    fsw_u32                       fndr_info[8];
};

/* Endianess swappers */

static inline fsw_u16
be16_to_cpu(fsw_u16 x)
{
    return FSW_SWAPVALUE_U16(x);
}

static inline fsw_u32
be32_to_cpu(fsw_u32 x)
{
    return FSW_SWAPVALUE_U32(x);
}

static inline fsw_u64
be64_to_cpu(fsw_u64 x)
{
    return FSW_SWAPVALUE_U64(x);
}

#endif
