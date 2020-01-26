#ifndef BMF_FILE_H
#define BMF_FILE_H

#pragma pack(1)

typedef struct {
  UINT8  identifier;
  UINT32 size;
} BMF_BLOCK_HEADER;

VERIFY_SIZE_OF (BMF_BLOCK_HEADER, 5);

#define BMF_BLOCK_INFO_ID  1

#define BMF_BLOCK_INFO_BF_UNICODE  BIT1

//
// This tag holds information on how the font was generated.
//
typedef PACKED struct {
  INT16  fontSize;     // The size of the true type font.
  UINT8  bitField;
  UINT8  charSet;      // The name of the OEM charset used (when not unicode).
  UINT16 stretchH;     // The font height stretch in percentage. 100% means no stretch.
  UINT8  aa;           // The supersampling level used. 1 means no supersampling was used
  UINT8  paddingUp;    // The padding for each character (up, right, down, left).
  UINT8  paddingRight; // The padding for each character (up, right, down, left).
  UINT8  paddingDown;  // The padding for each character (up, right, down, left).
  UINT8  paddingLeft;  // The padding for each character (up, right, down, left).
  UINT8  spacingHoriz; // The spacing for each character (horizontal, vertical).
  UINT8  spacingVert;  // The spacing for each character (horizontal, vertical).
  UINT8  outline;      // The outline thickness for the characters.
  CHAR8  fontName[];   // This is the name of the true type font.
} BMF_BLOCK_INFO;

VERIFY_SIZE_OF (BMF_BLOCK_INFO, 14);

#define BMF_BLOCK_COMMON_ID  2

//
// This tag holds information common to all characters.
//
typedef PACKED struct {
  UINT16 lineHeight; // This is the distance in pixels between each line of text.
  UINT16 base;       // The number of pixels from the absolute top of the line to the base of the characters.
  UINT16 scaleW;     // The width of the texture, normally used to scale the x pos of the character image.
  UINT16 scaleH;     // The height of the texture, normally used to scale the y pos of the character image.
  UINT16 pages;      // The number of texture pages included in the font.
  UINT8  bitField;
  UINT8  alphaChnl;  // Set to 0 if the channel holds the glyph data, 1 if it holds the outline, 2 if it holds the glyph and the outline, 3 if its set to zero, and 4 if its set to one.
  UINT8  redChnl;    // Set to 0 if the channel holds the glyph data, 1 if it holds the outline, 2 if it holds the glyph and the outline, 3 if its set to zero, and 4 if its set to one.
  UINT8  greenChnl;  // Set to 0 if the channel holds the glyph data, 1 if it holds the outline, 2 if it holds the glyph and the outline, 3 if its set to zero, and 4 if its set to one.
  UINT8  blueChnl;   // Set to 0 if the channel holds the glyph data, 1 if it holds the outline, 2 if it holds the glyph and the outline, 3 if its set to zero, and 4 if its set to one.
} BMF_BLOCK_COMMON;

VERIFY_SIZE_OF (BMF_BLOCK_COMMON, 15);

#define BMF_BLOCK_PAGES_ID  3

//
// This tag gives the name of a texture file. There is one for each page in the font.
//
typedef CHAR8 BMF_BLOCK_PAGES;

typedef PACKED struct {
  UINT32 id;       // The character id.
  UINT16 x;        // The left position of the character image in the texture.
  UINT16 y;        // The top position of the character image in the texture.
  UINT16 width;    // The width of the character image in the texture.
  UINT16 height;   // The height of the character image in the texture.
  INT16  xoffset;  // How much the current position should be offset when copying the image from the texture to the screen.
  INT16  yoffset;  // How much the current position should be offset when copying the image from the texture to the screen.
  INT16  xadvance; // How much the current position should be advanced after drawing the character.
  UINT8  page;     // The texture page where the character image is found.
  UINT8  chnl;     // The texture channel where the character image is found (1 = blue, 2 = green, 4 = red, 8 = alpha, 15 = all channels).
} BMF_CHAR;

VERIFY_SIZE_OF (BMF_CHAR, 20);

#define BMF_BLOCK_CHARS_ID  4

//
// This tag describes on character in the font. There is one for each included character in the font.
//
typedef BMF_CHAR BMF_BLOCK_CHARS;

typedef PACKED struct {
  UINT32 first; // The first character id.
  UINT32 second; // The second character id.
  INT16  amount; // How much the x position should be adjusted when drawing the second character immediately following the first.
} BMF_KERNING_PAIR;

VERIFY_SIZE_OF (BMF_KERNING_PAIR, 10);

#define BMF_BLOCK_KERNING_PAIRS_ID  5

//
// The kerning information is used to adjust the distance between certain characters, e.g. some characters should be placed closer to each other than others.
//
typedef BMF_KERNING_PAIR BMF_BLOCK_KERNING_PAIRS;

typedef PACKED struct {
  UINT8 signature[3];
  UINT8 version;
} BMF_HEADER;

VERIFY_SIZE_OF (BMF_HEADER, 4);

#pragma pack()

#endif // BMF_FILE_H
