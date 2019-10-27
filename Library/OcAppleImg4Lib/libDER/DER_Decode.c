/*
 * Copyright (c) 2005-2010 Apple Inc. All Rights Reserved.
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

/* 
 * DER_Decode.c - DER decoding routines
 */
 
#include "DER_Decode.h"
#include "asn1Types.h"

#include "libDER_config.h"

#ifndef	DER_DECODE_ENABLE
#error Please define DER_DECODE_ENABLE.
#endif

#if		DER_DECODE_ENABLE

#define DER_DECODE_DEBUG	0
#if		DER_DECODE_DEBUG
#include <stdio.h>
#define derDecDbg(a)			printf(a)
#define derDecDbg1(a, b)		printf(a, b)
#define derDecDbg2(a, b, c)		printf(a, b, c)
#define derDecDbg3(a, b, c, d)	printf(a, b, c, d)
#else	
#define derDecDbg(a)	
#define derDecDbg1(a, b)	
#define derDecDbg2(a, b, c)	
#define derDecDbg3(a, b, c, d)
#endif	/* DER_DECODE_DEBUG */

/*
 *  Basic decoding primitive. Only works with:
 *
 *  -- definite length encoding 
 *  -- one-byte tags 
 *  -- max content length fits in a DERSize
 *
 * No malloc or copy of the contents is performed; the returned 
 * content->content.data is a pointer into the incoming der data.
 */
DERReturn DERDecodeItem(
	const DERItem	*der,			/* data to decode */
	DERDecodedInfo	*decoded)		/* RETURNED */
{
	DERByte tag1;			/* first tag byte */
	DERByte len1;			/* first length byte */
	DERTag tagNumber;       /* tag number without class and method bits */
	DERByte *derPtr = der->data;
	DERSize derLen = der->length;

    /* The tag decoding below is fully BER complient.  We support a max tag
       value of 2 ^ ((sizeof(DERTag) * 8) - 3) - 1 so for tag size 1 byte we
       support tag values from 0 - 0x1F.  For tag size 2 tag values
       from 0 - 0x1FFF and for tag size 4 values from 0 - 0x1FFFFFFF. */
	if(derLen < 2) {
		return DR_DecodeError;
	}
    /* Grab the first byte of the tag. */
	tag1 = *derPtr++;
	derLen--;
	tagNumber = tag1 & 0x1F;
	if(tagNumber == 0x1F) {
#ifdef DER_MULTIBYTE_TAGS
        /* Long tag form: bit 8 of each octet shall be set to one unless it is
           the last octet of the tag */
        const DERTag overflowMask = ((DERTag)0x7F << (sizeof(DERTag) * 8 - 7));
        DERByte tagByte;
        tagNumber = 0;
        do {
            if(derLen < 2 || (tagNumber & overflowMask) != 0) {
                return DR_DecodeError;
            }
            tagByte = *derPtr++;
            derLen--;
            tagNumber = (tagNumber << 7) | (tagByte & 0x7F);
        } while((tagByte & 0x80) != 0);

        /* Check for any of the top 3 reserved bits being set. */
        if ((tagNumber & (overflowMask << 4)) != 0)
#endif
            return DR_DecodeError;
	}
    /* Returned tag, top 3 bits are class/method remaining bits are number. */
    decoded->tag = ((DERTag)(tag1 & 0xE0) << ((sizeof(DERTag) - 1) * 8)) | tagNumber;

    /* Tag decoding above ensured we have at least one more input byte left. */
	len1 = *derPtr++;
	derLen--;
	if(len1 & 0x80) {
		/* long length form - first byte is length of length */
		DERSize longLen = 0;	/* long form length */
		unsigned dex;

		len1 &= 0x7f;
		if((len1 > sizeof(DERSize)) || (len1 > derLen)) {
			/* no can do */
			return DR_DecodeError;
		}
		for(dex=0; dex<len1; dex++) {
			longLen <<= 8;
			longLen |= *derPtr++;
			derLen--;
		}
		if(longLen > derLen) {
			/* not enough data left for this encoding */
			return DR_DecodeError;
		}
		decoded->content.data = derPtr;
		decoded->content.length = longLen;
	}
	else {
		/* short length form, len1 is the length */
		if(len1 > derLen) {
			/* not enough data left for this encoding */
			return DR_DecodeError;
		}
		decoded->content.data = derPtr;
		decoded->content.length = len1;
	}

    return DR_Success;
}

/* 
 * Given a BIT_STRING, in the form of its raw content bytes, 
 * obtain the number of unused bits and the raw bit string bytes.
 */
DERReturn DERParseBitString(
	const DERItem	*contents,
	DERItem			*bitStringBytes,	/* RETURNED */
	DERByte			*numUnusedBits)		/* RETURNED */
{
	if(contents->length < 2) {
		/* not enough room for actual bits after the unused bits field */
		*numUnusedBits = 0;
		bitStringBytes->data = NULL;
		bitStringBytes->length = 0;
		return DR_Success;
	}
	*numUnusedBits = contents->data[0];
	bitStringBytes->data = contents->data + 1;
	bitStringBytes->length = contents->length - 1;
	return DR_Success;
}

/* 
 * Given a BOOLEAN, in the form of its raw content bytes, 
 * obtain it's value.
 */
DERReturn DERParseBoolean(
	const DERItem	*contents,
	bool			defaultValue,
	bool			*value) {	/* RETURNED */
    if (contents->length == 0) {
        *value = defaultValue;
        return DR_Success;
    }
    if (contents->length != 1 ||
        (contents->data[0] != 0 && contents->data[0] != 0xFF))
        return DR_DecodeError;

    *value = contents->data[0] != 0;
    return DR_Success;
}

DERReturn DERParseInteger(
	const DERItem	*contents,
	uint32_t        *result) {	/* RETURNED */
    DERSize ix, length = contents->length;
    if (length > 4)
        return DR_BufOverflow;
    uint32_t value = 0;
    for (ix = 0; ix < length; ++ix) {
        value <<= 8;
        value += contents->data[ix];
    }
    *result = value;
    return DR_Success;
}

/* Sequence/set support */

/* 
 * To decode a set or sequence, call DERDecodeSeqInit once, then
 * call DERDecodeSeqNext to get each enclosed item.
 * DERDecodeSeqNext returns DR_EndOfSequence when no more
 * items are available. 
 */
DERReturn DERDecodeSeqInit(
	const DERItem	*der,			/* data to decode */
	DERTag			*tag,			/* RETURNED tag of sequence/set. This will be
									 * either ASN1_CONSTR_SEQUENCE or ASN1_CONSTR_SET. */
	DERSequence		*derSeq)		/* RETURNED, to use in DERDecodeSeqNext */
{
	DERDecodedInfo decoded;
	DERReturn drtn;
	
	drtn = DERDecodeItem(der, &decoded);
	if(drtn) {
		return drtn;
	}
    *tag = decoded.tag;
	switch(decoded.tag) {
		case ASN1_CONSTR_SEQUENCE:
		case ASN1_CONSTR_SET:
			break;
		default:
			return DR_UnexpectedTag;
	}
	derSeq->nextItem = decoded.content.data;
	derSeq->end = decoded.content.data + decoded.content.length;
	return DR_Success;
}

/* 
 * Use this to start in on decoding a sequence's content, when 
 * the top-level tag and content have already been decoded. 
 */
DERReturn DERDecodeSeqContentInit(
	const DERItem	*content,
	DERSequence		*derSeq)		/* RETURNED, to use in DERDecodeSeqNext */
{
	/* just prepare for decoding items in content */
	derSeq->nextItem = content->data;
	derSeq->end = content->data + content->length;
	return DR_Success;
}

DERReturn DERDecodeSeqNext(
	DERSequence		*derSeq,
	DERDecodedInfo	*decoded)		/* RETURNED */
{
	DERReturn drtn;
	DERItem item;
	
	if(derSeq->nextItem >= derSeq->end) {
		/* normal termination, contents all used up */
		return DR_EndOfSequence;
	}
	
	/* decode next item */
	item.data = derSeq->nextItem;
	item.length = derSeq->end - derSeq->nextItem;
	drtn = DERDecodeItem(&item, decoded);
	if(drtn) {
		return drtn;
	}	
	
	/* skip over the item we just decoded */
	derSeq->nextItem = decoded->content.data + decoded->content.length;
	return DR_Success;
}

/* 
 * High level sequence parse, starting with top-level tag and content.
 * Top level tag must be ASN1_CONSTR_SEQUENCE - if it's not, and that's 
 * OK, use DERParseSequenceContent().
 */
DERReturn DERParseSequence(
	const DERItem			*der,
	DERShort				numItems,	/* size of itemSpecs[] */
	const DERItemSpec		*itemSpecs,
	void					*dest,		/* DERDecodedInfo(s) here RETURNED */
	DERSize					sizeToZero)	/* optional */
{
	DERReturn drtn;
	DERDecodedInfo topDecode;
	
	drtn = DERDecodeItem(der, &topDecode);
	if(drtn) {
		return drtn;
	}
	if(topDecode.tag != ASN1_CONSTR_SEQUENCE) {
		return DR_UnexpectedTag;
	}
	return DERParseSequenceContent(&topDecode.content,
		numItems, itemSpecs, dest, sizeToZero);
}

/* high level sequence parse, starting with sequence's content */
DERReturn DERParseSequenceContent(
	const DERItem			*content,
	DERShort				numItems,	/* size of itemSpecs[] */
	const DERItemSpec		*itemSpecs,
	void					*dest,		/* DERDecodedInfo(s) here RETURNED */
	DERSize					sizeToZero)	/* optional */
{
	DERSequence			derSeq;
	DERReturn			drtn;
	DERShort			itemDex;
	DERByte				*currDER;	/* full DER encoding of current item */
	
	if(sizeToZero) {
		DERMemset(dest, 0, sizeToZero);
	}
	
	drtn = DERDecodeSeqContentInit(content, &derSeq);
	if(drtn) {
		return drtn;
	}
	
	/* main loop */
	for(itemDex=0 ; itemDex<numItems; ) {
		DERDecodedInfo currDecoded;
		DERShort i;
		DERTag foundTag;
		char foundMatch = 0;
		
		/* save this in case of DER_DEC_SAVE_DER */
		currDER = derSeq.nextItem;
		
		drtn = DERDecodeSeqNext(&derSeq, &currDecoded);
		if(drtn) {
			/* 
			 * One legal error here is DR_EndOfSequence when 
			 * all remaining DERSequenceItems are optional. 
			 */
			if(drtn == DR_EndOfSequence) {
				for(i=itemDex; i<numItems; i++) {
					if(!(itemSpecs[i].options & DER_DEC_OPTIONAL)) {
						/* unexpected end of sequence */
						return DR_IncompleteSeq;
					}
				}
				/* the rest are optional; success */
				return DR_Success;
			}
			else {
				/* any other error is fatal */
				return drtn;
			}
		}	/* decode error */
		
		/* 
		 * Seek matching tag or ASN_ANY in itemSpecs, skipping 
		 * over optional items.
		 */
		foundTag = currDecoded.tag;
		derDecDbg1("--- foundTag 0x%x\n", foundTag);
		
		for(i=itemDex; i<numItems; i++) {
			const DERItemSpec *currItemSpec = &itemSpecs[i];
			DERShort currOptions = currItemSpec->options;
			derDecDbg3("--- currItem %u expectTag 0x%x currOptions 0x%x\n", 
				i, currItemSpec->tag, currOptions);
			
			if((currOptions & DER_DEC_ASN_ANY) ||
			   (foundTag == currItemSpec->tag)) {
				/* 
				 * We're good with this one. Cook up destination address
				 * as appropriate. 
				 */
				if(!(currOptions & DER_DEC_SKIP)) {
					derDecDbg1("--- MATCH at currItem %u\n", i);
					DERByte *byteDst = (DERByte *)dest + currItemSpec->offset;
					DERItem *dst = (DERItem *)byteDst;
					*dst = currDecoded.content;
					if(currOptions & DER_DEC_SAVE_DER) {
						/* recreate full DER encoding of this item */
						derDecDbg1("--- SAVE_DER at currItem %u\n", i);
						dst->data = currDER;
						dst->length += (currDecoded.content.data - currDER);
					}
				}

				/* on to next item */
				itemDex = i + 1;
				
				/* is this the end? */
				if(itemDex == numItems) {
					/* normal termination */
					return DR_Success;
				}
				else {
					/* on to next item */ 
					foundMatch = 1;
					break;
				}
			} /* ASN_ANY, or match */
			
			/* 
			 * If current itemSpec isn't optional, abort - else on to 
			 * next item
			 */
			if(!(currOptions & DER_DEC_OPTIONAL)) {
				derDecDbg1("--- MISMATCH at currItem %u, !OPTIONAL, abort\n", i);
				return DR_UnexpectedTag;
			}
			
			/* else this was optional, on to next item */
		} /* searching for tag match */
		
		if(foundMatch == 0) {
			/* 
			 * Found an item we couldn't match to any tag spec and we're at
			 * the end. 
			 */
			derDecDbg("--- TAG NOT FOUND, abort\n");
			return DR_UnexpectedTag;
		}
		
		/* else on to next item */
	}	/* main loop */
	
	/*
	 * If we get here, there appears to be more to process, but we've
	 * given the caller everything they want. 
	 */
	return DR_Success;
}

#if 0
/* 
 * High level sequence parse, starting with top-level tag and content.
 * Top level tag must be ASN1_CONSTR_SEQUENCE - if it's not, and that's 
 * OK, use DERParseSequenceContent().
 */
DERReturn DERParseSequenceOf(
	const DERItem			*der,
	DERShort				numItems,	/* size of itemSpecs[] */
	const DERItemSpec		*itemSpecs,
	void					*dest,		/* DERDecodedInfo(s) here RETURNED */
	DERSize					*numDestItems)	/* output */
{
	DERReturn drtn;
	DERDecodedInfo topDecode;
	
	drtn = DERDecodeItem(der, &topDecode);
	if(drtn) {
		return drtn;
	}
	if(topDecode.tag != ASN1_CONSTR_SEQUENCE) {
		return DR_UnexpectedTag;
	}
	return DERParseSequenceContent(&topDecode.content,
		numItems, itemSpecs, dest, sizeToZero);
}

/* 
 * High level set of parse, starting with top-level tag and content.
 * Top level tag must be ASN1_CONSTR_SET - if it's not, and that's 
 * OK, use DERParseSetOrSequenceOfContent().
 */
DERReturn DERParseSetOf(
	const DERItem			*der,
	DERShort				numItems,	/* size of itemSpecs[] */
	const DERItemSpec		*itemSpecs,
	void					*dest,		/* DERDecodedInfo(s) here RETURNED */
	DERSize					*numDestItems)	/* output */
{
	DERReturn drtn;
	DERDecodedInfo topDecode;
	
	drtn = DERDecodeItem(der, &topDecode);
	if(drtn) {
		return drtn;
	}
	if(topDecode.tag != ASN1_CONSTR_SET) {
		return DR_UnexpectedTag;
	}
	return DERParseSetOrSequenceOfContent(&topDecode.content,
		numItems, itemSpecs, dest, numDestItems);
}

/* High level set of or sequence of parse, starting with set or
   sequence's content */
DERReturn DERParseSetOrSequenceOfContent(
    const DERItem			*content,
    void(*itemHandeler)(void *, const DERDecodedInfo *)
	void					*itemHandelerContext);
{
    DERSequence			derSeq;
    DERShort			itemDex;

    drtn = DERDecodeSeqContentInit(content, &derSeq);
    require_noerr_quiet(drtn, badCert);

    /* main loop */
    for (;;) {
        DERDecodedInfo currDecoded;
        DERShort i;
        DERByte foundTag;
        char foundMatch = 0;

        drtn = DERDecodeSeqNext(&derSeq, &currDecoded);
        if(drtn) {
            /* The only legal error here is DR_EndOfSequence. */
            if(drtn == DR_EndOfSequence) {
                /* no more items left in the sequence; success */
                return DR_Success;
            }
            else {
                /* any other error is fatal */
                require_noerr_quiet(drtn, badCert);
            }
        }	/* decode error */
        
        /* Each element can be anything. */
        foundTag = currDecoded.tag;

        /* 
         * We're good with this one. Cook up destination address
         * as appropriate. 
         */
        DERByte *byteDst = (DERByte *)dest + currItemSpec->offset;
        DERItem *dst = (DERItem *)byteDst;
        *dst = currDecoded.content;
        if(currOptions & DER_DEC_SAVE_DER) {
            /* recreate full DER encoding of this item */
            derDecDbg1("--- SAVE_DER at currItem %u\n", i);
            dst->data = currDER;
            dst->length += (currDecoded.content.data - currDER);
        }

        /* on to next item */
        itemDex = i + 1;
        
        /* is this the end? */
        if(itemDex == numItems) {
            /* normal termination */
            return DR_Success;
        }
        else {
            /* on to next item */ 
            foundMatch = 1;
            break;
        }
            
            /* 
             * If current itemSpec isn't optional, abort - else on to 
             * next item
             */
            if(!(currOptions & DER_DEC_OPTIONAL)) {
                derDecDbg1("--- MISMATCH at currItem %u, !OPTIONAL, abort\n", i);
                return DR_UnexpectedTag;
            }
            
            /* else this was optional, on to next item */
        } /* searching for tag match */
        
        if(foundMatch == 0) {
            /* 
             * Found an item we couldn't match to any tag spec and we're at
             * the end. 
             */
            derDecDbg("--- TAG NOT FOUND, abort\n");
            return DR_UnexpectedTag;
        }
        
        /* else on to next item */
    }	/* main loop */

    /*
     * If we get here, there appears to be more to process, but we've
     * given the caller everything they want. 
     */
    return DR_Success;
    }
}
#endif

#endif	/* DER_DECODE_ENABLE */
