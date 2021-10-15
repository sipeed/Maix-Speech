#define IDSTRING "fbviewer 1.0"
#define DEFAULT_FRAMEBUFFER "/dev/fb0"
#define FBV_SUPPORT_JPEG
#define FBV_SUPPORT_PNG
#define FBV_SUPPORT_BMP

#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <asm/types.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Public Use Functions:
 *
 * extern int fb_display(unsigned char *rgbbuff,
 *	 int x_size, int y_size,
 *	 int x_pan, int y_pan,
 *	 int x_offs, int y_offs);
 *
 * extern int getCurrentRes(int *x,int *y);
 *
 */

__u16 red[256], green[256], blue[256];
struct fb_cmap map332 = {0, 256, red, green, blue, NULL};
__u16 red_b[256], green_b[256], blue_b[256];
struct fb_cmap map_back = {0, 256, red_b, green_b, blue_b, NULL};


int openFB(const char *name);
void closeFB(int fh);
void getVarScreenInfo(int fh, struct fb_var_screeninfo *var);
void setVarScreenInfo(int fh, struct fb_var_screeninfo *var);
void getFixScreenInfo(int fh, struct fb_fix_screeninfo *fix);
void set332map(int fh);
void* convertRGB2FB(int fh, unsigned char *rgbbuff, unsigned long count, int bpp, int *cpp);
void blit2FB(int fh, unsigned char *fbbuff, unsigned char *alpha,
             unsigned int pic_xs, unsigned int pic_ys,
             unsigned int scr_xs, unsigned int scr_ys,
             unsigned int xp, unsigned int yp,
             unsigned int xoffs, unsigned int yoffs,
             int cpp);

//https://blog.csdn.net/qwaszx523/article/details/72954308
int fb_get_bpp(void)
{
    struct fb_var_screeninfo var;
    int fh = -1;
    /* get the framebuffer device handle */
    fh = openFB(NULL);
    if(fh == -1)
        return -1;
    /* read current video mode */
    getVarScreenInfo(fh, &var);
    /* close device */
    closeFB(fh);
    return var.bits_per_pixel;
}


int fb_display(unsigned char *rgbbuff, unsigned char * alpha,
               unsigned int x_size, unsigned int y_size,
               unsigned int x_pan, unsigned int y_pan,
               unsigned int x_offs, unsigned int y_offs)
{
    struct fb_var_screeninfo var;
    struct fb_fix_screeninfo fix;
    unsigned char *fbbuff = NULL;
    int fh = -1, bp = 0;
    unsigned int x_stride;

    /* get the framebuffer device handle */
    fh = openFB(NULL);
    if(fh == -1)
        return -1;

    /* read current video mode */
    getVarScreenInfo(fh, &var);
    getFixScreenInfo(fh, &fix);

    x_stride = (fix.line_length * 8) / var.bits_per_pixel;

    /* correct panning */
    if(x_pan > x_size - x_stride) x_pan = 0;
    if(y_pan > y_size - var.yres) y_pan = 0;
    /* correct offset */
    if(x_offs + x_size > x_stride) x_offs = 0;
    if(y_offs + y_size > var.yres) y_offs = 0;
    
    /* blit buffer 2 fb */
    fbbuff = (unsigned char*)convertRGB2FB(fh, rgbbuff, x_size * y_size, var.bits_per_pixel, &bp);
#if 0
    blit2FB(fh, fbbuff, alpha, x_size, y_size, x_stride, var.yres, x_pan, y_pan, x_offs, y_offs, bp);
#else
    blit2FB(fh, fbbuff, alpha, x_size, y_size, x_stride, var.yres_virtual, x_pan, y_pan, x_offs, y_offs + var.yoffset, bp);
#endif
    free(fbbuff);

    /* close device */
    closeFB(fh);
    return 0;
}

int getCurrentRes(int *x, int *y)
{
    struct fb_var_screeninfo var;
    int fh;
    fh = openFB(NULL);
    if(fh == -1)
        return -1;
    getVarScreenInfo(fh, &var);
    *x = var.xres;
    *y = var.yres;
    closeFB(fh);
    return 0;
}

int openFB(const char *name)
{
    int fh;
    char *dev;

    if(name == NULL)
    {
        dev = getenv("FRAMEBUFFER");
        if(dev) name = dev;
        else name = DEFAULT_FRAMEBUFFER;
    }

    if((fh = open(name, O_RDWR)) == -1)
    {
        fprintf(stderr, "open %s: %s\n", name, strerror(errno));
        return -1;
    }
    return fh;
}

void closeFB(int fh)
{
    close(fh);
}

void getVarScreenInfo(int fh, struct fb_var_screeninfo *var)
{
    if(ioctl(fh, FBIOGET_VSCREENINFO, var))
    {
        fprintf(stderr, "ioctl FBIOGET_VSCREENINFO: %s\n", strerror(errno));
        exit(1);
    }
}

void setVarScreenInfo(int fh, struct fb_var_screeninfo *var)
{
    if(ioctl(fh, FBIOPUT_VSCREENINFO, var))
    {
        fprintf(stderr, "ioctl FBIOPUT_VSCREENINFO: %s\n", strerror(errno));
        exit(1);
    }
}

void getFixScreenInfo(int fh, struct fb_fix_screeninfo *fix)
{
    if (ioctl(fh, FBIOGET_FSCREENINFO, fix))
    {
        fprintf(stderr, "ioctl FBIOGET_FSCREENINFO: %s\n", strerror(errno));
        exit(1);
    }
}

void make332map(struct fb_cmap *map)
{
    int rs, gs, bs, i;
    int r = 8, g = 8, b = 4;

    map->red = red;
    map->green = green;
    map->blue = blue;

    rs = 256 / (r - 1);
    gs = 256 / (g - 1);
    bs = 256 / (b - 1);

    for (i = 0; i < 256; i++)
    {
        map->red[i]   = (rs * ((i / (g * b)) % r)) * 255;
        map->green[i] = (gs * ((i / b) % g)) * 255;
        map->blue[i]  = (bs * ((i) % b)) * 255;
    }
}

void set8map(int fh, struct fb_cmap *map)
{
    if (ioctl(fh, FBIOPUTCMAP, map) < 0)
    {
        fprintf(stderr, "Error putting colormap");
        exit(1);
    }
}

void get8map(int fh, struct fb_cmap *map)
{
    if (ioctl(fh, FBIOGETCMAP, map) < 0)
    {
        fprintf(stderr, "Error getting colormap");
        exit(1);
    }
}

void set332map(int fh)
{
    make332map(&map332);
    set8map(fh, &map332);
}

void blit2FB(int fh, unsigned char *fbbuff, unsigned char *alpha,
             unsigned int pic_xs, unsigned int pic_ys,
             unsigned int scr_xs, unsigned int scr_ys,
             unsigned int xp, unsigned int yp,
             unsigned int xoffs, unsigned int yoffs,
             int cpp)
{
    int i, xc, yc;
    unsigned char *fb;

    unsigned char *fbptr;
    unsigned char *imptr;

    xc = (pic_xs > scr_xs) ? scr_xs : pic_xs;
    yc = (pic_ys > scr_ys) ? scr_ys : pic_ys;
    //printf("scr_xs * scr_ys * cpp=%d, %d, %d; xc,yc=%d,%d\n", scr_xs , scr_ys , cpp, xc, yc);

    fb = (unsigned char*)mmap(NULL, scr_xs * scr_ys * cpp, PROT_WRITE | PROT_READ, MAP_SHARED, fh, 0);

    if(fb == MAP_FAILED)
    {
        perror("mmap");
        return;
    }

    if(cpp == 1)
    {
        get8map(fh, &map_back);
        set332map(fh);
    }

    fbptr = fb + (yoffs * scr_xs + xoffs) * cpp;
    imptr = fbbuff + (yp * pic_xs + xp) * cpp;

    if(alpha)
    {
        unsigned char * alphaptr;
        int from, to, x;

        alphaptr = alpha + (yp  * pic_xs + xp);

        for(i = 0; i < yc; i++, fbptr += scr_xs * cpp, imptr += pic_xs * cpp, alphaptr += pic_xs)
        {
            for(x = 0; x<xc; x++)
            {
                int v;

                from = to = -1;
                for(v = x; v<xc; v++)
                {
                    if(from == -1)
                    {
                        if(alphaptr[v] > 0x80) from = v;
                    }
                    else
                    {
                        if(alphaptr[v] < 0x80)
                        {
                            to = v;
                            break;
                        }
                    }
                }
                if(from == -1)
                    break;

                if(to == -1) to = xc;

                memcpy(fbptr + (from * cpp), imptr + (from * cpp), (to - from - 1) * cpp);
                x += to - from - 1;
            }
        }
    }
    else{printf("#");
        for(i = 0; i < yc; i++, fbptr += scr_xs * cpp, imptr += pic_xs * cpp){
            memcpy(fbptr, imptr, xc * cpp);
        }
    }

    if(cpp == 1)
        set8map(fh, &map_back);

    munmap(fb, scr_xs * scr_ys * cpp);
}

inline static unsigned char make8color(unsigned char r, unsigned char g, unsigned char b)
{
    return (
            (((r >> 5) & 7) << 5) |
            (((g >> 5) & 7) << 2) |
            ((b >> 6) & 3)	   );
}

inline static unsigned short make15color(unsigned char r, unsigned char g, unsigned char b)
{
    return (
            (((r >> 3) & 31) << 10) |
            (((g >> 3) & 31) << 5)  |
            ((b >> 3) & 31)		);
}

inline static unsigned short make16color(unsigned char r, unsigned char g, unsigned char b)
{
    return (
            (((r >> 3) & 31) << 11) |
            (((g >> 2) & 63) << 5)  |
            ((b >> 3) & 31)		);
}

void* convertRGB2FB(int fh, unsigned char *rgbbuff, unsigned long count, int bpp, int *cpp)
{
    unsigned long i;
    void *fbbuff = NULL;
    u_int8_t  *c_fbbuff;
    u_int16_t *s_fbbuff;
    u_int32_t *i_fbbuff;

    switch(bpp)
    {
        case 8:
            *cpp = 1;
            c_fbbuff = (unsigned char *) malloc(count * sizeof(unsigned char));
            for(i = 0; i < count; i++)
                c_fbbuff[i] = make8color(rgbbuff[i*3], rgbbuff[i*3+1], rgbbuff[i*3+2]);
            fbbuff = (void *) c_fbbuff;
            break;
        case 15:
            *cpp = 2;
            s_fbbuff = (unsigned short *) malloc(count * sizeof(unsigned short));
            for(i = 0; i < count ; i++)
                s_fbbuff[i] = make15color(rgbbuff[i*3], rgbbuff[i*3+1], rgbbuff[i*3+2]);
            fbbuff = (void *) s_fbbuff;
            break;
        case 16:
            *cpp = 2;
            s_fbbuff = (unsigned short *) malloc(count * sizeof(unsigned short));
            for(i = 0; i < count ; i++)
                s_fbbuff[i] = make16color(rgbbuff[i*3], rgbbuff[i*3+1], rgbbuff[i*3+2]);
            fbbuff = (void *) s_fbbuff;
            break;
        case 24:
            *cpp = 3;
            c_fbbuff = (unsigned char *) malloc(count * 3 * sizeof(unsigned char));
            for(i = 0; i < count ; i++) {
                c_fbbuff[i * 3 + 0] = rgbbuff[i * 3 + 2];
                c_fbbuff[i * 3 + 1] = rgbbuff[i * 3 + 1];
                c_fbbuff[i * 3 + 2] = rgbbuff[i * 3 + 0];
            }
            fbbuff = (void *) c_fbbuff;
            break;
        case 32:
            *cpp = 4;
            i_fbbuff = (unsigned int *) malloc(count * sizeof(unsigned int));
            for(i = 0; i < count ; i++)
                i_fbbuff[i] = ((0xFF << 24) & 0xFF000000) |
                              ((rgbbuff[i*3] << 16) & 0xFF0000) |
                              ((rgbbuff[i*3+1] << 8) & 0xFF00) |
                              (rgbbuff[i*3+2] & 0xFF);
            fbbuff = (void *) i_fbbuff;
            break;
        default:
            fprintf(stderr, "Unsupported video mode! You've got: %dbpp\n", bpp);
            exit(1);
    }
    return fbbuff;
}
