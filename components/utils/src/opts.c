#include "opts.h"
#include "ms_asr.h"

/*************************************private***************************************/
static int _get_device_type(char* p_val)
{
    int res = -1;
    if(strcmp(p_val, "pcm")==0) {
        res = DEVICE_PCM;
    } else if(strcmp(p_val, "mic")==0) {
        res = DEVICE_MIC;
    } else if(strcmp(p_val, "wav")==0) {
        res = DEVICE_WAV;
    } else if(strcmp(p_val, "mic2")==0) {
        res = DEVICE_MIC2;
    } else if(strcmp(p_val, "mic4")==0) {
        res = DEVICE_MIC4;
    } else if(strcmp(p_val, "custom")==0) {
        res = DEVICE_CUSTOM;
    } else {
        printf("device_type %s not support!\n", p_val);
        res = -1;
    }
    return res;
}

static int _get_phone_type(char* p_val)
{
    int res = -1;
    if(strcmp(p_val, "phone")==0) {
        res = CN_PHONE;
    } else if(strcmp(p_val, "pny")==0) {
        res = CN_PNY;
    } else if(strcmp(p_val, "pnytone")==0) {
        res = CN_PNYTONE;
    } else if(strcmp(p_val, "han")==0) {
        res = CN_HAN;
    } else if(strcmp(p_val, "sw1k")==0) {
        res = EN_SW1K;
    } else if(strcmp(p_val, "sw3k")==0) {
        res = EN_SW3K;
    } else if(strcmp(p_val, "sw5k")==0) {
        res = EN_SW5K;
    } else {
        printf("phone_type %s not support!\n", p_val);
        res = -1;
    }
    return res;
}

/*************************************public****************************************/
opts_t opts;
static char content[1000];
//文件末尾需要有空行; key:val 冒号分割，不能有空格
int parse_opts(char* cfg_file)
{
    memset((void*)&opts, 0, sizeof(opts_t));
    
    FILE* fp = fopen(cfg_file, "r");
    if(!fp) {
        printf("open cfg file %s failed\n", cfg_file);
        return -1;
    }
    fseek(fp,0L,SEEK_END);
    int size=ftell(fp);
    
    if(size >= 1000) {
        printf("cfg_file too big\n");
        fclose(fp);
        return -1;
    }
    fseek(fp,0L,SEEK_SET);
    fread(content, 1, size, fp);
    fclose(fp);
    //printf("%s\n", content);
    int res = 0;
    for(char* p = content; p < content+size; )
    {
        char* p_n = strchr(p, '\n');
        if(!p_n) break;
        if(*p=='#'){
            //printf("# skip comment\n");
            p = p_n+1;
            continue;
        }
        *p_n=0;
        if(*(p_n-1)=='\r') *(p_n-1)=0;  //兼容win的格式
        //printf("%s\n", p);
        
        char* p_colon = strchr(p, ':');
        if(!p_colon) {
            printf("no colon at this line: %s\n", p);
            p = p_n+1;
            continue;
        }

        *p_colon=0;
        char* p_val = p_colon+1;
        //printf("%s\n", p);
        //device config
        if(strcmp(p, "device_type") == 0) { 
            opts.device_type = _get_device_type(p_val);
            if(opts.device_type<0) break;
        } else if(strcmp(p, "device_name") == 0) { 
            opts.device_name = p_val;
        //am config
        }else if(strcmp(p, "model_name") == 0) {
            opts.model_name = p_val;
        } else if(strcmp(p, "model_in_len") == 0) {
            opts.model_in_len = atoi(p_val);
        } else if(strcmp(p, "strip_l") == 0) {
            opts.strip_l = atoi(p_val);
        } else if(strcmp(p, "strip_r") == 0) {
            opts.strip_r = atoi(p_val);
        } else if(strcmp(p, "agc") == 0) {
            opts.agc = atoi(p_val);
        } else if(strcmp(p, "phone_type") == 0) {
            opts.phone_type = _get_phone_type(p_val);
            if(opts.phone_type<0) break;
        //lm config
        } else if(strcmp(p, "sfst_name") == 0) {  
            opts.sfst_name = p_val;
        } else if(strcmp(p, "sym_name") == 0) {  
            opts.sym_name = p_val;
        } else if(strcmp(p, "phones_txt") == 0) {  
            opts.phones_txt = p_val;
        } else if(strcmp(p, "words_txt") == 0) {  
            opts.words_txt = p_val;
        } else if(strcmp(p, "font_path") == 0) {  
            opts.font_path = p_val;
        } else if(strcmp(p, "beam") == 0) {  
            opts.beam = atof(p_val);
        } else if(strcmp(p, "bg_prob") == 0) {  
            opts.bg_prob = atof(p_val);
        } else if(strcmp(p, "scale") == 0) {  
            opts.scale = atof(p_val);
        } else if(strcmp(p, "is_mmap") == 0) {
            opts.is_mmap = atoi(p_val);
        //lib dbg
        } else if(strcmp(p, "dbg_micraw") == 0) {
            opts.dbg_micraw = atoi(p_val);
        } else if(strcmp(p, "dbg_mic") == 0) {
            opts.dbg_mic = atoi(p_val);
        } else if(strcmp(p, "dbg_strip") == 0) {
            opts.dbg_strip = atoi(p_val);
        } else if(strcmp(p, "dbg_lvcsr") == 0) {
            opts.dbg_lvcsr = atoi(p_val);
        } else if(strcmp(p, "dbgt_pp") == 0) {
            opts.dbgt_pp = atoi(p_val);
        } else if(strcmp(p, "dbgt_am") == 0) {
            opts.dbgt_am = atoi(p_val);
        } else if(strcmp(p, "dbgt_kws") == 0) {
            opts.dbgt_kws = atoi(p_val);
        } else if(strcmp(p, "dbgt_wfst") == 0) {
            opts.dbgt_wfst = atoi(p_val);
        //app config
        } else if(strcmp(p, "do_raw") == 0) {
            opts.do_raw = atoi(p_val);
        } else if(strcmp(p, "do_dig") == 0) {
            opts.do_dig = atoi(p_val);
        } else if(strcmp(p, "do_kws") == 0) {
            opts.do_kws = atoi(p_val);
        } else if(strcmp(p, "do_lvcsr") == 0) {
            opts.do_lvcsr = atoi(p_val);
        // testbench config
        } else if(strcmp(p, "testpath") == 0) {
            opts.testpath = p_val;
        } else if(strcmp(p, "testpny") == 0) {
            opts.testpny = atoi(p_val);
        } else if(strcmp(p, "testhan") == 0) {
            opts.testhan = atoi(p_val);
        } else if(strcmp(p, "testlm") == 0) {
            opts.testlm = p_val;
        } else {
            printf("opt %s not support\n", p);
        }         
        p = p_n+1;
    }

    //dump arg config
    if(res == 0) {
        char* device_type_str[3] = {"pcm", "mic", "wav"};
        printf("Get args:\n");
        printf("device config: \n    device_type=%s, device_name=%s\n", \
            device_type_str[opts.device_type], opts.device_name==NULL?"NULL":opts.device_name);

        printf("am config: \n    model_name=%s, model_in_len=%d, strip_l=%d, strip_r=%d, agc=%d, phone_type=%d\n", \
            opts.model_name, opts.model_in_len, opts.strip_l, opts.strip_r, opts.agc, opts.phone_type);

        printf("lm config: \n    sfst_name=%s, sym_name=%s, phones_txt=%s, words_txt=%s, font_path=%s\n    beam=%.3f, bg_prob=%.3f, scale=%.3f, is_mmap=%d\n", \
            opts.sfst_name, opts.sym_name, opts.phones_txt, opts.words_txt, opts.font_path, opts.beam, opts.bg_prob, opts.scale, opts.is_mmap);

        printf("lib config: \n    dbg_micraw=%d, dbg_mic=%d, dbg_strip=%d, dbg_lvcsr=%d, dbgt_pp=%d, dbgt_am=%d, dgbt_kws=%d, dbgt_wfst=%d\n",\
            opts.dbg_micraw, opts.dbg_mic, opts.dbg_strip, opts.dbg_lvcsr, opts.dbgt_pp, opts.dbgt_am, opts.dbgt_kws, opts.dbgt_wfst);

        printf("app config: \n    do_raw=%d, do_dig=%d, do_kws=%d, do_lvcsr=%d\n", \
            opts.do_raw, opts.do_dig, opts.do_kws, opts.do_lvcsr);
        
        printf("testbench config: \n    testpath=%s, testpny=%d, testhan=%d; testlm=%s\n", \
            opts.testpath==NULL?"NULL":opts.testpath, opts.testpny, opts.testhan, opts.testlm==NULL?"NULL":opts.testlm);
        printf("\n");
    }
    // check args
    if( opts.device_type>DEVICE_WAV || opts.device_name==NULL ||\
        opts.model_name == NULL ||  opts.model_in_len==0 || opts.phone_type<0 ||\
        opts.sfst_name==NULL || opts.sym_name==NULL || opts.phones_txt==NULL || opts.words_txt==NULL || opts.font_path==NULL ||\
            opts.beam==0.0 || opts.bg_prob==0 || opts.scale==0) {
        printf("opts setting error!\n");
        res = -1;
    }
    
    return res;
}