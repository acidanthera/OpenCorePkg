/**
 * \file fsw_posix.c
 * POSIX user space host environment code.
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

#include "fsw_posix.h"


#ifndef FSTYPE
/** The file system type name to use. */
#define FSTYPE ext2
#endif


// function prototypes

fsw_status_t fsw_posix_open_dno(struct fsw_posix_volume *pvol, const char *path, fsw_dnode_kind_t required_kind,
                                struct fsw_shandle *shand);

void fsw_posix_change_blocksize(struct fsw_volume *vol,
                              fsw_u32 old_phys_blocksize, fsw_u32 old_log_blocksize,
                              fsw_u32 new_phys_blocksize, fsw_u32 new_log_blocksize);
fsw_status_t fsw_posix_read_block(struct fsw_volume *vol, fsw_u32 phys_bno, void *buffer);

/**
 * Dispatch table for our FSW host driver.
 */

struct fsw_host_table   fsw_posix_host_table = {
    FSW_STRING_KIND_UTF8,

    fsw_posix_change_blocksize,
    fsw_posix_read_block
};

extern struct fsw_fstype_table   FSW_FSTYPE_TABLE_NAME(FSTYPE);


/**
 * Mount function.
 */

struct fsw_posix_volume * fsw_posix_mount(const char *path, struct fsw_fstype_table *fstype_table)
{
    fsw_status_t        status;
    struct fsw_posix_volume *pvol;

    // allocate volume structure
    status = fsw_alloc_zero(sizeof(struct fsw_posix_volume), (void **)&pvol);
    if (status != FSW_SUCCESS)
        return NULL;

    // open underlying file/device
    pvol->fd = open(path, O_RDONLY | O_BINARY, 0);
    if (pvol->fd < 0) {
        fprintf(stderr, "fsw_posix_mount: %s: %s\n", path, strerror(errno));
        fsw_free(pvol);
        return NULL;
    }

    // mount the filesystem
    if (fstype_table == NULL)
        fstype_table = &FSW_FSTYPE_TABLE_NAME(FSTYPE);
    status = fsw_mount(pvol, &fsw_posix_host_table, fstype_table, &pvol->vol);
    if (status != FSW_SUCCESS) {
        fprintf(stderr, "fsw_posix_mount: fsw_mount returned %d\n", status);
        fsw_free(pvol);
        return NULL;
    }

    return pvol;
}

/**
 * Unmount function.
 */

int fsw_posix_unmount(struct fsw_posix_volume *pvol)
{
    if (pvol->vol != NULL)
        fsw_unmount(pvol->vol);
    fsw_free(pvol);
    return 0;
}

/**
 * Open a named regular file.
 */

struct fsw_posix_file * fsw_posix_open(struct fsw_posix_volume *pvol, const char *path, int flags, mode_t mode)
{
    fsw_status_t        status;
    struct fsw_posix_file *file = NULL;

    // TODO: check flags for unwanted values

    // allocate file structure
    status = fsw_alloc_zero(sizeof(struct fsw_posix_file), (void **)&file);
    if (status != FSW_SUCCESS)
        return NULL;
    file->pvol = pvol;

    // open the file
    status = fsw_posix_open_dno(pvol, path, FSW_DNODE_KIND_FILE, &file->shand);
    if (status != FSW_SUCCESS) {
        fprintf(stderr, "fsw_posix_open: open_dno returned %d\n", status);
        fsw_free(file);
        return NULL;
    }

    return file;
}

/**
 * Read data from a regular file.
 */

ssize_t fsw_posix_read(struct fsw_posix_file *file, void *buf, size_t nbytes)
{
    fsw_status_t        status;
    fsw_u32             buffer_size;

    buffer_size = (fsw_u32)nbytes;
    status = fsw_shandle_read(&file->shand, &buffer_size, buf);
    if (status != FSW_SUCCESS)
        return -1;
    return buffer_size;
}

/**
 * Change position within a regular file.
 */

off_t fsw_posix_lseek(struct fsw_posix_file *file, off_t offset, int whence)
{
    fsw_u64             base_offset = 0;

    // get base offset
    base_offset = 0;
    if (whence == SEEK_CUR)
        base_offset = file->shand.pos;
    else if (whence == SEEK_END)
        base_offset = file->shand.dnode->size;

    // calculate new offset, prevent seeks before the start of the file
    if (offset < 0 && -offset > base_offset)
        file->shand.pos = 0;
    else
        file->shand.pos = base_offset + offset;

    return (off_t)(file->shand.pos);
}

/**
 * Close a regular file.
 */

int fsw_posix_close(struct fsw_posix_file *file)
{
    fsw_shandle_close(&file->shand);
    fsw_free(file);
    return 0;
}

/**
 * Open a directory for iteration.
 */

struct fsw_posix_dir * fsw_posix_opendir(struct fsw_posix_volume *pvol, const char *path)
{
    fsw_status_t        status;
    struct fsw_posix_dir *dir = NULL;

    // allocate file structure
    status = fsw_alloc(sizeof(struct fsw_posix_dir), &dir);
    if (status != FSW_SUCCESS)
        return NULL;
    dir->pvol = pvol;

    // open the directory
    status = fsw_posix_open_dno(pvol, path, FSW_DNODE_KIND_DIR, &dir->shand);
    if (status != FSW_SUCCESS) {
        fprintf(stderr, "fsw_posix_opendir: open_dno returned %d\n", status);
        fsw_free(dir);
        return NULL;
    }

    return dir;
}

/**
 * Read the next entry from a directory.
 */

struct dirent * fsw_posix_readdir(struct fsw_posix_dir *dir)
{
	fsw_status_t        status;
	struct fsw_dnode    *dno;
	static struct dirent dent;
	
	// get next entry from file system
	status = fsw_dnode_dir_read(&dir->shand, &dno);
	if (status != FSW_SUCCESS) {
		if (status != FSW_NOT_FOUND)
			fprintf(stderr, "fsw_posix_readdir: fsw_dnode_dir_read returned %d\n", status);
		return NULL;
	}
	status = fsw_dnode_fill(dno);
	if (status != FSW_SUCCESS) {
		fprintf(stderr, "fsw_posix_readdir: fsw_dnode_fill returned %d\n", status);
		fsw_dnode_release(dno);
		return NULL;
	}

	// fill dirent structure
	dent.d_ino = dno->dnode_id;

	switch (dno->dkind) {
		case FSW_DNODE_KIND_FILE:
			dent.d_type = DT_REG;
			break;
		case FSW_DNODE_KIND_DIR:
			dent.d_type = DT_DIR;
			break;
		case FSW_DNODE_KIND_SYMLINK:
			dent.d_type = DT_LNK;
			break;
		default:
			dent.d_type = DT_UNKNOWN;
			break;
	}

	dent.d_namlen = dno->name.size;	// XXX: chars/letters are not bytes any more
	(void) memcpy(dent.d_name, dno->name.data, dent.d_namlen);
	dent.d_name[dent.d_namlen] = 0;
	dent.d_reclen = ((char *)&dent.d_name - (char *)&dent.d_ino) + dent.d_namlen + 1;

	fsw_dnode_release(dno);

	return &dent;
}

/**
 * Rewind a directory to the start.
 */

void fsw_posix_rewinddir(struct fsw_posix_dir *dir)
{
    dir->shand.pos = 0;
}

/**
 * Close a directory.
 */

int fsw_posix_closedir(struct fsw_posix_dir *dir)
{
    fsw_shandle_close(&dir->shand);
    fsw_free(dir);
    return 0;
}

/**
 * Open a shand of a required type by path.
 */

fsw_status_t fsw_posix_open_dno(struct fsw_posix_volume *pvol, const char *path, fsw_dnode_kind_t required_kind, struct fsw_shandle *shand)
{
    fsw_status_t        status;
    struct fsw_dnode    *dno;
    struct fsw_dnode    *target_dno;
    struct fsw_string   lookup_path;
    int lplen;

    lplen = (int)strlen(path);
    fsw_string_setter(&lookup_path, FSW_STRING_KIND_ISO88591, lplen, lplen, (void *)path);

    // resolve the path (symlinks along the way are automatically resolved)

    status = fsw_dnode_lookup_path(pvol->vol->root, &lookup_path, '/', &dno);

    if (status != FSW_SUCCESS) {
        fprintf(stderr, "fsw_posix_open_dno: fsw_dnode_lookup_path returned %d\n", status);
        return status;
    }

    // if the final node is a symlink, also resolve it

    status = fsw_dnode_resolve(dno, &target_dno);
    fsw_dnode_release(dno);

    if (status != FSW_SUCCESS) {
        fprintf(stderr, "fsw_posix_open_dno: fsw_dnode_resolve returned %d\n", status);
        return status;
    }

    dno = target_dno;

    // check that it is a regular file

    status = fsw_dnode_fill(dno);

    if (status != FSW_SUCCESS) {
        fprintf(stderr, "fsw_posix_open_dno: fsw_dnode_fill returned %d\n", status);
        fsw_dnode_release(dno);
        return status;
    }

    if (dno->dkind != required_kind) {
        fprintf(stderr, "fsw_posix_open_dno: dnode is not of the requested type\n");
        fsw_dnode_release(dno);
        return FSW_UNSUPPORTED;
    }

    // open shandle

    status = fsw_shandle_open(dno, shand);

    if (status != FSW_SUCCESS) {
        fprintf(stderr, "fsw_posix_open_dno: fsw_shandle_open returned %d\n", status);
    }

    fsw_dnode_release(dno);
    return status;
}

/**
 * FSW interface function for block size changes. This function is called by the FSW core
 * when the file system driver changes the block sizes for the volume.
 */

void fsw_posix_change_blocksize(struct fsw_volume *vol,
                                fsw_u32 old_phys_blocksize, fsw_u32 old_log_blocksize,
                                fsw_u32 new_phys_blocksize, fsw_u32 new_log_blocksize)
{
    // nothing to do
}

/**
 * FSW interface function to read data blocks. This function is called by the FSW core
 * to read a block of data from the device. The buffer is allocated by the core code.
 */

fsw_status_t fsw_posix_read_block(struct fsw_volume *vol, fsw_u32 phys_bno, void *buffer)
{
    struct fsw_posix_volume *pvol = (struct fsw_posix_volume *)vol->host_data;
    off_t           block_offset, seek_result;
    ssize_t         read_result;

    // read from disk
    block_offset = (off_t)phys_bno * vol->phys_blocksize;
    seek_result = lseek(pvol->fd, block_offset, SEEK_SET);
    if (seek_result != block_offset)
        return FSW_IO_ERROR;
    read_result = read(pvol->fd, buffer, vol->phys_blocksize);
    if (read_result != vol->phys_blocksize)
        return FSW_IO_ERROR;

    return FSW_SUCCESS;
}

// EOF
