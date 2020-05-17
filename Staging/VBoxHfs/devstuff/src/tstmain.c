/**
 * \file tstmain.c
 * Test program for the POSIX user space environment.
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

#ifndef ITERATIONS
#define ITERATIONS 1
#endif

void usage(char* pname);
void catfile(struct fsw_posix_volume *pvol, char *path);
void id2path(struct fsw_posix_volume *pvol, char *idnum);
void viewdir(struct fsw_posix_volume *pvol, char *path, int level, int rflag, int docat);

extern struct fsw_fstype_table FSW_FSTYPE_TABLE_NAME(FSTYPE);

static struct fsw_fstype_table *fstypes[] = {
    &FSW_FSTYPE_TABLE_NAME(FSTYPE)
};

FILE* outfile = NULL;

void
catfile(struct fsw_posix_volume *pvol, char *path)
{
    struct fsw_posix_file *pfile;
    ssize_t r;
    char buf[4096];

    pfile = fsw_posix_open(pvol, path, 0, 0);
    if (pfile == NULL) {
        fprintf(stderr, "open(%s) call failed.\n", path);
        return;
    }
    while ((r = fsw_posix_read(pfile, buf, sizeof(buf))) > 0)
    {
        if (outfile != NULL)
        (void) fwrite(buf, r, 1, outfile);
    }
    fsw_posix_close(pfile);
}

void
id2path(struct fsw_posix_volume *pvol, char *idnum)
{
    fsw_status_t status;
    struct fsw_string_list *path;
    struct fsw_string_list *pe;
    fsw_u32 dnid;

    path = NULL;
    dnid = (fsw_u32) atol(idnum);
    status = fsw_dnode_id_fullpath(pvol->vol, dnid, FSW_STRING_KIND_ISO88591, &path);

    if (status != FSW_SUCCESS) {
        fprintf(stderr, "fsw_dnode_id_fullpath(%u) returned %d.\n", dnid, status);
		return;
    }

    fprintf(outfile, "Path for #%u is ", dnid);

    for (pe = path; pe != NULL; pe = pe->flink) {
	fprintf(outfile, "/%*s", fsw_strlen(pe->str), (char *) fsw_strchars(pe->str));
    }

    fprintf(outfile, "\n");
	fsw_string_list_free(path);
}

void
viewdir(struct fsw_posix_volume *pvol, char *path, int level, int rflag, int docat)
{
	struct fsw_posix_dir *dir;
	struct dirent *dent;
	int i;
	char subpath[4096];
	
	dir = fsw_posix_opendir(pvol, path);
	if (dir == NULL) {
		fprintf(stderr, "opendir(%s) call failed.\n", path);
		return;
	}
	while ((dent = fsw_posix_readdir(dir)) != NULL) {
		if (outfile != NULL) {
			for (i = 0; i < level*2; i++)
				fputc(' ', outfile);
			fprintf(outfile, "0x%04x %8d (#%u)", dent->d_type, dent->d_reclen, (unsigned int)(dent->d_fileno));
#if 1
			fprintf(outfile, "%*s", dent->d_reclen, dent->d_name);
#endif
			fputc('\n', outfile);
		}
		
		if (rflag && dent->d_type == DT_DIR) {
			snprintf(subpath, sizeof(subpath) - 1, "%s%s/", path, dent->d_name);
			viewdir(pvol, subpath, level + 1, rflag, docat);
		} else if (docat && dent->d_type == DT_REG) {
			snprintf(subpath, sizeof(subpath) - 1, "%s%s", path, dent->d_name);
			catfile(pvol, subpath);
		}
	}
	fsw_posix_closedir(dir);
}

void
usage(char* pname)
{
    fprintf(stderr, "Usage: %s <file/device> i|l|c[r] dir|file\n", pname);
    exit(1);
}

int
main(int argc, char *argv[])
{
  int cflag, iflag, lflag, rflag;
  struct fsw_posix_volume *pvol = NULL;
  int i;
  char* cp;
  struct fsw_fstype_table* fst;
  
  if (argc != 4) {
    usage(argv[0]);
  }
  
  cflag = iflag = lflag = rflag = 0;
  
  for (cp = argv[2]; *cp != '\0'; cp++) {
    switch (*cp) {
      case 'c':
        cflag = 1;
        break;
      case 'i':
        iflag = 1;
        break;
      case 'l':
        lflag = 1;
        break;
      case 'r':
        rflag = 1;
        break;
      default:
        usage(argv[0]);
    }
  }
  
  fst = fstypes[0];
  pvol = fsw_posix_mount(argv[1], fst);
  
  if (pvol == NULL) {
    fprintf(stderr, "Mounting failed.\n");
    return 1;
  }
  
  outfile = stdout;
  
  for (i = 0; i < ITERATIONS; i++) {
    if (cflag) {
      catfile(pvol, argv[3]);
    } else if (iflag) {
      id2path(pvol, argv[3]);
    } else if (lflag) {
      viewdir(pvol, argv[3], 0, rflag, cflag);
    }
  }
  
  fsw_posix_unmount(pvol);
  
  if (outfile != NULL) {
    (void) fclose (outfile);
  }
  (void) fclose (stdout);
  (void) fclose (stderr);
  return 0;
}

// EOF
