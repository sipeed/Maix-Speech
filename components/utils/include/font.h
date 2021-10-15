#ifndef _FONT_H
#define _FONT_H

#include "stdio.h"
#include "stdint.h"

#define FONT_NO_BG   0xff000000
#define FONT_C_BLACK 0x00000000
#define FONT_C_BLUE  0x00ff0000
#define FONT_C_GREEN 0x0000ff00
#define FONT_C_RED   0x000000ff
#define FONT_C_YELLOW 0x0000ffff
#define FONT_C_WHITE 0x00ffffff


int font_reg(const char* font_path, int font_size);
void font_unreg(int font_size);
int font_draw(uint8_t* fb, int bpp, int w, int h, int char_size, int x_oft, int y_oft, int c_color, int bg_color, char* str);
int font_get_color(uint8_t red, uint8_t green, uint8_t blue);

#endif

