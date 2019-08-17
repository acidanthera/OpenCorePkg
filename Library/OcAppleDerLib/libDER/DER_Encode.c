/* Copyright (c) 2005-2007 Apple Inc. All Rights Reserved. */

/*
 * DER_Encode.h - DER encoding routines
 *
 * Created Dec. 2 2005 by dmitch
 */

#include "DER_Encode.h"
#include "asn1Types.h"
#include "libDER_config.h"
#include "DER_Decode.h"

#ifndef	DER_ENCODE_ENABLE
#error Please define DER_ENCODE_ENABLE.
#endif

#if		DER_ENCODE_ENABLE

/* calculate size of encoded tag */
static DERSize DERLengthOfTag(
	DERTag tag)
{
	DERSize rtn = 1;

    tag &= ASN1_TAGNUM_MASK;
    if (tag >= 0x1F) {
        /* Shift 7-bit digits out of the tag integer until it's zero. */
        while(tag != 0) {
            rtn++;
            tag >>= 7;
        }
    }

    return rtn;
}

/* encode tag */
static DERReturn DEREncodeTag(
	DERTag tag,
	DERByte *buf,		/* encoded length goes here */
	DERSize *inOutLen)	/* IN/OUT */
{
    DERSize outLen = DERLengthOfTag(tag);
    DERTag tagNumber = tag & ASN1_TAGNUM_MASK;
    DERByte tag1 = (tag >> (sizeof(DERTag) * 8 - 8)) & 0xE0;

	if(outLen > *inOutLen) {
		return DR_BufOverflow;
	}

    if(outLen == 1) {
		/* short form */
		*buf = tag1 | tagNumber;
	}
	else {
        /* long form */
        DERByte *tagBytes = buf + outLen;	// l.s. digit of tag
        *buf = tag1 | 0x1F;                 // tag class / method indicator
        *--tagBytes = tagNumber & 0x7F;
        tagNumber >>= 7;
        while(tagNumber != 0) {
            *--tagBytes = (tagNumber & 0x7F) | 0x80;
            tagNumber >>= 7;
        }
    }
	*inOutLen = outLen;
	return DR_Success;
}

/* calculate size of encoded length */
DERSize DERLengthOfLength(
	DERSize length)
{
	DERSize rtn;
	
	if(length < 0x80) {
		/* short form length */
		return 1;
	}
	
	/* long form - one length-of-length byte plus length bytes */
	rtn = 1;
	while(length != 0) {
		rtn++;
		length >>= 8;
	}
	return rtn;
}

/* encode length */
DERReturn DEREncodeLength(
	DERSize length,
	DERByte *buf,		/* encoded length goes here */
	DERSize *inOutLen)	/* IN/OUT */
{
	DERByte *lenBytes;
	DERSize outLen = DERLengthOfLength(length);
	
	if(outLen > *inOutLen) {
		return DR_BufOverflow;
	}
	
	if(length < 0x80) {
		/* short form */
		*buf = (DERByte)length;
		*inOutLen = 1;
		return DR_Success;
	}
	
	/* long form */
	*buf = (outLen - 1) | 0x80;		// length of length, long form indicator
	lenBytes = buf + outLen - 1;	// l.s. digit of length 
	while(length != 0) {
		*lenBytes-- = (DERByte)length;
		length >>= 8;
	}
	*inOutLen = outLen;
	return DR_Success;
}

DERSize DERLengthOfItem(
	DERTag tag,
	DERSize length)
{
    return DERLengthOfTag(tag) + DERLengthOfLength(length) + length;
}

DERReturn DEREncodeItem(
	DERTag tag,
	DERSize length,
    const DERByte *src,
	DERByte *derOut,	/* encoded item goes here */
	DERSize *inOutLen)	/* IN/OUT */
{
	DERReturn		drtn;
	DERSize			itemLen;
	DERByte			*currPtr = derOut;
	DERSize         bytesLeft = DERLengthOfItem(tag, length);
	if(bytesLeft > *inOutLen) {
		return DR_BufOverflow;
	}
	*inOutLen = bytesLeft;

	/* top level tag */
	itemLen = bytesLeft;
	drtn = DEREncodeTag(tag, currPtr, &itemLen);
	if(drtn) {
		return drtn;
	}
	currPtr += itemLen;
	bytesLeft -= itemLen;
	itemLen = bytesLeft;
	drtn = DEREncodeLength(length, currPtr, &itemLen);
	if(drtn) {
		return drtn;
	}
	currPtr += itemLen;
	bytesLeft -= itemLen;
	DERMemmove(currPtr, src, length);

	return DR_Success;
}

static /* calculate the content length of an encoded sequence */
DERSize DERContentLengthOfEncodedSequence(
	const void			*src,		/* generally a ptr to a struct full of 
									 *    DERItems */
	DERShort			numItems,	/* size of itemSpecs[] */
	const DERItemSpec	*itemSpecs)
{
	DERSize contentLen = 0;
	unsigned dex;
	DERSize thisContentLen;
	
	/* find length of each item */
	for(dex=0; dex<numItems; dex++) {
		const DERItemSpec *currItemSpec = &itemSpecs[dex];
		DERShort currOptions = currItemSpec->options;
		const DERByte *byteSrc = (const DERByte *)src + currItemSpec->offset;
		const DERItem *itemSrc = (const DERItem *)byteSrc;

		if(currOptions & DER_ENC_WRITE_DER) {
			/* easy case - no encode */
			contentLen += itemSrc->length;
			continue;
		}
		
        if ((currOptions & DER_DEC_OPTIONAL) && itemSrc->length == 0) {
            /* If an optional item isn't present we don't encode a
               tag and len. */
            continue;
        }

		/* 
		 * length of this item = 
		 *   tag (one byte) +
		 *   length of length +
		 *   content length +
		 *   optional zero byte for signed integer
		 */
		contentLen += DERLengthOfTag(currItemSpec->tag);

		/* check need for pad byte before calculating lengthOfLength... */
		thisContentLen = itemSrc->length;
		if((currOptions & DER_ENC_SIGNED_INT) &&
		   (itemSrc->length != 0)) {
			if(itemSrc->data[0] & 0x80) {
				/* insert zero keep it positive */
				thisContentLen++;
			}
		}
		contentLen += DERLengthOfLength(thisContentLen);
		contentLen += thisContentLen;
	}
	return contentLen;
}

DERReturn DEREncodeSequence(
	DERTag				topTag,		/* ASN1_CONSTR_SEQUENCE, ASN1_CONSTR_SET */
	const void			*src,		/* generally a ptr to a struct full of 
									 *    DERItems */
	DERShort			numItems,	/* size of itemSpecs[] */
	const DERItemSpec	*itemSpecs,
	DERByte				*derOut,	/* encoded data written here */
	DERSize				*inOutLen)	/* IN/OUT */
{
	const DERByte	*endPtr = derOut + *inOutLen;
	DERByte			*currPtr = derOut;
	DERSize			bytesLeft = *inOutLen;
	DERSize			contentLen;
	DERReturn		drtn;
	DERSize			itemLen;
	unsigned		dex;
	
	/* top level tag */
	itemLen = bytesLeft;
    drtn = DEREncodeTag(topTag, currPtr, &itemLen);
	if(drtn) {
		return drtn;
	}
	currPtr += itemLen;
	bytesLeft -= itemLen;
	if(currPtr >= endPtr) {	
		return DR_BufOverflow;
	}
	
	/* content length */
	contentLen = DERContentLengthOfEncodedSequence(src, numItems, itemSpecs);	
	itemLen = bytesLeft;
	drtn = DEREncodeLength(contentLen, currPtr, &itemLen);
	if(drtn) {
		return drtn;
	}
	currPtr += itemLen;
	bytesLeft -= itemLen;
	if(currPtr + contentLen > endPtr) {
		return DR_BufOverflow;
	}
	/* we don't have to check for overflow any more */
	
	/* grind thru the items */
	for(dex=0; dex<numItems; dex++) {
		const DERItemSpec *currItemSpec = &itemSpecs[dex];
		DERShort currOptions = currItemSpec->options;
		const DERByte *byteSrc = (const DERByte *)src + currItemSpec->offset;
		const DERItem *itemSrc = (const DERItem *)byteSrc;
		int prependZero = 0;
		
		if(currOptions & DER_ENC_WRITE_DER) {
			/* easy case */
			DERMemmove(currPtr, itemSrc->data, itemSrc->length);
			currPtr += itemSrc->length;
			bytesLeft -= itemSrc->length;
			continue;
		}

        if ((currOptions & DER_DEC_OPTIONAL) && itemSrc->length == 0) {
            /* If an optional item isn't present we skip it. */
            continue;
        }

        /* encode one item: first the tag */
        itemLen = bytesLeft;
        drtn = DEREncodeTag(currItemSpec->tag, currPtr, &itemLen);
        if(drtn) {
            return drtn;
        }
        currPtr += itemLen;
        bytesLeft -= itemLen;

		/* do we need to prepend a zero to content? */
		contentLen = itemSrc->length;
		if((currOptions & DER_ENC_SIGNED_INT) &&
		   (itemSrc->length != 0)) {
			if(itemSrc->data[0] & 0x80) {
				/* insert zero keep it positive */
				contentLen++;
				prependZero = 1;
			}
		}

		/* encode content length */
		itemLen = bytesLeft;
		drtn = DEREncodeLength(contentLen, currPtr, &itemLen);
		if(drtn) {
			return drtn;
		}
		currPtr += itemLen;
		bytesLeft -= itemLen;
		
		/* now the content, with possible leading zero added */
		if(prependZero) {
			*currPtr++ = 0;
			bytesLeft--;
		}
		DERMemmove(currPtr, itemSrc->data, itemSrc->length);
		currPtr += itemSrc->length;
		bytesLeft -= itemSrc->length;
	}
	*inOutLen = (currPtr - derOut);
	return DR_Success;
}

/* calculate the length of an encoded sequence. */
DERSize DERLengthOfEncodedSequence(
    DERTag				topTag,
	const void			*src,		/* generally a ptr to a struct full of 
									 *    DERItems */
	DERShort			numItems,	/* size of itemSpecs[] */
	const DERItemSpec	*itemSpecs)
{
	DERSize contentLen = DERContentLengthOfEncodedSequence(
		src, numItems, itemSpecs);

	return DERLengthOfTag(topTag) +
        DERLengthOfLength(contentLen) +
		contentLen;
}

#endif	/* DER_ENCODE_ENABLE */

