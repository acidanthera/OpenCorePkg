/* $Id: fsw_hfs.c $ */

/** @file
 * fsw_hfs.c - HFS file system driver code, see
 *
 *   http://developer.apple.com/technotes/tn/tn1150.html
 *
 * Current limitations:
 *  - Doesn't support permissions
 *  - Complete Unicode case-insensitiveness disabled (large tables)
 *  - No links
 *  - Only supports pure HFS+ (i.e. no HFS, or HFS+ embedded to HFS)
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

#include "fsw_hfs.h"

#ifdef HOST_POSIX
#define DPRINT(x) printf(x)
#define DPRINT2(x,y) printf(x,y)
#endif

#ifdef HOST_MSWIN
#define DPRINT(x) printf(x)
#define DPRINT2(x,y) printf(x,y)
#endif

#ifndef DPRINT
#define CONCAT(x,y) x##y
#define DPRINT(x) Print(CONCAT(L,x))
#define DPRINT2(x,y) Print(CONCAT(L,x), y)
#endif

/*
 * Handy type to deal with memory holding btnode datum
 */

typedef struct {
	BTNodeDescriptor ndesc;
	fsw_u16 shorts[1];
} btnode_datum_t;

typedef struct {
  fsw_u32 id;
  fsw_dnode_kind_t kind;
  struct fsw_string name;
  fsw_u32 creator;
  fsw_u32 crtype;
  fsw_u32 ilink;
  fsw_u64 size;
  fsw_u64 used;
  fsw_u32 ctime;
  fsw_u32 mtime;
  HFSPlusExtentRecord extents;
} file_info_t;

// functions

static fsw_status_t fsw_hfs_unistr_be2string (
	struct fsw_string* fs,
	fsw_string_kind_t fskind,
	HFSUniStr255* us
);

static fsw_status_t fsw_hfs_string2unistr (
	HFSUniStr255* us,
	struct fsw_string* fs
);

static fsw_status_t fsw_hfs_volume_mount (
  struct fsw_hfs_volume *vol
);
static void fsw_hfs_volume_free (
  struct fsw_hfs_volume *vol
);
static fsw_status_t fsw_hfs_volume_stat (
  struct fsw_hfs_volume *vol,
  struct fsw_volume_stat *sb
);

static fsw_status_t fsw_hfs_dnode_fill (
  struct fsw_hfs_volume *vol,
  struct fsw_hfs_dnode *dno
);
static void fsw_hfs_dnode_free (
  struct fsw_hfs_volume *vol,
  struct fsw_hfs_dnode *dno
);
static fsw_status_t fsw_hfs_dnode_stat (
  struct fsw_hfs_volume *vol,
  struct fsw_hfs_dnode *dno,
  struct fsw_dnode_stat *sb
);
static fsw_status_t fsw_hfs_get_extent (
  struct fsw_hfs_volume *vol,
  struct fsw_hfs_dnode *dno,
  struct fsw_extent *extent
);

static fsw_status_t fsw_hfs_dir_lookup (
  struct fsw_hfs_volume *vol,
  struct fsw_hfs_dnode *dno,
  struct fsw_string *lookup_name,
  struct fsw_hfs_dnode **child_dno_out
);
static fsw_status_t fsw_hfs_dir_read (
  struct fsw_hfs_volume *vol,
  struct fsw_hfs_dnode *dno,
  struct fsw_shandle *shand,
  struct fsw_hfs_dnode **child_dno_out
);

static fsw_status_t fsw_hfs_readlink (
  struct fsw_hfs_volume *vol,
  struct fsw_hfs_dnode *dno,
  struct fsw_string *link_target
);

static fsw_status_t fsw_hfs_btree_read_node (
	struct fsw_hfs_btree *btree,
	fsw_u32 nodenum,
	btnode_datum_t** outbuf
);

static BTreeKey *fsw_hfs_btnode_key (
	struct fsw_hfs_btree *btree,
	btnode_datum_t* node,
	fsw_u32 tuplenum
);

static void * fsw_hfs_btnode_record_ptr (
	BTreeKey *currkey
);

static fsw_status_t fsw_hfs_dnode_readinfo (
	struct fsw_hfs_volume *vol,
	struct fsw_hfs_dnode *dno
);

static int fsw_hfs_cmpb_catkey (
	BTreeKey *btkey1,
	BTreeKey *btkey2
);

static int fsw_hfs_cmpf_catkey (
	BTreeKey *btkey1,
	BTreeKey *btkey2
);

static int fsw_hfs_cmpt_catkey (
	BTreeKey *btkey1,
	BTreeKey *btkey2
);

fsw_u32 fsw_hfs_vol_bless_id (
	struct fsw_hfs_volume *vol,
	fsw_hfs_bless_kind_t bkind
);

static fsw_status_t fsw_hfs_btree_search (
	struct fsw_hfs_btree *btree,
	BTreeKey *key,
	int (*compare_keys) (BTreeKey *key1, BTreeKey *key2),
	btnode_datum_t **btnode_out,
	fsw_u32 *tuplenum_out
);

static void hfs_dnode_stuff_info (
	struct fsw_hfs_dnode *dno,
	file_info_t *file_info
);

//
// Dispatch Table
//

struct fsw_fstype_table FSW_FSTYPE_TABLE_NAME (
  hfs
) = {
  { FSW_STRING_KIND_ISO88591, 4, 4, "hfs"},
  sizeof (struct fsw_hfs_volume),
  sizeof (struct fsw_hfs_dnode),
  fsw_hfs_volume_mount,
  fsw_hfs_volume_free,
  fsw_hfs_volume_stat,
  fsw_hfs_dnode_fill,
  fsw_hfs_dnode_free,
  fsw_hfs_dnode_stat,
  fsw_hfs_get_extent,
  fsw_hfs_dir_lookup,
  fsw_hfs_dir_read,
  fsw_hfs_readlink
};

static fsw_status_t
fsw_hfs_unistr_be2string(struct fsw_string* fs, fsw_string_kind_t fskind, HFSUniStr255* us)
{
	fsw_u16 uslen;
	struct fsw_string ws;

	uslen = be16_to_cpu(us->length);
	fsw_string_setter(&ws, FSW_STRING_KIND_UTF16_BE, uslen, sizeof(fsw_u16) * uslen, us->unicode);

	return fsw_strdup_coerce(fs, fskind, &ws);
}

static fsw_status_t
fsw_hfs_string2unistr(HFSUniStr255* us, struct fsw_string* fs)
{
	fsw_status_t status;
	struct fsw_string ws;

	fsw_string_setter(&ws, FSW_STRING_KIND_EMPTY, 0, 0, NULL);
	status = fsw_strdup_coerce(&ws, FSW_STRING_KIND_UTF16, fs);

	if (status == FSW_SUCCESS) {
		if ((us->length = (fsw_u16) fsw_strlen(&ws)) > 0)
			fsw_memcpy(us->unicode, fsw_strchars(&ws), fsw_strsize(&ws));
	}

	fsw_string_mkempty (&ws);	/* Free memory used for chars */

	return status;
}

static HFSPlusCatalogKey *
fsw_hfs_make_catkey(fsw_u32 pid, struct fsw_string *str) {
	fsw_status_t sts = FSW_SUCCESS;
	fsw_u16 cklen;
	HFSPlusCatalogKey *ck = NULL;

	cklen = sizeof (ck->parentID) + sizeof (ck->nodeName.length);
	cklen += (fsw_u16) (fsw_strlen(str) * sizeof (ck->nodeName.unicode[0]));
	sts = fsw_alloc_zero(cklen + sizeof (ck->keyLength), (void **) &ck);

	if (sts == FSW_SUCCESS) {
		sts = fsw_hfs_string2unistr(&ck->nodeName, str);

		if (sts != FSW_SUCCESS) {
			fsw_free(ck);
			ck = NULL;
		} else {
			ck->keyLength = cklen;
			ck->parentID = pid;
		}
	}

	return ck;
}

static fsw_s32
fsw_hfs_read_block (struct fsw_hfs_dnode *dno, fsw_u32 log_bno, fsw_u32 off, fsw_s32 len, fsw_u8 *buf)
{
	fsw_status_t status;
	struct fsw_extent extent;
	fsw_u8 *buffer;

	fsw_memzero(&extent, sizeof(extent));
	extent.log_start = log_bno;
	status = fsw_hfs_get_extent (dno->g.vol, dno, &extent);

	if (status == FSW_SUCCESS) {
		fsw_u32 phys_bno;

		phys_bno = extent.phys_start;
		status = fsw_block_get (dno->g.vol, phys_bno, 0, (void **) &buffer);

		if (status == FSW_SUCCESS) {
			fsw_memcpy (buf, buffer + off, len);
			fsw_block_release (dno->g.vol, phys_bno, buffer);
		}
	}

	return status;
}

/* Read data from HFS file. */

static fsw_s32
fsw_hfs_read_file (struct fsw_hfs_dnode *dno, fsw_u64 pos, fsw_s32 len, fsw_u8 *buf)
{
	fsw_status_t status;
	fsw_s32 read = 0;
	fsw_u32 block_size_bits = dno->g.vol->block_size_shift;
	fsw_u32 block_size = (1 << block_size_bits);
	fsw_u32 block_size_mask = block_size - 1;
	
	while (len > 0) {
		fsw_u32 log_bno;
		fsw_u32 off = (fsw_u32) (pos & block_size_mask);
		fsw_s32 next_len = len;
		
		log_bno = (fsw_u32) FSW_U64_SHR (pos, block_size_bits);
		
		if (next_len >= 0 && (fsw_u32) next_len > block_size)
			next_len = block_size;
		
		status = fsw_hfs_read_block (dno, log_bno, off, next_len, buf);
		
		if (status != FSW_SUCCESS)
			return -1;
		
		buf += next_len;
		pos += next_len;
		len -= next_len;
		read += next_len;
	}
	
	return read;
}

static fsw_s32
fsw_hfs_compute_shift (fsw_u32 size)
{
	fsw_s32 i;

	for (i = 0; i < 32; i++) {

		if ((size >> i) == 0)
			return i - 1;
	}

	return 0;
}

static BTHeaderRec *
fsw_hfs_btree_read_hdrec (struct fsw_hfs_btree* btree)
{
	fsw_status_t status;
	BTHeaderRec* hr = NULL;

	status = fsw_alloc(sizeof (BTHeaderRec), &hr);

	if (status == FSW_SUCCESS) {
		fsw_s32 rv;

		rv = fsw_hfs_read_file (btree->btfile, sizeof (BTNodeDescriptor), sizeof (BTHeaderRec), (fsw_u8 *) hr);

		if (rv != sizeof (BTHeaderRec)) {
			fsw_free(hr);
			hr = NULL;
		}
	}

	return hr;
}

static fsw_status_t
fsw_hfs_volume_btree_setup ( struct fsw_hfs_btree* btree)
{
	BTHeaderRec* hr;

	hr = fsw_hfs_btree_read_hdrec(btree);

	if (hr != NULL) {
		btree->btroot_node = be32_to_cpu (hr->rootNode);
		btree->btnode_size = be16_to_cpu (hr->nodeSize);
		fsw_free(hr);

		return FSW_SUCCESS;
	}

	return FSW_VOLUME_CORRUPTED;
}

static HFSPlusCatalogThread *
fsw_hfs_cnid2threadrecord (struct fsw_hfs_volume *vol, fsw_u32 cnid, btnode_datum_t **btnode)
{
	fsw_status_t status;
	fsw_u32 tuplenum;
	HFSPlusCatalogKey *catkey;
	HFSPlusCatalogThread *trp = NULL;

	catkey = fsw_hfs_make_catkey(cnid, NULL);

	if (catkey != NULL) {
		status = fsw_hfs_btree_search (&vol->catalog_tree, (BTreeKey *) catkey, fsw_hfs_cmpt_catkey, btnode, &tuplenum);

		if (status == FSW_SUCCESS) {
			trp = (HFSPlusCatalogThread *) fsw_hfs_btnode_record_ptr (fsw_hfs_btnode_key (&vol->catalog_tree, *btnode, tuplenum));
		}
	}

	fsw_free(catkey);

	return trp;
}

static fsw_status_t
fsw_hfs_volume_catalog_setup (struct fsw_hfs_volume *vol)
{
	fsw_status_t status;
	BTHeaderRec* hr;
	btnode_datum_t *btnode = NULL;
	HFSPlusCatalogThread *trp = NULL;

	hr = fsw_hfs_btree_read_hdrec(&vol->catalog_tree);

	if (hr != NULL) {
		vol->btkey_compare = hr->keyCompareType == kHFSBinaryCompare ? fsw_hfs_cmpb_catkey : fsw_hfs_cmpf_catkey;
		fsw_free(hr);
	} else {
		return FSW_VOLUME_CORRUPTED;
	}

	/* Set default volume name */

	fsw_string_setter (&vol->g.label, FSW_STRING_KIND_EMPTY, 0, 0, NULL);

	/*
	 * Volume label lives in thread record for kHFSRootFolderID (root folder is nameless by design)
	 */

	status = FSW_VOLUME_CORRUPTED;
	trp = fsw_hfs_cnid2threadrecord(vol, kHFSRootFolderID, &btnode);

	if (trp != NULL)
		status = fsw_hfs_unistr_be2string(&vol->g.label, vol->g.host_string_kind, &trp->nodeName);

	fsw_free(btnode);

	return status;
}

/**
 * Mount an HFS+ volume. Reads the superblock and constructs the
 * root directory dnode.
 */

static fsw_status_t
fsw_hfs_volume_mount (struct fsw_hfs_volume *vol)
{
	fsw_status_t status, rv;
	void *buffer = NULL;
	fsw_u32 blockno;

	vol->primary_voldesc = NULL;
	fsw_set_blocksize (vol, HFS_BLOCKSIZE, HFS_BLOCKSIZE);
	blockno = HFS_SUPERBLOCK_BLOCKNO;

#define CHECK(sx) if (sx != FSW_SUCCESS) { rv = sx; break; }

	do {
		HFSPlusVolumeHeader *voldesc;
		fsw_u32 block_size;
		fsw_u16 signature;

		status = fsw_block_get (vol, blockno, 0, &buffer);
		CHECK (status);
		voldesc = (HFSPlusVolumeHeader *) buffer;
		signature = be16_to_cpu (voldesc->signature);

		if ((signature != kHFSPlusSigWord) && (signature != kHFSXSigWord)) {
			rv = FSW_UNSUPPORTED;
			break;
		}

		status = fsw_memdup ((void **) &vol->primary_voldesc, voldesc, sizeof (*voldesc));
		CHECK (status);

		block_size = be32_to_cpu (voldesc->blockSize);
		vol->block_size_shift = fsw_hfs_compute_shift (block_size);

		fsw_block_release (vol, blockno, buffer);
		buffer = NULL;
		voldesc = NULL;

		fsw_set_blocksize (vol, block_size, block_size);
		fsw_memcpy (&vol->fndr_info, &vol->primary_voldesc->finderInfo, sizeof (vol->fndr_info));

		/* Setup catalog dnode */

		status = fsw_dnode_create_root (vol, kHFSCatalogFileID, &vol->catalog_tree.btfile);
		CHECK (status);
		fsw_memcpy (vol->catalog_tree.btfile->extents,
					vol->primary_voldesc->catalogFile.extents,
					sizeof (vol->catalog_tree.btfile->extents));
		vol->catalog_tree.btfile->g.size =
		be64_to_cpu (vol->primary_voldesc->catalogFile.logicalSize);

		/* Setup extents overflow file */

		status = fsw_dnode_create_root (vol, kHFSExtentsFileID, &vol->extents_tree.btfile);
		CHECK (status);
		fsw_memcpy (vol->extents_tree.btfile->extents,
					vol->primary_voldesc->extentsFile.extents,
					sizeof (vol->extents_tree.btfile->extents));
		vol->extents_tree.btfile->g.size =
		be64_to_cpu (vol->primary_voldesc->extentsFile.logicalSize);

		/* Setup the root dnode */

		status = fsw_dnode_create_root (vol, kHFSRootFolderID, &vol->g.root);
		CHECK (status);


		status = fsw_hfs_volume_btree_setup (&vol->catalog_tree);
		CHECK (status);

		/* Setup extents overflow file */

		status = fsw_hfs_volume_btree_setup (&vol->extents_tree);
		CHECK(status);

		rv = fsw_hfs_volume_catalog_setup (vol);
	} while (0);

#undef CHECK

	if (buffer != NULL)
		fsw_block_release (vol, blockno, buffer);

	return rv;
}

/**
 * Free the volume data structure. Called by the core after an unmount or after
 * an unsuccessful mount to release the memory used by the file system type specific
 * part of the volume structure.
 */

static void
fsw_hfs_volume_free (struct fsw_hfs_volume *vol)
{
	if (vol->primary_voldesc != NULL) {
		fsw_free (vol->primary_voldesc);
		vol->primary_voldesc = NULL;
	}

	if (vol->catalog_tree.btfile != NULL) {
		fsw_dnode_release ((struct fsw_dnode *) (vol->catalog_tree.btfile));
		vol->catalog_tree.btfile = NULL;
	}

	if (vol->extents_tree.btfile != NULL) {
		fsw_dnode_release ((struct fsw_dnode *) (vol->extents_tree.btfile));
		vol->extents_tree.btfile = NULL;
	}
}

/**
 * Get in-depth information on a volume.
 */

static fsw_status_t
fsw_hfs_volume_stat (struct fsw_hfs_volume *vol, struct fsw_volume_stat *sb)
{
	sb->total_bytes =
	FSW_U64_SHL (be32_to_cpu (vol->primary_voldesc->totalBlocks),
				 vol->block_size_shift);
	sb->free_bytes =
	FSW_U64_SHL (be32_to_cpu (vol->primary_voldesc->freeBlocks),
				 vol->block_size_shift);

	return FSW_SUCCESS;
}

/*
 * Get blessed item ID
 */

fsw_u32
fsw_hfs_vol_bless_id (struct fsw_hfs_volume *vol, fsw_hfs_bless_kind_t bkind) {
	fsw_u32 bnid = 0;

	if ((int) bkind < 6)
		bnid = vol->fndr_info[bkind];

	return be32_to_cpu(bnid);
}

/**
 * Get full information on a dnode from disk. This function is called by the core
 * whenever it needs to access fields in the dnode structure that may not
 * be filled immediately upon creation of the dnode.
 */

static fsw_status_t
fsw_hfs_dnode_fill (struct fsw_hfs_volume *vol, struct fsw_hfs_dnode *dno)
{
	fsw_status_t status = FSW_UNKNOWN_ERROR;

	if (dno->g.fullinfo)
		return FSW_SUCCESS;

	status = fsw_hfs_dnode_readinfo(vol, dno);

	return status;
}

/**
 * Free the dnode data structure. Called by the core when deallocating a dnode
 * structure to release the memory used by the file system type specific part
 * of the dnode structure.
 */

static void
fsw_hfs_dnode_free (struct fsw_hfs_volume *vol, struct fsw_hfs_dnode *dno)
{
}

static fsw_u32
mac_to_posix (fsw_u32 mac_time)
{
	/* Mac time is 1904 year based */

	return mac_time ? mac_time - 2082844800 : 0;
}

/**
 * Get in-depth information on a dnode. The core makes sure that fsw_hfs_dnode_fill
 * has been called on the dnode before this function is called. Note that some
 * data is not directly stored into the structure, but passed to a host-specific
 * callback that converts it to the host-specific format.
 */

static fsw_status_t
fsw_hfs_dnode_stat (struct fsw_hfs_volume *vol, struct fsw_hfs_dnode *dno, struct fsw_dnode_stat *sb)
{
	sb->used_bytes = dno->used_bytes;
	sb->store_time_posix (sb, FSW_DNODE_STAT_CTIME, mac_to_posix (dno->ctime));
	sb->store_time_posix (sb, FSW_DNODE_STAT_MTIME, mac_to_posix (dno->mtime));
	sb->store_time_posix (sb, FSW_DNODE_STAT_ATIME, 0);
	sb->store_attr_posix (sb, 0700);

	return FSW_SUCCESS;
}

static int
fsw_hfs_find_block (HFSPlusExtentRecord *exts, fsw_u32 *lbno, fsw_u32 *pbno)
{
	int i;
	fsw_u32 cur_lbno = *lbno;

	for (i = 0; i < 8; i++) {
		fsw_u32 start = be32_to_cpu ((*exts)[i].startBlock);
		fsw_u32 count = be32_to_cpu ((*exts)[i].blockCount);

		if (cur_lbno < count) {
			*pbno = start + cur_lbno;
			return 1;
		}

		cur_lbno -= count;
	}

	*lbno = cur_lbno;

	return 0;
}

/* Find in btnode key offset for given tuple (key, record) number */

static fsw_u32
fsw_hfs_btnode_keyoffset (struct fsw_hfs_btree *btree, btnode_datum_t *btnode, fsw_u32 tuplenum)
{
	fsw_u16 koff = 0;
	fsw_u16 count;

	count = be16_to_cpu (btnode->ndesc.numRecords);

	if (count > 0 && tuplenum < count) {
		fsw_u32 ix;

		ix = ((btree->btnode_size - sizeof (BTNodeDescriptor)) / sizeof (fsw_u16)) - 1 - tuplenum;
		koff = be16_to_cpu(btnode->shorts[ix]);
	}

	return koff;
}

/* Pointer to the key inside btnode for given tuple (key, record) number */

static BTreeKey *
fsw_hfs_btnode_key (struct fsw_hfs_btree *btree, btnode_datum_t* btnode, fsw_u32 tuplenum)
{
	fsw_u8 *cnode = (fsw_u8 *) btnode;
	fsw_u32 offset;

	offset = fsw_hfs_btnode_keyoffset (btree, btnode, tuplenum);

	if (offset < sizeof(BTNodeDescriptor) || offset > btree->btnode_size - sizeof (fsw_u16)) {
		return NULL;
	}

	return (BTreeKey *) (cnode + offset);
}

static void *
fsw_hfs_btnode_record_ptr (BTreeKey *currkey)
{
	fsw_u8 *ptr;

	ptr = ((fsw_u8 *) currkey + be16_to_cpu (currkey->length16) + sizeof (currkey->length16));

	return (void *) ptr;
}

static fsw_status_t
fsw_hfs_btree_read_node (struct fsw_hfs_btree *btree, fsw_u32 nodenum, btnode_datum_t** outnode)
{
	fsw_status_t status;
	btnode_datum_t* btnode;

	status = fsw_alloc (btree->btnode_size, &btnode);

	if (status == FSW_SUCCESS) {
		fsw_u64 bstart;
		fsw_s32 rv;

		status = FSW_VOLUME_CORRUPTED;
		bstart = (fsw_u64) nodenum * btree->btnode_size;
		fsw_memzero (btnode, btree->btnode_size);
		rv = fsw_hfs_read_file(btree->btfile, bstart, btree->btnode_size, (fsw_u8 *) btnode);

		if ((fsw_u32) rv == btree->btnode_size) {
			fsw_u32 roffset = fsw_hfs_btnode_keyoffset(btree, btnode, 0);

			if (roffset == sizeof (BTNodeDescriptor)) {
				*outnode = btnode;
				status = FSW_SUCCESS;
			}
		}
	}

	if (status != FSW_SUCCESS)
		fsw_free(btnode);

	return status;
}

static fsw_u32
fsw_hfs_btree_ix_next_btnodenum (BTreeKey *btkey)
{
	fsw_u32 nn;

	nn = *((fsw_u32 *)fsw_hfs_btnode_record_ptr (btkey));

	return be32_to_cpu(nn);
}

static fsw_status_t
fsw_hfs_btree_search (struct fsw_hfs_btree *btree, BTreeKey *key, int (*compare_keys) (BTreeKey *key1, BTreeKey *key2), btnode_datum_t **btnode_out, fsw_u32 *tuplenum_out)
{
	fsw_status_t status;
	btnode_datum_t *btnode = NULL;
	fsw_u32 btnodenum;
	fsw_u32 tuplenum;

	btnodenum = btree->btroot_node;

	for (;;) {
		fsw_s32 cmp = 0;
		fsw_u32 count;
		BTreeKey *currkey = NULL;

		status = fsw_hfs_btree_read_node (btree, btnodenum, &btnode);

		if (status != FSW_SUCCESS)
			break;

		count = be16_to_cpu (btnode->ndesc.numRecords);

		for (tuplenum = 0; tuplenum < count; tuplenum++) {
			currkey = fsw_hfs_btnode_key (btree, btnode, tuplenum);
			cmp = compare_keys (currkey, key);

			if (cmp <= 0 && btnode->ndesc.kind == kBTIndexNode) {
				btnodenum = fsw_hfs_btree_ix_next_btnodenum (currkey);
			}

			if (cmp >= 0)
				break;
		}

		if (btnode->ndesc.kind == kBTLeafNode) {
			status = FSW_NOT_FOUND;

			if (cmp == 0) {
				*btnode_out = btnode;
				*tuplenum_out = tuplenum;
				status = FSW_SUCCESS;
			}

			break;
		}

		if (tuplenum == 0 && cmp > 0) {
			status = FSW_NOT_FOUND;
			break;
		}

		fsw_free(btnode);
		btnode = NULL;
	}

	if (status != FSW_SUCCESS)
		fsw_free (btnode);

	return status;
}

static void
fill_fileinfo (struct fsw_hfs_volume* vol, BTreeKey* btkey, file_info_t* finfo)
{
	fsw_u8* base;
	fsw_u16 rec_type;

	base = (fsw_u8 *) fsw_hfs_btnode_record_ptr (btkey);
	rec_type = be16_to_cpu (*(fsw_u16 *) base);

	/** @todo: read additional info */

	switch (rec_type) {
		case kHFSPlusFolderRecord:
		{
			HFSPlusCatalogFolder *info = (HFSPlusCatalogFolder *) (void *)base;

			finfo->kind = FSW_DNODE_KIND_DIR;
			finfo->id = be32_to_cpu (info->folderID);

			/* @todo: return number of elements, maybe use smth else */

			finfo->size = be32_to_cpu (info->valence);
			finfo->used = be32_to_cpu (info->valence);
			finfo->ctime = be32_to_cpu (info->createDate);
			finfo->mtime = be32_to_cpu (info->contentModDate);
			break;
		}
		case kHFSPlusFileRecord:
		{
			HFSPlusCatalogFile *info = (HFSPlusCatalogFile *) (void *)base;

			finfo->kind = FSW_DNODE_KIND_FILE;
			finfo->id = be32_to_cpu (info->fileID);

			finfo->creator = be32_to_cpu (info->userInfo.fdCreator);
			finfo->crtype = be32_to_cpu (info->userInfo.fdType);

			/* Is the file any kind of link? */

			if ((finfo->creator == kSymLinkCreator && finfo->crtype == kSymLinkFileType) ||
				(finfo->creator == kHFSPlusCreator && finfo->crtype == kHardLinkFileType)) {
				finfo->kind = FSW_DNODE_KIND_SYMLINK;
				finfo->ilink = be32_to_cpu (info->bsdInfo.special.iNodeNum);
			}

			finfo->size = be64_to_cpu (info->dataFork.logicalSize);
			finfo->used = FSW_U64_SHL (be32_to_cpu (info->dataFork.totalBlocks), vol->block_size_shift);
			finfo->ctime = be32_to_cpu (info->createDate);
			finfo->mtime = be32_to_cpu (info->contentModDate);
			fsw_memcpy (&finfo->extents, &info->dataFork.extents, sizeof (finfo->extents));
			break;
		}
		default:
			finfo->kind = FSW_DNODE_KIND_UNKNOWN;
			break;
	}
}

typedef struct {
	fsw_u32 cur_pos;              /* current position */
	fsw_u32 parent;
	struct fsw_hfs_volume *vol;
	struct fsw_shandle *shandle;  /* this one track iterator's state */
	file_info_t file_info;
} visitor_parameter_t;

static int
fsw_hfs_btnode_visit_record (BTreeKey *btkey, void *param)
{
	visitor_parameter_t *vp = (visitor_parameter_t *) param;
	struct HFSPlusCatalogKey *cat_key = (HFSPlusCatalogKey *) btkey;
	fsw_u8 *base = (fsw_u8 *) fsw_hfs_btnode_record_ptr (btkey);
	fsw_u16 rec_type = be16_to_cpu (*(fsw_u16 *) base);

	if (be32_to_cpu (cat_key->parentID) != vp->parent)
		return -1;

	vp->file_info.kind = FSW_DNODE_KIND_UNKNOWN;


	if (vp->shandle->pos != vp->cur_pos++)
		return 0;

	switch (rec_type) {
		case kHFSPlusFolderThreadRecord:
		case kHFSPlusFileThreadRecord:
			vp->shandle->pos++; /* not smth we care about, skip it */
			return 0;

		default:
			break;
	}

	fill_fileinfo (vp->vol, btkey, &vp->file_info);
	(void) fsw_hfs_unistr_be2string(&vp->file_info.name, FSW_STRING_KIND_UTF16, &cat_key->nodeName);	/* XXX: (void)? */
	vp->shandle->pos++;

	return 1;
}

static fsw_status_t
fsw_hfs_btnode_iterate_records (struct fsw_hfs_btree *btree, btnode_datum_t *first_btnode, fsw_u32 first_tuplenum, int (*callback) (BTreeKey *record, void *param), void *param)
{
	fsw_status_t status;

	/* We modify node, so make a copy */

	btnode_datum_t *btnode = first_btnode;

	for (;;) {
		fsw_u32 i;
		fsw_u32 next_btnode;
		fsw_u32 count = be16_to_cpu (btnode->ndesc.numRecords);

		/* Iterate over all records in this node */

		for (i = first_tuplenum; i < count; i++) {
			int rv = callback (fsw_hfs_btnode_key (btree, btnode, i), param);

			switch (rv) {
				case 1:
					status = FSW_SUCCESS;
					goto done;
				case -1:
					status = FSW_NOT_FOUND;
					goto done;
			}

			/* if callback returned 0 - continue */
		}

		next_btnode = be32_to_cpu (btnode->ndesc.fLink);

		if (next_btnode == 0) {
			status = FSW_NOT_FOUND;
			break;
		}

		fsw_free(btnode);
		btnode = NULL;
		status = fsw_hfs_btree_read_node (btree, next_btnode, &btnode);

		if (status != FSW_SUCCESS)
			break;

		first_tuplenum = 0;
	}

done:
	fsw_free(btnode);

	return status;
}

static int
fsw_hfs_cmp_extkey (BTreeKey *key1, BTreeKey *key2)
{
	HFSPlusExtentKey *ekey1 = (HFSPlusExtentKey *) key1;
	HFSPlusExtentKey *ekey2 = (HFSPlusExtentKey *) key2;
	int result;

	/* First key is read from the FS data, second is in-memory in CPU endianess */

	result = be32_to_cpu (ekey1->fileID) - ekey2->fileID;

	if (result)
		return result;

	result = ekey1->forkType - ekey2->forkType;

	if (result)
		return result;

	result = be32_to_cpu (ekey1->startBlock) - ekey2->startBlock;

	return result;
}

/* Thread key search */

static int
fsw_hfs_cmpt_catkey (BTreeKey *btkey1, BTreeKey *btkey2)
{
	HFSPlusCatalogKey *ckey1 = (HFSPlusCatalogKey *) btkey1;
	HFSPlusCatalogKey *ckey2 = (HFSPlusCatalogKey *) btkey2;
	int rv;

	rv = be32_to_cpu(ckey1->parentID) - ckey2->parentID;

	if (rv != 0)
		return rv;

	return ckey1->nodeName.length;
}

static int
fsw_hfs_cmpf_catkey (BTreeKey *btkey1, BTreeKey *btkey2)
{
	int rv;
	int apos;
	int bpos;
	int ckey1nlen;
	int ckey2nlen;
	fsw_u16 ac;
	fsw_u16 bc;
	fsw_u16 *p1;
	fsw_u16 *p2;
	HFSPlusCatalogKey *ckey1 = (HFSPlusCatalogKey *) btkey1;
	HFSPlusCatalogKey *ckey2 = (HFSPlusCatalogKey *) btkey2;

	rv = be32_to_cpu(ckey1->parentID) - ckey2->parentID;

	if (rv != 0)
		return rv;

	ckey1nlen = be16_to_cpu (ckey1->nodeName.length);
	ckey2nlen = ckey2->nodeName.length;

	if (ckey1nlen == 0 || ckey2nlen == 0)
		return ckey1nlen - ckey2nlen;

	p1 = &ckey1->nodeName.unicode[0];
	p2 = &ckey2->nodeName.unicode[0];

	apos = bpos = 0;

	for (;;) {
		/* get next valid character from ckey1 */

		for (ac = 0; ac == 0 && apos < ckey1nlen; apos++) {
			ac = be16_to_cpu (p1[apos]);
			ac = fsw_to_lower (ac);
		}

		/* get next valid character from ckey2 */

		for (bc = 0; bc == 0 && bpos < ckey2nlen; bpos++) {
			bc = p2[bpos];
			bc = fsw_to_lower (bc);
		}

		if (ac != bc)
			break;

		if (ac == 0)
			return 0;
	}

	return (ac - bc);
}

static int
fsw_hfs_cmpb_catkey (BTreeKey *btkey1, BTreeKey *btkey2)
{
	HFSPlusCatalogKey *ckey1 = (HFSPlusCatalogKey *) btkey1;
	HFSPlusCatalogKey *ckey2 = (HFSPlusCatalogKey *) btkey2;
	int rv;
	int ckey1nlen;
	int ckey2nlen;
	int commlen;
	fsw_u16 *p1;
	fsw_u16 *p2;
	int i;

	rv = be32_to_cpu(ckey1->parentID) - ckey2->parentID;

	if (rv != 0)
		return rv;

	ckey1nlen = be16_to_cpu (ckey1->nodeName.length);
	ckey2nlen = ckey2->nodeName.length;
	commlen = ckey1nlen > ckey2nlen ? ckey2nlen : ckey1nlen;

	p1 = &ckey1->nodeName.unicode[0];
	p2 = &ckey2->nodeName.unicode[0];

	for (i = 0; i < commlen; i++) {
		rv = be16_to_cpu(p1[i]) - p2[i];
		if (rv != 0)
			return rv;
	}

	return ckey1nlen - ckey2nlen;
}

/**
 * Retrieve file data mapping information. This function is called by the core when
 * fsw_shandle_read needs to know where on the disk the required piece of the file's
 * data can be found. The core makes sure that fsw_hfs_dnode_fill has been called
 * on the dnode before. Our task here is to get the physical disk block number for
 * the requested logical block number.
 */

static fsw_status_t
fsw_hfs_get_extent (struct fsw_hfs_volume *vol, struct fsw_hfs_dnode *dno, struct fsw_extent *extent)
{
	fsw_status_t status;
	fsw_u32 lbno;
	HFSPlusExtentRecord *exts;
	btnode_datum_t *btnode = NULL;

	extent->exkind = FSW_EXTENT_KIND_PHYSBLOCK;
	extent->log_count = 1;
	lbno = extent->log_start;

	/* we only care about data forks atm, do we? */

	exts = &dno->extents;

	for (;;) {
		struct HFSPlusExtentKey *key;
		struct HFSPlusExtentKey overflowkey;
		fsw_u32 tuplenum;
		fsw_u32 phys_bno;

		if (fsw_hfs_find_block (exts, &lbno, &phys_bno)) {
			extent->phys_start = phys_bno;
			status = FSW_SUCCESS;
			break;
		}

		/* Find appropriate overflow record */

		overflowkey.forkType = 0; /* data fork */
		overflowkey.fileID = dno->g.dnode_id;
		overflowkey.startBlock = extent->log_start - lbno;

		fsw_free (btnode);
		btnode = NULL;

		status = fsw_hfs_btree_search (&vol->extents_tree, (BTreeKey *) &overflowkey, fsw_hfs_cmp_extkey, &btnode, &tuplenum);

		if (status != FSW_SUCCESS)
			break;

		key = (struct HFSPlusExtentKey *) fsw_hfs_btnode_key (&vol->extents_tree, btnode, tuplenum);
		exts = (HFSPlusExtentRecord *) (key + 1);
	}

	fsw_free (btnode);

	return status;
}

static void
hfs_dnode_stuff_info (struct fsw_hfs_dnode *dno, file_info_t *file_info)
{
	dno->g.dkind = file_info->kind;
	dno->g.size = file_info->size;
	dno->used_bytes = file_info->used;
	dno->ctime = file_info->ctime;
	dno->mtime = file_info->mtime;

	/* Fill-in extents info */

	if (file_info->kind == FSW_DNODE_KIND_FILE) {
		fsw_memcpy (dno->extents, &file_info->extents, sizeof (file_info->extents));
	}

	/* Fill-in link file info */

	if (file_info->kind == FSW_DNODE_KIND_SYMLINK) {
		dno->creator = file_info->creator;
		dno->crtype = file_info->crtype;
		dno->ilink = file_info->ilink;
		fsw_memcpy(dno->extents, &file_info->extents, sizeof (file_info->extents));
	}
}

static fsw_status_t
create_hfs_dnode (struct fsw_hfs_dnode *dno, file_info_t *file_info, struct fsw_hfs_dnode **child_dno_out)
{
	fsw_status_t status;
	struct fsw_hfs_dnode *baby;

	status = fsw_dnode_create (dno->g.vol, dno, file_info->id, file_info->kind, &file_info->name, &baby);

	if (status == FSW_SUCCESS) {
		hfs_dnode_stuff_info(baby, file_info);
		*child_dno_out = baby;
	}

	return status;
}

/**
 * Lookup a directory's child dnode by name. This function is called on a directory
 * to retrieve the directory entry with the given name. A dnode is constructed for
 * this entry and returned. The core makes sure that fsw_hfs_dnode_fill has been called
 * and the dnode is actually a directory.
 */

static fsw_status_t
fsw_hfs_dir_lookup (struct fsw_hfs_volume *vol, struct fsw_hfs_dnode *dno, struct fsw_string *lookup_name, struct fsw_hfs_dnode **child_dno_out)
{
	fsw_status_t status;
	HFSPlusCatalogKey *catkey;

	catkey = fsw_hfs_make_catkey((dno->g).dnode_id, lookup_name);

	if (catkey != NULL) {
		fsw_u32 tuplenum;
		btnode_datum_t *btnode = NULL;

		status = fsw_hfs_btree_search (&vol->catalog_tree, (BTreeKey *) catkey, vol->btkey_compare, &btnode, &tuplenum);

		if (status == FSW_SUCCESS) {
			HFSPlusCatalogKey *file_key;
			file_info_t file_info;

			fsw_memzero (&file_info, sizeof (file_info));

			file_key = (HFSPlusCatalogKey *) fsw_hfs_btnode_key (&vol->catalog_tree, btnode, tuplenum);
			fill_fileinfo (vol, (BTreeKey *) file_key, &file_info);
			status = create_hfs_dnode (dno, &file_info, child_dno_out);
			fsw_string_mkempty (&file_info.name);
		}

		fsw_free (btnode);
	} else
		status = FSW_OUT_OF_MEMORY;

	fsw_free (catkey);

	return status;
}

/**
 * Get the next directory entry when reading a directory. This function is called during
 * directory iteration to retrieve the next directory entry. A dnode is constructed for
 * the entry and returned. The core makes sure that fsw_hfs_dnode_fill has been called
 * and the dnode is actually a directory. The shandle provided by the caller is used to
 * record the position in the directory between calls.
 */

static fsw_status_t
fsw_hfs_dir_read (struct fsw_hfs_volume *vol, struct fsw_hfs_dnode *dno, struct fsw_shandle *shand, struct fsw_hfs_dnode **child_dno_out)
{
	fsw_status_t status;
	struct HFSPlusCatalogKey *catkey;
	fsw_u32 tuplenum;

	catkey = fsw_hfs_make_catkey(dno->g.dnode_id, NULL);

	if (catkey != NULL) {
		btnode_datum_t *btnode = NULL;

		status = fsw_hfs_btree_search (&vol->catalog_tree, (BTreeKey *) catkey, vol->btkey_compare, &btnode, &tuplenum);

		if (status == FSW_SUCCESS) {
			visitor_parameter_t param;

			fsw_memzero (&param, sizeof (param));

			/* Iterator updates shand state */

			param.vol = vol;
			param.shandle = shand;
			param.parent = dno->g.dnode_id;
			status = fsw_hfs_btnode_iterate_records (&vol->catalog_tree, btnode, tuplenum, fsw_hfs_btnode_visit_record, &param);

			if (status == FSW_SUCCESS)
				status = create_hfs_dnode (dno, &param.file_info, child_dno_out);

			fsw_string_mkempty (&param.file_info.name);
		}
	} else
		status = FSW_OUT_OF_MEMORY;

	fsw_free(catkey);

	return status;
}

/**
 * Get the target path of a symbolic link. This function is called when a symbolic
 * link needs to be resolved. The core makes sure that the fsw_hfs_dnode_fill has been
 * called on the dnode and that it really is a link.
 *
 */

static fsw_status_t
fsw_hfs_readlink (struct fsw_hfs_volume *vol, struct fsw_hfs_dnode *dno, struct fsw_string *link_target)
{
  /*
   * XXX: Hardlinks for directories -- not yet.
   */

  if(dno->creator == kHFSPlusCreator && dno->crtype == kHardLinkFileType) {
#define MPRFSIZE (sizeof (metaprefix))
#define MPRFINUM (MPRFSIZE - 1 - 10)
    static fsw_u8 metaprefix[] = "/\0\0\0\0HFS+ Private Data/iNode0123456789";
    fsw_u32 sz = 0;

    link_target->skind = FSW_STRING_KIND_ISO88591;
    link_target->size = MPRFSIZE;
    fsw_memdup (&link_target->data, metaprefix, link_target->size);
    sz = (fsw_u32) fsw_u32_to_str(((char *) link_target->data) + MPRFINUM, 10, dno->ilink);
    link_target->len = MPRFINUM + sz;

    return FSW_SUCCESS;
#undef MPRFINUM
#undef MPRFSIZE
  } else if (dno->creator == kSymLinkCreator && dno->crtype == kSymLinkFileType) {
    return fsw_dnode_readlink_data(dno, link_target);
  }

  /* Unknown link type */

  return FSW_UNSUPPORTED;
}

static fsw_status_t
fsw_hfs_dnode_readinfo (struct fsw_hfs_volume *vol, struct fsw_hfs_dnode *dno)
{
	fsw_status_t status = FSW_NOT_FOUND;
	btnode_datum_t *btnode = NULL;
	HFSPlusCatalogThread *trp = NULL;

	trp = fsw_hfs_cnid2threadrecord(vol, dno->g.dnode_id, &btnode);

	if (trp != NULL) {
		dno->g.parent_id = be32_to_cpu(trp->parentID);
		status = fsw_hfs_unistr_be2string(&dno->g.name, vol->g.host_string_kind, &trp->nodeName);
	}

	fsw_free(btnode);

	if (status == FSW_SUCCESS) {
		struct HFSPlusCatalogKey *catkey = NULL;
		fsw_u32 tuplenum;

		catkey = fsw_hfs_make_catkey(dno->g.parent_id, &dno->g.name);
		status = fsw_hfs_btree_search (&vol->catalog_tree, (BTreeKey *) catkey, vol->btkey_compare, &btnode, &tuplenum);

		if (status == FSW_SUCCESS) {
			file_info_t file_info;

			fsw_memzero (&file_info, sizeof (file_info));
			fill_fileinfo(vol, fsw_hfs_btnode_key (&vol->catalog_tree, btnode, tuplenum), &file_info);
			hfs_dnode_stuff_info (dno, &file_info);
			dno->g.fullinfo = 1;
			fsw_string_mkempty (&file_info.name);
			fsw_free(btnode);
		}

		fsw_free(catkey);
	}

	return status;
}
