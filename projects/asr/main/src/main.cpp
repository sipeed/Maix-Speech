
extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include "string.h"
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/select.h>
#include "fbviewer.h"

#include <linux/input.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>



#include "opts.h"
#include "font.h"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#include "benchmark.h" 
#include "lm_benchmark.h" 
#include "ms_asr.h"

extern void decoder_kws_test(void);
extern void am_infer_test(void);
extern void mic_test(void);
}

#define DEV_PATH "/dev/input/event0" //keys
//#include "opencv2/opencv.hpp"
//using namespace cv;

#define DBG_LINE()  //printf("### L%d\n", __LINE__)
#define DBG(format, ...) printf(format, __VA_ARGS__)

#define LCD_W 240
#define LCD_H 240
uint8_t lcd_buf[LCD_W*LCD_H*4];
static int l_bpp = 24;
volatile int exit_flag = 0;


static int tick = 0;
void my_rawcb(void* data, int len)  //raw��lenָ���������ÿ������BEAM_CNT�����
{
    pnyp_t* res = (pnyp_t*)data;
    printf("===================================\n");
    for(int t=0; t<len; t++) {
        pnyp_t* pp = res+BEAM_CNT*t;
        if(1){ 
            printf("T=%04d ====:", tick);
            tick += 1;  //ȫ��ʱ��+1
            for(int i=0; i < 3; i++) { //BEAM_CNT
                printf("  %4d %-6s: %.3f;", pp[i].idx, am_vocab[pp[i].idx], ((float)(pp[i].p)));
            }
        }
        printf("\n");
    }
    printf("####\n");
    return;
}

/*
int fb_display(unsigned char *rgbbuff, unsigned char * alpha,
               unsigned int x_size, unsigned int y_size,
               unsigned int x_pan, unsigned int y_pan,
               unsigned int x_offs, unsigned int y_offs);
*/
//DIGIT_LOG_LEN
#define DIG_H 32
void my_digitcb(void* data, int len)
{
    char* digit_res = (char*) data;
    char digit_res1[32];
    memset(digit_res1, ' ', 32);
    digit_res1[32-1]=0;
    memcpy(digit_res1, digit_res, strlen(digit_res));
    printf("digit_res: %s\n", digit_res);

    memset(lcd_buf+32*LCD_W*l_bpp/8, 0, LCD_W*(LCD_H-32)*l_bpp/8);
    //font_draw(lcd_buf, l_bpp, LCD_W, LCD_H, 32, 0, 0, FONT_C_WHITE, FONT_C_BLACK, (char*)"DIGIT");
    int last_y = font_draw(lcd_buf, l_bpp, LCD_W, LCD_H, 32, 0, 32, FONT_C_YELLOW, FONT_C_BLUE, digit_res);
    fb_display(lcd_buf, 0, LCD_W, last_y, 0, 0, 0, 0);
    return;
}


void my_lvcsrcb(void* data, int len)
{
    char* words = ((char**)data)[0];
    char* pnys = ((char**)data)[1];

    memset(lcd_buf+32*LCD_W*l_bpp/8, 0, LCD_W*(LCD_H-32)*l_bpp/8);
    //font_draw(lcd_buf, l_bpp, LCD_W, LCD_H, 32, 0, 0, FONT_C_WHITE, FONT_C_BLACK, (char*)"LVCSR");
    int last_y = font_draw(lcd_buf, l_bpp, LCD_W, LCD_H, 32, 0, 32, FONT_C_YELLOW, FONT_C_BLUE, words);
    int flash_h = last_y>LCD_H ? LCD_H : last_y;
    fb_display(lcd_buf, 0, LCD_W, flash_h, 0, 0, 0, 0);
    printf("PNYS: %s\n", pnys);
    return ;
}

//int font_draw(uint8_t* fb, int lcd_w, int lcd_h, int char_size, int x_oft, int y_oft, int c_color, int bg_color, char* str);
/*int fb_display(unsigned char *rgbbuff, unsigned char * alpha,
               unsigned int x_size, unsigned int y_size,
               unsigned int x_pan, unsigned int y_pan,
               unsigned int x_offs, unsigned int y_offs) */
char* my_kw_tbl[3]={
    (char*)"xiao3 ai4 tong2 xue2",
    (char*)"tian1 mao1 jing1 ling2",
    (char*)"tian1 qi4 zen3 me yang4"
};
char* my_kw_str[3]={
    (char*)"XIAO AI TONG XU",
    (char*)"TIAN MAO JING L",
    (char*)"TIAN QI ZEN ME"
};
float my_kw_gate[3] = {0.1, 0.1, 0.1};
void my_kwscb(void* data, int len)
{
    float* p = (float*) data;
    float maxp = -1;
    int   maxi = 0;
    for(int i=0; i<len; i++){
        printf("\tkw%d: %.3f;", i, p[i]);
        if(p[i] > maxp){
            maxp = p[i];
            maxi = i;
        }
    }
    printf("\n");

    memset(lcd_buf+32*LCD_W*l_bpp/8, 0, LCD_W*(LCD_H-32)*l_bpp/8);
    //font_draw(lcd_buf, l_bpp, LCD_W, LCD_H, 32, 0, 0, FONT_C_WHITE, FONT_C_BLACK, (char*)"KWS");
    if(maxp > 0.1){
        font_draw(lcd_buf, l_bpp, LCD_W, LCD_H, 32, 0, (maxi+1)*32, FONT_C_YELLOW, FONT_C_BLUE, my_kw_str[maxi]);
    }
    fb_display(lcd_buf, 0, LCD_W, 32*5, 0, 0, 0, 0); 
    return ;
}


void my_handler(int s){
    printf("Caught signal %d\n",s);
    exit_flag = 1;
    return;
}


void font_test(void)
{   
    char test_data[3] = {(char)0xc8, (char)0x41, (char)0};
    font_draw(lcd_buf, l_bpp, LCD_W, LCD_H, 16, 0, 0, FONT_C_RED, FONT_C_BLACK, (char*)"�ֿ����123abc");
    font_draw(lcd_buf, l_bpp, LCD_W, LCD_H, 24, 0, 16, FONT_C_GREEN, FONT_C_BLACK, (char*)"�ֿ����123abc");
    font_draw(lcd_buf, l_bpp, LCD_W, LCD_H, 32, 0, 16+24, FONT_C_BLUE, FONT_C_BLACK, (char*)"�ֿ����123abc");
    font_draw(lcd_buf, l_bpp, LCD_W, LCD_H, 48, 0, 16+24+32, FONT_C_WHITE, FONT_C_BLACK, (char*)"�ֿ����123abc");
    font_draw(lcd_buf, l_bpp, LCD_W, LCD_H, 48, 0, 16+24+32+48, FONT_C_WHITE, FONT_C_BLACK, test_data);
    fb_display(lcd_buf, 0, LCD_W, LCD_H, 0, 0, 0, 0);
    return;
}

void lcd_init(void)
{
    l_bpp = fb_get_bpp();
    printf("### bpp=%d\n", l_bpp);
    return;
}

void lcd_clear(void)
{return;
    //while(1)
    {
        memset(lcd_buf, 0xff, LCD_W*LCD_H*l_bpp/8);
        fb_display(lcd_buf, 0, LCD_W, LCD_H, 0, 0, 0, 0);
        //sleep(1);
        memset(lcd_buf, 0x00, LCD_W*LCD_H*l_bpp/8);
        fb_display(lcd_buf, 0, LCD_W, LCD_H, 0, 0, 0, 0);
        //sleep(1);
    }
    return;
}

static int keys_fd;
void key_init(void)
{
    keys_fd=open(DEV_PATH, O_RDONLY);
    if(keys_fd <= 0)
    {
        printf("open /dev/input/event0 device error!\n");
        return;
    }
}

int key_read(void)
{
    int res = 0;
    struct input_event t;
    struct timeval timeout;
    fd_set set;
    int rv;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1000;
    FD_ZERO(&set); /* clear the set */
    FD_SET(keys_fd, &set); 
    rv = select(keys_fd + 1, &set, NULL, NULL, &timeout);
    if(rv == -1) {
        perror("select"); /* an error accured */
    } else if(rv == 0) {
        printf("key timeout; "); /* a timeout occured */
    } else {
        int len = read(keys_fd, &t, sizeof(t)); /* there was data to read */
        if(len==sizeof(t)) {
            if(t.type == EV_KEY) {
                if(t.value == 0 || t.value==1){
                    printf("key %d %s\n", t.code, (t.value) ? "Pressed" : "Released");
                    if(t.value==1) res = 1;
                }
            }
        }
    }
    return res;
}

void key_deinit(void)
{
    close(keys_fd);
    return;
}

void set_dig(int flag)
{
    size_t decoder_args[10]; 
    int res = 0;
    if(flag){ //init
        decoder_args[0] = 640;  //blank_t ms
        res = ms_asr_decoder_cfg(DECODER_DIG, my_digitcb , &decoder_args, 1);
        if(res != 0) {printf("DECODER_DIG init error!\n");};
        lcd_clear(); 
        font_draw(lcd_buf, l_bpp, LCD_W, LCD_H, 32, 0, 0, FONT_C_WHITE, FONT_C_BLACK, (char*)"DIGIT");
        fb_display(lcd_buf, 0, LCD_W, LCD_H, 0, 0, 0, 0);
    } else {
        ms_asr_decoder_cfg(DECODER_DIG, NULL , NULL, 0);
    }
    return;
}

void set_kws(int flag)
{
    size_t decoder_args[10]; 
    int res = 0;
    if(flag){ //init 
        decoder_args[0] = (size_t)my_kw_tbl;
        decoder_args[1] = (size_t)my_kw_gate;
        decoder_args[2] = 3;
        decoder_args[3] = 1;  //auto similar //自动去除音调 近音处理
        res = ms_asr_decoder_cfg(DECODER_KWS, my_kwscb , &decoder_args, 3);
        if(res != 0) {printf("DECODER_KWS init error!\n");goto out1;};
        char* similar_pnys0[1] = {(char*)"xiang3"};    //每个最多注册10个近音词
        ms_asr_kws_reg_similar((char*)"xiao3", similar_pnys0, 1);
        char* similar_pnys1[3] = {(char*)"xin1", (char*)"ting1", (char*)"jin1"};
        ms_asr_kws_reg_similar((char*)"jing1", similar_pnys1, 3);
        lcd_clear(); 
        font_draw(lcd_buf, l_bpp, LCD_W, LCD_H, 32, 0, 0, FONT_C_WHITE, FONT_C_BLACK, (char*)"KWS");
        fb_display(lcd_buf, 0, LCD_W, LCD_H, 0, 0, 0, 0);
    } else {
        ms_asr_decoder_cfg(DECODER_KWS, NULL , NULL, 0);
    }
out1:
    return;
}

void set_lvcsr(int flag)
{
    size_t decoder_args[10]; 
    int res = 0;
    if(flag){ //init 
        decoder_args[0] = (size_t)opts.sfst_name;
        decoder_args[1] = (size_t)opts.sym_name;
        decoder_args[2] = (size_t)opts.phones_txt;
        decoder_args[3] = (size_t)opts.words_txt;
        memcpy(&decoder_args[4], &(opts.beam), sizeof(float));
        memcpy(&decoder_args[5], &(opts.bg_prob), sizeof(float));
        memcpy(&decoder_args[6], &(opts.scale), sizeof(float));
        decoder_args[7] = (size_t)opts.is_mmap;
        res = ms_asr_decoder_cfg(DECODER_LVCSR, my_lvcsrcb , &decoder_args, 8);
        if(res != 0) {printf("DECODER_LVCSR init error!\n");};
        lcd_clear(); 
        font_draw(lcd_buf, l_bpp, LCD_W, LCD_H, 32, 0, 0, FONT_C_WHITE, FONT_C_BLACK, (char*)"LVCSR");
        fb_display(lcd_buf, 0, LCD_W, LCD_H, 0, 0, 0, 0);
    } else {
        ms_asr_decoder_cfg(DECODER_LVCSR, NULL , NULL, 0);
    }
    return;
}


int main(int argc, char const* argv[])
{	
	printf("==============================================\n");
	printf("MaixSense R329 ASR Demo\n");
	printf("Developed by Sipeed.   \n");
	printf("Email: support@sipeed.com\n");
	printf("Usage: ./zhouyi_asr cfg_file\n");
    printf("config file contains key:value every line\n");
	printf("==============================================\n\n");

    signal(SIGINT, my_handler); 
    key_init();

    int res = 0;
    int demo_idx = 0;
    //��������
    char* cfg_file = NULL;
	if(argc < 2) {
		printf("argc=%d error\n", argc);
		return -1;
	}
    cfg_file = (char*)argv[1];
    if(parse_opts(cfg_file) != 0) {
        printf("parse_opts failed! check your cfg file\n");
        return -1;
    }
    //��ʼ���ֿ�
    char font_path[100];
    strcpy(font_path, opts.font_path); strcat(font_path, "16.FON"); res += font_reg(font_path, 16);
    strcpy(font_path, opts.font_path); strcat(font_path, "24.FON"); res += font_reg(font_path, 24);
    strcpy(font_path, opts.font_path); strcat(font_path, "32.FON"); res += font_reg(font_path, 32);
    //�ֿ���ʾ����
    //font_test();
    lcd_init();

    if(opts.testpath != NULL){
        benchmark(opts.testpath, opts.testpny, opts.testhan);
        return 0;
    }
    if(opts.testlm != NULL){
        lm_benchmark(opts.testlm);
        return 0;
    }

    //mic_test();return 0;

    //��ʼ��asr lib
    am_args_t am_args = {opts.model_name, opts.model_in_len, opts.strip_l, opts.strip_r, opts.phone_type, opts.agc};
    int dbg_flag = opts.dbg_micraw*DBG_MICRAW + opts.dbg_mic*DBG_MIC + \
        opts.dbg_strip*DBG_STRIP + opts.dbg_lvcsr*DBG_LVCSR +\
        opts.dbgt_pp*DBGT_PP + opts.dbgt_am*DBGT_AM +\
        opts.dbgt_kws*DBGT_KWS + opts.dbgt_wfst*DBGT_WFST;
    printf("dbg_flag = 0x%x\n", dbg_flag);
    res = ms_asr_init(opts.device_type, opts.device_name, &am_args, dbg_flag);

    if(res != 0) {
        printf("ms_asr_init error!\n");
        return -1;
    }

    //am_infer_test();
    //decoder_kws_test();
    //return 0;

    //��ʼ��������
    
    if(opts.do_raw) {
        res = ms_asr_decoder_cfg(DECODER_RAW, my_rawcb , NULL, 0);
        if(res != 0) {printf("DECODER_RAW init error!\n");goto free_decoder;};
    }
 #if 1   
    if(opts.do_dig) set_dig(1);
    if(opts.do_kws) set_kws(1);
    if(opts.do_lvcsr) set_lvcsr(1);

    lcd_clear();
    //����
    while(!exit_flag) {
        int frames = ms_asr_run(1); //ÿ����һ֡
        if(frames<1) {
            printf("run out\n");
            break;
        }
        if(key_read()==1) {
            printf("Reset ASR state!\n");
            //ms_asr_clear();
            lcd_clear();
        }
    }
#else 
    lcd_clear();
    
    set_dig(1);
    //����
    while(!exit_flag) {
        int frames = ms_asr_run(1); //ÿ����һ֡
        if(frames<1) {
            printf("run out\n");
            break;
        }
        if(key_read()==1) {
            printf("Change ASR state!\n");
            demo_idx+=1;
            demo_idx = demo_idx%3;
            if(demo_idx==0){
                set_dig(1); set_kws(0); set_lvcsr(0);
            } else if(demo_idx==1){
                set_dig(0); set_kws(1); set_lvcsr(0); 
            }else{
                set_dig(0); set_kws(0); set_lvcsr(1);
            }
        }
    }
#endif
    printf("Done~~~\n");
    //�ͷ�
free_decoder:
    ms_asr_deinit();
    font_unreg(16);
    font_unreg(24);
    font_unreg(32);
    //font_unreg(48);
    key_deinit();
	return res;
}
