/** @file

EfiResTool -- tool to work with APPL efires archives

Copyright (c) 2018, stek29

All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

**/

#ifdef WIN32

#include <stdlib.h>
#include <stdio.h>

int main(int argc, const char* argv[]) {
    fprintf(stderr, "This utility is not yet supported on Windows\n");
    return EXIT_FAILURE;
}

#else

#include <sys/types.h>
#include <unistd.h>       // write
#include <fcntl.h>        // open, close
#include <stdio.h>        // fprintf
#include <string.h>       // strerror, strdup, strchr
#include <stdlib.h>       // abort, free, EXIT_*
#include <sys/mman.h>     // mmap, munmap
#include <sys/stat.h>     // fstat
#include <errno.h>        // errno
#include <dirent.h>       // DIR, dirent, opendir, readdir
#include <stdint.h>       // UINT32_MAX

typedef struct {
    char     name[64];
    uint32_t offset;
    uint32_t length;
}
__attribute__((packed, aligned(1)))
efires_file_t;

#define EFIRES_CURRENT_REVISION 2
typedef struct {
    uint16_t revision;   // EFIRES_CURRENT_REVISION
    uint16_t nentries;   // count of entries
    efires_file_t entries[/* nentries */];
}
__attribute__((packed, aligned(1)))
efires_hdr_t;

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define le16toh(x) (x)
#define le32toh(x) (x)
#define htole16(x) (x)
#define htole32(x) (x)
#elif defined(__APPLE__)
#include <libkern/OSByteOrder.h>
#define le16toh(x) OSSwapLittleToHostInt16(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#else
#include <endian.h>
#endif

typedef enum {
    ONLY_LIST = 1,
} unpack_flag;

int unpack_efires(const char* fname, const char* destination, unpack_flag flags, char** filelist[]);
int pack_efires(const char* fname, const char* fromdir, const char* filelist[]);

int write_filelist(const char** filelist, const char* fname);
const char** parse_filelist(const char* fname);
void free_filelist(char** filelist);

#define ACTION_UNPACK  "unpack"
#define ACTION_PACK    "pack"
#define ACTION_LIST    "list"

void print_usage(const char* prog) {
    fprintf(stderr,
        "efirestool -- tool to work with APPL efires archives\n"
        "\n"
        "Usage:\n"
        "    %s " ACTION_UNPACK " efires destination [filelist]\n"
        "    %s " ACTION_PACK " efires from [filelist]\n"
        "    %s " ACTION_LIST " efires [-f filelist]\n"
        , prog, prog, prog);
}

int main(int argc, const char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char* action = argv[1];
    const char* efires = argv[2];
    const char* directory = NULL;
    const char* filelist_fname = NULL;
    const char** filelist = NULL;

    int retval = 0;

    if (argc > 3) {
        directory = argv[3];
    }

    if (argc > 4) {
        filelist_fname = argv[4];
    }

    if ((strcmp(action, ACTION_UNPACK) == 0) || (strcmp(action, ACTION_LIST) == 0)) {
        unpack_flag flags = 0;

        if (strcmp(action, ACTION_LIST) == 0) flags |= ONLY_LIST;

        retval = unpack_efires(efires, directory, flags, (char***) ((filelist_fname) ? &filelist : NULL));

        if (!retval && filelist_fname) {
            if (filelist == NULL) {
                fprintf(stderr, "Failed to build filelist\n");
                retval = 1;
            } else {
                retval = write_filelist(filelist, filelist_fname);
            }
        }
    } else if (strcmp(action, ACTION_PACK) == 0) {
        filelist = parse_filelist(filelist_fname);

        if (filelist == NULL) {
            fprintf(stderr, "Failed to parse filelist\n");
            retval = 1;
        } else {
            retval = pack_efires(efires, directory, filelist);
        }
    } else {
        print_usage(argv[0]);
        retval = EXIT_FAILURE;
    }

    if (filelist) {
        free_filelist((char**)filelist);
    }

    return retval;
}

void free_filelist(char** filelist) {
    for (char** p = filelist; *p != NULL; ++p) {
        free((char*)*p);
    }

    free(filelist);
}

const char** parse_filelist(const char* fname) {
    FILE *f = fopen(fname, "r");
    if (f == NULL) {
        fprintf(stderr, "Cant open filelist (%s): %s\n", fname, strerror(errno));
        return NULL;
    }

    // XXX realloc and grow
    size_t res_size = sizeof(char*) * UINT32_MAX / sizeof(efires_file_t);
    char** res = malloc(res_size);

    if (res == NULL) {
        fprintf(stderr, "Cant allocate memory for filelist\n");
        fclose(f);
        return NULL;
    }

    ssize_t linelen = 0;
    char** itm = res;
    size_t n = 0;

    for (*itm = NULL, n = 0;
        (itm < (res + res_size - 1)) && ((linelen = getline(itm, &n, f)) != -1);
        ++itm, *itm = NULL, n = 0, linelen = 0)
    {
        (*itm)[linelen - 1] = '\0';
    }

    if (linelen == 0) {
        *itm = NULL;
    } else if (*itm) {
        free(*itm);
        *itm = NULL;
    }

    fclose(f);

    return (const char**)res;
}

int write_filelist(const char** filelist, const char* fname) {
    if (filelist == NULL) {
        fprintf(stderr, "Cant write NULL filelist\n");
        return 1;
    }
    FILE *f = fopen(fname, "w");

    if (f == NULL) {
        fprintf(stderr, "Cant open filelist (%s): %s\n", fname, strerror(errno));
        return 1;
    }

    for (const char** itm = filelist; *itm != NULL; ++itm) {
        fprintf(f, "%s\n", *itm);
    }

    fclose(f);
    return 0;
}

int unpack_efires(const char* fname, const char* destination, unpack_flag flags, char** filelist[]) {
    int result = 1;
    size_t file_size = 0;
    const void *file_map = NULL;

    if (filelist) *filelist = NULL;

    if (((flags & ONLY_LIST) == 0) && (destination == NULL)) {
        fprintf(stderr, "Cant determine destination\n");
        goto out;
    }

    int fd = open(fname, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Cant open resource file (%s): %s\n", fname, strerror(errno));
        goto out;
    }

    struct stat s;
    if (fstat(fd, &s) != 0) {
        fprintf(stderr, "fstat failed for (%s): %s\n", fname, strerror(errno));
        goto out;
    }

    file_size = s.st_size;

    if (file_size < sizeof(efires_hdr_t)) {
        fprintf(stderr, "File is too short to be an efires file\n");
        goto out;
    }

    file_map = mmap(NULL, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (file_map == MAP_FAILED) {
        fprintf(stderr, "Cant mmap file (%s): %s\n", fname, strerror(errno));
        file_map = NULL;
        goto out;
    }

    const efires_hdr_t *hdr = (const efires_hdr_t *) file_map;
    if (le16toh(hdr->revision) != EFIRES_CURRENT_REVISION) {
        fprintf(stderr, "Wrong efires revision: 0x%02x (expected 0x%02x)\n", le16toh(hdr->revision), EFIRES_CURRENT_REVISION);
        goto out;
    }

    uint16_t nentries = le16toh(hdr->nentries);
    fprintf(stderr, "File with 0x%x entries: %s\n", nentries, fname);

    if (nentries * sizeof(efires_file_t) + sizeof(efires_hdr_t) > file_size) {
        fprintf(stderr, "File is too small to have so many entries\n");
        goto out;
    }

    char** filelist_iter = NULL;

    if (filelist) {
        *filelist = malloc((nentries + 1) * sizeof(char*));
        filelist_iter = *filelist;
        if (filelist_iter) {
            *filelist_iter = NULL;
        } else {
            fprintf(stderr, "Cant allocate memory for filelist\n");
            goto out;
        }
    }

    if ((flags & ONLY_LIST) == 0) {
        if (mkdir(destination, 0755) != 0) {
            fprintf(stderr, "Cant create destination directory '%s': %s\n", destination, strerror(errno));
            goto out;
        }

        if (chdir(destination) != 0) {
            fprintf(stderr, "Cant chdir to destination directory '%s': %s\n", destination, strerror(errno));
            goto out;
        }
    }

    for (uint16_t i = 0; i != nentries; ++i) {
        const efires_file_t *ent = &hdr->entries[i];
        uint32_t off = le32toh(ent->offset);
        uint32_t len = le32toh(ent->length);

        printf("0x%04x (0x%08x - 0x%08x): %s\n", i, off, off + len, ent->name);

        if (filelist_iter) *(filelist_iter++) = strdup(ent->name);

        if (flags & ONLY_LIST) continue;

        if (off + len > file_size) {
            fprintf(stderr, "File 0x%04x: overflows efires file -- skipping\n", i);
            continue;
        }

        int f = open(ent->name, O_WRONLY|O_CREAT|O_EXCL, 0755);

        if (f == -1) {
            fprintf(stderr, "File 0x%04x: Failed to create file: %s\n", i, strerror(errno));
            continue;
        }

        int wrote = write(f, (void*) ((uintptr_t)file_map + off), len);
        if ((uint32_t)wrote != len) {
            fprintf(stderr, "File 0x%04x: Expected to write %d bytes, wrote %d: %s\n", i, len, wrote, strerror(errno));
        }

        close(f);
    }

    result = 0;

out:;

    if (result && filelist && *filelist) {
        free_filelist(*filelist);
        *filelist = NULL;
    }

    if (file_map) {
        munmap((void*)file_map, file_size);
    }
    return result;
}

int pack_efires(const char* fname, const char* fromdir, const char* filelist[]) {
    int result = 1;
    DIR *dir = NULL;
    int dfd = -1;
    int outfd = -1;
    size_t file_size = 0;
    void *file_map = NULL;
    uint32_t nentries = 0;

    dir = opendir(fromdir);
    dfd = dirfd(dir);
    if (dir == NULL || dfd == -1) {
        fprintf(stderr, "Cant open directory to pack (%s) : %s\n", fromdir, strerror(errno));
        goto out;
    }

    outfd = open(fname, O_RDWR | O_CREAT | O_EXCL, 0644);
    if (outfd == -1) {
        fprintf(stderr, "Cant open output file (%s) : %s\n", fname, strerror(errno));
        goto out;
    }

    {
        // write space for header
        efires_hdr_t tmp;
        if (write(outfd, &tmp, sizeof(tmp)) == -1)
            abort();
    }

    // header and one reserved zeroed entry
    uint32_t full_file_len = sizeof(efires_hdr_t) + sizeof(efires_file_t);

    efires_file_t cur_entr;
    struct dirent *ep = NULL;
    const char** itm = filelist;

    while ((itm && *itm) || ((itm == NULL) && (ep = readdir(dir)) != NULL)) {
        struct stat s;

        const char* d_name = NULL;

        if (itm) {
            d_name = *(itm++);
        } else {
            d_name = ep->d_name;
        }

        if (fstatat(dfd, d_name, &s, 0) != 0) {
            fprintf(stderr, "Cant stat file, skipping (%s/%s) : %s\n", fromdir, d_name, strerror(errno));
            continue;
        }

        if ((s.st_mode & S_IFMT) != S_IFREG) {
            fprintf(stderr, "Entry isn't regular file, skipping (%s/%s)\n", fromdir, d_name);
            continue;
        }

        if (s.st_size > UINT32_MAX) {
            fprintf(stderr, "File too big for efires, skipping (%s/%s)\n", fromdir, d_name);
            continue;
        }

        cur_entr.length = (uint32_t) s.st_size;

        if (full_file_len + cur_entr.length > UINT32_MAX) {
            fprintf(stderr, "File too big to fit in current state, skipping (%u bytes left, %u bytes needed) (%s/%s)\n", UINT32_MAX - full_file_len, cur_entr.length, fromdir, d_name);
            continue;
        }

        size_t e_name_len = strlen(d_name);
        if (e_name_len > sizeof(cur_entr.name)) {
            fprintf(stderr, "Filename too long, skipping (%s/%s)\n", fromdir, d_name);
            continue;
        }

        ++nentries;
        memcpy(cur_entr.name, d_name, e_name_len);

        if (e_name_len < sizeof(cur_entr.name)) {
            memset(cur_entr.name + e_name_len, 0, sizeof(cur_entr.name) - e_name_len);
        }

        if (write(outfd, &cur_entr, sizeof(cur_entr)) != sizeof(cur_entr)) {
            fprintf(stderr, "Write to result file failed: %s\n", strerror(errno));
            goto out;
        }

        full_file_len += sizeof(cur_entr) + cur_entr.length;

        if (nentries + 1 == UINT32_MAX) {
            fprintf(stderr, "Too many entries, only packing 0x%08x\n", nentries);
            break;
        }
    }

    // reserved zeroed entry
    memset(&cur_entr, 0, sizeof(cur_entr));
    if (write(outfd, &cur_entr, sizeof(cur_entr)) != sizeof(cur_entr)) {
        fprintf(stderr, "Write to result file failed: %s\n", strerror(errno));
        goto out;
    }

    if (ftruncate(outfd, full_file_len) != 0) {
        fprintf(stderr, "Failed to expand result file to needed size: %s\n", strerror(errno));
        goto out;
    }

    file_map = mmap(NULL, full_file_len, PROT_READ | PROT_WRITE, MAP_SHARED, outfd, 0);

    if (file_map == MAP_FAILED) {
        fprintf(stderr, "Cant mmap result file: %s\n", strerror(errno));
        file_map = NULL;
        goto out;
    }

    efires_hdr_t *hdr = (efires_hdr_t *) file_map;
    hdr->revision = htole16(EFIRES_CURRENT_REVISION);
    hdr->nentries = htole16(nentries);

    // header + nentries entries + reserved zeroed entry
    uint32_t current_offset = sizeof(efires_hdr_t) + (nentries + 1) * sizeof(efires_file_t);
    for (uint16_t i = 0; i != nentries; ++i) {
        efires_file_t *ent = &hdr->entries[i];

        uint32_t length = ent->length;
        uint32_t offset = current_offset;
        current_offset += length;

        ent->length = htole32(length);
        ent->offset = htole32(offset);

        printf("0x%04x (0x%08x - 0x%08x): %s\n", i, offset, offset + length, ent->name);

        int entfd = openat(dfd, ent->name, O_RDONLY);

        if (entfd == -1) {
            fprintf(stderr, "Cant open file, leaving zeroed (%s/%s): %s\n", fromdir, ent->name, strerror(errno));
            continue;
        }

        if (read(entfd, (void*) ((uintptr_t)file_map + offset), length) != length) {
            fprintf(stderr, "Cant read %u bytes from file, contents in efires undefined (%s/%s): %s\n", length, fromdir, ent->name, strerror(errno));
        }

        close(entfd);
    }

    result = 0;

out:;
    if (dir != NULL) {
        closedir(dir);
    }

    if (file_map != NULL) {
        munmap(file_map, file_size);
    }

    if (outfd != -1) {
        close(outfd);

        // delete file if error occured
        if (result) {
            unlink(fname);
        }
    }

    return result;
}

#endif // WIN32
