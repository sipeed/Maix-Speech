Maix-Speech
===================

[中文](./README_ZH.md) | English

## Brief

Now only support Chinese, See [中文](./README_ZH.md)



## Build

### Clone code by:
```
git clone https://github.com/sipeed/Maix-Speech
```

### Compile

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

## License

**MIT**， see [LICENSE](./LICENSE)

