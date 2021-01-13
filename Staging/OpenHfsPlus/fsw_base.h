/**
 * \file fsw_base.h
 * Base definitions switch.
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

#ifndef _FSW_BASE_H_
#define _FSW_BASE_H_


#ifndef FSW_DEBUG_LEVEL
/**
 * Global debugging level. Can be set locally for the scope of a single
 * file by defining the macro before fsw_base.h is included.
 */
#define FSW_DEBUG_LEVEL 1
#endif


#ifdef HOST_EFI
#include "fsw_efi_base.h"
#endif

#ifdef HOST_POSIX
#include "fsw_posix_base.h"
#endif

// message printing

#if FSW_DEBUG_LEVEL >= 1
#define FSW_MSG_ASSERT(params) FSW_MSGFUNC(params)
#else
#define FSW_MSG_ASSERT(params)
#endif

#if FSW_DEBUG_LEVEL >= 2
#define FSW_MSG_DEBUG(params) FSW_MSGFUNC(params)
#else
#define FSW_MSG_DEBUG(params)
#endif

#if FSW_DEBUG_LEVEL >= 3
#define FSW_MSG_DEBUGV(params) FSW_MSGFUNC(params)
#else
#define FSW_MSG_DEBUGV(params)
#endif


// Documentation for system-dependent defines

/**
 * \typedef fsw_s8
 * Signed 8-bit integer.
 */

/**
 * \typedef fsw_u8
 * Unsigned 8-bit integer.
 */

/**
 * \typedef fsw_s16
 * Signed 16-bit integer.
 */

/**
 * \typedef fsw_u16
 * Unsigned 16-bit integer.
 */

/**
 * \typedef fsw_s32
 * Signed 32-bit integer.
 */

/**
 * \typedef fsw_u32
 * Unsigned 32-bit integer.
 */

/**
 * \typedef fsw_s64
 * Signed 64-bit integer.
 */

/**
 * \typedef fsw_u64
 * Unsigned 64-bit integer.
 */


/**
 * \def fsw_alloc(size,ptrptr)
 * Allocate memory on the heap. This function or macro allocates \a size
 * bytes of memory using host-specific methods. The address of the
 * allocated memory block is stored into the pointer variable pointed
 * to by \a ptrptr. A status code is returned; FSW_SUCCESS if the block
 * was allocated or FSW_OUT_OF_MEMORY if there is not enough memory
 * to allocated the requested block.
 */

/**
 * \def fsw_free(ptr)
 * Release allocated memory. This function or macro returns an allocated
 * memory block to the heap for reuse. Does not return a status.
 */

/**
 * \def fsw_memcpy(dest,src,size)
 * Copies a block of memory from \a src to \a dest. The two memory blocks
 * must not overlap, or the result of the operation will be undefined.
 * Does not return a status.
 */

/**
 * \def fsw_memeq(dest,src,size)
 * Compares two blocks of memory for equality. Returns boolean true if the
 * memory blocks are equal, boolean false if they are different.
 */

/**
 * \def fsw_memzero(dest,size)
 * Initializes a block of memory with zeros. Does not return a status.
 */


#endif
