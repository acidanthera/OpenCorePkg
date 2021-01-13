/** @file
  HFS+ file system driver.

  Copyright (c) 2020, Vladislav Yaroshchuk <yaroshchuk2000@gmail.com>
  Copyright (C) 2017, Gabriel L. Somlo <gsomlo@gmail.com>

  This program and the accompanying materials are licensed and made
  available under the terms and conditions of the BSD License which
  accompanies this distribution.   The full text of the license may
  be found at http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS"
  BASIS, WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER
  EXPRESS OR IMPLIED.
**/

#include "fsw_hfsplus.h"

// --------------------------------------------------------
// HFS+ FSW Methods declaration
//

/* Mount an HFS+ volume. Read volume header (equivalent of superblock),
 * and set up dnodes for the root folder and B-Tree file(s).
 * Return FSW_SUCCESS or error code.
 */
static fsw_status_t
fsw_hfsplus_vol_mount(struct fsw_hfsplus_volume *v);

/* Free the volume data structure. Called by the core after an unmount or
 * unsuccessful mount, to release the memory used by the file system type
 * specific part of the volume structure.
 */
static void
fsw_hfsplus_vol_free(struct fsw_hfsplus_volume *v);

/* Get in-depth information on a volume.
 */
static fsw_status_t
fsw_hfsplus_vol_stat(struct fsw_hfsplus_volume *v, struct fsw_volume_stat *s);

/* Get full information on a dnode from disk. This function is called by
 * the core whenever it needs to access fields in the dnode structure that
 * may not be filled immediately upon creation of the dnode.
 */
static fsw_status_t
fsw_hfsplus_dno_fill(struct fsw_hfsplus_volume *v, struct fsw_hfsplus_dnode *d);

/* Free the dnode data structure. Called by the core when deallocating a dnode
 * structure to release the memory used by the file system type specific part
 * of the dnode structure.
 */
static void
fsw_hfsplus_dno_free(struct fsw_hfsplus_volume *v, struct fsw_hfsplus_dnode *d);

/* Get in-depth dnode information. The core ensures fsw_hfsplus_dno_fill()
 * has been called on the dnode before this function is called. Note that some
 * data is not directly stored into the structure, but passed to a host-specific
 * callback that converts it to the host-specific format.
 */
static fsw_status_t
fsw_hfsplus_dno_stat(struct fsw_hfsplus_volume *v, struct fsw_hfsplus_dnode *d,
                     struct fsw_dnode_stat *s);

/* Retrieve file data mapping information. This function is called by
 * the core when fsw_shandle_read needs to know where on the disk the
 * required piece of the file's data can be found. The core makes sure
 * that fsw_hfsplus_dno_fill has been called on the dnode before.
 * Our task here is to get the physical disk block number for the
 * requested logical block number.
 * NOTE: logical and physical block sizes are the same (see mount method).
 */
static fsw_status_t
fsw_hfsplus_get_ext(struct fsw_hfsplus_volume *v, struct fsw_hfsplus_dnode *d,
                    struct fsw_extent *e);


/* Lookup a directory's child dnode by name. This function is called on a
 * directory to retrieve the directory entry with the given name. A dnode
 * is constructed for this entry and returned. The core makes sure that
 * fsw_hfsplus_dno_fill has been called and the dnode is actually a directory.
 */
static fsw_status_t
fsw_hfsplus_dir_get(struct fsw_hfsplus_volume *v, struct fsw_hfsplus_dnode *d,
                    struct fsw_string *name, struct fsw_hfsplus_dnode **d_out);

/* Get the next directory entry when reading a directory. This function is
 * called during directory iteration to retrieve the next directory entry.
 * A dnode is constructed for the entry and returned. The core makes sure
 * that fsw_hfsplus_dno_fill has been called and the dnode is actually a
 * directory. The shandle provided by the caller is used to record the
 * position in the directory between calls.
 */
static fsw_status_t
fsw_hfsplus_dir_read(struct fsw_hfsplus_volume *v, struct fsw_hfsplus_dnode *d,
                     struct fsw_shandle *sh, struct fsw_hfsplus_dnode **d_out);

/* Get the target path of a symbolic link. This function is called when a
 * symbolic link needs to be resolved. The core makes sure that the
 * fsw_hfsplus_dno_fill has been called on the dnode and that it really is a
 * symlink.
 */
static fsw_status_t
fsw_hfsplus_readlink(struct fsw_hfsplus_volume *v, struct fsw_hfsplus_dnode *d,
                     struct fsw_string *lnk_tgt);
/**
 * Get dnode associated with the given bless type
 * @param vol volume
 * @param bless_type bless type
 * @param dno_out resulting dnode
 * @return FSW_SUCCESS on success
 */
static fsw_status_t
fsw_hfsplus_get_bless_info(struct fsw_hfsplus_volume *vol, /* in */
                           fsw_u32 bless_type, /* in */
                           struct fsw_hfsplus_dnode **dno_out /* out */);

/**
 * HFS+ FSW Method Dispatch Table
 */
struct fsw_fstype_table FSW_FSTYPE_TABLE_NAME(hfsplus) = {
        { FSW_STRING_TYPE_ISO88591, 4, 4, "hfsplus" },
        sizeof(struct fsw_hfsplus_volume), sizeof(struct fsw_hfsplus_dnode),
        fsw_hfsplus_vol_mount, fsw_hfsplus_vol_free, fsw_hfsplus_vol_stat,
        fsw_hfsplus_dno_fill, fsw_hfsplus_dno_free, fsw_hfsplus_dno_stat,
        fsw_hfsplus_get_ext, fsw_hfsplus_dir_get, fsw_hfsplus_dir_read,
        fsw_hfsplus_readlink, fsw_hfsplus_get_bless_info
};

// -------------------------------------------------------------------
// Other declarations

/* Given dnode 'd' and offset 'pos' into the data file it represents,
 * retrieve 'len' bytes of data into buffer 'buf';
 * Return FSW_SUCCESS or error code.
 */
static fsw_status_t
fsw_hfsplus_read(struct fsw_hfsplus_dnode *d, fsw_u64 pos,
                 fsw_u32 len, void *buf);

/* Given the volume being mounted ('v'), and the ID & fork data of the B-Tree
 * file being set up ('dn_id' and 'f', respectively), populate a cached,
 * in-memory record of the B-Tree file at the location pointed to by 'btp';
 * Return FSW_SUCCESS or error code.
 */
static fsw_status_t
fsw_hfsplus_btf_setup(struct fsw_hfsplus_volume *v,
                      fsw_u32 dn_id, HFSPlusForkData *f,
                      struct fsw_hfsplus_dnode **btp);

/* HFS+ to Posix timestamp conversion
 */
static fsw_u32
fsw_hfsplus_posix_time(fsw_u32 t);

/* Given a B-Tree node pointed to by 'btnode', with node size 'size',
 * locate the record given by its record number 'rnum';
 * Return a pointer to the B-Tree key at the beginning of the record.
 */
static HFSPlusBTKey *
fsw_hfsplus_btnode_get_rec(BTNodeDescriptor* btnode, fsw_u16 size, fsw_u32 rnum);

/* Given a B-Tree record pointer 'k', return a pointer to the data
 * immediately following the key record; IOW, skip the key record which
 * prefixes the record data payload.
 */
static void *
fsw_hfsplus_bt_rec_skip_key(HFSPlusBTKey *k);

/* Return the child node number immediately following the key record 'k' of
 * an index node
 */
static fsw_u32
fsw_hfsplus_bt_idx_get_child(HFSPlusBTKey *k);

/**
 * Create minimal dnode that will be complete in future with fsw_hfsplus_dnode_complete() call.
 * fsw_hfsplus_dnode_fill() also makes dnode complete
 * @param parent_id parent dnode id
 * @param dnode_id dnode id
 * @param name dnode name
 * @param d_out created dnode
 * @return FSW_SUCCESS on success
 */
static fsw_status_t
fsw_hfsplus_dnode_create_minimal(struct fsw_hfsplus_volume *v /* in */,
                                 fsw_u32 parent_id /* in */,
                                 fsw_u32 dnode_id /* in */,
                                 struct fsw_string *name /* in */,
                                 struct fsw_hfsplus_dnode **d_out /* out */);

/**
 * Complete dnode from HFSPlusCatalogRecord information.
 * We assume that d_out's name, id and parent_id already filled
 * @param rec catalog record
 * @param parent parent dnode
 * @param d_out dnode to complete
 */
static void
fsw_hfsplus_dnode_complete(HFSPlusCatalogRecord *rec /* in */,
                           struct fsw_hfsplus_dnode *parent /* in */,
                           struct fsw_hfsplus_dnode *d_out /* out */);

/**
 * Create dnode and complete its data with 'rec' info
 * @param rec catalog record
 * @param parent parent dnode
 * @param name dnode name
 * @param d_out created dnode
 * @return FSW_SUCCESS on success
 */
static fsw_status_t
fsw_hfsplus_dnode_create_full(HFSPlusCatalogRecord *rec /* in */,
                              struct fsw_hfsplus_dnode *parent /* in */,
                              struct fsw_string *name /* in */,
                              struct fsw_hfsplus_dnode **d_out /* out */);

/** Compare an on-disk catalog B-Tree trial key ('tk') with an in-memory
 * search key ('sk'). Precedence is parentID (keyLength does not
 * factor into the comparison). Allows only empty 'tk' nodeName
 * @param sk search key
 * @param on-disk catalog B-Tree trial key
 * @return -1/0/1 if 'tk'is smaller/equal/larger than 'sk', respectively.
 */
static int
fsw_hfsplus_thread_cmp(HFSPlusBTKey *tk /* in */, HFSPlusBTKey *sk /* in */);

/** Compare an on-disk catalog B-Tree trial key ('tk') with an in-memory
 * search key ('sk'). Precedence is parentID, nodeName (keyLength does not
 * factor into the comparison).
 * @param sk search key
 * @param on-disk catalog B-Tree trial key
 * @return -1/0/1 if 'tk'is smaller/equal/larger than 'sk', respectively.
 */
static int
fsw_hfsplus_cat_cmp(HFSPlusBTKey *tk /* in */, HFSPlusBTKey *sk /* in */);


/**
 * Convert fsw string to HFSUniStr255 (FSW_STRING_TYPE_UTF16 content type).
 * Used to prepare names for fsw_hfsplus_bt_search() search key
 * @param t target string
 * @param src source
 * @return FSW_SUCCESS on success
 */
static fsw_status_t
fsw_hfsplus_fswstr2unistr(HFSUniStr255* t /* out */,
                          struct fsw_string* src /* in */);

/* Search an HFS+ special file's B-Tree (given by 'bt'), for a search key
 * matching 'sk', using comparison procedure 'k_cmp' to determine when a key
 * match occurs;
 * Finish by filling a caller-provided B-Tree node buffer ('btnode'), and returning
 * record number of the matching record inside 'btnode', via 'rec_num';
 * On error, set fsw_status_t return code acoordingly.
 *
 * NOTE: A HFS+ volume has a few "special" files, linked directly from the
 *       volume header. For the purpose of this driver, we mainly care about
 *       two of them: the "catalog" and "extents" files. All of these files
 *       are organized as B-Tree structures. This means that, overlaid on
 *       top of the linear span of each file there is an array of nodes of
 *       a given size (node_size), internally cross-linked with "pointers"
 *       to parent/child/sibling nodes, which are essentially the "index"
 *       (or 'node-number') of the target node in this overlaid array.
 *       Ultimately, (node-number * node-size) is a byte offset into the
 *       file, to the location where the referenced node's data begins.
 *       Each B-Tree file's "dnode" information is available in the HFS+
 *       volume header. The node at the very beginning of each file (at
 *       index or node-number == 0) contains a "header node", which provides
 *       the 'node-number' of the B-Tree's "root" node, as well as the
 *       'node-size' of all nodes in that B-Tree file.
 */
static fsw_status_t
fsw_hfsplus_bt_search(struct fsw_hfsplus_dnode *bt, /* in */
                      HFSPlusBTKey *sk, /* in */
                      k_cmp_t k_cmp, /* in */
                      BTNodeDescriptor *btnode, /* out */
                      fsw_u32 *rec_num /* out */);

/* Compare unsigned integers 'a' and 'b';
 * Return -1/0/1 if 'a' is less/equal/greater than 'b'.
 */
static int
fsw_hfsplus_int_cmp(fsw_u32 a, fsw_u32 b);

/* Basic latin unicode lowercase
 */
static fsw_u16
fsw_hfsplus_ucblatin_tolower(fsw_u16 c);

/**
 * Iterate B-Tree records searching for `rec_skip` node starting from `rec_num` record of `btnode`
 * @param bt B-Tree root
 * @param parent_id parent dnode id (used as stop criteria: finished on unequal parent ids)
 * @param btnode BTNode to start iterate from. On finish filled with actual value
 * @param rec_num BTNode record number to start iterate from. On finish filled with actual value
 * @param rec_skip Number of records to skip
 * @return FSW_SUCCESS on success, else FSW_NOT_FOUND
 */
static fsw_status_t
fsw_hfsplus_btree_get_rec(struct fsw_hfsplus_dnode *bt, /* in */
                          fsw_u32 parent_id,  /* in */
                          BTNodeDescriptor *btnode, /* in */ /* out */
                          fsw_u32 *rec_num, /* in */ /* out */
                          fsw_u64 *rec_skip /* in */ /* out */);

/**
 * Find thread record by the existing dnid (CNID in HFS+ specs)
 * @param v volume
 * @param dnid given dnid
 * @param btnode pre-allocated btnode used for b-tree search
 * @param thread_out found thread record
 * @return FSW_SUCCESS on success 
 */
static fsw_status_t
fsw_hfsplus_dnid2thread(struct fsw_hfsplus_volume *v /* in */, 
                        fsw_u32 dnid /* in */,
                        BTNodeDescriptor *btnode /* in */ /* out */, 
                        HFSPlusCatalogThread **thread_out /* out */);
        
/**
 * Create d_node from existing thread record. 
 * Created dnode is not complete and should be filled by fsw_dnode_fill() call
 * @param v volume
 * @param thread known thread record 
 * @param btnode pre-allocated btnode used for b-tree search
 * @param d_out created dnode
 * @return FSW_SUCCESS on success 
 */
static fsw_status_t
fsw_hfsplus_thread2dnode(struct fsw_hfsplus_volume *v /* in */,
                         HFSPlusCatalogThread *thread /* in */,
                         BTNodeDescriptor *btnode /* in */ /* out */,
                        struct fsw_hfsplus_dnode **d_out /* out */);

/**
 * Create dnode from known dnid (CNID in HFS+ specs). 
 * Created dnode is not complete and should be filled by fsw_dnode_fill() call
 * @param v volume
 * @param dnid known dnid
 * @param d_out created dnode
 * @return FSW_SUCCESS on success
 */
static fsw_status_t
fsw_hfsplus_dnid2dnode(struct fsw_hfsplus_volume *v /* in */,
                        fsw_u32 dnid /* in */,
                        struct fsw_hfsplus_dnode **d_out /* out */);
/**
* Read requested Bless info type from the volume header, returns according dnid (CNID in HFS+ specs)
* @param vol volume
* @param bless_type bless info type
* @param dnid readed dnid
* @return FSW_SUCCESS if found
*/
static fsw_status_t 
fsw_hfsplus_bless_type2dnid(struct fsw_hfsplus_volume *vol, /* in */
                            fsw_u32 bless_type, /* in */
                            fsw_u32 *dnid /* out */);

/**
 * Convert fsw_u32 to string (char[]). Memory will be allocated depending on the string length required
 * @param num Source number
 * @param dest Destination array
 * @param len Destination string length
 * @return FSW_OUT_OF_MEMORY on fsw_alloc() errored, else FSW_SUCCESS
 */
static fsw_status_t
fsw_hfsplus_itoa(fsw_u32 num, char** dest, int* len);

/**
 * Concatenate two strings
 * @param s1 First string
 * @param len1 First string length
 * @param s2 Second string
 * @param len2 Second string length
 * @param dest Destination buffer
 * @return FSW_OUT_OF_MEMORY on fsw_alloc() errored, else FSW_SUCCESS
 */
static fsw_status_t
fsw_hfsplus_sconcat(const char *const s1, int len1, const char *const s2, int len2, char **dest);

// ---------------------------------------------------------------------


static fsw_status_t
fsw_hfsplus_read(struct fsw_hfsplus_dnode *d, fsw_u64 pos,
                 fsw_u32 len, void *buf)
{
    struct fsw_shandle sh;
    fsw_u32            buflen;
    fsw_status_t       status;

    status = fsw_shandle_open(d, &sh);
    if (status)
        return status;

    sh.pos = pos;
    buflen = len;
    status = fsw_shandle_read(&sh, &buflen, buf);
    if (!status && buflen != len)
        status = FSW_IO_ERROR;

    fsw_shandle_close(&sh);
    return status;
}

static fsw_status_t
fsw_hfsplus_btf_setup(struct fsw_hfsplus_volume *v,
                      fsw_u32 dn_id, HFSPlusForkData *f,
                      struct fsw_hfsplus_dnode **btp)
{
    BTHeaderRec  hdr_rec;
    fsw_status_t status;

    status = fsw_dnode_create_root(v, dn_id, btp);
    if (status)
        return status;

    (*btp)->g.size = fsw_u64_be_swap(f->logicalSize);
    fsw_memcpy((*btp)->extents, f->extents, sizeof(HFSPlusExtentRecord));

    // read header record (from node 0, immediately past the node descriptor)
    status = fsw_hfsplus_read(*btp, sizeof(BTNodeDescriptor),
                              sizeof(BTHeaderRec), &hdr_rec);
    if (status)
        return status;

    // grab root node index and node size from header record
    (*btp)->bt_root = fsw_u32_be_swap(hdr_rec.rootNode);
    (*btp)->bt_ndsz = fsw_u16_be_swap(hdr_rec.nodeSize);

    return FSW_SUCCESS;
}

static fsw_status_t
fsw_hfsplus_vol_mount(struct fsw_hfsplus_volume *v)
{
    void                    *buf;
    fsw_u32                 bs;
    BTNodeDescriptor        *btnode;
    HFSPlusCatalogThread    *thread;
    struct fsw_string       label;
    fsw_status_t            status;

    // allocate memory for vol. header
    status = fsw_alloc(sizeof(HFSPlusVolumeHeader), &v->vh);
    if (status)
        return status;

    // read vol. header into buffer
    fsw_set_blocksize(v, kHFSBlockSize, kHFSBlockSize);
    status = fsw_block_get(v, kMasterDirectoryBlock, 0, &buf);
    if (status)
        return status;
    fsw_memcpy(v->vh, buf, sizeof(HFSPlusVolumeHeader));
    fsw_block_release(v, kMasterDirectoryBlock, buf);

    // check vol. header
    if (fsw_u16_be_swap(v->vh->signature) != kHFSPlusSigWord)
        return FSW_UNSUPPORTED;

    // use block size specified by vol. header
    bs = fsw_u32_be_swap(v->vh->blockSize);
    fsw_set_blocksize(v, bs, bs);

    // set up catalog B-Tree file:
    status = fsw_hfsplus_btf_setup(v, kHFSCatalogFileID, &v->vh->catalogFile,
                                   &v->catf);
    if (status)
        return status;

    // set up root folder:
    status = fsw_dnode_create_root(v, kHFSRootFolderID, &v->g.root);
    if (status)
        return status;

    // Get volume label from kHFSRootFolderID thread record
    // Not-readed volume label is not fatal error,
    // we can proceed without it leaving label empty
    status = fsw_alloc(v->catf->bt_ndsz, &btnode);
    if (!status) {
        status = fsw_hfsplus_dnid2thread(v, kHFSRootFolderID, btnode, &thread);
        if (status)
            goto label_read_fail;

        label.len = fsw_u16_be_swap(thread->nodeName.length);
        label.size = sizeof(fsw_u16) * label.len;
        label.data = thread->nodeName.unicode;
        label.type = FSW_STRING_TYPE_UTF16_BE;

        status = fsw_strdup_coerce(&v->g.label, FSW_STRING_TYPE_UTF16, &label);
        if (status)
            goto label_read_fail;

        fsw_free(btnode);
    }

    return FSW_SUCCESS;

// If volume label reading failed we leave it empty and return SUCCESS status
label_read_fail:
    fsw_free(btnode);
    v->g.label.type = FSW_STRING_TYPE_EMPTY;
    return FSW_SUCCESS;

}

static void
fsw_hfsplus_vol_free(struct fsw_hfsplus_volume *v)
{
    if (v->vh)
        fsw_free(v->vh);
    if (v->catf)
        fsw_dnode_release((struct fsw_dnode *)v->catf);
}

static fsw_status_t
fsw_hfsplus_vol_stat(struct fsw_hfsplus_volume *v, struct fsw_volume_stat *s)
{
    fsw_u32 bs = fsw_u32_be_swap(v->vh->blockSize);

    s->total_bytes = bs * fsw_u32_be_swap(v->vh->totalBlocks);
    s->free_bytes = bs * fsw_u32_be_swap(v->vh->freeBlocks);

    return FSW_SUCCESS;
}

static fsw_status_t
fsw_hfsplus_dno_fill(struct fsw_hfsplus_volume *v, struct fsw_hfsplus_dnode *d)
{
    fsw_status_t                status;
    struct fsw_hfsplus_dnode    *parent;
    HFSPlusCatalogKey           sk, *tk;
    HFSPlusCatalogRecord        *rec;
    fsw_u32                     rec_num;
    BTNodeDescriptor            *btnode;

    // pre-allocate bt-node buffer for use by search function:
    status = fsw_alloc(v->catf->bt_ndsz, &btnode);
    if (status) {
        return status;
    }

    sk.parentID = d->parent_id;
    status = fsw_hfsplus_fswstr2unistr(&(sk.nodeName), &(d->g.name));
    if (status) {
        goto done;
    }

    status = fsw_hfsplus_bt_search(v->catf,
                                   (HFSPlusBTKey *)&sk,
                                   fsw_hfsplus_cat_cmp,
                                   btnode, &rec_num);

    tk = (HFSPlusCatalogKey *) fsw_hfsplus_btnode_get_rec(btnode, v->catf->bt_ndsz, rec_num);
    rec = (HFSPlusCatalogRecord *) fsw_hfsplus_bt_rec_skip_key((HFSPlusBTKey *)tk);

    status = fsw_hfsplus_dnid2dnode(v, d->parent_id, &parent);
    if (status) {
       goto done;
    }

    fsw_hfsplus_dnode_complete(rec, parent, d);

done:
    fsw_free(btnode);

    return status;
}

static void
fsw_hfsplus_dno_free(struct fsw_hfsplus_volume *v, struct fsw_hfsplus_dnode *d)
{
    // NOTE: not applicable to HFS+ dnodes!
    return;
}

static fsw_u32
fsw_hfsplus_posix_time(fsw_u32 t)
{
    // t is in seconds since midnight, January 1, 1904, GMT
    return (t > 2082844800) ? t - 2082844800 : 0;
}

static fsw_status_t
fsw_hfsplus_dno_stat(struct fsw_hfsplus_volume *v, struct fsw_hfsplus_dnode *d,
                     struct fsw_dnode_stat *s)
{
    s->used_bytes = d->g.size;
    s->store_attr_posix(s, 0500);
    s->store_time_posix(s, FSW_DNODE_STAT_CTIME, fsw_hfsplus_posix_time(d->ct));
    s->store_time_posix(s, FSW_DNODE_STAT_MTIME, fsw_hfsplus_posix_time(d->mt));
    s->store_time_posix(s, FSW_DNODE_STAT_ATIME, fsw_hfsplus_posix_time(d->at));

    return FSW_SUCCESS;
}

static HFSPlusBTKey *
fsw_hfsplus_btnode_get_rec(BTNodeDescriptor* btnode, fsw_u16 size, fsw_u32 rnum)
{
    fsw_u16 *off = (fsw_u16 *) ((fsw_u8 *) btnode + size) - 1 - rnum;
    return (HFSPlusBTKey *)((fsw_u8 *)btnode + fsw_u16_be_swap(*off));
}

static void *
fsw_hfsplus_bt_rec_skip_key(HFSPlusBTKey *k)
{
    return (fsw_u8 *)k + sizeof(k->keyLength) + fsw_u16_be_swap(k->keyLength);
}

static fsw_u32
fsw_hfsplus_bt_idx_get_child(HFSPlusBTKey *k)
{
    fsw_u32 *child;
    child = (fsw_u32 *)fsw_hfsplus_bt_rec_skip_key(k);
    return fsw_u32_be_swap(*child);
}

static fsw_status_t
fsw_hfsplus_bt_search(struct fsw_hfsplus_dnode *bt,
                      HFSPlusBTKey *sk,
                      k_cmp_t k_cmp,
                      BTNodeDescriptor *btnode,
                      fsw_u32 *rec_num)
{
    fsw_u32      node;
    fsw_u16      rec, lo, hi;
    HFSPlusBTKey *tk;    // trial key
    int          cmp;
    fsw_status_t status;

    // start searching from the B-Tree root node:
    node = bt->bt_root;

    for (;;) {
        // load data for current node into caller-provided buffer 'btnode'
        status = fsw_hfsplus_read(bt, (fsw_u64)node * bt->bt_ndsz,
                                  bt->bt_ndsz, btnode);
        if (status)
            return status;

        // sanity check: record 0 located immediately after node descriptor
        if ((fsw_u8 *)btnode + sizeof(BTNodeDescriptor) !=
            (fsw_u8 *)fsw_hfsplus_btnode_get_rec(btnode, bt->bt_ndsz, 0))
            return FSW_VOLUME_CORRUPTED;

        // search records within current node
        lo = 0;
        hi = fsw_u16_be_swap(btnode->numRecords) - 1;
        while (lo <= hi) {
             // access record data, then compare to search key 'sk'
             rec = (lo + hi) >> 1;
             tk = fsw_hfsplus_btnode_get_rec(btnode, bt->bt_ndsz, rec);
             cmp = k_cmp(tk, sk);

             if (cmp < 0)      // (tk < sk)
                 lo = rec + 1;
             else if (cmp > 0) // (tk > sk)
                 hi = rec - 1;
             else {            // (tk == sk)
                 if (btnode->kind != kBTLeafNode) {
                     hi = rec;
                     break;
                 }

                 *rec_num = rec;
                 return FSW_SUCCESS;
             }
        }

        // NOTE: following the binary search, 'hi' now points at the
        //       record with the largest 'tk' for which (tk <= sk)

        if (btnode->kind != kBTIndexNode)
            break;

        // on an index node, so descend to child
        tk = fsw_hfsplus_btnode_get_rec(btnode, bt->bt_ndsz, hi);
        node = fsw_hfsplus_bt_idx_get_child(tk);
    }

    // search key 'sk' not found
    return FSW_NOT_FOUND;
}


static int
fsw_hfsplus_int_cmp(fsw_u32 a, fsw_u32 b)
{
    return (a < b) ? -1 : (a > b) ? 1 : 0;
}

static fsw_u16
fsw_hfsplus_ucblatin_tolower(fsw_u16 c)
{
    if (c == 0)
        return 0xFFFF;
    if (c == 0x00C6 || c == 0x00D0 || c == 0x00D8 || c == 0x00DE ||
        (c >= 0x0041 && c <= 0x005A))
        return c + 0x0020;
    return c;
}

static int
fsw_hfsplus_thread_cmp(HFSPlusBTKey *tk, HFSPlusBTKey *sk)
{
    int ret;
    // NOTE: all 'tk' fields are stored as big-endian values and must be
    // converted to CPU endianness before any comparison to corresponding
    // fields in 'sk'.

    // Here is enough to compare parent's IDs and
    // check if the name is an empty string
    ret = fsw_hfsplus_int_cmp(fsw_u32_be_swap(tk->catKey.parentID), sk->catKey.parentID);
    if (ret)
        return ret;

    // NOTE:
    // When failing name length check there's no reason of continuing B-Tree traverse
    // because we can have only one child of 'thread' type (with an empty name), and it
    // should be the 1st record. But let the search traverse be stopped
    // later on parents comparison for simplicity
    return tk->catKey.nodeName.length > 0;
}

static int
fsw_hfsplus_cat_cmp(HFSPlusBTKey *tk, HFSPlusBTKey *sk)
{
    fsw_u16 *t_str, *s_str;
    fsw_u16 t_len, s_len;
    fsw_u16 t_char, s_char;
    int ret;

    // NOTE: all 'tk' fields are stored as big-endian values and must be
    // converted to CPU endianness before any comparison to corresponding
    // fields in 'sk'.

    // compare parent IDs: if unequal, we're done!
    ret = fsw_hfsplus_int_cmp(fsw_u32_be_swap(tk->catKey.parentID),
                              sk->catKey.parentID);
    if (ret)
        return ret;

    // unicode string pointers and lengths:
    t_len = fsw_u16_be_swap(tk->catKey.nodeName.length);
    t_str = tk->catKey.nodeName.unicode;
    s_len = sk->catKey.nodeName.length;
    s_str = sk->catKey.nodeName.unicode;

    for (;;) {
        // start by assuming strings are empty:
        t_char = s_char = 0;

        // find next valid char from on-disk key string:
        while (t_char == 0 && t_len > 0) {
            t_char = fsw_hfsplus_ucblatin_tolower(fsw_u16_be_swap(*t_str));
            t_len--;
            t_str++;
        }

        // find next valid char from memory key string:
        while (s_char == 0 && s_len > 0) {
            s_char = fsw_hfsplus_ucblatin_tolower(*s_str);
            s_len--;
            s_str++;
        }

        // stop if difference or both strings exhausted:
        ret = fsw_hfsplus_int_cmp(t_char, s_char);
        if (ret || s_char == 0)
            break;
    }

    return ret;
}

static fsw_status_t
fsw_hfsplus_get_ext(struct fsw_hfsplus_volume *v, struct fsw_hfsplus_dnode *d,
                    struct fsw_extent *e)
{
    fsw_u32             off, bc;
    HFSPlusExtentRecord *er;
    int                 i;

    // set initial offset to provided starting logical block number:
    off = e->log_start;

    // start with dnode's initial extent record:
    er = &d->extents;

    // search extent record:
    for (i = 0; i < kHFSPlusExtentDensity; i++) {
        // get block count for current extent descriptor:
        bc = fsw_u32_be_swap((*er)[i].blockCount);

        // have we exhausted all available extents?
        if (bc == 0)
            return FSW_NOT_FOUND;

        // offset is relative to current extent's physical startBlock:
        if (off < bc) {
            e->type = FSW_EXTENT_TYPE_PHYSBLOCK;
            e->phys_start = fsw_u32_be_swap((*er)[i].startBlock) + off;
            e->log_count = bc - off;
            return FSW_SUCCESS;
        }

        // update offset to NEXT extent descriptor:
        off -= bc;
    }

    // FIXME: more than 8 fragments not yet supported!
    FSW_MSG_ASSERT((FSW_MSGSTR("FswHfsPlus: get_ext: got_here\n")));

    return FSW_UNSUPPORTED;
}

static fsw_status_t
fsw_hfsplus_fswstr2unistr(HFSUniStr255* t, struct fsw_string* src)
{
    fsw_status_t status;
    struct fsw_string ws;

    status = fsw_strdup_coerce(&ws, FSW_STRING_TYPE_UTF16, src);
    if (status == FSW_SUCCESS) {
        t->length = (fsw_u16) fsw_strlen(&ws);
        if (t->length > 0) {
            fsw_memcpy(t->unicode, fsw_strdata(&ws), fsw_strsize(&ws));
        }
    }
    fsw_strfree (&ws);
    return status;
}

static fsw_status_t
fsw_hfsplus_btree_get_rec(struct fsw_hfsplus_dnode *bt,
                        fsw_u32 parent_id,
                        BTNodeDescriptor *btnode,
                        fsw_u32 *rec_num,
                        fsw_u64 *rec_skip)
{
    fsw_status_t            status;
    fsw_u32                 btnode_next;
    fsw_u32                 num_records;
    fsw_u32                 counter;
    HFSPlusBTKey            *tk;
    HFSPlusCatalogRecord    *rec;
    fsw_u32                 i;

    counter = 0;
    for (;;) {
        status = FSW_NOT_FOUND;
        num_records = fsw_u16_be_swap(btnode->numRecords);

        for (i = *rec_num; i < num_records; ++i) {
            tk = fsw_hfsplus_btnode_get_rec(btnode, bt->bt_ndsz, i);

            if (fsw_u32_be_swap(((HFSPlusCatalogKey*)(tk))->parentID) != parent_id) {
                return status;
            }

            if (counter++ != *rec_skip) {
                continue;
            }

            rec = fsw_hfsplus_bt_rec_skip_key(tk);
            ++(*rec_skip);
            switch (fsw_u16_be_swap(rec->recordType)) {
                case kHFSPlusFileRecord:
                case kHFSPlusFolderRecord:
                    *rec_num = i;
                    return FSW_SUCCESS;
                    break;

                case kHFSPlusFolderThreadRecord:
                case kHFSPlusFileThreadRecord:
                default:
                    break;
            }
        }

        btnode_next = fsw_u32_be_swap(btnode->fLink);
        if (btnode_next == 0) {
            return status;
        }

        status = fsw_hfsplus_read(bt, (fsw_u64) btnode_next * bt->bt_ndsz,
                                  bt->bt_ndsz, btnode);
        *rec_num = 0;
        if (status) {
            return status;
        }
    }

}

static fsw_status_t
fsw_hfsplus_dnode_create_minimal(struct fsw_hfsplus_volume *v,
                                 fsw_u32 parent_id,
                                 fsw_u32 dnode_id,
                                 struct fsw_string *name,
                                 struct fsw_hfsplus_dnode **d_out)
{
    fsw_status_t status;

    status = fsw_dnode_create(v, NULL, dnode_id, FSW_DNODE_TYPE_UNKNOWN, name, d_out);
    if (!status) {
        (*d_out)->parent_id = parent_id;
    }
    return status;
}

static void
fsw_hfsplus_dnode_complete(HFSPlusCatalogRecord *rec,
                       struct fsw_hfsplus_dnode *parent,
                       struct fsw_hfsplus_dnode *d_out)
{

    // figure out type of child dnode:
    switch (fsw_u16_be_swap(rec->recordType)) {
        case kHFSPlusFolderRecord:
            d_out->g.type = FSW_DNODE_TYPE_DIR;

            d_out->ct = fsw_u32_be_swap(rec->folderRecord.createDate);
            d_out->mt = fsw_u32_be_swap(rec->folderRecord.contentModDate);
            d_out->at = fsw_u32_be_swap(rec->folderRecord.accessDate);
            break;

        case kHFSPlusFileRecord:
            d_out->g.type = FSW_DNODE_TYPE_FILE;
            d_out->g.size = fsw_u64_be_swap(rec->fileRecord.dataFork.logicalSize);

            fsw_memcpy(d_out->extents, &rec->fileRecord.dataFork.extents, sizeof(HFSPlusExtentRecord));

            d_out->ct = fsw_u32_be_swap(rec->fileRecord.createDate);
            d_out->mt = fsw_u32_be_swap(rec->fileRecord.contentModDate);
            d_out->at = fsw_u32_be_swap(rec->fileRecord.accessDate);

            d_out->fd_type = fsw_u32_be_swap(rec->fileRecord.userInfo.fdType);
            d_out->fd_creator = fsw_u32_be_swap(rec->fileRecord.userInfo.fdCreator);

            if ((d_out->fd_creator == kHFSPlusSymlinkCreator && d_out->fd_type == kHFSPlusSymlinkType) ||
                (d_out->fd_creator == kHFSPlusHFSPlusCreator && d_out->fd_type == kHFSPlusHardlinkType)) {

                d_out->g.type = FSW_DNODE_TYPE_SYMLINK;
                d_out->inode_num = fsw_u32_be_swap (rec->fileRecord.permissions.special.iNodeNum);
            }
            break;

        default:
            d_out->g.type = FSW_DNODE_TYPE_UNKNOWN;

            d_out->ct = 0;
            d_out->mt = 0;
            d_out->at = 0;
            break;
    }

    d_out->g.parent = parent;
    fsw_dnode_retain(&parent->g);

    // mark dnode as complete
    fsw_dnode_mkcomplete(d_out);
}

static fsw_status_t
fsw_hfsplus_dnode_create_full(HFSPlusCatalogRecord *rec,
                              struct fsw_hfsplus_dnode *parent,
                              struct fsw_string *name,
                              struct fsw_hfsplus_dnode **d_out)
{
    fsw_status_t    status;
    fsw_u32         dnid;

    switch (fsw_u16_be_swap(rec->recordType)) {
        case kHFSPlusFolderRecord:
            dnid = fsw_u32_be_swap(rec->folderRecord.folderID);
            break;
        case kHFSPlusFileRecord:
            dnid = fsw_u32_be_swap(rec->fileRecord.fileID);
            break;
        default:
            dnid = 0;
            break;
    }

    status = fsw_hfsplus_dnode_create_minimal(parent->g.vol, parent->g.dnode_id, dnid, name, d_out);

    if (!status) {
        fsw_hfsplus_dnode_complete(rec, parent, *d_out);
    }
    return status;
}

static fsw_status_t
fsw_hfsplus_dir_get(struct fsw_hfsplus_volume *v, struct fsw_hfsplus_dnode *d,
                    struct fsw_string *name, struct fsw_hfsplus_dnode **d_out)
{
    BTNodeDescriptor     *btnode;
    HFSPlusCatalogKey    sk, *tk;
    HFSPlusCatalogRecord *rec;
    fsw_status_t         status;
    fsw_u32              rec_num;
    fsw_u32 i;

    // search catalog file for child named by 'name':
    sk.parentID = d->g.dnode_id;
    status = fsw_hfsplus_fswstr2unistr(&(sk.nodeName), name);

    if (status)
        return status;

    FSW_MSG_DEBUG((FSW_MSGSTR("FswHfsPlus: dir_get: parent=%d name: "), d->g.dnode_id));
    for (i = 0; i < sk.nodeName.length; i++)
        FSW_MSG_DEBUG((FSW_MSGSTR("%c"), ((fsw_u16 *)sk.nodeName.unicode)[i]));
    FSW_MSG_DEBUG((FSW_MSGSTR("\n")));

    // pre-allocate bt-node buffer for use by search function:
    status = fsw_alloc(v->catf->bt_ndsz, &btnode);
    if (status) {
        return status;
    }

    status = fsw_hfsplus_bt_search(v->catf,
                                   (HFSPlusBTKey *)&sk,
                                   fsw_hfsplus_cat_cmp,
                                   btnode,
                                   &rec_num);
    if (status) {
        goto done;
    }

    tk = (HFSPlusCatalogKey *)fsw_hfsplus_btnode_get_rec(btnode, v->catf->bt_ndsz, rec_num);
    rec = fsw_hfsplus_bt_rec_skip_key((HFSPlusBTKey *)tk);
    status = fsw_hfsplus_dnode_create_full(rec, d, name, d_out);

done:
    fsw_free(btnode);
    return status;
}

static fsw_status_t
fsw_hfsplus_dir_read(struct fsw_hfsplus_volume *v, struct fsw_hfsplus_dnode *d,
                     struct fsw_shandle *sh, struct fsw_hfsplus_dnode **d_out)
{
    BTNodeDescriptor     *btnode;
    HFSPlusCatalogKey    sk, *tk;
    HFSPlusCatalogRecord *rec;
    struct fsw_string    name;
    fsw_u32              rec_num;
    int                  i;
    fsw_status_t         status;

    FSW_MSG_DEBUG((FSW_MSGSTR("FswHfsPlus: dir_read.1: parent=%ld, sh->pos=%ld\n"), d->g.dnode_id, sh->pos));

    // pre-allocate bt-node buffer for use by search function:
    status = fsw_alloc(v->catf->bt_ndsz, &btnode);
    if (status) {
        return status;
    }

    // search catalog file for first child of 'd' (a.k.a. "./"):
    sk.parentID = d->g.dnode_id;
    sk.nodeName.length = 0;
    // NOTE: keyLength not used in search, setting only for completeness:

    sk.keyLength = sizeof(sk.parentID) + sizeof(sk.nodeName.length);
    status = fsw_hfsplus_bt_search(v->catf,
                                   (HFSPlusBTKey *)&sk,
                                   fsw_hfsplus_cat_cmp,
                                   btnode, &rec_num);
    if (status) {
        goto done;
    }

    status = fsw_hfsplus_btree_get_rec(v->catf,
                                     sk.parentID,
                                     btnode,
                                     &rec_num,
                                     &sh->pos);
    if (status) {
        goto done;
    }

    tk = (HFSPlusCatalogKey *)fsw_hfsplus_btnode_get_rec(btnode, v->catf->bt_ndsz, rec_num);
    rec = fsw_hfsplus_bt_rec_skip_key((HFSPlusBTKey *)tk);

    // Fill child name
    name.type = FSW_STRING_TYPE_UTF16;
    name.len = fsw_u16_be_swap(tk->nodeName.length);
    name.size = name.len * 2;
    status = fsw_alloc(name.size, &name.data);
    if (status) {
        goto done;
    }

    FSW_MSG_DEBUG((FSW_MSGSTR("FswHfsPlus: dir_read.2: child name: ")));
    for (i = 0; i < name.len; i++) {
        ((fsw_u16 *) name.data)[i] = fsw_u16_be_swap(tk->nodeName.unicode[i]);
        FSW_MSG_DEBUG((FSW_MSGSTR("%c"), ((fsw_u16 *) name.data)[i]));
    }
    FSW_MSG_DEBUG((FSW_MSGSTR("\n")));

    status = fsw_hfsplus_dnode_create_full(rec, d, &name, d_out);
    fsw_strfree(&name);

done:
    fsw_free(btnode);
    return status;
}

static fsw_status_t
fsw_hfsplus_itoa(fsw_u32 num, char** dest, int* len)
{
    fsw_status_t status;
    int i, j;
    int num_copy;
    char c;

    (*len) = 0;

    // Pre-calculating string length
    num_copy = num;
    do {
        ++(*len);
    } while ((num_copy /= 10) > 0);

    // Allocating memory for the string
    status = fsw_alloc_zero(sizeof(char) * (*len), (void **) dest);
    if (status) {
        return status;
    }

    // Converting
    i = 0;
    do {
        (*dest)[i++] = num % 10 + '0';
    } while ((num /= 10) > 0);

    // Reversing the string
    for (i = 0, j = *len - 1; i < j; i++, j--) {
        c = (*dest)[i];
        (*dest)[i] = (*dest)[j];
        (*dest)[j] = c;
    }
    return status;
}

static fsw_status_t
fsw_hfsplus_sconcat(const char *const s1, int len1, const char *const s2, int len2, char **dest)
{
    int i;
    fsw_status_t status;

    status = fsw_alloc_zero(sizeof(char) * (len1 + len2), (void **) dest);
    if (status) {
        return status;
    }

    for (i = 0; i < len1; ++i) {
        (*dest)[i] = s1[i];
    }

    for (i = 0; i < len2; ++i) {
        (*dest)[i + len1] = s2[i];
    }
    return status;
}

static fsw_status_t
fsw_hfsplus_readlink(struct fsw_hfsplus_volume *v, struct fsw_hfsplus_dnode *d,
                     struct fsw_string *lnk_tgt)
{
    // Note:
    //   We assume the size = length * sizeof(char)
    //   And NO strings are null-terminated

    static const char hlink_prefix[] = "/\0\0\0\0HFS+ Private Data/iNode";
    static const int hlink_prefix_len = 28;

    fsw_status_t status;
    char *buf;
    int buf_len;

    if(d->fd_creator == kHFSPlusHFSPlusCreator && d->fd_type == kHFSPlusHardlinkType) {
        status = fsw_hfsplus_itoa(d->inode_num, &buf, &buf_len);
        if (status) {
            return status;
        }

        status = fsw_hfsplus_sconcat(hlink_prefix, hlink_prefix_len, buf, buf_len, (char**) &lnk_tgt->data);
        if (status) {
            return status;
        }

        lnk_tgt->type = FSW_STRING_TYPE_ISO88591;
        lnk_tgt->size = (hlink_prefix_len + buf_len) * sizeof(char);
        lnk_tgt->len = hlink_prefix_len + buf_len;
        fsw_free(buf);

        FSW_MSG_DEBUG((FSW_MSGSTR("FswHfsPlus: readlink: inode no: %d\n"), d->g.dnode_id));
        return FSW_SUCCESS;
    } else if (d->fd_creator == kHFSPlusSymlinkCreator && d->fd_type == kHFSPlusSymlinkType) {
        return fsw_dnode_readlink_data(d, lnk_tgt);
    }
    return FSW_UNSUPPORTED;
}

static fsw_status_t
fsw_hfsplus_dnid2thread(struct fsw_hfsplus_volume *v, fsw_u32 dnid,
        BTNodeDescriptor *btnode, HFSPlusCatalogThread **thread_out)
{
    fsw_status_t            status;
    HFSPlusCatalogKey       sk, *tk;
    fsw_u32                 rec_num;

    // Known parent id is enough to find a thread
    sk.parentID = dnid;
    status = fsw_hfsplus_bt_search(v->catf,
                                   (HFSPlusBTKey *)&sk,
                                   fsw_hfsplus_thread_cmp,
                                   btnode, &rec_num);
    if (status) {
        return status;
    }
    
    tk = (HFSPlusCatalogKey *) fsw_hfsplus_btnode_get_rec(btnode, v->catf->bt_ndsz, rec_num);
    *thread_out = (HFSPlusCatalogThread *) fsw_hfsplus_bt_rec_skip_key((HFSPlusBTKey *)tk);
    return FSW_SUCCESS;
}

static fsw_status_t
fsw_hfsplus_thread2dnode(struct fsw_hfsplus_volume *v, HFSPlusCatalogThread *thread,
        BTNodeDescriptor *btnode, struct fsw_hfsplus_dnode **d_out)
{
    fsw_status_t            status;
    struct fsw_string       name;
    fsw_u32                 rec_num;
    HFSPlusCatalogKey       sk, *tk;
    HFSPlusCatalogRecord    *rec;

    // Prepare search key
    sk.parentID = fsw_u32_be_swap(thread->parentID);

    name.len = fsw_u16_be_swap(thread->nodeName.length);
    name.size = sizeof(fsw_u16) * name.len;
    name.data = thread->nodeName.unicode;
    name.type = FSW_STRING_TYPE_UTF16_BE;
    
    status = fsw_hfsplus_fswstr2unistr(&(sk.nodeName), &name);
    if (status) {
        return status;
    }

    // Try to find btree node by its parent id and name
    status = fsw_hfsplus_bt_search(v->catf,
                                   (HFSPlusBTKey *)&sk,
                                   fsw_hfsplus_cat_cmp,
                                   btnode, &rec_num);
    if (status) {
        return status;
    }
    
    tk = (HFSPlusCatalogKey *) fsw_hfsplus_btnode_get_rec(btnode, v->catf->bt_ndsz, rec_num);
    rec = (HFSPlusCatalogRecord *) fsw_hfsplus_bt_rec_skip_key((HFSPlusBTKey *)tk);
    
    // Just a sanity check
    switch (fsw_u16_be_swap(rec->recordType)) {
        case kHFSPlusFolderRecord:
        case kHFSPlusFileRecord:
            break;
        default:
            return FSW_NOT_FOUND;
    }

    // We have same dnode id field position
    // for file (rec->fileRecord.fileID)
    // and folder (rec->folderRecord.folderID), so use one of them
    status = fsw_hfsplus_dnode_create_minimal(v,
                                              fsw_u32_be_swap(thread->parentID),
                                              fsw_u32_be_swap(rec->folderRecord.folderID),
                                              &name, d_out);
    if (status) {
        return status;
    }

    return status;
}

static fsw_status_t
fsw_hfsplus_dnid2dnode(struct fsw_hfsplus_volume *v, fsw_u32 dnid, struct fsw_hfsplus_dnode **d_out)
{
    fsw_status_t status;
    BTNodeDescriptor *btnode_thread, *btnode_dnode;
    HFSPlusCatalogThread *thread;

    status = fsw_alloc(v->catf->bt_ndsz, &btnode_thread);
    if (status) {
        return status;
    }

    status = fsw_alloc(v->catf->bt_ndsz, &btnode_dnode);
    if (status) {
        goto free_btnode_thread;
    }

    status = fsw_hfsplus_dnid2thread(v, dnid, btnode_thread, &thread);
    if (status) {
        goto free_all;
    }

    status = fsw_hfsplus_thread2dnode(v, thread, btnode_dnode, d_out);

free_all:
    fsw_free(btnode_dnode);

free_btnode_thread:
    fsw_free(btnode_thread);

    return status;
}

static fsw_status_t 
fsw_hfsplus_bless_type2dnid(struct fsw_hfsplus_volume *vol,
                                        fsw_u32 bless_type,
                                        fsw_u32 *dnid)
{
    HFSPlusVolumeFinderInfo     *finderInfo;

    finderInfo = (HFSPlusVolumeFinderInfo*) &vol->vh->finderInfo;
    switch (bless_type) {
        case BLESSED_TYPE_SYSTEM_FILE:
            *dnid = fsw_u32_be_swap(finderInfo->blessedSystemFileID);
            break;
        case BLESSED_TYPE_SYSTEM_FOLDER:
            *dnid = fsw_u32_be_swap(finderInfo->blessedSystemFolderID);
            break;
        case BLESSED_TYPE_OSX_FOLDER:
            *dnid = fsw_u32_be_swap(finderInfo->blessedOSXFolderID);
            break;
        default:
            return FSW_NOT_FOUND;
            break;
    }

    return FSW_SUCCESS;
}

static fsw_status_t
fsw_hfsplus_get_bless_info(struct fsw_hfsplus_volume *vol,
                            fsw_u32 bless_type, 
                            struct fsw_hfsplus_dnode **dno_out) {
    fsw_status_t                status;
    fsw_u32                     dnid;

    status = fsw_hfsplus_bless_type2dnid(vol, bless_type, &dnid);
    if (status)
        return status;

    status = fsw_hfsplus_dnid2dnode(vol, dnid, dno_out);
    return status;
}

