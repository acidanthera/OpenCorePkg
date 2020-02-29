/* ----------------------------------------------------------------------------
 * umm_malloc.c - a memory allocator for embedded systems (microcontrollers)
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Ralph Hempel
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ----------------------------------------------------------------------------
 *
 * R.Hempel 2007-09-22 - Original
 * R.Hempel 2008-12-11 - Added MIT License biolerplate
 *                     - realloc() now looks to see if previous block is free
 *                     - made common operations functions
 * R.Hempel 2009-03-02 - Added macros to disable tasking
 *                     - Added function to dump heap and check for valid free
 *                        pointer
 * R.Hempel 2009-03-09 - Changed name to umm_malloc to avoid conflicts with
 *                        the mm_malloc() library functions
 *                     - Added some test code to assimilate a free block
 *                        with the very block if possible. Complicated and
 *                        not worth the grief.
 * D.Frank 2014-04-02  - Fixed heap configuration when UMM_TEST_MAIN is NOT set,
 *                        added user-dependent configuration file umm_malloc_cfg.h
 * R.Hempel 2016-12-04 - Add support for Unity test framework
 *                     - Reorganize source files to avoid redundant content
 *                     - Move integrity and poison checking to separate file
 * R.Hempel 2017-12-29 - Fix bug in realloc when requesting a new block that
 *                        results in OOM error - see Issue 11
 * vit9696  2018-02-07 - Changed types, masks and limits to support 32-bit pools
 *                     - Removed realloc and calloc I do not need
 *                     - Added pointer range check in free to detect memory that
 *                       was not allocated by us
 *                     - Made pool initialization external to avoid memset deps
 *                       and to support initialization state
 *                     - Switched to UEFI types, pragmas, renamed external API
 * ----------------------------------------------------------------------------
 */

#include <Library/OcMemoryLib.h>

STATIC UINT8   *default_umm_heap;
STATIC UINT32  default_umm_heap_size;

#define UMM_MALLOC_CFG_HEAP_SIZE default_umm_heap_size
#define UMM_MALLOC_CFG_HEAP_ADDR default_umm_heap

#define UMM_BEST_FIT

#define DBGLOG_DEBUG(format, ...) do { } while (0)
#define DBGLOG_TRACE(froamt, ...) do { } while (0)

#define UMM_CRITICAL_ENTRY()
#define UMM_CRITICAL_EXIT()

/* ------------------------------------------------------------------------- */

#pragma pack(1)

typedef struct umm_ptr_t {
  UINT32 next;
  UINT32 prev;
} umm_ptr;

typedef struct umm_block_t {
  union {
    umm_ptr used;
  } header;
  union {
    umm_ptr free;
    UINT8 data[4];
  } body;
} umm_block;

#pragma pack()

#define UMM_FREELIST_MASK (0x80000000)
#define UMM_BLOCKNO_MASK  (0x7FFFFFFF)

/* ------------------------------------------------------------------------- */

umm_block *umm_heap = NULL;
UINT32 umm_numblocks = 0;

#define UMM_NUMBLOCKS (umm_numblocks)

/* ------------------------------------------------------------------------ */

#define UMM_BLOCK(b)  (umm_heap[b])

#define UMM_NBLOCK(b) (UMM_BLOCK(b).header.used.next)
#define UMM_PBLOCK(b) (UMM_BLOCK(b).header.used.prev)
#define UMM_NFREE(b)  (UMM_BLOCK(b).body.free.next)
#define UMM_PFREE(b)  (UMM_BLOCK(b).body.free.prev)
#define UMM_DATA(b)   (UMM_BLOCK(b).body.data)

/* ------------------------------------------------------------------------ */

STATIC UINT32 umm_blocks( UINT32 size ) {

  /*
   * The calculation of the block size is not too difficult, but there are
   * a few little things that we need to be mindful of.
   *
   * When a block removed from the free list, the space used by the free
   * pointers is available for data. That's what the first calculation
   * of size is doing.
   */

  if( size <= (sizeof(((umm_block *)0)->body)) )
    return( 1 );

  /*
   * If it's for more than that, then we need to figure out the number of
   * additional whole blocks the size of an umm_block are required.
   */

  size -= ( 1 + (sizeof(((umm_block *)0)->body)) );

  return( 2 + size/(sizeof(umm_block)) );
}

/* ------------------------------------------------------------------------ */
/*
 * Split the block `c` into two blocks: `c` and `c + blocks`.
 *
 * - `new_freemask` should be `0` if `c + blocks` used, or `UMM_FREELIST_MASK`
 *   otherwise.
 *
 * Note that free pointers are NOT modified by this function.
 */
STATIC VOID umm_split_block( UINT32 c,
    UINT32 blocks,
    UINT32 new_freemask ) {

  UMM_NBLOCK(c+blocks) = (UMM_NBLOCK(c) & UMM_BLOCKNO_MASK) | new_freemask;
  UMM_PBLOCK(c+blocks) = c;

  UMM_PBLOCK(UMM_NBLOCK(c) & UMM_BLOCKNO_MASK) = (c+blocks);
  UMM_NBLOCK(c)                                = (c+blocks);
}

/* ------------------------------------------------------------------------ */

STATIC VOID umm_disconnect_from_free_list( UINT32 c ) {
  /* Disconnect this block from the FREE list */

  UMM_NFREE(UMM_PFREE(c)) = UMM_NFREE(c);
  UMM_PFREE(UMM_NFREE(c)) = UMM_PFREE(c);

  /* And clear the free block indicator */

  UMM_NBLOCK(c) &= (~UMM_FREELIST_MASK);
}

/* ------------------------------------------------------------------------
 * The umm_assimilate_up() function assumes that UMM_NBLOCK(c) does NOT
 * have the UMM_FREELIST_MASK bit set!
 */

STATIC VOID umm_assimilate_up( UINT32 c ) {

  if( UMM_NBLOCK(UMM_NBLOCK(c)) & UMM_FREELIST_MASK ) {
    /*
     * The next block is a free block, so assimilate up and remove it from
     * the free list
     */

    DBGLOG_DEBUG( "Assimilate up to next block, which is FREE\n" );

    /* Disconnect the next block from the FREE list */

    umm_disconnect_from_free_list( UMM_NBLOCK(c) );

    /* Assimilate the next block with this one */

    UMM_PBLOCK(UMM_NBLOCK(UMM_NBLOCK(c)) & UMM_BLOCKNO_MASK) = c;
    UMM_NBLOCK(c) = UMM_NBLOCK(UMM_NBLOCK(c)) & UMM_BLOCKNO_MASK;
  }
}

/* ------------------------------------------------------------------------
 * The umm_assimilate_down() function assumes that UMM_NBLOCK(c) does NOT
 * have the UMM_FREELIST_MASK bit set!
 */

STATIC UINT32 umm_assimilate_down( UINT32 c, UINT32 freemask ) {

  UMM_NBLOCK(UMM_PBLOCK(c)) = UMM_NBLOCK(c) | freemask;
  UMM_PBLOCK(UMM_NBLOCK(c)) = UMM_PBLOCK(c);

  return( UMM_PBLOCK(c) );
}

/* ------------------------------------------------------------------------ */

VOID umm_init( VOID ) {
  /* init heap pointer and size, and memset it to 0 */
  umm_heap = (umm_block *)UMM_MALLOC_CFG_HEAP_ADDR;
  umm_numblocks = (UMM_MALLOC_CFG_HEAP_SIZE / sizeof(umm_block));

  /*
   * This is done at allocation step!
   * memset(umm_heap, 0x00, UMM_MALLOC_CFG_HEAP_SIZE);
   */

  /* setup initial blank heap structure */
  {
    /* index of the 0th `umm_block` */
    CONST UINT32 block_0th = 0;
    /* index of the 1st `umm_block` */
    CONST UINT32 block_1th = 1;
    /* index of the latest `umm_block` */
    CONST UINT32 block_last = UMM_NUMBLOCKS - 1;

    /* setup the 0th `umm_block`, which just points to the 1st */
    UMM_NBLOCK(block_0th) = block_1th;
    UMM_NFREE(block_0th)  = block_1th;
    UMM_PFREE(block_0th)  = block_1th;

    /*
     * Now, we need to set the whole heap space as a huge free block. We should
     * not touch the 0th `umm_block`, since it's special: the 0th `umm_block`
     * is the head of the free block list. It's a part of the heap invariant.
     *
     * See the detailed explanation at the beginning of the file.
     */

    /*
     * 1th `umm_block` has pointers:
     *
     * - next `umm_block`: the latest one
     * - prev `umm_block`: the 0th
     *
     * Plus, it's a free `umm_block`, so we need to apply `UMM_FREELIST_MASK`
     *
     * And it's the last free block, so the next free block is 0.
     */
    UMM_NBLOCK(block_1th) = block_last | UMM_FREELIST_MASK;
    UMM_NFREE(block_1th)  = 0;
    UMM_PBLOCK(block_1th) = block_0th;
    UMM_PFREE(block_1th)  = block_0th;

    /*
     * latest `umm_block` has pointers:
     *
     * - next `umm_block`: 0 (meaning, there are no more `umm_blocks`)
     * - prev `umm_block`: the 1st
     *
     * It's not a free block, so we don't touch NFREE / PFREE at all.
     */
    UMM_NBLOCK(block_last) = 0;
    UMM_PBLOCK(block_last) = block_1th;
  }
}

/* ------------------------------------------------------------------------ */

BOOLEAN UmmInitialized ( VOID ) {
  return default_umm_heap != NULL;
}

/* ------------------------------------------------------------------------ */

VOID UmmSetHeap( VOID *heap, UINT32 size ) {
  default_umm_heap = (UINT8 *)heap;
  default_umm_heap_size = size;
  umm_init();
}

/* ------------------------------------------------------------------------ */

BOOLEAN UmmFree( VOID *ptr ) {

  UINT32 c;
  UINT8 *cptr = (UINT8 *)ptr;

  /* If we are not initialised, reuturn false! */
  if ( !UmmInitialized() )
    return FALSE;

  /* If we're being asked to free a NULL pointer, well that's just silly! */

  if( (VOID *)0 == ptr ) {
    DBGLOG_DEBUG( "free a null pointer -> do nothing\n" );

    return FALSE;
  }

  /* If we're being asked to free an unrelated pointer, return FALSE as well! */

  if (cptr < default_umm_heap || cptr >= default_umm_heap + UMM_MALLOC_CFG_HEAP_SIZE)
    return FALSE;

  /*
   * FIXME: At some point it might be a good idea to add a check to make sure
   *        that the pointer we're being asked to free up is actually within
   *        the umm_heap!
   *
   * NOTE:  See the new umm_info() function that you can use to see if a ptr is
   *        on the free list!
   */

  /* Protect the critical section... */
  UMM_CRITICAL_ENTRY();

  /* Figure out which block we're in. Note the use of truncated division... */

  c = (UINT32)((((UINT8 *)ptr)-(UINT8 *)(&(umm_heap[0])))/sizeof(umm_block));

  DBGLOG_DEBUG( "Freeing block %6i\n", c );

  /* Now let's assimilate this block with the next one if possible. */

  umm_assimilate_up( c );

  /* Then assimilate with the previous block if possible */

  if( UMM_NBLOCK(UMM_PBLOCK(c)) & UMM_FREELIST_MASK ) {

    DBGLOG_DEBUG( "Assimilate down to next block, which is FREE\n" );

    c = umm_assimilate_down(c, UMM_FREELIST_MASK);
  } else {
    /*
     * The previous block is not a free block, so add this one to the head
     * of the free list
     */

    DBGLOG_DEBUG( "Just add to head of free list\n" );

    UMM_PFREE(UMM_NFREE(0)) = c;
    UMM_NFREE(c)            = UMM_NFREE(0);
    UMM_PFREE(c)            = 0;
    UMM_NFREE(0)            = c;

    UMM_NBLOCK(c)          |= UMM_FREELIST_MASK;
  }

  /* Release the critical section... */
  UMM_CRITICAL_EXIT();

  return TRUE;
}

/* ------------------------------------------------------------------------ */

VOID *UmmMalloc( UINT32 size ) {
  UINT32 blocks;
  UINT32 blockSize = 0;

  UINT32 bestSize;
  UINT32 bestBlock;

  UINT32 cf;

  /* If we are not initialised, reuturn false! */
  if ( !UmmInitialized() )
    return NULL;

  /*
   * the very first thing we do is figure out if we're being asked to allocate
   * a size of 0 - and if we are we'll simply return a null pointer. if not
   * then reduce the size by 1 byte so that the subsequent calculations on
   * the number of blocks to allocate are easier...
   */

  if( 0 == size ) {
    DBGLOG_DEBUG( "malloc a block of 0 bytes -> do nothing\n" );

    return( (VOID *)NULL );
  }

  /* Protect the critical section... */
  UMM_CRITICAL_ENTRY();

  blocks = umm_blocks( size );

  /*
   * Now we can scan through the free list until we find a space that's big
   * enough to hold the number of blocks we need.
   *
   * This part may be customized to be a best-fit, worst-fit, or first-fit
   * algorithm
   */

  cf = UMM_NFREE(0);

  bestBlock = UMM_NFREE(0);
  bestSize  = 0x7FFFFFFF;

  while( cf ) {
    blockSize = (UMM_NBLOCK(cf) & UMM_BLOCKNO_MASK) - cf;

    DBGLOG_TRACE( "Looking at block %6i size %6i\n", cf, blockSize );

#if defined UMM_BEST_FIT
    if( (blockSize >= blocks) && (blockSize < bestSize) ) {
      bestBlock = cf;
      bestSize  = blockSize;
    }
#elif defined UMM_FIRST_FIT
    /* This is the first block that fits! */
    if( (blockSize >= blocks) )
      break;
#else
#  error "No UMM_*_FIT is defined - check umm_malloc_cfg.h"
#endif

    cf = UMM_NFREE(cf);
  }

  if( 0x7FFFFFFF != bestSize ) {
    cf        = bestBlock;
    blockSize = bestSize;
  }

  if( UMM_NBLOCK(cf) & UMM_BLOCKNO_MASK && blockSize >= blocks ) {
    /*
     * This is an existing block in the memory heap, we just need to split off
     * what we need, unlink it from the free list and mark it as in use, and
     * link the rest of the block back into the freelist as if it was a new
     * block on the free list...
     */

    if( blockSize == blocks ) {
      /* It's an exact fit and we don't neet to split off a block. */
      DBGLOG_DEBUG( "Allocating %6i blocks starting at %6i - exact\n", blocks, cf );

      /* Disconnect this block from the FREE list */

      umm_disconnect_from_free_list( cf );

    } else {
      /* It's not an exact fit and we need to split off a block. */
      DBGLOG_DEBUG( "Allocating %6i blocks starting at %6i - existing\n", blocks, cf );

      /*
       * split current free block `cf` into two blocks. The first one will be
       * returned to user, so it's not free, and the second one will be free.
       */
      umm_split_block( cf, blocks, UMM_FREELIST_MASK /*new block is free*/ );

      /*
       * `umm_split_block()` does not update the free pointers (it affects
       * only free flags), but effectively we've just moved beginning of the
       * free block from `cf` to `cf + blocks`. So we have to adjust pointers
       * to and from adjacent free blocks.
       */

      /* previous free block */
      UMM_NFREE( UMM_PFREE(cf) ) = cf + blocks;
      UMM_PFREE( cf + blocks ) = UMM_PFREE(cf);

      /* next free block */
      UMM_PFREE( UMM_NFREE(cf) ) = cf + blocks;
      UMM_NFREE( cf + blocks ) = UMM_NFREE(cf);
    }
  } else {
    /* Out of memory */

    DBGLOG_DEBUG(  "Can't allocate %5i blocks\n", blocks );

    /* Release the critical section... */
    UMM_CRITICAL_EXIT();

    return( (VOID *)NULL );
  }

  /* Release the critical section... */
  UMM_CRITICAL_EXIT();

  return( (VOID *)&UMM_DATA(cf) );
}

/* ------------------------------------------------------------------------ */
