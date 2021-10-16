Maix-Speech 详细使用文档
======


Maix ASR 目前完成的功能模块包括：连续数字识别(DIG), 关键词识别(KWS), 大词汇量连续语音识别(LVCSR)  
该库基于 AM+LM 两段式识别，用户可灵活配置 声学模型和语言模型 来适应不同的嵌入式场景，也可加入热词等。


## Maix ASR 运行流程简述

1. asr_device 采集指定长度的原始音频，输出到int16_t数组里
2. asr_preprocess 对原始音频进行 agc/anc/aec 等操作，并进行stft计算，生成80维MEL Fbank数据
3. asr_am 对 fbank 进行声学模型计算，输出每个时刻(默认64ms一格)的拼音预测结果（前BEAM_CNT个）
4. asr_decoder 对am的输出结果进行相关解码处理，实现对应功能，如 DIG/KWS/LVCSR
5. decoder 运行用户注册的回调函数，执行用户逻辑代码


## ms_asr.h中的宏定义和数据类型
### 发布版本
~~~
#define VERSION_RELEASE 0
~~~
默认公开发布的库文件为RELEASE版，失能了DBG相关宏功能

### 推理类型
~~~
#define INFER_ZHOUYI    0 //周易AIPU 推理
#define INFER_V83X      1 //V83X AWNN推理
#define INFER_CPU0      2 //CPU 推理
#define INFER_CPU1      3 //CPU 推理(弃用)
~~~

### 调试选项
在release版里失能
~~~
#define DBG_MICRAW  (1<<0) //运行时dump原始麦克风音频到 micraw.pcm
#define DBG_MIC     (1<<1) //运行时dump 前端处理后的音频到 mic.pcm
#define DBG_STRIP   (1<<2) //显示边界信息
#define DBG_LVCSR   (1<<3) //显示LVCSR解码详情
#define DBGT_PP     (1<<4) //音频前端处理计时
#define DBGT_AM     (1<<5) //AM模型运算计时
#define DBGT_KWS    (1<<6) //关键词解码计时
#define DBGT_WFST   (1<<7) //WFST解码计时
~~~

### 建模单元
声学模型建模单元，目前仅实现汉语识别的 CN_PNYTONE（含声调拼音）   
其他宏定义预留   
~~~
#define CN_PHONE        0
#define CN_PNY          1
#define CN_PNYTONE      2
#define CN_HAN          3
#define EN_PHONE        4
#define EN_SW1K         5
#define EN_SW3K         6
#define EN_SW5K         7
//en subword: https://bpemb.h-its.org/en/
~~~

### 量化方式
声学模型输出结果的量化方式
~~~
#define QUANT_NONE      0
#define QUANT_INT8      1
#define QUANT_UINT8     2
~~~

### 常量定义
~~~
#define BEAM_CNT        (10)             //每个时刻保留前BEAM_CNT个概率结果
#define ASR_KW_MAX_PNY  (6)              //每个关键词最多6个字
#define ASR_KW_MAX      (100)            //最多100个关键词
~~~

### 音频设备类型
目前支持 pcm, mic, wav 或者 用户自定义设备输入
~~~
#define DEVICE_PCM    0
#define DEVICE_MIC    1
#define DEVICE_WAV    2
#define DEVICE_MIC2   3
#define DEVICE_MIC4   4
#define DEVICE_CUSTOM 5
~~~

### 音频接口类
当用户有自定义的音频输入设备，或者需要使用自己的声学前端处理程序，可以实现以下音频设备接口   
初始化时使用 DEVICE_CUSTOM 来选择自定义设备。
~~~
typedef struct{
    int  (*init)(char* device_name);            //初始化音频设备/文件
    int  (*read)(int16_t* data_buf, int len);   //读取指定长度音频到buf，返回读取到的数量
    void (*clear)(void);                        //清缓存
    void (*deinit)(void);						//释放音频设备
}asr_device_t;
~~~

### 解码器类型
解码器以位段形式表示，目前支持以下四种解码器（详细使用方式见下节）
~~~
#define DECODER_RAW   1      //raw解码器，返回AM输出的原始结果
#define DECODER_DIG   2		 //数字解码器，输出中文数字识别结果
#define DECODER_LVCSR 4		 //连续语音识别解码器，输出汉字结果
#define DECODER_KWS   8      //关键词识别解码器，输出当前时刻的各个关键词概率
#define DECODER_ALL   0xffff //选中所有解码器
~~~

### AM初始化参数
AM初始化参数，参考例程填写
~~~
typedef struct{
    char* model_name;
    int model_in_len;
    int strip_l;
    int strip_r;
    int phone_type;
    int agc;
}am_args_t;
~~~

### 拼音概率
AM输出结果为 pnyp_t 数组，每个时刻 BEAM_CNT个。  
可使用 am_vocab[idx]获取对应下标的拼音的字符串形式  
~~~
typedef struct
{
    uint32_t idx;  //pny的下标
    float p;
}pnyp_t;
~~~
***
## ms_asr.h中的函数接口
### 初始化
初始化asr库，用户选择对应的音频设备，AM模型参数，和调试选项  
当用户使用自定义音频设备时，device_type填 DEVICE_CUSTOM ，device_name 填对应的 asr_device_t指针    
注意解码器设置由另外独立API设置  
~~~
int  ms_asr_init(int device_type, char* device_name, am_args_t* am_args, int dbg_flag);
~~~

### 释放
释放asr库相关资源
~~~
void ms_asr_deinit(void);
~~~

### 解码器设置
用户可以注册若干个解码器（当然也可以不注册），解码器的作用是解码声学模型的结果，并执行对应的用户回调
~~~
typedef void (*decoder_cb_t)(void* data, int cnt);
int  ms_asr_decoder_cfg(int decoder_type, decoder_cb_t decoder_cb, void* decoder_args, int decoder_argc);
~~~
**DECODER_RAW**
function: 输出原始AM的预测结果  
decoder_args：None  
cb_data: pnyp_t 指针，最近一帧的拼音概率列表，每格时刻保留 BEAM_CNT个拼音  
cb_len: 最近一帧的解码格数，即: pnyp_t cb_data[cb_len][BEAM_CNT]  

**DECODER_DIG**
function：输出最近4s内的中文数字识别结果  
decoder_args：blank_ms, 超过该值则在输出结果里插入一个'_'表示空闲静音  
cb_data: 最近4s的中文数字识别结果，字符串形式，支持 0123456789 .(点) S(十) B(百) Q(千) W(万)  
cb_len: strlen(cb_data)  

**DECODER_KWS**
function: 输出最近一帧所有注册的关键词的概率列表，用户可以观察概率值，自行设定阈值进行唤醒  
decoder_args：  
~~~
arg0: 关键词列表，以拼音间隔空格填写，如：
    char* my_kw_tbl[3]={
    (char*)"xiao3 ai4 tong2 xue2",
    (char*)"tian1 mao1 jing1 ling2",
    (char*)"tian1 qi4 zen3 me yang4"
    };
arg1: 关键词概率门限表
    虽然回调会输出所有关键词的概率，但是也需要概率门限表来去除上一帧输出过的有效关键词来排重
    按顺序排列输入即可
arg2: 关键词数量
arg3: 是否进行自动近音处理，置1则会自动将不同声调的拼音作为近音词来合计概率
~~~
cb_data: 最近一帧注册的所有关键词的概率（float,0~1）  
cb_len: 关键词数量  
> 注：cb_data中输出的值如果是负数，则表示对应的关键词上一帧刚输出过，需要排重。

相关函数：  
手工注册静音词，每个拼音可以注册最多10个近音词。  
注意，使用该接口注册近音词会覆盖使能 "自动近音处理" 里自动生成的近音表  
~~~
int ms_asr_kws_reg_similar(char* pny, char** similar_pnys, int similar_cnt);
DEMO：
char* similar_pnys[3] = {(char*)"xin1", (char*)"ting1", (char*)"jin1"};
ms_asr_kws_reg_similar((char*)"jing1", similar_pnys, 3);
~~~

**DECODER_LVCSR**
function: 输出连续语音识别结果（小于1024个汉字结果）  
decoder_args：  
~~~
arg0: sfst_name, sfst文件路径（LG.fst）
arg1: sym_name, sym文件路径（输出符号表）
arg2: phones_txt, phones.bin的路径（拼音表）
arg3: words_txt, words.bin的路径（词典表）
arg4: beam, WFST搜索的beam大小，默认为5，建议在3~9之间，越大搜索空间越大，越精确但越慢
arg5: bg_prob, 在BEAM_CNT之外的背景拼音的默认概率值的自然对数的绝对值，默认为10
arg6: scale, acoustics_cost = log(pny_prob)*scale
arg7: is_mmap, 是否使用mmap方式加载WFST解码图。
    使用mmap方式则无需加载sfst文件到内存，使用即时磁盘读取的方式读取解码图，速度较慢，beam<=5时可以实时解码
    使用非mmap方式，读取整个sfst文件到内存，解码速度最快，beam上限可以到10
~~~
cb_data:   
~~~
data0: char* words, 识别的汉字结果字符串，注意编码方式根据输入的words.bin的编码方式而定，可以为GBK或者UTF-8
data1: char* pnys, 识别的拼音字符串结果
注意，字符串内可能有','或者'.'表示语句停顿，如不需要则自行滤除
~~~
cb_len: 固定为2，表示返回了2个参数  


### 清缓存
重置内部缓存操作，在开启新的语音识别前清一次   
~~~
void ms_asr_clear(void);  
~~~

### 运行语音识别
frame: 每次run的帧数; 返回实际run的帧数  
用户可以每次run 1帧，run完做其他处理；或者一个线程里一直run，由外部线程stop来停止  
内部自动调用asr_device的read接口读入相应长度的数据进行出来  
~~~
int  ms_asr_run(int frame); 
~~~

### 获取一帧的时间
返回一帧的时间 ms  
~~~
int ms_asr_get_frame_time(void); 
~~~

### 获取声学模型词典
~~~
void ms_asr_get_am_vocab(char** vocab, int* cnt);
~~~

### 设置asr数据源
重新设置asr的数据源，一般在pcm/wav识别时使用，比如进行下一个wav文件识别  
内部会自动进行缓存清除操作  
~~~
int ms_asr_set_dev(int device_type, char* device_name); 
~~~

### 单独运行WFST解码
输入 T_CNTxBEAM_CNT 个拼音结果，进行decoder推理，用于lm的测试  
~~~
void ms_asr_wfst_run(pnyp_t* pnyp_list); 
~~~

## 配置文件解析
例程使用opts.c来解析配置文件，用户可以在其上添加参数，或者自行编写配置解析函数。  
例程的配置文件为极简的格式，行首#则记为注释(行尾#无效)，  
一行参数使用':'分割，格式为 arg_name:arg_value  
参数说明(行尾的#仅为文档里的说明使用，实际不含该注释)：  
~~~
#device config
device_type:wav      #设备类型：wav,pcm,mic,custom
device_name:1.2.wav  #设备名

#am config
model_name:asr_7332_192.bin #am模型路径
model_in_len:192    #模型输入长度
strip_l:6           #左右边界长度
strip_r:6
phone_type:pnytone  #模型建模类型
agc:1               #是否使能AGC（自动增益控制）

#lm config
sfst_name:lg_6c.sfst #解码图文件路径
sym_name:lg_6c.sym   #输出符号表路径
phones_txt:phones.bin#拼音表路径
words_txt:words.bin  #词典表路径
font_path:./         #字体路径，可选
beam:8.0             #解码搜索宽度
bg_prob:10.0         #背景概率对数值的绝对值
scale:0.5            #声学分比例
is_mmap:0            #是否以mmap方式加载语言模型

#lib dbg
dbg_micraw:0         #是否dump原始mic音频到micraw.pcm
dbg_mic:0            #是否dump preproces后的音频到mic.raw
dbg_strip:0          #是否显示边界信息
dbg_lvcsr:0          #是否打印wfst解码详情
dbgt_pp:0            #preprocess计时
dbgt_am:0            #am模型运算计时
dbgt_kws:0           #kws解码计时
dbgt_wfst:0          #wfst解码计时

#app config
do_raw:0             #打印原始am模型结果
do_dig:0             #注册数字识别回调
do_kws:0             #注册关键词识别回调
do_lvcsr:1           #注册连续语音识别回调

#testbench config    #跑分测试配置
#testpath:/mnt/kws/aishell_test_1_10/ #待测试wav目录，不测试则注释掉改行
testpny:1            #输出pny测试结果
testhan:1            #输出han测试结果
#testlm:lm_test.txt  #LM测试文件（每行一句话的拼音字符串，空格分割，\n换行）
~~~

