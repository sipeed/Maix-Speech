#ifndef _UTILS_H
#define _UTILS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h> 

typedef struct{
    //device config
    int device_type;
    char* device_name;
    //am config
    char* model_name;
    int model_in_len;
    int strip_l;
    int strip_r;
    int phone_type;
    int agc;
    //lm config
    char* sfst_name;
    char* sym_name;
    char* phones_txt;
    char* words_txt;
    char* font_path;
    float beam;
    float bg_prob;
    float scale;
    int is_mmap;
    //lib config
    int dbg_micraw;
    int dbg_mic;
    int dbg_strip;
    int dbg_lvcsr;
    int dbgt_pp;
    int dbgt_am;
    int dbgt_kws;
    int dbgt_wfst;
    //app config
    int do_raw;
    int do_dig;
    int do_kws;
    int do_lvcsr;
    //testbench config
    char* testpath;
    int testpny;
    int testhan;
    char* testlm; //lm测试文件路径
}opts_t;

extern opts_t opts;

int parse_opts(char* cfg_file);

#endif



