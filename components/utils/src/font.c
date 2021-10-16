#include "stdio.h"
#include <fcntl.h>
#include<sys/stat.h>
#include<sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include "string.h"
#include "font.h"



extern uint8_t ascii_font_16[];
extern uint8_t ascii_font_24[];
extern uint8_t ascii_font_32[];
extern uint8_t ascii_font_48[];

#define SIZE2IDX(x)  ((x)/8-1)

static uint8_t* font_buf[8]={0,0,0,0,0,0,0,0}; //8,16,...,64
static size_t font_fsize[8]={0,0,0,0,0,0,0,0};
static uint8_t* ascii_buf[8]={0, ascii_font_16, ascii_font_24, ascii_font_32, 0, ascii_font_48, 0, 0};

//从左到右，从上到下，低位在前的扫描方式的字模
/**************************Font Private********************************/
static uint8_t* _font_get_ascii(int char_size, uint8_t data)
{
    if(ascii_buf[SIZE2IDX(char_size)] == 0) return NULL;
    if(data>=128) return NULL;
    int ascii_csize;
    if((char_size/2)%8 != 0) {
        ascii_csize = char_size*(char_size/8/2+1);
    } else {
        ascii_csize = char_size*char_size/8/2;
    }
    ascii_csize = char_size*(char_size/8/2+ (((char_size/2)%8)?1:0));
    //printf("##font %d csize=%d\n", char_size, ascii_csize);
    uint8_t* buf = ascii_buf[SIZE2IDX(char_size)];
    uint8_t* p = buf+ascii_csize*data;
    return p;
}

static uint8_t* _font_get_gbk(int char_size, uint8_t* data)
{
    if(font_buf[SIZE2IDX(char_size)] == 0) return NULL;
    int gbk_csize = char_size*char_size/8;
    uint8_t* buf = font_buf[SIZE2IDX(char_size)];
    uint8_t ch = data[0];
    uint8_t cl = data[1];
    if(ch < 0xa1 || cl < 0xa1 || ch > 0xf7 ||
       (ch >= 0xaa && ch <= 0xaf)) {
        return NULL;
    }
    
    ch -= 0xa1;
    cl -= 0xa1;
    uint32_t font_offset = (ch * 94 + cl) * gbk_csize;
    return buf+font_offset;
}

static void _font_char_draw_rgb(uint8_t* fb, int lcd_w, int lcd_h, int char_w, int char_h, int* x_oft, int* y_oft, uint8_t* char_buf, int c_color, int bg_color)
{
    if((*x_oft) + char_w > lcd_w) {
        *x_oft = 0;
        (*y_oft) += char_h;
    } 
    if((*y_oft) + char_h > lcd_h) {
        return; //已经超出屏幕边界
    }
    uint8_t* pcolor;
    pcolor = (uint8_t*)&c_color;
    uint8_t red = pcolor[0];uint8_t green = pcolor[1];uint8_t blue = pcolor[2];
    pcolor = (uint8_t*)&bg_color;
    uint8_t red_bg = pcolor[0];uint8_t green_bg = pcolor[1];uint8_t blue_bg = pcolor[2];
    //int char_point = char_w*char_h;
    
    if(bg_color!=FONT_NO_BG) {
        for(int i=0; i < char_w*char_h; i++) {
            
        }
    }
    for(int y =0; y < char_h; y++) {
        for(int x =0; x < char_w; x++) {
            int lcd_x = x+ (*x_oft);
            int lcd_y = y+ (*y_oft);
            uint8_t data = char_buf[y*(char_w/8+((char_w%8)?1:0))+x/8];
            uint8_t data_point = data&(1<<(x%8)) ? 1:0;
            if(data_point) {    //画点
                fb[(lcd_x+lcd_y*lcd_w)*3+0] = red;
                fb[(lcd_x+lcd_y*lcd_w)*3+1] = green;
                fb[(lcd_x+lcd_y*lcd_w)*3+2] = blue;
            } else {
                if(bg_color!=FONT_NO_BG) {
                    fb[(lcd_x+lcd_y*lcd_w)*3+0] = red_bg;
                    fb[(lcd_x+lcd_y*lcd_w)*3+1] = green_bg;
                    fb[(lcd_x+lcd_y*lcd_w)*3+2] = blue_bg;
                }
            }
        } 
    }
    (*x_oft) += char_w;
    return;
}

static void _font_char_draw_argb(uint8_t* fb, int lcd_w, int lcd_h, int char_w, int char_h, int* x_oft, int* y_oft, uint8_t* char_buf, int c_color, int bg_color)
{
    if((*x_oft) + char_w > lcd_w) {
        *x_oft = 0;
        (*y_oft) += char_h;
    } 
    if((*y_oft) + char_h > lcd_h) {
        return; //已经超出屏幕边界
    }
    uint8_t* pcolor;
    pcolor = (uint8_t*)&c_color;
    uint8_t red = pcolor[0];uint8_t green = pcolor[1];uint8_t blue = pcolor[2];
    pcolor = (uint8_t*)&bg_color;
    uint8_t red_bg = pcolor[0];uint8_t green_bg = pcolor[1];uint8_t blue_bg = pcolor[2];
    //int char_point = char_w*char_h;
    
    if(bg_color!=FONT_NO_BG) {
        for(int i=0; i < char_w*char_h; i++) {
            
        }
    }
    for(int y =0; y < char_h; y++) {
        for(int x =0; x < char_w; x++) {
            int lcd_x = x+ (*x_oft);
            int lcd_y = y+ (*y_oft);
            uint8_t data = char_buf[y*(char_w/8+((char_w%8)?1:0))+x/8];
            uint8_t data_point = data&(1<<(x%8)) ? 1:0;
            if(data_point) {    //画点
                fb[(lcd_x+lcd_y*lcd_w)*4+0] = red;
                fb[(lcd_x+lcd_y*lcd_w)*4+1] = green;
                fb[(lcd_x+lcd_y*lcd_w)*4+2] = blue;
                fb[(lcd_x+lcd_y*lcd_w)*4+3] = 0xff;
            } else {
                if(bg_color!=FONT_NO_BG) {
                    fb[(lcd_x+lcd_y*lcd_w)*4+0] = red_bg;
                    fb[(lcd_x+lcd_y*lcd_w)*4+1] = green_bg;
                    fb[(lcd_x+lcd_y*lcd_w)*4+2] = blue_bg;
                    fb[(lcd_x+lcd_y*lcd_w)*4+3] = 0xff;
                }
            }
        } 
    }
    (*x_oft) += char_w;
    return;
}

/**************************Font Public********************************/

int font_reg(const char* font_path, int font_size)
{
    if(font_buf[SIZE2IDX(font_size)] != NULL) {
        printf("Font %d already inited!\n", font_size);
        return -1;
    }
    int fd; //, nread;
    struct stat sb;
    if((fd = open(font_path, O_RDONLY)) < 0){
        printf("mmap open font failed\n");
        return -1;
    } 
    if((fstat(fd, &sb)) == -1 ){
        printf("fstat failed\n");
        return -1;
    }   
    font_buf[SIZE2IDX(font_size)] = (uint8_t*)mmap(\
        NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0); //MAP_SHARED, MAP_PRIVATE
    if((void*)font_buf[SIZE2IDX(font_size)] ==(void*) -1){
        printf("mmap failed\n");
        close(fd); 
        return -1;
    } 
    close(fd); 
    font_fsize[SIZE2IDX(font_size)] = (size_t)sb.st_size;
    printf("Font %d open ok! size = %ld\n", font_size, (long int)sb.st_size);
    return 0;
}

void font_unreg(int font_size)
{
    if(font_buf[SIZE2IDX(font_size)] == NULL) {
        printf("Font %d didn't inited!\n", font_size);
        return;
    }
    munmap(font_buf[SIZE2IDX(font_size)], font_fsize[SIZE2IDX(font_size)]); 
    font_buf[SIZE2IDX(font_size)] = 0;
    font_fsize[SIZE2IDX(font_size)] = 0;
    return;
}

static uint8_t unknow_charbuf[64*64/8];
int font_draw(uint8_t* fb, int bpp, int lcd_w, int lcd_h, int char_size, int x_oft, int y_oft, int c_color, int bg_color, char* str)
{
    uint8_t* char_buf = 0; 
    int char_w = char_size;
    int char_h = char_size;
    int ascii_csize = char_size*char_size/2/8;
    int gbk_csize = char_size*char_size/8;
    //int res = 0;
    if(char_size != 16 && char_size != 24 && char_size != 32 && char_size != 48) {
        printf("invalid font size %d!\n", char_size);
        return -1;
    }
    if(bpp!=24 && bpp!=32 && bpp!=16){
        printf("unsupport bpp=%d\n", bpp);
        return y_oft;
    }
    int _x_oft = x_oft;
    int _y_oft = y_oft;
    
    for(uint8_t* p = (uint8_t*)str; *p != 0; ){
        if(*p < 128) {  //ascii
            char_buf = _font_get_ascii(char_size, *p);
            if(char_buf == 0) {
                //printf("get font err for ascii %d\n", *p);
                char_buf = unknow_charbuf;
                memset(char_buf, 0xff, ascii_csize);
            }
            char_w = char_size/2;
            p+=1;
        } else {    //GBK2312
            char_buf = _font_get_gbk(char_size, p);
            if(char_buf == 0) {
                //printf("get font err for gbk %2x %2x\n", p[0], p[1]);
                char_buf = unknow_charbuf;
                memset(char_buf, 0xff, gbk_csize);p+=2;
            }
            char_w = char_size;
            p+=2;
        }
        if(bpp == 24 || bpp==16){
            _font_char_draw_rgb(fb, lcd_w, lcd_h, char_w, char_h, &_x_oft, &_y_oft, char_buf, c_color, bg_color);
        } else if(bpp == 32) {
            _font_char_draw_argb(fb, lcd_w, lcd_h, char_w, char_h, &_x_oft, &_y_oft, char_buf, c_color, bg_color);
        }
        
    }
    return _y_oft+char_h;
}

int font_get_color(uint8_t red, uint8_t green, uint8_t blue)
{
    return ((red|(green<<8)|(blue<<16))&(0x00ffffff));
    
}


