#ifndef _SP_ASR_H
#define _SP_ASR_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ms_asr_tbl.h"

/******************************* MARCO CONFIG ************************************/
#define VERSION_RELEASE 0

#define INFER_ZHOUYI    0
#define INFER_V83X      1
#define INFER_CPU0      2
#define INFER_CPU1      3

//#define INFER_TYPE      INFER_CPU  //cmake里设置

/******************************* MARCO ************************************/
#define DEVICE_PCM    0
#define DEVICE_MIC    1
#define DEVICE_WAV    2
#define DEVICE_MIC2   3
#define DEVICE_MIC4   4
#define DEVICE_CUSTOM 5

#define DECODER_RAW   1
#define DECODER_DIG   2
#define DECODER_LVCSR 4
#define DECODER_KWS   8 
#define DECODER_ALL   0xffff 

//asr lib的调试标志位，最多32个
//release 版本时，所有DBG选项失效
#if VERSION_RELEASE 
    #define DBG_MICRAW  0
    #define DBG_MIC     0
    #define DBG_STRIP   0
    #define DBG_LVCSR   0
    #define DBGT_PP     0
    #define DBGT_AM     0
    #define DBGT_KWS    0
    #define DBGT_WFST   0
#else 
    #define DBG_MICRAW  (1<<0)
    #define DBG_MIC     (1<<1)
    #define DBG_STRIP   (1<<2)
    #define DBG_LVCSR   (1<<3)
    #define DBGT_PP     (1<<4)
    #define DBGT_AM     (1<<5)
    #define DBGT_KWS    (1<<6)
    #define DBGT_WFST   (1<<7)
#endif

#define CN_PHONE        0
#define CN_PNY          1
#define CN_PNYTONE      2
#define CN_HAN          3
#define EN_SW1K         4
#define EN_SW3K         5
#define EN_SW5K         6
//en subword: https://bpemb.h-its.org/en/

#define QUANT_NONE      0
#define QUANT_INT8      1
#define QUANT_UINT8     2


#define BEAM_CNT        (10)             //每个时刻保留CNT个解码结果
#define ASR_KW_MAX_PNY  (6)              //最多6个字
#define ASR_KW_MAX      (100)            //最多100个关键词


typedef struct{
    int  (*init)(char* device_name);            //初始化音频设备/文件
    int  (*read)(int16_t* data_buf, int len);   //返回读取到的数量
    void (*clear)(void);                        //清缓存
    void (*deinit)(void);
}asr_device_t;

typedef struct{
    char* model_name;
    int model_in_len;
    int strip_l;
    int strip_r;
    int phone_type;
    int agc;
}am_args_t;

typedef struct
{
	uint32_t idx;  //pny的下标
	float p;
}pnyp_t;


typedef void (*decoder_cb_t)(void* data, int cnt);

int  ms_asr_init(int device_type, char* device_name, am_args_t* am_args, int dbg_flag);
void ms_asr_deinit(void);
int  ms_asr_decoder_cfg(int decoder_type, decoder_cb_t decoder_cb, void* decoder_args, int decoder_argc);
void ms_asr_clear(void);    //重置内部缓存操作，在开启新的语音识别前清一次 
int  ms_asr_run(int frame); 
    //frmae: 每次run的帧数; 返回实际run的帧数
    //用户可以每次run 1帧，run完做其他处理；或者一个线程里一直run，由外部线程stop来停止
int ms_asr_get_frame_time(void); //返回一帧的时间 ms
void ms_asr_get_am_vocab(char** vocab, int* cnt);
int ms_asr_set_dev(int device_type, char* device_name); //重新设置asr的数据源，一般在pcm/wav识别时使用
int ms_asr_kws_reg_similar(char* pny, char** similar_pnys, int similar_cnt);
void ms_asr_wfst_run(pnyp_t* pnyp_list); //输入 T_CNT*BEAM_CNT 个拼音结果，进行decoder推理，用于lm的测试
//int ms_asr_get_frame_time(void); //返回一帧的时间 ms
#endif



