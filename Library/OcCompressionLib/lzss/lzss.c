/*
 * Copyright (c) 2006 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * This file contains Original Code and/or Modifications of Original Code
 * as defined in and that are subject to the Apple Public Source License
 * Version 2.0 (the 'License'). You may not use this file except in
 * compliance with the License. Please obtain a copy of the License at
 * http://www.opensource.apple.com/apsl/ and read it before using this
 * file.
 *
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */
#include "lzss.h"

/*******************************************************************************
*******************************************************************************/
u_int32_t local_adler32(u_int8_t * buffer, int32_t length)
{
    int32_t cnt;
    u_int32_t  result, lowHalf, highHalf;

    lowHalf = 1;
    highHalf = 0;

    for (cnt = 0; cnt < length; cnt++) {
        if ((cnt % 5000) == 0) {
            lowHalf  %= 65521L;
            highHalf %= 65521L;
        }

        lowHalf += buffer[cnt];
        highHalf += lowHalf;
    }

    lowHalf  %= 65521L;
    highHalf %= 65521L;

    result = (highHalf << 16) | lowHalf;

    return result;
}

/**************************************************************
 LZSS.C -- A Data Compression Program
***************************************************************
    4/6/1989 Haruhiko Okumura
    Use, distribute, and modify this program freely.
    Please send me your improved versions.
        PC-VAN      SCIENCE
        NIFTY-Serve PAF01022
        CompuServe  74050,1022

**************************************************************/

#define N         4096  /* size of ring buffer - must be power of 2 */
#define F         18    /* upper limit for match_length */
#define THRESHOLD 2     /* encode string into position and length
                           if match_length is greater than this */
#define NIL       N     /* index for root of binary search trees */

struct encode_state {
    /*
     * left & right children & parent. These constitute binary search trees.
     */
    int lchild[N + 1], rchild[N + 257], parent[N + 1];

    /* ring buffer of size N, with extra F-1 bytes to aid string comparison */
    u_int8_t text_buf[N + F - 1];

    /*
     * match_length of longest match.
     * These are set by the insert_node() procedure.
     */
    int match_position, match_length;
};


/*******************************************************************************
*******************************************************************************/
u_int32_t decompress_lzss(
    u_int8_t       * dst,
    u_int32_t        dstlen,
    u_int8_t       * src,
    u_int32_t        srclen)
{
    /* ring buffer of size N, with extra F-1 bytes to aid string comparison */
    u_int8_t text_buf[N + F - 1];
    u_int8_t * dststart = dst;
    const u_int8_t * dstend = dst + dstlen;
    const u_int8_t * srcend = src + srclen;
    int  i, j, k, r;
    u_int8_t c;
    unsigned int flags;

    if (dstlen > OC_COMPRESSION_MAX_LENGTH || srclen > OC_COMPRESSION_MAX_LENGTH) {
        return 0;
    }

    dst = dststart;
    memset(text_buf, ' ', N - F);
    r = N - F;
    flags = 0;
    for ( ; ; ) {
        if (((flags >>= 1) & 0x100) == 0) {
            if (src < srcend) c = *src++; else break;
            flags = c | 0xFF00;  /* uses higher byte cleverly */
        }   /* to count eight */
        if (flags & 1) {
            if (src < srcend) c = *src++; else break;
            if (dst < dstend) *dst++ = c; else break;
            text_buf[r++] = c;
            r &= (N - 1);
        } else {
            if (src < srcend) i = *src++; else break;
            if (src < srcend) j = *src++; else break;
            i |= ((j & 0xF0) << 4);
            j  =  (j & 0x0F) + THRESHOLD;
            for (k = 0; k <= j; k++) {
                c = text_buf[(i + k) & (N - 1)];
                if (dst < dstend) *dst++ = c; else break;
                text_buf[r++] = c;
                r &= (N - 1);
            }
        }
    }

    return (u_int32_t)(dst - dststart);
}

/*
 * initialize state, mostly the trees
 *
 * For i = 0 to N - 1, rchild[i] and lchild[i] will be the right and left
 * children of node i.  These nodes need not be initialized.  Also, parent[i]
 * is the parent of node i.  These are initialized to NIL (= N), which stands
 * for 'not used.'  For i = 0 to 255, rchild[N + i + 1] is the root of the
 * tree for strings that begin with character i.  These are initialized to NIL.
 * Note there are 256 trees. */
static void init_state(struct encode_state *sp)
{
    bzero(sp, sizeof(*sp));
    memset(&sp->text_buf[0], ' ', N - F);
    bzero(&sp->rchild[N + 1], 256 * sizeof(sp->rchild[0]));
    bzero(&sp->parent[0], N * sizeof(sp->parent[0]));
}

/*
 * Inserts string of length F, text_buf[r..r+F-1], into one of the trees
 * (text_buf[r]'th tree) and returns the longest-match position and length
 * via the global variables match_position and match_length.
 * If match_length = F, then removes the old node in favor of the new one,
 * because the old one will be deleted sooner. Note r plays double role,
 * as tree node and position in buffer.
 */
static void insert_node(struct encode_state *sp, int r)
{
    int  i, p, cmp;
    u_int8_t  *key;

    cmp = 1;
    key = &sp->text_buf[r];
    p = N + 1 + key[0];
    sp->rchild[r] = sp->lchild[r] = NIL;
    sp->match_length = 0;
    for ( ; ; ) {
        if (cmp >= 0) {
            if (sp->rchild[p] != NIL)
                p = sp->rchild[p];
            else {
                sp->rchild[p] = r;
                sp->parent[r] = p;
                return;
            }
        } else {
            if (sp->lchild[p] != NIL)
                p = sp->lchild[p];
            else {
                sp->lchild[p] = r;
                sp->parent[r] = p;
                return;
            }
        }
        for (i = 1; i < F; i++) {
            if ((cmp = key[i] - sp->text_buf[p + i]) != 0)
                break;
        }
        if (i > sp->match_length) {
            sp->match_position = p;
            if ((sp->match_length = i) >= F)
                break;
        }
    }
    sp->parent[r] = sp->parent[p];
    sp->lchild[r] = sp->lchild[p];
    sp->rchild[r] = sp->rchild[p];
    sp->parent[sp->lchild[p]] = r;
    sp->parent[sp->rchild[p]] = r;
    if (sp->rchild[sp->parent[p]] == p)
        sp->rchild[sp->parent[p]] = r;
    else
        sp->lchild[sp->parent[p]] = r;
    sp->parent[p] = NIL;  /* remove p */
}

/* deletes node p from tree */
static void delete_node(struct encode_state *sp, int p)
{
    int  q;

    if (sp->parent[p] == NIL)
        return;  /* not in tree */
    if (sp->rchild[p] == NIL)
        q = sp->lchild[p];
    else if (sp->lchild[p] == NIL)
        q = sp->rchild[p];
    else {
        q = sp->lchild[p];
        if (sp->rchild[q] != NIL) {
            do {
                q = sp->rchild[q];
            } while (sp->rchild[q] != NIL);
            sp->rchild[sp->parent[q]] = sp->lchild[q];
            sp->parent[sp->lchild[q]] = sp->parent[q];
            sp->lchild[q] = sp->lchild[p];
            sp->parent[sp->lchild[p]] = q;
        }
        sp->rchild[q] = sp->rchild[p];
        sp->parent[sp->rchild[p]] = q;
    }
    sp->parent[q] = sp->parent[p];
    if (sp->rchild[sp->parent[p]] == p)
        sp->rchild[sp->parent[p]] = q;
    else
        sp->lchild[sp->parent[p]] = q;
    sp->parent[p] = NIL;
}

/*******************************************************************************
*******************************************************************************/
u_int8_t * compress_lzss(
    u_int8_t       * dst,
    u_int32_t        dstlen,
    u_int8_t       * src,
    u_int32_t        srclen)
{
    u_int8_t * result = NULL;
    /* Encoding state, mostly tree but some current match stuff */
    struct encode_state *sp;

    int  i, len, r, s, last_match_length, code_buf_ptr;
    u_int8_t c;
    u_int8_t code_buf[17], mask;
    u_int8_t * srcend = src + srclen;
    u_int8_t *dstend = dst + dstlen;

    if (dstlen > OC_COMPRESSION_MAX_LENGTH || srclen > OC_COMPRESSION_MAX_LENGTH) {
        return NULL;
    }

    /* initialize trees */
    sp = (struct encode_state *) malloc(sizeof(*sp));
    if (!sp) goto finish;

    init_state(sp);

    /*
     * code_buf[1..16] saves eight units of code, and code_buf[0] works
     * as eight flags, "1" representing that the unit is an unencoded
     * letter (1 byte), "0" a position-and-length pair (2 bytes).
     * Thus, eight units require at most 16 bytes of code.
     */
    code_buf[0] = 0;
    code_buf_ptr = mask = 1;

    /* Clear the buffer with any character that will appear often. */
    s = 0;  r = N - F;

    /* Read F bytes into the last F bytes of the buffer */
    for (len = 0; len < F && src < srcend; len++)
        sp->text_buf[r + len] = *src++;
    if (!len)
        goto finish;

    /*
     * Insert the F strings, each of which begins with one or more
     * 'space' characters.  Note the order in which these strings are
     * inserted.  This way, degenerate trees will be less likely to occur.
     */
    for (i = 1; i <= F; i++)
        insert_node(sp, r - i);

    /*
     * Finally, insert the whole string just read.
     * The global variables match_length and match_position are set.
     */
    insert_node(sp, r);
    do {
        /* match_length may be spuriously long near the end of text. */
        if (sp->match_length > len)
            sp->match_length = len;
        if (sp->match_length <= THRESHOLD) {
            sp->match_length = 1;  /* Not long enough match.  Send one byte. */
            code_buf[0] |= mask;  /* 'send one byte' flag */
            code_buf[code_buf_ptr++] = sp->text_buf[r];  /* Send uncoded. */
        } else {
            /* Send position and length pair. Note match_length > THRESHOLD. */
            code_buf[code_buf_ptr++] = (u_int8_t) sp->match_position;
            code_buf[code_buf_ptr++] = (u_int8_t)
                ( ((sp->match_position >> 4) & 0xF0)
                |  (sp->match_length - (THRESHOLD + 1)) );
        }
        if ((mask <<= 1) == 0) {  /* Shift mask left one bit. */
                /* Send at most 8 units of code together */
            for (i = 0; i < code_buf_ptr; i++)
                if (dst < dstend)
                    *dst++ = code_buf[i];
                else
                    goto finish;
            code_buf[0] = 0;
            code_buf_ptr = mask = 1;
        }
        last_match_length = sp->match_length;
        for (i = 0; i < last_match_length && src < srcend; i++) {
            delete_node(sp, s);    /* Delete old strings and */
            c = *src++;
            sp->text_buf[s] = c;    /* read new bytes */

            /*
             * If the position is near the end of buffer, extend the buffer
             * to make string comparison easier.
             */
            if (s < F - 1)
                sp->text_buf[s + N] = c;

            /* Since this is a ring buffer, increment the position modulo N. */
            s = (s + 1) & (N - 1);
            r = (r + 1) & (N - 1);

            /* Register the string in text_buf[r..r+F-1] */
            insert_node(sp, r);
        }
        while (i++ < last_match_length) {
        delete_node(sp, s);

            /* After the end of text, no need to read, */
            s = (s + 1) & (N - 1);
            r = (r + 1) & (N - 1);
            /* but buffer may not be empty. */
            if (--len)
                insert_node(sp, r);
        }
    } while (len > 0);   /* until length of string to be processed is zero */

    if (code_buf_ptr > 1) {    /* Send remaining code. */
        for (i = 0; i < code_buf_ptr; i++)
            if (dst < dstend)
                *dst++ = code_buf[i];
            else
                goto finish;
    }

    result = dst;

finish:
    if (sp) free(sp);

    return result;
}
