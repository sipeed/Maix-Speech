Maix-Speech
===================

中文 | [English](./README.md)




## 快速上手

### 克隆代码:
```
git clone https://github.com/sipeed/Maix-Speech
```

### 编译

* x86x64

```
cd projects/asr

python project.py clean_conf
python project.py menuconfig

python project.py build
# python project.py build --verbose

./build/asr

python project.py clean
python project.py distclean
# python project.py clean_conf
```

* R329

```
cd projects/asr
python project.py --toolchain /opt/toolchain/bin --toolchain-prefix aarch64-openwrt-linux- config
python project.py menuconfig
python project.py build
```

## More project structure usage

See [github.com/Neutree/c_cpp_project_framework](https://github.com/Neutree/c_cpp_project_framework)

## 相关参考项目

* [ESP_IDF](https://github.com/espressif/esp-idf)： `ESP32` 的 `SDK`， 写得挺不错
* [Kconfiglib](https://github.com/ulfalizer/Kconfiglib)： `Kconfig` `Python` 实现
* [RT-Thread](https://github.com/RT-Thread/rt-thread)：不是用的 `CMake`, 但是也是使用了组件的概念

