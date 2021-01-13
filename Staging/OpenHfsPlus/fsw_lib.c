/**
 * \file fsw_lib.c
 * Core file system wrapper library functions.
 */

/*-
 * Copyright (c) 2020 Vladislav Yaroshchuk <yaroshchuk2000@gmail.com>
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

/* Include generated string encoding specific functions */
#include "fsw_strfunc.h"


/**
 * Allocate memory and clear it.
 */

fsw_status_t fsw_alloc_zero(int len, void **ptr_out)
{
    fsw_status_t status;
    
    status = fsw_alloc(len, ptr_out);
    if (status)
        return status;
    fsw_memzero(*ptr_out, len);
    return FSW_SUCCESS;
}

/**
 * Duplicate a piece of data.
 */

fsw_status_t fsw_memdup(void **dest_out, void *src, int len)
{
    fsw_status_t status;
    
    status = fsw_alloc(len, dest_out);
    if (status)
        return status;
    fsw_memcpy(*dest_out, src, len);
    return FSW_SUCCESS;
}

/**
 * Get the length of a string. Returns the number of characters in the string.
 */

int fsw_strlen(struct fsw_string *s)
{
    if (s == NULL || s->type == FSW_STRING_TYPE_EMPTY)
        return 0;
    return s->len;
}

/**
 * Get the size of a string in bytes.
 */
int fsw_strsize(struct fsw_string *s)
{
    if (s == NULL || s->type == FSW_STRING_TYPE_EMPTY)
        return 0;

    return s->size;
}

/**
 * Get the data of a string.
 */
void *fsw_strdata(struct fsw_string *s)
{
    if (s == NULL || s->type == FSW_STRING_TYPE_EMPTY)
        return NULL;

    return (void *) s->data;
}

/**
 * Compare two strings for equality. The two strings are compared, taking their
 * encoding into account. If they are considered equal, boolean true is returned.
 * Otherwise, boolean false is returned.
 */

int fsw_streq(struct fsw_string *s1, struct fsw_string *s2)
{
    struct fsw_string temp_s;
    
    // handle empty strings
    if (s1->type == FSW_STRING_TYPE_EMPTY) {
        temp_s.type = FSW_STRING_TYPE_ISO88591;
        temp_s.size = temp_s.len = 0;
        temp_s.data = NULL;
        return fsw_streq(&temp_s, s2);
    }
    if (s2->type == FSW_STRING_TYPE_EMPTY) {
        temp_s.type = FSW_STRING_TYPE_ISO88591;
        temp_s.size = temp_s.len = 0;
        temp_s.data = NULL;
        return fsw_streq(s1, &temp_s);
    }
    
    // check length (count of chars)
    if (s1->len != s2->len)
        return 0;
    if (s1->len == 0)   // both strings are empty
        return 1;
    
    if (s1->type == s2->type) {
        // same type, do a dumb memory compare
        if (s1->size != s2->size)
            return 0;
        return fsw_memeq(s1->data, s2->data, s1->size);
    }
    
    // dispatch to type-specific functions
    #define STREQ_DISPATCH(type1, type2) \
      if (s1->type == FSW_STRING_TYPE_##type1 && s2->type == FSW_STRING_TYPE_##type2) \
        return fsw_streq_##type1##_##type2(s1->data, s2->data, s1->len); \
      if (s2->type == FSW_STRING_TYPE_##type1 && s1->type == FSW_STRING_TYPE_##type2) \
        return fsw_streq_##type1##_##type2(s2->data, s1->data, s1->len);
    STREQ_DISPATCH(ISO88591, UTF8);
    STREQ_DISPATCH(ISO88591, UTF16);
    STREQ_DISPATCH(ISO88591, UTF16_SWAPPED);
    STREQ_DISPATCH(UTF8, UTF16);
    STREQ_DISPATCH(UTF8, UTF16_SWAPPED);
    STREQ_DISPATCH(UTF16, UTF16_SWAPPED);
    
    // final fallback
    return 0;
}

/**
 * Compare a string with a C string constant. This sets up a string descriptor
 * for the string constant (second argument) and runs fsw_streq on the two
 * strings. Currently the C string is interpreted as ISO 8859-1.
 * Returns boolean true if the strings are considered equal, boolean false otherwise.
 */

int fsw_streq_cstr(struct fsw_string *s1, const char *s2)
{
    struct fsw_string temp_s;
    int i;
    
    for (i = 0; s2[i]; i++)
        ;
    
    temp_s.type = FSW_STRING_TYPE_ISO88591;
    temp_s.size = temp_s.len = i;
    temp_s.data = (char *)s2;
    
    return fsw_streq(s1, &temp_s);
}

/**
 * Creates a duplicate of a string, converting it to the given encoding during the copy.
 * If the function returns FSW_SUCCESS, the caller must free the string later with
 * fsw_strfree.
 */

fsw_status_t fsw_strdup_coerce(struct fsw_string *dest, int type, struct fsw_string *src)
{
    fsw_status_t    status;
    
    if (src->type == FSW_STRING_TYPE_EMPTY || src->len == 0) {
        dest->type = type;
        dest->size = dest->len = 0;
        dest->data = NULL;
        return FSW_SUCCESS;
    }
    
    if (src->type == type) {
        dest->type = type;
        dest->len  = src->len;
        dest->size = src->size;
        status = fsw_alloc(dest->size, &dest->data);
        if (status)
            return status;
        
        fsw_memcpy(dest->data, src->data, dest->size);
        return FSW_SUCCESS;
    }
    
    // dispatch to type-specific functions
    #define STRCOERCE_DISPATCH(type1, type2) \
      if (src->type == FSW_STRING_TYPE_##type1 && type == FSW_STRING_TYPE_##type2) \
        return fsw_strcoerce_##type1##_##type2(src->data, src->len, dest);
    STRCOERCE_DISPATCH(UTF8, ISO88591);
    STRCOERCE_DISPATCH(UTF16, ISO88591);
    STRCOERCE_DISPATCH(UTF16_SWAPPED, ISO88591);
    STRCOERCE_DISPATCH(ISO88591, UTF8);
    STRCOERCE_DISPATCH(UTF16, UTF8);
    STRCOERCE_DISPATCH(UTF16_SWAPPED, UTF8);
    STRCOERCE_DISPATCH(ISO88591, UTF16);
    STRCOERCE_DISPATCH(UTF8, UTF16);
    STRCOERCE_DISPATCH(UTF16_SWAPPED, UTF16);
    
    return FSW_UNSUPPORTED;
}

/**
 * Splits a string at the first occurence of the separator character.
 * The buffer string is searched for the separator character. If it is found, the
 * element string descriptor is filled to point at the part of the buffer string
 * before the separator. The buffer string itself is adjusted to point at the
 * remaining part of the string (without the separator).
 *
 * If the separator is not found in the buffer string, then element is changed to
 * point at the whole buffer string, and the buffer string itself is changed into
 * an empty string.
 *
 * This function only manipulates the pointers and lengths in the two string descriptors,
 * it does not change the actual string. If the buffer string is dynamically allocated,
 * you must make a copy of it so that you can release it later.
 */

void fsw_strsplit(struct fsw_string *element, struct fsw_string *buffer, char separator)
{
    int i, maxlen;
    
    if (buffer->type == FSW_STRING_TYPE_EMPTY || buffer->len == 0) {
        element->type = FSW_STRING_TYPE_EMPTY;
        return;
    }
    
    maxlen = buffer->len;
    *element = *buffer;
    
    if (buffer->type == FSW_STRING_TYPE_ISO88591) {
        fsw_u8 *p;
        
        p = (fsw_u8 *)element->data;
        for (i = 0; i < maxlen; i++, p++) {
            if (*p == separator) {
                buffer->data = p + 1;
                buffer->len -= i + 1;
                break;
            }
        }
        element->len = i;
        if (i == maxlen) {
            buffer->data = p;
            buffer->len -= i;
        }
        
        element->size = element->len;
        buffer->size  = buffer->len;
        
    } else if (buffer->type == FSW_STRING_TYPE_UTF16) {
        fsw_u16 *p;
        
        p = (fsw_u16 *)element->data;
        for (i = 0; i < maxlen; i++, p++) {
            if (*p == separator) {
                buffer->data = p + 1;
                buffer->len -= i + 1;
                break;
            }
        }
        element->len = i;
        if (i == maxlen) {
            buffer->data = p;
            buffer->len -= i;
        }
        
        element->size = element->len * sizeof(fsw_u16);
        buffer->size  = buffer->len  * sizeof(fsw_u16);
        
    } else {
        // fallback
        buffer->type = FSW_STRING_TYPE_EMPTY;
    }
    
    // TODO: support UTF8 and UTF16_SWAPPED
}

/**
 * Frees the memory used by a string returned from fsw_strdup_coerce.
 */

void fsw_strfree(struct fsw_string *s)
{
    if (s->type != FSW_STRING_TYPE_EMPTY && s->data)
        fsw_free(s->data);
    s->type = FSW_STRING_TYPE_EMPTY;
}

// EOF
