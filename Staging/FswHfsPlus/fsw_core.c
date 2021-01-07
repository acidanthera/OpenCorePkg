/**
 * \file fsw_core.c
 * Core file system wrapper abstraction layer code.
 */

/*-
 * Copyright (c) 2006 Christoph Pfisterer
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *  * Neither the name of Christoph Pfisterer nor the names of the
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsw_core.h"


// functions

static void fsw_blockcache_free(struct fsw_volume *vol);

#define MAX_CACHE_LEVEL (5)


/**
 * Mount a volume with a given file system driver. This function is called by the
 * host driver to make a volume accessible. The file system driver to use is specified
 * by a pointer to its dispatch table. The file system driver will look at the
 * data on the volume to determine if it can read the format. If the volume is found
 * unsuitable, FSW_UNSUPPORTED is returned.
 *
 * If this function returns FSW_SUCCESS, *vol_out points at a valid volume data
 * structure. The caller must release it later by calling fsw_unmount.
 *
 * If this function returns an error status, the caller only needs to clean up its
 * own buffers that may have been allocated through the read_block interface.
 */

fsw_status_t fsw_mount(void *host_data,
                       struct fsw_host_table *host_table,
                       struct fsw_fstype_table *fstype_table,
                       struct fsw_volume **vol_out)
{
    fsw_status_t    status;
    struct fsw_volume *vol;
    
    // allocate memory for the structure
    status = fsw_alloc_zero(fstype_table->volume_struct_size, (void **)&vol);
    if (status)
        return status;
    
    // initialize fields
    vol->phys_blocksize = 512;
    vol->log_blocksize  = 512;
    vol->label.type     = FSW_STRING_TYPE_EMPTY;
    vol->host_data      = host_data;
    vol->host_table     = host_table;
    vol->fstype_table   = fstype_table;
    vol->host_string_type = host_table->native_string_type;
    
    // let the fs driver mount the file system
    status = vol->fstype_table->volume_mount(vol);
    if (status)
        goto errorexit;
    
    // TODO: anything else?
    
    *vol_out = vol;
    return FSW_SUCCESS;
    
errorexit:
    fsw_unmount(vol);
    return status;
}

/**
 * Unmount a volume by releasing all memory associated with it. This function is
 * called by the host driver when a volume is no longer needed. It is also called
 * by the core after a failed mount to clean up any allocated memory.
 *
 * Note that all dnodes must have been released before calling this function.
 */

void fsw_unmount(struct fsw_volume *vol)
{
    if (vol->root)
        fsw_dnode_release(vol->root);
    // TODO: check that no other dnodes are still around
    
    vol->fstype_table->volume_free(vol);
    
    fsw_blockcache_free(vol);
    fsw_strfree(&vol->label);
    fsw_free(vol);
}

/**
 * Get in-depth information on the volume. This function can be called by the host
 * driver to get additional information on the volume.
 */

fsw_status_t fsw_volume_stat(struct fsw_volume *vol, struct fsw_volume_stat *sb)
{
    return vol->fstype_table->volume_stat(vol, sb);
}

/**
 * Set the physical and logical block sizes of the volume. This functions is called by
 * the file system driver to announce the block sizes it wants to use for accessing
 * the disk (physical) and for addressing file contents (logical).
 * Usually both sizes will be the same but there may be file systems that need to access
 * metadata at a smaller block size than the allocation unit for files.
 *
 * Calling this function causes the block cache to be dropped. All pointers returned
 * from fsw_block_get become invalid. This function should only be called while
 * mounting the file system, not as a part of file access operations.
 *
 * Both sizes are measured in bytes, must be powers of 2, and must not be smaller
 * than 512 bytes. The logical block size cannot be smaller than the physical block size.
 */

void fsw_set_blocksize(struct fsw_volume *vol, fsw_u32 phys_blocksize, fsw_u32 log_blocksize)
{
    // TODO: Check the sizes. Both must be powers of 2. log_blocksize must not be smaller than
    //  phys_blocksize.
    
    // drop core block cache if present
    fsw_blockcache_free(vol);
    
    // signal host driver to drop caches etc.
    vol->host_table->change_blocksize(vol,
                                      vol->phys_blocksize, vol->log_blocksize,
                                      phys_blocksize, log_blocksize);
    
    vol->phys_blocksize = phys_blocksize;
    vol->log_blocksize = log_blocksize;
}

/**
 * Get a block of data from the disk. This function is called by the file system driver
 * or by core functions. It calls through to the host driver's device access routine.
 * Given a physical block number, it reads the block into memory (or fetches it from the
 * block cache) and returns the address of the memory buffer. The caller should provide
 * an indication of how important the block is in the cache_level parameter. Blocks with
 * a low level are purged first. Some suggestions for cache levels:
 *
 *  - 0: File data
 *  - 1: Directory data, symlink data
 *  - 2: File system metadata
 *  - 3..5: File system metadata with a high rate of access
 *
 * If this function returns successfully, the returned data pointer is valid until the
 * caller calls fsw_block_release.
 */

fsw_status_t fsw_block_get(struct VOLSTRUCTNAME *vol, fsw_u32 phys_bno, fsw_u32 cache_level, void **buffer_out)
{
    fsw_status_t    status;
    fsw_u32         i, discard_level, new_bcache_size;
    struct fsw_blockcache *new_bcache;
    
    // TODO: allow the host driver to do its own caching; just call through if
    //  the appropriate function pointers are set
    
    if (cache_level > MAX_CACHE_LEVEL)
        cache_level = MAX_CACHE_LEVEL;
    
    // check block cache
    for (i = 0; i < vol->bcache_size; i++) {
        if (vol->bcache[i].phys_bno == phys_bno) {
            // cache hit!
            if (vol->bcache[i].cache_level < cache_level)
                vol->bcache[i].cache_level = cache_level;  // promote the entry
            vol->bcache[i].refcount++;
            *buffer_out = vol->bcache[i].data;
            return FSW_SUCCESS;
        }
    }
    
    // find a free entry in the cache table
    for (i = 0; i < vol->bcache_size; i++) {
        if (vol->bcache[i].phys_bno == FSW_INVALID_BNO)
            break;
    }
    if (i >= vol->bcache_size) {
        for (discard_level = 0; discard_level <= MAX_CACHE_LEVEL; discard_level++) {
            for (i = 0; i < vol->bcache_size; i++) {
                if (vol->bcache[i].refcount == 0 && vol->bcache[i].cache_level <= discard_level)
                    break;
            }
            if (i < vol->bcache_size)
                break;
        }
    }
    if (i >= vol->bcache_size) {
        // enlarge / create the cache
        if (vol->bcache_size < 16)
            new_bcache_size = 16;
        else
            new_bcache_size = vol->bcache_size << 1;
        status = fsw_alloc(new_bcache_size * sizeof(struct fsw_blockcache), &new_bcache);
        if (status)
            return status;
        if (vol->bcache_size > 0)
            fsw_memcpy(new_bcache, vol->bcache, vol->bcache_size * sizeof(struct fsw_blockcache));
        for (i = vol->bcache_size; i < new_bcache_size; i++) {
            new_bcache[i].refcount = 0;
            new_bcache[i].cache_level = 0;
            new_bcache[i].phys_bno = FSW_INVALID_BNO;
            new_bcache[i].data = NULL;
        }
        i = vol->bcache_size;
        
        // switch caches
        if (vol->bcache != NULL)
            fsw_free(vol->bcache);
        vol->bcache = new_bcache;
        vol->bcache_size = new_bcache_size;
    }
    vol->bcache[i].phys_bno = FSW_INVALID_BNO;
    
    // read the data
    if (vol->bcache[i].data == NULL) {
        status = fsw_alloc(vol->phys_blocksize, &vol->bcache[i].data);
        if (status)
            return status;
    }
    status = vol->host_table->read_block(vol, phys_bno, vol->bcache[i].data);
    if (status)
        return status;
    
    vol->bcache[i].phys_bno = phys_bno;
    vol->bcache[i].cache_level = cache_level;
    vol->bcache[i].refcount = 1;
    *buffer_out = vol->bcache[i].data;
    return FSW_SUCCESS;
}

/**
 * Releases a disk block. This function must be called to release disk blocks returned
 * from fsw_block_get.
 */

void fsw_block_release(struct VOLSTRUCTNAME *vol, fsw_u32 phys_bno, void *buffer)
{
    fsw_u32 i;
    
    // TODO: allow the host driver to do its own caching; just call through if
    //  the appropriate function pointers are set
    
    // update block cache
    for (i = 0; i < vol->bcache_size; i++) {
        if (vol->bcache[i].phys_bno == phys_bno && vol->bcache[i].refcount > 0)
            vol->bcache[i].refcount--;
    }
}

/**
 * Release the block cache. Called internally when changing block sizes and when
 * unmounting the volume. It frees all data occupied by the generic block cache.
 */

static void fsw_blockcache_free(struct fsw_volume *vol)
{
    fsw_u32 i;
    
    for (i = 0; i < vol->bcache_size; i++) {
        if (vol->bcache[i].data != NULL)
            fsw_free(vol->bcache[i].data);
    }
    if (vol->bcache != NULL) {
        fsw_free(vol->bcache);
        vol->bcache = NULL;
    }
    vol->bcache_size = 0;
}

/**
 * Add a new dnode to the list of known dnodes. This internal function is used when a
 * dnode is created to add it to the dnode list that is used to search for existing
 * dnodes by id.
 */

static void fsw_dnode_register(struct fsw_volume *vol, struct fsw_dnode *dno)
{
    dno->next = vol->dnode_head;
    if (vol->dnode_head != NULL)
        vol->dnode_head->prev = dno;
    dno->prev = NULL;
    vol->dnode_head = dno;
}

/**
 * Create a dnode representing the root directory. This function is called by the file system
 * driver while mounting the file system. The root directory is special because it has no parent
 * dnode, its name is defined to be empty, and its type is also fixed. Otherwise, this functions
 * behaves in the same way as fsw_dnode_create.
 */

fsw_status_t fsw_dnode_create_root(struct fsw_volume *vol, fsw_u32 dnode_id, struct fsw_dnode **dno_out)
{
    fsw_status_t    status;
    struct fsw_dnode *dno;

    // allocate memory for the structure
    status = fsw_alloc_zero(vol->fstype_table->dnode_struct_size, (void **)&dno);
    if (status)
        return status;
    
    // fill the structure
    dno->vol = vol;
    dno->parent = NULL;
    dno->dnode_id = dnode_id;
    dno->type = FSW_DNODE_TYPE_DIR;
    dno->refcount = 1;
    dno->complete = 1;
    dno->name.type = FSW_STRING_TYPE_EMPTY;
    // TODO: instead, call a function to create an empty string in the native string type
    
    fsw_dnode_register(vol, dno);
    fsw_dnode_mkcomplete(dno);

    *dno_out = dno;
    return FSW_SUCCESS;
}

int fsw_dnode_is_root(struct fsw_dnode *dno) {
    return dno->parent == NULL && dno->type == FSW_DNODE_TYPE_DIR && fsw_strlen(&dno->name) == 0;
}

/**
 * Create a new dnode representing a file system object. This function is called by
 * the file system driver in response to directory lookup or read requests. Note that
 * if there already is a dnode with the given dnode_id on record, then no new object
 * is created. Instead, the existing dnode is returned and its reference count
 * increased. All other parameters are ignored in this case.
 *
 * The type passed into this function may be FSW_DNODE_TYPE_UNKNOWN. It is sufficient
 * to fill the type field during the dnode_fill call.
 *
 * The name parameter must describe a string with the object's name. A copy will be
 * stored in the dnode structure for future reference. The name will not be used to
 * shortcut directory lookups, but may be used to reconstruct paths.
 *
 * If the function returns successfully, *dno_out contains a pointer to the dnode
 * that must be released by the caller with fsw_dnode_release.
 */

fsw_status_t fsw_dnode_create(struct fsw_volume *vol, struct fsw_dnode *parent_dno, fsw_u32 dnode_id, int type,
                              struct fsw_string *name, struct fsw_dnode **dno_out)
{
    fsw_status_t    status;
    struct fsw_dnode *dno;
    // check if we already have a dnode with the same id
    for (dno = vol->dnode_head; dno; dno = dno->next) {
        if (dno->dnode_id == dnode_id) {
            fsw_dnode_retain(dno);
            *dno_out = dno;
            return FSW_SUCCESS;
        }
    }
    
    // allocate memory for the structure
    status = fsw_alloc_zero(vol->fstype_table->dnode_struct_size, (void **)&dno);
    if (status)
        return status;

    // Flag must be set by fsw_dnode_mkcomplete() call
    dno->complete = 0;

    // fill the structure
    dno->vol = vol;
    dno->parent = parent_dno;

    // Parent may be null and filled in future by fsw_dnode_fill() call
    if (parent_dno != NULL) {
        fsw_dnode_retain(dno->parent);
    }
    dno->dnode_id = dnode_id;
    dno->type = type;
    dno->refcount = 1;
    status = fsw_strdup_coerce(&dno->name, vol->host_table->native_string_type, name);
    if (status) {
        fsw_free(dno);
        return status;
    }
    
    fsw_dnode_register(vol, dno);
    
    *dno_out = dno;
    return FSW_SUCCESS;
}

/**
 * Mark dnode as 'complete' one
 */
void fsw_dnode_mkcomplete(struct fsw_dnode *dno) {
    dno->complete = 1;
}

/**
 * Increases the reference count of a dnode. This must be balanced with
 * fsw_dnode_release calls. Note that some dnode functions return a retained
 * dnode pointer to their caller.
 */

void fsw_dnode_retain(struct fsw_dnode *dno)
{
    dno->refcount++;
}

/**
 * Release a dnode pointer, deallocating it if this was the last reference.
 * This function decrements the reference counter of the dnode. If the counter
 * reaches zero, the dnode is freed. Since the parent dnode is released
 * during that process, this function may cause it to be freed, too.
 */

void fsw_dnode_release(struct fsw_dnode *dno)
{
    struct fsw_volume *vol = dno->vol;
    struct fsw_dnode *parent_dno;
    
    dno->refcount--;
    
    if (dno->refcount == 0) {
        parent_dno = dno->parent;
        
        // de-register from volume's list
        if (dno->next)
            dno->next->prev = dno->prev;
        if (dno->prev)
            dno->prev->next = dno->next;
        if (vol->dnode_head == dno)
            vol->dnode_head = dno->next;
        
        // run fstype-specific cleanup
        vol->fstype_table->dnode_free(vol, dno);
        
        fsw_strfree(&dno->name);
        fsw_free(dno);
        
        // release our pointer to the parent, possibly deallocating it, too
        if (parent_dno)
            fsw_dnode_release(parent_dno);
    }
}

/**
 * Get full information about a dnode from disk. This function is called by the host
 * driver as well as by the core functions. Some file systems defer reading full
 * information on a dnode until it is actually needed (i.e. separation between
 * directory and inode information). This function makes sure that all information
 * is available in the dnode structure. The following fields may not have a correct
 * value until fsw_dnode_fill has been called:
 *
 * type, size
 */

fsw_status_t fsw_dnode_fill(struct fsw_dnode *dno)
{
    // TODO: check a flag right here, call fstype's dnode_fill only once per dnode
    if (dno->complete) {
        return FSW_SUCCESS;
    }
    return dno->vol->fstype_table->dnode_fill(dno->vol, dno);
}

/**
 * Get extended information about a dnode. This function can be called by the host
 * driver to get a full compliment of information about a dnode in addition to the
 * fields of the fsw_dnode structure itself.
 *
 * Some data requires host-specific conversion to be useful (i.e. timestamps) and
 * will be passed to callback functions instead of being written into the structure.
 * These callbacks must be filled in by the caller.
 */

fsw_status_t fsw_dnode_stat(struct fsw_dnode *dno, struct fsw_dnode_stat *sb)
{
    fsw_status_t    status;
    
    status = fsw_dnode_fill(dno);
    if (status)
        return status;
    
    sb->used_bytes = 0;
    status = dno->vol->fstype_table->dnode_stat(dno->vol, dno, sb);
    if (!status && !sb->used_bytes)
        sb->used_bytes = DivU64x32(dno->size + dno->vol->log_blocksize - 1, dno->vol->log_blocksize, NULL);
    return status;
}

/**
 * Lookup a directory entry by name. This function is called by the host driver.
 * Given a directory dnode and a file name, it looks up the named entry in the
 * directory.
 *
 * If the dnode is not a directory, the call will fail. The caller is responsible for
 * resolving symbolic links before calling this function.
 *
 * If the function returns FSW_SUCCESS, *child_dno_out points to the requested directory
 * entry. The caller must call fsw_dnode_release on it.
 */

fsw_status_t fsw_dnode_lookup(struct fsw_dnode *dno,
                              struct fsw_string *lookup_name, struct fsw_dnode **child_dno_out)
{
    fsw_status_t    status;
    
    status = fsw_dnode_fill(dno);
    if (status)
        return status;
    if (dno->type != FSW_DNODE_TYPE_DIR)
        return FSW_UNSUPPORTED;
    
    return dno->vol->fstype_table->dir_lookup(dno->vol, dno, lookup_name, child_dno_out);
}

/**
 * Find a file system object by path. This function is called by the host driver.
 * Given a directory dnode and a relative or absolute path, it walks the directory
 * tree until it finds the target dnode. If an intermediate node turns out to be
 * a symlink, it is resolved automatically. If the target node is a symlink, it
 * is not resolved.
 *
 * If the function returns FSW_SUCCESS, *child_dno_out points to the requested directory
 * entry. The caller must call fsw_dnode_release on it.
 */

fsw_status_t fsw_dnode_lookup_path(struct fsw_dnode *dno,
                                   struct fsw_string *lookup_path, char separator,
                                   struct fsw_dnode **child_dno_out)
{
    fsw_status_t    status;
    struct fsw_volume *vol = dno->vol;
    struct fsw_dnode *child_dno = NULL;
    struct fsw_string lookup_name;
    struct fsw_string remaining_path;
    int             root_if_empty;
    
    remaining_path = *lookup_path;
    fsw_dnode_retain(dno);
    
    // loop over the path
    for (root_if_empty = 1; fsw_strlen(&remaining_path) > 0; root_if_empty = 0) {
        // parse next path component
        fsw_strsplit(&lookup_name, &remaining_path, separator);
        
        FSW_MSG_DEBUG((FSW_MSGSTR("fsw_dnode_lookup_path: split into %d '%s' and %d '%s'\n"),
                       lookup_name.len, lookup_name.data,
                       remaining_path.len, remaining_path.data));
        
        if (fsw_strlen(&lookup_name) == 0) {        // empty path component
            if (root_if_empty)
                child_dno = vol->root;
            else
                child_dno = dno;
            fsw_dnode_retain(child_dno);
            
        } else {
            // do an actual directory lookup
            
            // ensure we have full information
            status = fsw_dnode_fill(dno);
            if (status)
                goto errorexit;
            
            // resolve symlink if necessary
            if (dno->type == FSW_DNODE_TYPE_SYMLINK) {
                status = fsw_dnode_resolve(dno, &child_dno);
                if (status)
                    goto errorexit;
                
                // symlink target becomes the new dno
                fsw_dnode_release(dno);
                dno = child_dno;   // is already retained
                child_dno = NULL;
                
                // ensure we have full information
                status = fsw_dnode_fill(dno);
                if (status)
                    goto errorexit;
            }
            
            // make sure we operate on a directory
            if (dno->type != FSW_DNODE_TYPE_DIR) {
                return FSW_UNSUPPORTED;
                goto errorexit;
            }
            
            // check special paths
            if (fsw_streq_cstr(&lookup_name, ".")) {    // self directory
                child_dno = dno;
                fsw_dnode_retain(child_dno);
                
            } else if (fsw_streq_cstr(&lookup_name, "..")) {   // parent directory
                if (dno->parent == NULL) {
                    // We cannot go up from the root directory. Caution: Certain apps like the EFI shell
                    // rely on this behaviour!
                    status = FSW_NOT_FOUND;
                    goto errorexit;
                }
                child_dno = dno->parent;
                fsw_dnode_retain(child_dno);
                
            } else {
                // do an actual lookup
                status = vol->fstype_table->dir_lookup(vol, dno, &lookup_name, &child_dno);
                if (status)
                    goto errorexit;
            }
        }
        
        // child_dno becomes the new dno
        fsw_dnode_release(dno);
        dno = child_dno;   // is already retained
        child_dno = NULL;
        
        FSW_MSG_DEBUG((FSW_MSGSTR("fsw_dnode_lookup_path: now at inode %d\n"), dno->dnode_id));
    }
    
    *child_dno_out = dno;
    return FSW_SUCCESS;
    
errorexit:
    FSW_MSG_DEBUG((FSW_MSGSTR("fsw_dnode_lookup_path: leaving with error %d\n"), status));
    fsw_dnode_release(dno);
    if (child_dno != NULL)
        fsw_dnode_release(child_dno);
    return status;
}

/**
 * Get the next directory item in sequential order. This function is called by the
 * host driver to read the complete contents of a directory in sequential (file system
 * defined) order. Calling this function returns the next entry. Iteration state is
 * kept by a shandle on the directory's dnode. The caller must set up the shandle
 * when starting the iteration.
 *
 * When the end of the directory is reached, this function returns FSW_NOT_FOUND.
 * If the function returns FSW_SUCCESS, *child_dno_out points to the next directory
 * entry. The caller must call fsw_dnode_release on it.
 */

fsw_status_t fsw_dnode_dir_read(struct fsw_shandle *shand, struct fsw_dnode **child_dno_out)
{
    fsw_status_t    status;
    struct fsw_dnode *dno = shand->dnode;
    fsw_u64         saved_pos;
    
    if (dno->type != FSW_DNODE_TYPE_DIR)
        return FSW_UNSUPPORTED;
    
    saved_pos = shand->pos;
    status = dno->vol->fstype_table->dir_read(dno->vol, dno, shand, child_dno_out);
    if (status)
        shand->pos = saved_pos;
    return status;
}

/**
 * Read the target path of a symbolic link. This function is called by the host driver
 * to read the "content" of a symbolic link, that is the relative or absolute path
 * it points to.
 *
 * If the function returns FSW_SUCCESS, the string handle provided by the caller is
 * filled with a string in the host's preferred encoding. The caller is responsible
 * for calling fsw_strfree on the string.
 */

fsw_status_t fsw_dnode_readlink(struct fsw_dnode *dno, struct fsw_string *target_name)
{
    fsw_status_t    status;
    
    status = fsw_dnode_fill(dno);
    if (status)
        return status;
    if (dno->type != FSW_DNODE_TYPE_SYMLINK)
        return FSW_UNSUPPORTED;
    
    return dno->vol->fstype_table->readlink(dno->vol, dno, target_name);
}

/**
 * Read the target path of a symbolic link by accessing file data. This function can
 * be called by the file system driver if the file system stores the target path
 * as normal file data. This function will open an shandle, read the whole content
 * of the file into a buffer, and build a string from that. Currently the encoding
 * for the string is fixed as FSW_STRING_TYPE_ISO88591.
 *
 * If the function returns FSW_SUCCESS, the string handle provided by the caller is
 * filled with a string in the host's preferred encoding. The caller is responsible
 * for calling fsw_strfree on the string.
 */

fsw_status_t fsw_dnode_readlink_data(struct fsw_dnode *dno, struct fsw_string *link_target)
{
    fsw_status_t    status;
    struct fsw_shandle shand;
    fsw_u32         buffer_size;
    char            buffer[FSW_PATH_MAX];
    struct fsw_string s;
    
    if (dno->size > FSW_PATH_MAX)
        return FSW_VOLUME_CORRUPTED;
    
    s.type = FSW_STRING_TYPE_ISO88591;
    s.size = s.len = (int)dno->size;
    s.data = buffer;
    
    // open shandle and read the data
    status = fsw_shandle_open(dno, &shand);
    if (status)
        return status;
    buffer_size = (fsw_u32)s.size;
    status = fsw_shandle_read(&shand, &buffer_size, buffer);
    fsw_shandle_close(&shand);
    if (status)
        return status;
    if ((int)buffer_size < s.size)
        return FSW_VOLUME_CORRUPTED;
    
    status = fsw_strdup_coerce(link_target, dno->vol->host_string_type, &s);
    return status;
}

/**
 * Resolve a symbolic link. This function can be called by the host driver to make
 * sure the a dnode is fully resolved instead of pointing at a symlink. If the dnode
 * passed in is not a symlink, it is returned unmodified.
 *
 * Note that absolute paths will be resolved relative to the root directory of the
 * volume. If the host is an operating system with its own VFS layer, it should
 * resolve symlinks on its own.
 *
 * If the function returns FSW_SUCCESS, *target_dno_out points at a dnode that is
 * not a symlink. The caller is responsible for calling fsw_dnode_release on it.
 */

fsw_status_t fsw_dnode_resolve(struct fsw_dnode *dno, struct fsw_dnode **target_dno_out)
{
    fsw_status_t    status;
    struct fsw_string target_name;
    struct fsw_dnode *target_dno;
    
    fsw_dnode_retain(dno);
    
    while (1) {
        // get full information
        status = fsw_dnode_fill(dno);
        if (status)
            goto errorexit;
        if (dno->type != FSW_DNODE_TYPE_SYMLINK) {
            // found a non-symlink target, return it
            *target_dno_out = dno;
            return FSW_SUCCESS;
        }
        if (dno->parent == NULL) {    // safety measure, cannot happen in theory
            status = FSW_NOT_FOUND;
            goto errorexit;
        }
        
        // read the link's target
        status = fsw_dnode_readlink(dno, &target_name);
        if (status)
            goto errorexit;
        
        // resolve it
        status = fsw_dnode_lookup_path(dno->parent, &target_name, '/', &target_dno);
        fsw_strfree(&target_name);
        if (status)
            goto errorexit;
        
        // target_dno becomes the new dno
        fsw_dnode_release(dno);
        dno = target_dno;   // is already retained
    }
    
errorexit:
    fsw_dnode_release(dno);
    return status;
}

/**
 * Set up a shandle (storage handle) to access a file's data. This function is called
 * by the host driver and by the core when they need to access a file's data. It is also
 * used in accessing the raw data of directories and symlinks if the file system uses
 * the same mechanisms for storing the data of those items.
 *
 * The storage for the fsw_shandle structure is provided by the caller. The dnode and pos
 * fields may be accessed, pos may also be written to to set the file pointer. The file's
 * data size is available as shand->dnode->size.
 *
 * If this function returns FSW_SUCCESS, the caller must call fsw_shandle_close to release
 * the dnode reference held by the shandle.
 */

fsw_status_t fsw_shandle_open(struct fsw_dnode *dno, struct fsw_shandle *shand)
{
    fsw_status_t    status;

    // read full dnode information into memory
    status = fsw_dnode_fill(dno);
    if (status)
        return status;
    
    // setup shandle
    fsw_dnode_retain(dno);
    
    shand->dnode = dno;
    shand->pos = 0;
    shand->extent.type = FSW_EXTENT_TYPE_INVALID;
    
    return FSW_SUCCESS;
}

/**
 * Close a shandle after accessing the dnode's data. This function is called by the host
 * driver or core functions when they are finished with accessing a file's data. It
 * releases the dnode reference and frees any buffers associated with the shandle itself.
 * The dnode is only released if this was the last reference using it.
 */

void fsw_shandle_close(struct fsw_shandle *shand)
{
    if (shand->extent.type == FSW_EXTENT_TYPE_BUFFER)
        fsw_free(shand->extent.buffer);
    fsw_dnode_release(shand->dnode);
}

/**
 * Read data from a shandle (storage handle for a dnode). This function is called by the
 * host driver or internally when data is read from a file. TODO: more
 */

fsw_status_t fsw_shandle_read(struct fsw_shandle *shand, fsw_u32 *buffer_size_inout, void *buffer_in)
{
    fsw_status_t    status;
    struct fsw_dnode *dno = shand->dnode;
    struct fsw_volume *vol = dno->vol;
    fsw_u8          *buffer, *block_buffer;
    fsw_u32         buflen, copylen, pos;
    fsw_u32         log_bno, pos_in_extent, phys_bno, pos_in_physblock;
    fsw_u32         cache_level;
    
    if (shand->pos >= dno->size) {   // already at EOF
        *buffer_size_inout = 0;
        return FSW_SUCCESS;
    }
    
    // initialize vars
    buffer = buffer_in;
    buflen = *buffer_size_inout;
    pos = (fsw_u32)shand->pos;
    cache_level = (dno->type != FSW_DNODE_TYPE_FILE) ? 1 : 0;
    // restrict read to file size
    if (buflen > dno->size - pos)
        buflen = (fsw_u32)(dno->size - pos);
    
    while (buflen > 0) {
        // get extent for the current logical block
        log_bno = pos / vol->log_blocksize;
        if (shand->extent.type == FSW_EXTENT_TYPE_INVALID ||
            log_bno < shand->extent.log_start ||
            log_bno >= shand->extent.log_start + shand->extent.log_count) {
            
            if (shand->extent.type == FSW_EXTENT_TYPE_BUFFER)
                fsw_free(shand->extent.buffer);
            
            // ask the file system for the proper extent
            shand->extent.log_start = log_bno;
            status = vol->fstype_table->get_extent(vol, dno, &shand->extent);
            if (status) {
                shand->extent.type = FSW_EXTENT_TYPE_INVALID;
                return status;
            }
        }
        
        pos_in_extent = pos - shand->extent.log_start * vol->log_blocksize;
        
        // dispatch by extent type
        if (shand->extent.type == FSW_EXTENT_TYPE_PHYSBLOCK) {
            // convert to physical block number and offset
            phys_bno = shand->extent.phys_start + pos_in_extent / vol->phys_blocksize;
            pos_in_physblock = pos_in_extent & (vol->phys_blocksize - 1);
            copylen = vol->phys_blocksize - pos_in_physblock;
            if (copylen > buflen)
                copylen = buflen;
            
            // get one physical block
            status = fsw_block_get(vol, phys_bno, cache_level, (void **)&block_buffer);
            if (status)
                return status;
            
            // copy data from it
            fsw_memcpy(buffer, block_buffer + pos_in_physblock, copylen);
            fsw_block_release(vol, phys_bno, block_buffer);
            
        } else if (shand->extent.type == FSW_EXTENT_TYPE_BUFFER) {
            copylen = shand->extent.log_count * vol->log_blocksize - pos_in_extent;
            if (copylen > buflen)
                copylen = buflen;
            fsw_memcpy(buffer, (fsw_u8 *)shand->extent.buffer + pos_in_extent, copylen);
            
        } else {   // _SPARSE or _INVALID
            copylen = shand->extent.log_count * vol->log_blocksize - pos_in_extent;
            if (copylen > buflen)
                copylen = buflen;
            fsw_memzero(buffer, copylen);
            
        }
        
        buffer += copylen;
        buflen -= copylen;
        pos    += copylen;
    }
    
    *buffer_size_inout = (fsw_u32)(pos - shand->pos);
    shand->pos = pos;
    
    return FSW_SUCCESS;
}

fsw_status_t fsw_dnode_get_path(struct fsw_volume *vol, struct fsw_dnode *dno, struct fsw_string *out_path) {
    fsw_status_t        status;
    struct fsw_string   *array, *temp_arr_ptr;
    fsw_u32             array_capacity, array_len;
    fsw_s32             i;
    fsw_u16             *temp_char_ptr;

    // Simple growable array
#define START_BUF_SIZE 5
    array_capacity = START_BUF_SIZE;
    array_len = 0;
    temp_arr_ptr = NULL;
    status = fsw_alloc(array_capacity * sizeof(struct fsw_string), &array);
    if (status) {
        return status;
    }
#undef START_BUF_SIZE

    out_path->len = 0;
    out_path->size = 0;

    while (status == FSW_SUCCESS && !fsw_dnode_is_root(dno)) {
        // Extend array capacity if needed
        if (array_len == array_capacity) {
            temp_arr_ptr = array;
            array_capacity *= 2;
            status = fsw_alloc(array_capacity * sizeof(struct fsw_string), &array);

            if (!status) {
                // Copy array contents
                for (i = 0; i < array_len; ++i)
                    array[i] = temp_arr_ptr[i];

                // Free the old pool
                fsw_free(temp_arr_ptr);
                temp_arr_ptr = NULL;
            } else
                // Store actual pointer into 'array'
                array = temp_arr_ptr;
        }

        // If no errors - copy dnode name
        if (status == FSW_SUCCESS)
            status = fsw_strdup_coerce(&array[array_len], FSW_STRING_TYPE_UTF16, &dno->name);

        // On any error free all allocated memory
        if (status)
            goto done;

        out_path->len += array[array_len].len + 1;

        ++array_len;
        dno = dno->parent;
    }

#define SEP_CHAR L'\\'
    out_path->type = FSW_STRING_TYPE_UTF16;

    if (out_path->len == 0) {
        out_path->len = 1;
    }

    out_path->size = out_path->len * sizeof(fsw_u16);
    status = fsw_alloc(out_path->size, &out_path->data);
    if (status)
        goto done;

    temp_char_ptr = (fsw_u16 *) out_path->data;
    if (array_len > 0) {
        for (i = array_len - 1; i >= 0; --i) {
            *(temp_char_ptr++) = SEP_CHAR;
            fsw_memcpy(temp_char_ptr, array[i].data, array[i].size);
            temp_char_ptr += array[i].len;
        }
    } else {
        *temp_char_ptr = SEP_CHAR;
    }

#undef SEP_CHAR

done:
    for (int i = 0; i < array_len; ++i)
       fsw_strfree(&array[i]);
    fsw_free(array);

    return status;
}

fsw_status_t fsw_get_bless_info(struct fsw_volume *vol, int type, struct fsw_string *out_path) {
    fsw_status_t status;
    struct fsw_dnode *dnode;

    status = vol->fstype_table->get_bless_info(vol, type, &dnode);
    if (status) {
        return status;
    }

    status = fsw_dnode_get_path(vol, dnode, out_path);
    return status;
}

// EOF
