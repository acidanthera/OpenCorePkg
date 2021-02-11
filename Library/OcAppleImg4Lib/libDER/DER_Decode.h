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
 * DER_Decode.h - DER decoding routines
 */
 
#ifndef	_DER_DECODE_H_
#define _DER_DECODE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "libDER.h"

/*
 * Decoding one item consists of extracting its tag, a pointer
 * to the actual content, and the length of the content. Those
 * three are represented by a DERDecodedInfo.
 */
typedef struct {
	DERTag		tag;
	DERItem		content;
} DERDecodedInfo;

/*
 * Basic decoding primitive. Only works with:
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
	DERDecodedInfo	*decoded);		/* RETURNED */

/* 
 * Given a BIT_STRING, in the form of its raw content bytes, 
 * obtain the number of unused bits and the raw bit string bytes.
 */
DERReturn DERParseBitString(
	const DERItem	*contents,
	DERItem			*bitStringBytes,	/* RETURNED */
	DERByte			*numUnusedBits);	/* RETURNED */

/* 
 * Given a BOOLEAN, in the form of its raw content bytes, 
 * obtain its value.
 */
DERReturn DERParseBoolean(
	const DERItem	*contents,
	bool			defaultValue,
	bool			*value);	/* RETURNED */

DERReturn DERParseInteger(
	const DERItem	*contents,
	uint32_t        *value);	/* RETURNED */

/* 
 * Sequence/set decode support.
 */
 
/* state representing a sequence or set being decoded */
typedef struct {
	DERByte	*nextItem;
	DERByte	*end;
} DERSequence;

/* 
 * To decode a set or sequence, call DERDecodeSeqInit or
 * DERDecodeSeqContentInit once, then call DERDecodeSeqNext to 
 * get each enclosed item. 
 *
 * DERDecodeSeqNext returns DR_EndOfSequence when no more
 * items are available. 
 */
 
/* 
 * Use this to parse the top level sequence's tag and content length.
 */
DERReturn DERDecodeSeqInit(
	const DERItem	*der,			/* data to decode */
	DERTag			*tag,			/* RETURNED tag of sequence/set. This will be
									 *    either ASN1_CONSTR_SEQUENCE or 
									 *    ASN1_CONSTR_SET. */
	DERSequence		*derSeq);		/* RETURNED, to use in DERDecodeSeqNext */
	
/* 
 * Use this to start in on decoding a sequence's content, when 
 * the top-level tag and content have already been decoded. 
 */
DERReturn DERDecodeSeqContentInit(
	const DERItem	*content,
	DERSequence		*derSeq);		/* RETURNED, to use in DERDecodeSeqNext */

/* obtain the next decoded item in a sequence or set */
DERReturn DERDecodeSeqNext(
	DERSequence		*derSeq,
	DERDecodedInfo	*decoded);		/* RETURNED */
	
/*
 * High level sequence decode.
 */

/* 
 * Per-item decode options.
 */
 
/* Explicit default, no options */
#define DER_DEC_NO_OPTS		0x0000

/* This item optional, can be skipped during decode */
#define DER_DEC_OPTIONAL	0x0001	

/* Skip the tag check; accept anything. */
#define DER_DEC_ASN_ANY		0x0002	

/* Skip item, no write to DERDecodedInfo (but tag check still performed) */
#define DER_DEC_SKIP		0x0004		

/* Save full DER encoding in DERDecodedInfo, including tag and length. Normally
 * only the content is saved. */
#define DER_DEC_SAVE_DER	0x0008

/* 
 * High level sequence parse, starting with top-level tag and content.
 * Top level tag must be ASN1_CONSTR_SEQUENCE - if it's not, and that's 
 * OK, use DERParseSequenceContent().
 *
 * These never return DR_EndOfSequence - if an *unexpected* end of sequence
 * occurs, return DR_IncompleteSeq.
 *
 * Results of the decoding of one item are placed in a DERItem whose address 
 * is the dest arg plus the offset value in the associated DERItemSpec. 
 *
 * Items which are optional (DER_DEC_OPTIONAL) and which are not found, 
 * leave their associated DERDecodedInfos unmodified.
 *
 * Processing of a sequence ends on detection of any error or after the
 * last DERItemSpec is processed. 
 *
 * The sizeToZero argument, if nonzero, indicates the number of bytes
 * starting at dest to zero before processing the sequence. This is 
 * generally desirable, particularly if there are any DER_DEC_OPTIONAL
 * items in the sequence; skipped optional items are detected by the 
 * caller via a NULL DERDecodedInfo.content.data; if this hasn't been
 * explicitly zeroed (generally, by passing a nonzero value of sizeToZero),
 * skipped items can't be detected. 
 */
DERReturn DERParseSequence(
	const DERItem			*der,
	DERShort				numItems,		/* size of itemSpecs[] */
	const DERItemSpec		*itemSpecs,
	void					*dest,			/* DERDecodedInfo(s) here RETURNED */
	DERSize					sizeToZero);	/* optional */
	
/* high level sequence parse, starting with sequence's content */
DERReturn DERParseSequenceContent(
	const DERItem			*content,
	DERShort				numItems,		/* size of itemSpecs[] */
	const DERItemSpec		*itemSpecs,
	void					*dest,			/* DERDecodedInfo(s) here RETURNED */
	DERSize					sizeToZero);	/* optional */

#ifdef __cplusplus
}
#endif

#endif	/* _DER_DECODE_H_ */

