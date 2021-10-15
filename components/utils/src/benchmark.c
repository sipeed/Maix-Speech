#include "benchmark.h"
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include "opts.h"
#include "ms_asr.h"

static uint32_t _cal_dt_us(struct timespec *t0, struct timespec *t1)
{
    uint32_t dt;
    if(t1->tv_nsec < t0->tv_nsec) {
        long tmp = t1->tv_nsec + 1e9;
        long d_ns = tmp - t0->tv_nsec;
        long d_s  = t1->tv_sec-1-t0->tv_sec;
        dt = (uint32_t)(d_s*1000000+d_ns/1000);
    } else {
        long d_ns = t1->tv_nsec - t0->tv_nsec;
        long d_s  = t1->tv_sec-t0->tv_sec;
        dt = (uint32_t)(d_s*1000000+d_ns/1000);
    }
    return dt;
}

//CLOCK_MONOTONIC_COARSE  CLOCK_MONOTONIC
#define DBG_TIME_INIT()     struct timespec start,finish;clock_gettime(CLOCK_MONOTONIC,&start);
#define DBG_TIME_START()    clock_gettime(CLOCK_MONOTONIC,&start);
#define DBG_TIME(x)   {clock_gettime(CLOCK_MONOTONIC,&finish);printf("%s use %.3f ms\n", (x), (double)(_cal_dt_us(&start, &finish))/1000.0);clock_gettime(CLOCK_MONOTONIC,&start);}



/********************************** benchmark private *******************************************/
#define MAX_WAV_CNT (10000)
static char file_name[MAX_WAV_CNT][64];   //最大测试10K个文件
static int file_cnt = 0;
static int trave_dir(char* path, int depth)
{
    DIR *d; 
    struct dirent *file; 
    struct stat sb;   
    if(!(d = opendir(path)))
    {
        printf("error opendir %s!!!\n",path);
        return -1;
    }
    printf("##DIR=%s\n", path);
    while((file = readdir(d)) != NULL)
    { 
        if(file_cnt>=MAX_WAV_CNT) break;
        char full_path[128];
        strcpy(full_path, path);
        strcat(full_path, file->d_name);
        if(strncmp(file->d_name, ".", 1) == 0) {//把当前目录.，上一级目录..及隐藏文件都去掉，避免死循环遍历目录
            continue;
        } else if(stat(full_path, &sb) >= 0 && S_ISDIR(sb.st_mode) && depth <= 3)
        {   //如果是目录，遍历进去，最深3层
            strcat(full_path, "/");
            trave_dir(full_path, depth + 1);
        } else{
            int fname_len = strlen(full_path);
            if(fname_len<5) continue;
            if(strcmp(&full_path[fname_len-4], ".wav") == 0){
                strcpy(file_name[file_cnt++], full_path); //保存遍历到的wav
                //printf("%s\n", full_path);
            }
        }
    }
    closedir(d);
    return 0;
}

#define PNY_RESULT_LEN 512
static char pny_result[PNY_RESULT_LEN];
static void test_rawcb(void* data, int len) 
{
    char* dummy; 
    int vocab_cnt;
    ms_asr_get_am_vocab(&dummy, &vocab_cnt);
    pnyp_t* res = (pnyp_t*)data;
    for(int t=0; t<len; t++) {
        pnyp_t* pp = res+BEAM_CNT*t;
        if(pp[0].idx == vocab_cnt-1){
            continue;   //静音
        } else {
            strcat(pny_result, am_vocab[pp[0].idx]);
            strcat(pny_result, " ");
        }
    }
    //printf("%s\n", pny_result);
    return;
}

#define HAN_RESULT_LEN 512
static char han_result[HAN_RESULT_LEN];
static void test_lvcsrcb(void* data, int len) 
{
    char* words = ((char**)data)[0];
    //char* pnys = ((char**)data)[1];

    strcpy(han_result, words);
    return;
}

static FILE* fw_pny;
static FILE* fw_han;
static int init_am_lm(char* file_name, int testpny, int testhan)
{
    int res = 0;
    am_args_t am_args = {opts.model_name, opts.model_in_len, opts.strip_l, opts.strip_r, opts.phone_type, opts.agc};
    res = ms_asr_init(DEVICE_WAV, file_name, &am_args, 0);
    if(res != 0) {printf("ms_asr_init error!\n");return res;}
    if(testpny) {
        res = ms_asr_decoder_cfg(DECODER_RAW, test_rawcb , NULL, 0);
        if(res != 0) {printf("DECODER_RAW init error!\n");return res;};
        fw_pny = fopen("pny_result.txt", "w");
        printf("Init testpny ok!\n");
    }
    if(testhan){
        size_t decoder_args[10]; 
        decoder_args[0] = (size_t)opts.sfst_name;
        decoder_args[1] = (size_t)opts.sym_name;
        decoder_args[2] = (size_t)opts.phones_txt;
        decoder_args[3] = (size_t)opts.words_txt;
        memcpy(&decoder_args[4], &(opts.beam), sizeof(float));
        memcpy(&decoder_args[5], &(opts.bg_prob), sizeof(float));
        memcpy(&decoder_args[6], &(opts.scale), sizeof(float));
        decoder_args[7] = (size_t)opts.is_mmap;
        res = ms_asr_decoder_cfg(DECODER_LVCSR, test_lvcsrcb , &decoder_args, 8);
        if(res != 0) {printf("DECODER_LVCSR init error!\n");return res;};
        fw_han = fopen("han_result.txt", "w");
        printf("Init testhan ok!\n");
    }
    return res;
}

/********************************** benchmark public *******************************************/
extern volatile int exit_flag;
void benchmark(char* path, int testpny, int testhan)
{
    //Iter test wav files
    file_cnt = 0;
    int total_frames = 0;
    int res = trave_dir(path, 1);
    if(res!=0) return;

    // Init setting
    if(testpny) opts.do_raw = 1;
    if(testhan) opts.do_lvcsr = 1;
    opts.do_dig =0;
    opts.do_kws =0;
    if(testpny==0 && testhan==0){
        printf("Must select at least one item to test!\n");
        return;
    }

    // Init AM LM
    res = init_am_lm(file_name[0], testpny, testhan); 
    if(res!=0)goto out;

    //Testing
    DBG_TIME_INIT();DBG_TIME_START(); 
    for(int i = 0; i < file_cnt; i++)
    {   
        res = ms_asr_set_dev(DEVICE_WAV, file_name[i]); 
        if(res!=0)break;
        printf("%04d: predict %s\n", i, file_name[i]);
        memset(pny_result, 0, PNY_RESULT_LEN);
        memset(han_result, 0, HAN_RESULT_LEN);

        while(!exit_flag) {
            total_frames += 1;
            int frames = ms_asr_run(1); 
            if(frames<1) {
                break;
            }
        }
        if(exit_flag) break;
        if(fw_pny)fprintf(fw_pny, "%s\t%s\n", file_name[i], pny_result);
        if(fw_han)fprintf(fw_han, "%s\t%s\n", file_name[i], han_result);
        ms_asr_clear();
    }
    if(testpny)fclose(fw_pny);
    if(testhan)fclose(fw_han);
    clock_gettime(CLOCK_MONOTONIC,&finish);
    float dt = (float)(_cal_dt_us(&start, &finish))/1000.0;
    printf("Total %d files, %d frames, %.1fs wav use %.1fs cal, RTF = %.1f%%\n", file_cnt, total_frames, \
        total_frames*768.0/1000, dt/1000, 100.0*dt/(total_frames*768)); //edit 768 as your real model param

out:
    //TODO: free resource
    return;
}