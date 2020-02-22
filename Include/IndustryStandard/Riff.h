/*
 * File: Riff.h
 *
 * Description: Resource Interchange File Format format definition.
 *
 * Copyright (c) 2018 John Davis
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
 */

#ifndef EFI_RIFF_H
#define EFI_RIFF_H

/**
  See http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html.
**/

#include <Uefi.h>

//
// RIFF chunk IDs.
//
#define RIFF_CHUNK_ID_SIZE  4
#define RIFF_CHUNK_ID       "RIFF"
#define LIST_CHUNK_ID       "LIST"
#define PAL_CHUNK_ID        "PAL "
#define RDIB_CHUNK_ID       "RDIB"
#define RMID_CHUNK_ID       "RMID"
#define RMMP_CHUNK_ID       "RMMP"

//
// WAVE chunk IDs.
//
#define WAVE_CHUNK_ID               "WAVE"
#define WAVE_CUE_CHUNK_ID           "cue "
#define WAVE_DATA_CHUNK_ID          "data"
#define WAVE_FACT_CHUNK_ID          "fact"
#define WAVE_FILE_CHUNK_ID          "file"
#define WAVE_FORMAT_CHUNK_ID        "fmt "
#define WAVE_LABEL_CHUNK_ID         "labl"
#define WAVE_NOTE_CHUNK_ID          "note"
#define WAVE_PLAYLIST_CHUNK_ID      "plst"
#define WAVE_SILENCE_CHUNK_ID       "slnt"
#define WAVE_TEXT_DATA_CHUNK_ID     "ltxt"
#define WAVE_WAVE_LIST_CHUNK_ID     "wavl"

//
// WAVE format types.
//
#define WAVE_FORMAT_PCM         0x0001
#define WAVE_FORMAT_EXTENSIBLE  0xFFFE

#pragma pack(1)

/**
  RIFF chunk.
**/
typedef struct {
  CHAR8             Id[RIFF_CHUNK_ID_SIZE];
  UINT32            Size;
  UINT8             Data[];
} RIFF_CHUNK;

/**
  WAVE format data.
**/
typedef struct {
  UINT16            FormatTag;
  UINT16            Channels;
  UINT32            SamplesPerSec;
  UINT32            AvgBytesPerSec;
  UINT16            BlockAlign;
  UINT16            BitsPerSample;
} WAVE_FORMAT_DATA;

/**
  WAVE format data extended.
**/
typedef struct {
  WAVE_FORMAT_DATA  Header;
  UINT16            ExtensionSize;
  UINT16            ValidBitsPerSample;
  UINT32            ChannelMask;
  GUID              SubFormat;
} WAVE_FORMAT_DATA_EX;

#pragma pack()

#endif // EFI_RIFF_H
