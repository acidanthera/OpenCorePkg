/** @file

Read macOS .disk_label (.disk_label_2x) file and convert to .ppm.
Write macOS .disk_label (.disk_label_2x) files from text string.
Reference:
- http://refit.sourceforge.net/info/vollabel.html
- https://opensource.apple.com/source/bless/bless-181.0.1/libbless/Misc/BLGenerateOFLabel.c.auto.html

Copyright (c) 2019-2020, vit9696

All rights reserved.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define BigEndianToNative16(x) __builtin_bswap16(x)
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define BigEndianToNative16(x) (x)
#else
#include <arpa/inet.h>
#define BigEndianToNative16(x) ntohs(x)
#endif

#if defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>
#elif defined(WIN32) && !defined(_ISOC99_SOURCE)
#define _ISOC99_SOURCE
#endif // __APPLE__

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Reverse clut to pixels. */
static const uint8_t palette[256] = {
  [0x00] = /* 0xFF - */ 0x00,
  [0xF6] = /* 0xFF - */ 0x11,
  [0xF7] = /* 0xFF - */ 0x22,
  [0x2A] = /* 0xFF - */ 0x33,
  [0xF8] = /* 0xFF - */ 0x44,
  [0xF9] = /* 0xFF - */ 0x55,
  [0x55] = /* 0xFF - */ 0x66,
  [0xFA] = /* 0xFF - */ 0x77,
  [0xFB] = /* 0xFF - */ 0x88,
  [0x80] = /* 0xFF - */ 0x99,
  [0xFC] = /* 0xFF - */ 0xAA,
  [0xFD] = /* 0xFF - */ 0xBB,
  [0xAB] = /* 0xFF - */ 0xCC,
  [0xFE] = /* 0xFF - */ 0xDD,
  [0xFF] = /* 0xFF - */ 0xEE,
  [0xD6] = /* 0xFF - */ 0xFF
};

/* To fit BootPicker. */
#define LABEL_MAX_WIDTH      340
#define LABEL_MAX_HEIGHT     12
#define LABEL_TYPE_PALETTED  1
#define LABEL_TYPE_BGRA      2

#pragma pack(push, 1)
typedef struct DiskLabel_ {
  uint8_t  type;
  uint16_t width;
  uint16_t height;
  uint8_t  data[];
} DiskLabel;
#pragma pack(pop)

#ifdef __APPLE__

/* Antialiasing clut. */
static const uint8_t clut[] = {
  0x00, /* 0x00 0x00 0x00 white */
  0xF6, /* 0x11 0x11 0x11 */
  0xF7, /* 0x22 0x22 0x22 */
  0x2A, /* 0x33 = 1*6^2 + 1*6 + 1 = 43 colors */
  0xF8, /* 0x44 */
  0xF9, /* 0x55 */
  0x55, /* 0x66 = 2*(36 + 6 + 1) = 86 colors */
  0xFA, /* 0x77 */
  0xFB, /* 0x88 */
  0x80, /* 0x99 = (3*43) = 129 colors*/
  0xFC, /* 0xAA */
  0xFD, /* 0xBB */
  0xAB, /* 0xCC = 4*43 = 172 colors */
  0xFE, /* 0xDD */
  0xFF, /* 0xEE */
  0xD6, /* 0xFF = 5*43 = 215 */
};

static CTLineRef create_text_line(CGColorSpaceRef color_space, const char *str, int scale) {
  /* White text on black background, originating from OF/EFI bitmap. */
  const CGFloat components[] = {
    (CGFloat)1.0, (CGFloat)1.0
  };

  CTFontRef font_ref          = CTFontCreateWithName(CFSTR("Helvetica"), 10.0 * scale, NULL);
  CGColorRef color            = CGColorCreate(color_space, components);
  CFMutableDictionaryRef dict = CFDictionaryCreateMutable(kCFAllocatorDefault,
    0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

  CTLineRef result = NULL;

  if (font_ref != NULL && color != NULL && dict != NULL) {
    CFDictionarySetValue(dict, kCTForegroundColorAttributeName, color);
    CFDictionarySetValue(dict, kCTFontAttributeName, font_ref);

    CFStringRef tmp = CFStringCreateWithCString(kCFAllocatorDefault, str, kCFStringEncodingUTF8);
    if (tmp != NULL) {
      CFAttributedStringRef attr_tmp = CFAttributedStringCreate(kCFAllocatorDefault, tmp, dict);
      if (attr_tmp != NULL) {
        result = CTLineCreateWithAttributedString(attr_tmp);
        CFRelease(attr_tmp);
      }
      CFRelease(tmp);
    }
  }

  if (dict != NULL) {
    CFRelease(dict);
  }

  if (color != NULL) {
    CFRelease(color);
  }

  if (font_ref != NULL) {
    CFRelease(font_ref);
  }

  return result;
}

static int render_label(const char *label, uint8_t *bitmap_data,
  uint16_t width, uint16_t height, int scale, uint16_t *newwidth) {

  CGColorSpaceRef color_space = CGColorSpaceCreateDeviceGray();
  if (color_space == NULL) {
    fprintf(stderr, "Could not obtain color space\n");
    return 1;
  }

  CTLineRef line = create_text_line(color_space, label, scale);
  if (line == NULL) {
    fprintf(stderr, "Could not render text line\n");
    CGColorSpaceRelease(color_space);
    return 3;
  }

  CGContextRef context = CGBitmapContextCreate(bitmap_data, width, height,
    8, width, color_space, kCGImageAlphaNone);

  if(context == NULL) {
    fprintf(stderr, "Could not init CoreGraphics context\n");
    CFRelease(line);
    CGColorSpaceRelease(color_space);
    return 2;
  }

  CGRect rect = CTLineGetImageBounds(line, context);
  CGContextSetTextPosition(context, 2.0 * scale, 2.0 * scale);
  CTLineDraw(line, context);
  CGContextFlush(context);
  CFRelease(line);

  *newwidth = (uint16_t)(rect.size.width + 4 * scale);

  CGContextRelease(context);
  CGColorSpaceRelease(color_space);
  return 0;
}

static void *make_label(const char *label, int scale, bool bgra, uint16_t *label_size) {
  uint16_t width  = LABEL_MAX_WIDTH * scale;
  uint16_t height = LABEL_MAX_HEIGHT * scale;
  DiskLabel *label_data = calloc(1, sizeof(DiskLabel) + width*height*(bgra*3+1));
  if (label_data == NULL) {
    fprintf(stderr, "Label allocation failure\n");
    return NULL;
  }

  uint16_t newwidth;
  int err = render_label(label, label_data->data, width, height, scale, &newwidth);
  if (err != 0) {
    free(label_data);
    return NULL;
  }

  /* Cap at 340*scale pixels wide. */
  if (newwidth > width) {
    fprintf(stderr, "Label %s with %d pixels does not fit in %d pixels\n", label, newwidth, width);
    newwidth = width;
  }

  /* Refit to new width (111111000111111000111111000 -> 111111111111111111). */
  for (uint16_t row = 1; row < height; row++) {
    memmove(&label_data->data[row * newwidth], &label_data->data[row * width], newwidth);
  }

  label_data->type    = bgra ? LABEL_TYPE_BGRA : LABEL_TYPE_PALETTED;
  label_data->width   = htons(newwidth);
  label_data->height  = htons(height);

  if (bgra) {
    /* Perform conversion. */
    for (uint16_t i = 0; i < newwidth * height; i++) {
      uint32_t index = newwidth * height - i - 1;
      uint8_t alpha  = label_data->data[index];

      label_data->data[index * 4 + 0] = 0;
      label_data->data[index * 4 + 1] = 0;
      label_data->data[index * 4 + 2] = 0;
      label_data->data[index * 4 + 3] = 0xFF - alpha;
    }
  } else {
    /* Perform antialiasing. */
    for (uint16_t i = 0; i < newwidth * height; i++) {
      label_data->data[i] = clut[label_data->data[i] >> 4U];
    }
  }

  *label_size = sizeof(DiskLabel) + newwidth * height * (bgra*3+1);

  return label_data;
}

static int write_file(const char *filename, uint8_t *buffer, size_t size) {
  FILE *fh = fopen(filename, "wb+");
  if (!fh) {
    fprintf(stderr, "Cannot open file %s for writing!\n", filename);
    return -1;
  }

  if (fwrite(buffer, size, 1, fh) != 1) {
    fprintf(stderr, "Cannot write %zu bytes in %s!\n", size, filename);
    fclose(fh);
    return -1;
  }

  fclose(fh);
  return 0;
}
#endif // __APPLE__

static int read_file(const char *filename, uint8_t **buffer, size_t *size) {
  FILE *fh = fopen(filename, "rb");
  if (!fh) {
    fprintf(stderr, "Missing file %s!\n", filename);
    return -1;
  }

  if (fseek(fh, 0, SEEK_END)) {
    fprintf(stderr, "Failed to find end of %s!\n", filename);
    fclose(fh);
    return -1;
  }

  long pos = ftell(fh);

  if (pos <= 0) {
    fprintf(stderr, "Invalid file size (%ld) of %s!\n", pos, filename);
    fclose(fh);
    return -1;
  }

  if (fseek(fh, 0, SEEK_SET)) {
    fprintf(stderr, "Failed to rewind %s!\n", filename);
    fclose(fh);
    return -1;
  }

  *size = (size_t)pos;
  *buffer = (uint8_t *)malloc(*size);

  if (!*buffer) {
    fprintf(stderr, "Failed to allocate %zu bytes for %s!\n", *size, filename);
    fclose(fh);
    return -1;
  }

  if (fread(*buffer, *size, 1, fh) != 1) {
    fprintf(stderr, "Failed to read %zu bytes from %s!\n", *size, filename);
    fclose(fh);
    free(*buffer);
    return -1;
  }

  fclose(fh);
  return 0;
}

static int write_ppm(const char *filename, size_t width, size_t height, uint8_t *pixel, bool bgra) {
  FILE *fh = fopen(filename, "wb");
  if (!fh) {
    fprintf(stderr, "Failed to open out file %s!\n", filename);
    return -1;
  }

  // REF: http://netpbm.sourceforge.net/doc/pam.html
  if (fprintf(fh, "P7\nWIDTH %zu\nHEIGHT %zu\nDEPTH 4\nMAXVAL 255\nTUPLTYPE RGB_ALPHA\nENDHDR\n", width, height) < 0) {
    fprintf(stderr, "Failed to write ppm header to %s!\n", filename);
    fclose(fh);
    return -1;
  }

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x) {
      uint8_t col[4];
      if (bgra) {
        col[0] = pixel[2];
        col[1] = pixel[1];
        col[2] = pixel[0];
        col[3] = 0xFF - pixel[3];
        pixel += 4;
      } else {
        // Colour can be any and depends on the representation.
        col[0] = col[1] = col[2] = 0;
        // PAM 4th channel is opacity (0xFF is fully opaque, 0x00 is fully transparent).
        col[3] = palette[*pixel];
        pixel++;
      }
      if (fwrite(col, sizeof(col), 1, fh) != 1) {
        fprintf(stderr, "Failed to write ppm pixel %zux%zu to %s!\n", x, y, filename);
        fclose(fh);
        return -1;
      }
    }
  }

  fclose(fh);

  return 0;
}

static int decode_label(const char *infile, const char *outfile) {
  DiskLabel *label;
  size_t size;

  if (read_file(infile, (uint8_t **) &label, &size)) {
    return -1;
  }

  if (size < sizeof(DiskLabel)) {
    fprintf(stderr, "Too low size %zu!\n", size);
    free(label);
    return -1;
  }

  if (label->type != LABEL_TYPE_PALETTED && label->type != LABEL_TYPE_BGRA) {
    fprintf(stderr, "Invalid version %02X!\n", label->type);
    free(label);
    return -1;
  }

  size_t width  = BigEndianToNative16(label->width);
  size_t height = BigEndianToNative16(label->height);

  size_t exp_size = width * height;
  bool bgra = label->type == LABEL_TYPE_BGRA;
  if (bgra) {
    exp_size *= SIZE_MAX / 4 >= exp_size ? 4 : 0;
  }

  if (width * height == 0 || size - sizeof(DiskLabel) != exp_size) {
    fprintf(stderr, "Image type %d mismatch %zux%zu with size %zu!\n", label->type, width, height, size);
    free(label);
    return -1;
  }

  (void)remove(outfile);

  int ret = write_ppm(outfile, width, height, label->data, bgra);
  free(label);
  return ret;
}

static int encode_label(const char *label, bool bgra, const char *outfile, const char *outfile2x) {
#ifdef __APPLE__
  for (int scale = 1; scale <= 2; scale++) {
    const char *filename = scale == 1 ? outfile : outfile2x;
    uint16_t label_size;
    void *label_data = make_label(label, scale, bgra, &label_size);
    bool ok = label_data != NULL && write_file(filename, label_data, label_size) == 0;
    free(label_data);
    if (!ok) {
      return -2;
    }
  }
  return 0;
#else
  (void) label;
  (void) bgra;
  (void) outfile;
  (void) outfile2x;
  fprintf(stderr, "Encoding labels is unsupported!\n");
  return -1;
#endif
}

int main(int argc, char *argv[]) {
  if (argc == 4 && strcmp(argv[1], "-d") == 0) {
    return decode_label(argv[2], argv[3]);
  }

  if (argc == 5 && strcmp(argv[1], "-e") == 0) {
    return encode_label(argv[2], false, argv[3], argv[4]);
  }

  if (argc == 5 && strcmp(argv[1], "-bgra") == 0) {
    return encode_label(argv[2], true, argv[3], argv[4]);
  }

  fprintf(stderr,
    "Usage:\n"
    " disklabel -d .disk_label image.ppm!\n"
    " disklabel -e \"Label\" .disk_label .disk_label_2x\n"
    " disklabel -bgra \"Label\" .disk_label .disk_label_2x\n");
  return -1;
}
