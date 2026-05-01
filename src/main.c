#include <mo5_defs.h>
#include <mo5_video.h>

typedef unsigned char uint8_t;
#include "assets/320x200_1.h"
#include "assets/320x100_1.h"
#include "assets/155x200_1.h"
#include "assets/160x100_1.h"

#define BACKGROUND_COLOR C_BLACK
#define FOREGROUND_COLOR C_WHITE
#define DBG_BACKGROUND_COLOR C_ORANGE
#define DBG_FOREGROUND_COLOR C_MAGENTA

#define DBG_COLOR_GREEN COLOR(DBG_BACKGROUND_COLOR, C_LIGHT_GREEN)
#define DBG_COLOR_CYAN COLOR(DBG_BACKGROUND_COLOR, C_LIGHT_YELLOW)

#define SCREEN_WIDTH 320

void set_form_mode(void)
{
  *PRC |= 0x01;
}

void set_color_mode(void)
{
  *PRC &= ~0x01;
}

void init_screen()
{
  *PRC = 0x00;
  *VIDEO_REG |= 0x01;}
// mo5_mute_beep
void mute_beep(void) {
  *((unsigned char*)0xA7C1) = 0x00;
}
typedef struct
{
  unsigned char r;
  unsigned char g;
  unsigned char b;
} RGB;

unsigned char NUM_COLORS = 16;
static RGB mo5_palette[] = {
    {0, 0, 0},
    {255, 0, 0},
    {0, 255, 0},
    {255, 255, 0},
    {0, 0, 255},
    {255, 0, 255},
    {0, 255, 255},
    {255, 255, 255},
    {128, 128, 128},
    {255, 128, 128},
    {128, 255, 128},
    {255, 255, 128},
    {128, 128, 255},
    {255, 128, 255},
    {128, 255, 255},
    {255, 128, 0}};

unsigned int get_squared_relative_distance(unsigned char r1, unsigned char g1, unsigned char b1,
                                           unsigned char r2, unsigned char g2, unsigned char b2)
{
  return (r1 - r2) * (r1 - r2) + (g1 - g2) * (g1 - g2) + (b1 - b2) * (b1 - b2);
}

unsigned char find_nearest_mo5(unsigned char r, unsigned char g, unsigned char b)
{
  unsigned int best_distance = 65535;
  unsigned char nearest_color = 0;
  for (unsigned char c = 0; c < NUM_COLORS; ++c)
  {
    unsigned int dist = get_squared_relative_distance(r >> 1, g >> 1, b >> 1,
                                                      mo5_palette[c].r >> 1, mo5_palette[c].g >> 1, mo5_palette[c].b >> 1);
    if (dist < best_distance)
    {
      best_distance = dist;
      nearest_color = c;
    }
  }
  return nearest_color;
}

typedef enum
{
  NO_ERROR = 0,
  INVALID_SIGNATURE,
  UNSUPPORTED_TYPE,
  INVALID_WIDTH,
  INVALID_HEIGHT,
  INVALID_DEPTH
} BMP_STATUS;



// returns status
// size must be 320x200 or 320x-200
BMP_STATUS display_bmp1(const unsigned char *bmp)
{
  if (bmp[0] != 'B' || bmp[1] != 'M')
    return INVALID_SIGNATURE;
  if (bmp[14] != 40) // BMP4 only
    return UNSUPPORTED_TYPE;
  if (bmp[28] != 0x01 || bmp[29] != 0x00)
    return INVALID_DEPTH;
  unsigned int width16 = bmp[18] + 256 * bmp[19]; // 16 bit width
  if (bmp[20] != 0x00 || bmp[21] != 0x00)
  { // > 16 bits
    return INVALID_WIDTH;
  }
  if (width16 > 320)
    return INVALID_WIDTH;
  int height16;
  unsigned char topdown = 0;
  if (bmp[23] == 0x00 && bmp[24] == 0x00 && bmp[25] == 0x00) {
    height16 = (int)bmp[22];
  } else if (bmp[25] & 0xf0) { // negative
    height16 = (int)(bmp[22] ^ 0xff);
    topdown = 1;
  } else
    return INVALID_HEIGHT;
  if (height16 > 200)
    return INVALID_HEIGHT;

  unsigned int width_bytes16 = width16 / 8;
  if ((width_bytes16 * 8) < width16)
   width_bytes16++;
  unsigned  char width_bytes = (unsigned char)width_bytes16;
  // remap palette
  const unsigned char *palette = bmp + 54;
  unsigned char bg = find_nearest_mo5(palette[2], palette[1], palette[0]);
  unsigned char fg = find_nearest_mo5(palette[6], palette[5], palette[4]);

  mo5_clear_screen(COLOR(bg, fg));
set_form_mode();


unsigned int bmpstride16 = width16 / 32;
if ((bmpstride16 * 32) < width16)
  bmpstride16++;
bmpstride16 *= 4;
const unsigned char *src;
  for (int h = 0; h < height16; ++h) {
  unsigned char *dst = VRAM + h * SCREEN_WIDTH_BYTES;
  if (topdown == 0)
    src = bmp +  (bmpstride16 * (height16 - h - 1)) + 62;
  else
    src = bmp + (bmpstride16 * h) + 62;
     for (unsigned int w = 0; w < width_bytes16; ++w) {
      dst[w] = src[w];
    }
  }
   /*
    unsigned n = SCREEN_SIZE_BYTES;
   unsigned char* dst = VRAM;
    while(n--) {
    *dst++ = *src++;
  }
 */
  return NO_ERROR;
}

#define NUM_INPUTS 4

const unsigned char *inputs[] = {
  final_no_prescaled9_dither1bit,
    final_no_prescaled9_dither1bit_320x100,
    final_no_prescaled9_dither1bit_155x200,
    final_no_prescaled9_dither1bit_160x100
    };

int main(void) {
  mute_beep();
  init_screen();
  mo5_video_init(DBG_COLOR_GREEN);
  unsigned char cimage = 0;
  while (1) {
    BMP_STATUS status = display_bmp1(inputs[cimage]);
    if (status != NO_ERROR) {
      mo5_clear_screen(COLOR(status, status));
    }
    unsigned int frames = 100;
    while (frames--)
      mo5_wait_vbl();
    cimage = (cimage + 1) % NUM_INPUTS;
  }
  return 0;
}
