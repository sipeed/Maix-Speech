#include "lm_benchmark.h"
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include "opts.h"
#include "ms_asr.h"

/********************************** benchmark private *******************************************/
#define HAN_RESULT_LEN 512
static char han_result[HAN_RESULT_LEN];
static void test_lvcsrcb(void* data, int len) 
{
    char* words = ((char**)data)[0];
    //char* pnys = ((char**)data)[1];

    strcpy(han_result, words);
    return;
}

static FILE* fw_han;
static int init_am_lm(void)
{
    int res = 0;
    am_args_t am_args = {opts.model_name, opts.model_in_len, opts.strip_l, opts.strip_r, opts.phone_type, opts.agc};
    int dbg_flag = opts.dbg_micraw*DBG_MICRAW + opts.dbg_mic*DBG_MIC + \
        opts.dbg_strip*DBG_STRIP + opts.dbg_lvcsr*DBG_LVCSR +\
        opts.dbgt_pp*DBGT_PP + opts.dbgt_am*DBGT_AM +\
        opts.dbgt_kws*DBGT_KWS + opts.dbgt_wfst*DBGT_WFST;
    res = ms_asr_init(DEVICE_WAV, "mic.wav", &am_args, dbg_flag);
    printf("Doing lm test, please cp mic.wav as dummy file\n");
    if(res != 0) {printf("ms_asr_init error!\n");return res;}

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

    return res;
}

char* open_test_file(char* pny_txt)
{
    FILE* fp = fopen(pny_txt, "r");
    if(fp==NULL) {
        printf("%s open failed\n", pny_txt);
        return NULL;
    }
    fseek(fp,0L,SEEK_END);
    size_t txt_size = ftell(fp);
    uint8_t* txt_buf = (uint8_t*)malloc(txt_size);
    if(txt_buf==NULL) {
        printf("alloc buf %ld bytes failed\n", (long int)txt_size);
        return NULL;
    }
    fseek(fp,0L,SEEK_SET);
    size_t read_cnt = fread(txt_buf, 1, txt_size, fp);
    if (read_cnt != txt_size) {
        printf("fread %ld bytes but get %ld bytes\n", (long int)txt_size, (long int)read_cnt);
        return NULL;
    }
    fclose(fp);
    return txt_buf;
}

//input: start
//output: next (not this turn)
char* get_pny(char* pny_txt)
{
    char* p = pny_txt;
    for(; *p != 0; p++){
        if(*p == '\n'){
            *p = 0;
            p+=1;
            break;
        } else if(*p == 0){
            break;
        }
    }
    if(*p == 0){
        p = NULL;
    }
    return p;
}

//gen prob list from pny list 
// dirty code
int gen_mat_from_pny(char* pnys, pnyp_t* pnyp_list)
{
    char* p_list[HAN_RESULT_LEN];
    int idx = 0;
    memset(p_list, 0, HAN_RESULT_LEN);
    p_list[idx] = pnys;
    idx+=1;
    for(char* p = pnys; *p!=0; p++){
        if(*p==' '){
            *p = 0;
            p_list[idx] = p+1;
            idx += 1;
        }
    }
    for(int i=0; i < idx; i++){
        char* pny = p_list[i];
        int pnyi = -1;
        for(int j=0; j<1250; j++){
            if(strcmp(am_vocab[j], pny)==0){
                pnyi = j;
            }
        }
        if(pnyi==-1) pnyi = 1249;
        pnyp_list[(i*2)*BEAM_CNT+0].idx = pnyi;
        pnyp_list[(i*2)*BEAM_CNT+0].p = 1.0;
        for(int j=1; j<BEAM_CNT; j++){
            pnyp_list[(i*2)*BEAM_CNT+j].idx = j;
            pnyp_list[(i*2)*BEAM_CNT+j].p = 0;
        }
        //dummy blank
        pnyp_list[(i*2+1)*BEAM_CNT+0].idx = 1249;
        pnyp_list[(i*2+1)*BEAM_CNT+0].p = 1.0;
        for(int j=1; j<BEAM_CNT; j++){
            pnyp_list[(i*2+1)*BEAM_CNT+j].idx = j;
            pnyp_list[(i*2+1)*BEAM_CNT+j].p = 0;
        }
    }


    return idx*2;
}

/********************************** lm benchmark public *******************************************/
extern volatile int exit_flag;
//输入：测试文件名, 每行填写一句的拼音，空格分割
static pnyp_t pnyp_list[HAN_RESULT_LEN*BEAM_CNT];
void lm_benchmark(char* pny_txt)
{
    //open file
    char* pny_buf = open_test_file(pny_txt);
    if(pny_buf == NULL) return;
    int model_out_len = (opts.model_in_len/8);
    int model_core_len= (model_out_len-opts.strip_l-opts.strip_r); //输出中心可用的内容
    printf("model_core_len=%d\n", model_core_len);

    // Init setting
    opts.do_raw = 0;
    opts.do_dig =0;
    opts.do_kws = 0;
    opts.do_lvcsr = 1;

    // Init AM LM
    int res = init_am_lm(); 
    if(res!=0)goto out;

    //Testing
    char* p_cur = pny_buf;
    char* p_next;
    int idx = 0;

    while(p_cur != NULL && (*p_cur != 0)){
        p_next =get_pny(p_cur);
        //printf("%s\n", p_cur);
        idx += 1;
        if(idx%100==0)printf("%d\n", idx);
        int cnt = gen_mat_from_pny(p_cur, (pnyp_t*)pnyp_list);
        int rest_cnt = model_core_len-(cnt%model_core_len);
        for(int i=0; i < rest_cnt; i++){
            pnyp_list[(cnt+i)*BEAM_CNT+0].idx = 1249;
            pnyp_list[(cnt+i)*BEAM_CNT+0].p = 1.0;
            for(int j=1; j<BEAM_CNT; j++){
                pnyp_list[(cnt+i)*BEAM_CNT+j].idx = j;
                pnyp_list[(cnt+i)*BEAM_CNT+j].p = 0;
            }
        }
        cnt += rest_cnt;
        for(int i=0; i<cnt; i+=model_core_len){
            ms_asr_wfst_run(pnyp_list+BEAM_CNT*i); //T_CNT*BEAM_CNT
        }
        
        if(exit_flag) break;
        fprintf(fw_han, "%s\n", han_result);
        ms_asr_clear();
        p_cur = p_next;
    }
    fclose(fw_han);

out:
    //TODO: free resource
    free(pny_buf);
    return;
}