Maix-Speech
===================

中文 | [English](./README.md)

Maix-Speech 是专为嵌入式环境设计的离线语音库，设计目标包括：ASR/TTS/CHAT   
作者的设计初衷是完成一个低至Cortex-A7 1.0GHz 单核下可以实时运行的ASR库。   
目前市面上的离线语音库非常稀缺，即使有也对主控要求很高，Maix-Speech 针对语音识别算法进行了深度优化，在内存占用上达到了数量级上的领先，并且保持了优良的WER。  

Maix-Speech 目前以静态库形式提供给用户评估使用，有商业定制需求的用户可以发邮件到 support@sipeed.com 咨询

技术交流可以 扫下面的二维码加入 MaixSpeech微信群 或者 发邮件到 zepan@sipeed.com


<a href="https://imgtu.com/i/5J9xgO"><img width=300 src="https://z3.ax1x.com/2021/10/16/5J9xgO.png"/></a>

---

## Maix-Speech 的优势	

### 多平台支持

Maix-Speech 支持多种嵌入式平台

|平台标识|宏定义|芯片工具链|计算单元|评估板|
|:--:   |:--:  |:--:     |:--:   |:--:  |
|**r329**|INFER_ZHOUYI|全志R329,aarch64 gnu|周易AIPU|[MaixSense](https://item.taobao.com/item.htm?id=652879327858)|
|**v83x**|INFER_AWNN|全志V83x,armv7 musl|AWNN|[M2Dock](https://item.taobao.com/item.htm?id=635874427363)|
|**armv7musl**|INFER_CPU0|armv7 musl|CPU|[M2Dock](https://item.taobao.com/item.htm?id=635874427363)|
|**armv7**|INFER_CPU0|armv7 gnu|CPU|[LicheePi Zero](https://item.taobao.com/item.htm?id=585033586770)|
|**aarch64**|INFER_CPU0|aarch64 gnu|CPU|[MaixSense](https://item.taobao.com/item.htm?id=652879327858)|
|**x86**|INFER_CPU0|x86_64|CPU|Your Computer|
|**riscv64**|INFER_CPU0|RV64 C906|CPU|[Nezha D1](https://item.taobao.com/item.htm?id=644378932175)|
	

### 极低的内存要求和优良的正确率

Maix-Speech的内存占用相对于市面上的其他语音识别框架有数量级上的领先优势，并且保持良好的WER水平。  
Maix-Speech最低可以实时运行(RTF<1)于典型的 **1.0GHz Cortex-A7** 内核的芯片上，并且最低仅占用**25MB**左右内存，
也就意味着它可以实时运行在典型的内封64MB内存的A7芯片上。

**常见离线语音识别工具对比**

|Tool|[DeepSpeech](https://github.com/mozilla/DeepSpeech)|[VOSK](https://github.com/alphacep/vosk-api)<br>normal|VOSK<br>small|[wenet](https://github.com/wenet-e2e/wenet)<br>float|wenet<br>int8|Maix-Speech<br>7332|Maix-Speech<br>3332|Maix-Speech<br>3316|
|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|
|AM Model<br>(MB)|46|74|13|127|38|11|7|2.5|
|VmRSS(MB)|340|530|300|1148|586|<font color='red'> 34 </font>|30|25|
|VmData(MB)|7260|1600|1370|-|-|3|3|3|
|WER|45.2%|6.7%|9.2%|~5%|~5%|<font color='red'>5.4%</font>|5.9%|9.7%|
|PC RTF|0.757|0.191|0.086|0.095|0.088|0.143|0.052|0.024|
|1.5G A72 RTF|>>1|>>1|0.86|TODO|TODO|0.72|0.48|0.18|
|1.2G A53 RTF|>>1|>>1|>>1|>>1|>>1|>>1|1.22|0.41|
|1.0G A7 RTF|>>1|>>1|>>1|>>1|>>1|>>1|>>1|0.65|

> 注1：WER以非流式计算aishell测试集，选用可下载到的开源模型测试。  
> 注2：PC CPU为 Intel(R) Xeon(R) CPU E5-2678 v3 @ 2.50GHz  
> 注3: Maix-Speech以mmap形式使用语言模型，故语言模型不占用 VmRSS  
> 注4: Maix-Speech的WER数据使用beam=8, lmL, is_mmap=0测试  
> 注5：wenet为端到端模型  


### 细节优化

1. 优化了openfst及wfst解码，使得整个解码图无需载入内存即可实时读取解码
2. 可选载入内存的LG.fst解码图，压缩为lg.sfst, 尺寸为原始fst的1/3左右，占用内存为kaldi载入相同fst的内存占用的 1/20左右（kaldi需要6.5倍左右内存载入fst文件）
3. 使用新的sMBR等效的方式（无需修改loss）进行鉴别性训练，提升流式识别的准确率

## 效果展示

> 在全志 R329 上的运行效果，视频中板卡为 MaixSense   

连续中文数字识别 （DIGIT)

https://user-images.githubusercontent.com/8625829/137852798-efd0662d-c155-42e6-9fa7-c9de5cb348b9.mp4


关键词识别（KWS）

https://user-images.githubusercontent.com/8625829/137852791-c99a0179-5963-46c9-9e7a-6c6b086a6839.mp4

连续大词汇量语音识别（LVCSR）

https://user-images.githubusercontent.com/8625829/137852770-db0ca4a6-f32b-4c82-a40e-4fb8980f05ee.mp4

---

## Maix-Speech 工程结构

```
.
├── assets
│   └── test_files                # 提供的测试文件，方便上手测试
├── components                     # 组件
│   ├── asr_lib                   # 组件 asr_lib
│   │   ├── CMakeLists.txt       # 组件配置文件
│   │   ├── include              # 头文件
│   │   ├── Kconfig              # 组件 menuconfig 配置文件
│   │   ├── lib                  # 各个平台的库文件
│   │   └── src                  # 源文件
│   └── utils                     # 工具类组件，包括了跑分、字体等
├── Kconfig                       # 最顶级的 menuconfig 配置文件
├── LICENSE                       # 开源协议（证书）
├── projects                      # 工程
│   └── maix_asr                 # ASR 工程
│       ├── CMakeLists.txt       # 工程配置文件
│       ├── main                 # 工程里面的主组件
│       └── project.py           # 构建脚本，方便输入命令
├── README.md                     # 项目首页英文文档
├── README_ZH.md                  # 项目首页中文文档
├── tools                         # 项目构建相关代码，一般不用看
└── usage_zh.md                   # 使用方法
```

## 构建代码

项目支持多平台， 不同的平台使用的工具链和库可能有差异，注意区别

### 环境准备

* x86 (Linux) 或 在跑在其它架构的系统里编译，比如在`R329`的系统里使用`GCC`编译

    安装工具链和库(`Ubuntu` 为例)
    ```
    sudo apt update
    sudo apt install build-essential git libasound2-dev python3 cmake
    ```
    > python 只是用在编译脚本上的，方便简单地输入编译命令， 如果你电脑里有任何一个版本的 python 都是可以的， 为确保不出问题最好是`Python3`。如果实在不想装 python ， 也可以手动使用 cmake 命令进行编译

* R329 （交叉编译）
  * 电脑安装工具链和库(`Ubuntu` 为例)
  ```
  sudo apt update
  sudo apt install git python3 cmake
  ```
  * 下载工具链，并解压到指定文件夹
  从 [realease](https://github.com/sipeed/Maix-Speech/releases) 下载 `r329_toolchain.tar.gz`, 并解压到一个路径，比如 `/opt/r329_toolchain`

### 克隆代码:

```
git clone https://github.com/sipeed/Maix-Speech
```

### 编译

* x86 (Linux) 或 在跑在其它架构的系统里编译，比如在`R329`的系统里使用`GCC`编译

> 注意，conda 环境下工具链可能有问题，如果出现错误可以先尝试 退出conda环境使用原生环境编译   

```
cd projects/maix_asr

python project.py clean_conf    # 清除工具链配置
python project.py menuconfig    # 配置选择芯片架构（ARCH），默认就是 x86

python project.py build
#python project.py rebuild          # 如果有新建文件需要使用 rebuild
# python project.py build --verbose # 打印详细构建过程

./build/maix_asr                # 测试下运行可执行文件，可以执行即可

python project.py clean         # 清除构建内容
python project.py distclean     # 彻底清除构建内容

```

* R329 和其它架构（交叉编译）

```
cd projects/maix_asr
# 配置工具链位置和前缀， distclean 不会清除工具链配置, 这会在目录下生成一个 .config.mk 文件
python project.py --toolchain /opt/r329_toolchain/bin --toolchain-prefix aarch64-openwrt-linux- config

python project.py menuconfig       # 选择 R329 架构
python project.py build

# python project.py clean_conf     # 清除工具链配置
```

关于更详细地如何使用编译框架请看 [github.com/Neutree/c_cpp_project_framework](https://github.com/Neutree/c_cpp_project_framework)

## 运行语音识别例程

以 `x86(Linux)` 平台为例的快速验证 demo:

* 先保证编译通过， 可执行文件 `projects/maix_asr/build/maix_asr` 存在并且可以运行

* 在 [release 页面](https://github.com/sipeed/Maix-Speech/releases) 找到 `am_7332.zip` `lmM.zip` 文件并下载， 解压到`assets/test_files` 目录, `assets`目录结构如下
```
assets
├── image
└── test_files
    ├── 1.2.wav
    ├── AM
    ├── asr_wav.cfg
    └── lmM
```

* 进入到 `test_files` 目录

```
cd assets/test_files
```

* 执行测试

```
../../projects/maix_asr/build/maix_asr asr_wav.cfg
```

可以看到语音识别的结果
```
HANS: 一点 二三 四五 六七 八九 
PNYS: yi4 dian3 er4 san1 si4 wu3 liu4 qi1 ba1 jiu3 
```

如果是 `Windows` 需要 `GBK`编码则修改`asr_wav.cfg`中的
```
words_txt:lmM/words_utf.bin
```
为
```
words_txt:lmM/words.bin
```

测试其他 wav 文件只需要修改 asr_wav.cfg 中的 device_name 到对应测试 wav 路径即可   
**注意** wav 需要是 **16KHz** 采样，**S16_LE** 存储格式。另外还支持 PCM 或者 MIC 实时识别，详见 [usage_zh.md](./usage_zh.md) 中对 cfg 文件的介绍。

> 可以使用工具转换，比如 `arecord -d 5 -r 16000 -c 1 -f S16_LE audio.wav`


---

## 详细使用文档

请看[使用文档](./usage_zh.md)


## Maix ASR 模型选择

MAIX ASR **声学模型**按尺寸分为：7332,3332,3324,3316  
大小如下表：

|model|7332|3332|3324|3316|
|--|--|--|--|--|
|float(MB)|44|28|18|10|
|int8(MB)|11|7|4.5|2.5|

MAIX ASR **语言模型**可以自由选择，默认开放三种尺寸的模型：lm_s,lm_m,lm_l  
每种模型分成 sfst,sym,phones,words 四部分，  
其中 sym,phones,words 仅用于输出字符串使用，无需加载入内存，仅放在磁盘    
sfst为解码图文件（LG.fst的压缩版），可选载入内存或者mmap实时读取。  
表中wer表示 aishell 测试集的汉字转拼音作为输入，通过LM转汉字后的错误率

|model|lmS|lmM|lmL|lmXL|
|--|--|--|--|--|
|sfst|12MB|104MB|750MB|3700MB|
|sym|6.5MB|59MB|404MB|1940MB|
|phones|12KB|13KB|13KB|13KB|
|words|8.0MB|72MB|72MB|107MB|
|WER|2.78%|1.94%|1.61%|1.24%|

以下是各个模型的benchmark  
pny wer表示带声调的拼音错误率，lmX表示加上对应语言模型后的汉字错误率  

|model/len|pny wer |lmS 12M |lmM 104M|lmL 750M| 
|--       |--      |--     |--     |--     |
|7332 11MB|||||
|192      |17.7    |13.1   |11.1   |10.0   |
|ali192   |9.94    |9.02   |7.56   |6.53   |
|non-flow |8.63    |7.81   |6.60   |5.38   |
|3332 7MB |||||
|128      |16.4    |13.3   |11.9   |11.0   |
|192      |11.6    |8.73   |7.22   |6.48   |
|non-flow |10.6    |8.46   |6.71   |5.94   |
|3324 4.5MB|||||
|128      |23.4    |20.4   |19.4   |18.5   |
|192      |11.3    |13.7   |9.66   |8.55   |
|non-flow |12.2    |10.2   |8.65   |7.56   |
|3316 2.5M|||||
|128      |25.5    |19.2   |17.4   |16.0   |
|192      |16.9    |12.9   |11.4   |10.1   |
|non-flow |16.1    |11.2   |11.0   |9.68   |

模型说明：  
下划线后的数字表示选取的帧长度，如192表示一帧为192x8=768ms，asr库每采集完一帧后进行一次处理  
帧长度关系到识别延迟，如192就会最大有768ms延迟，128则为512ms，可见帧长的模型错误率更优，但是延迟稍长  
表中默认为流式识别，使用有限的上下文（一帧长度），noflow表示非流式识别（整体识别），可见非流式识别错误率大幅下降  
ali表示对齐优化后的结果，即类sMBR处理后的结果，可见对齐训练后错误率大幅下降。  
附件默认上传了192长度的流式识别模型，需要其他识别模型的可以联系矽速。


## Maix-Speech TTS 

TODO


## Maix-Speech CHAT

TODO

## TODO List

- [ ] Support English
- [ ] Test Conformer
- [ ] Support TTS
- [ ] Support CHAT

## FAQ

Q1. 代码是否以及何时开源？  
> A1. star 超过[MaixPy](https://github.com/sipeed/MaixPy)项目后会考虑开源  

Q2.  
> A2.  

## License

项目使用 [Apache 2.0](./LICENSE) 开源协议，以及其引用和使用的开源项目的开源协议见 [LICENSE](./LICENSE)

## 致谢

MaixSpeech 借鉴和使用了一些优秀的开源项目，并咨询了一些业内大佬，包括：

1. WFST 解码 [Kaldi](http://kaldi-asr.org/)
2. 前端推理框架 [MNN](https://github.com/alibaba/MNN)
3. ARM中国 周易团队,尤其是toby
4. wenet 彬彬大佬；原cvte大佬 pfluo；



