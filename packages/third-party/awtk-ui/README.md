# AWTK 简要说明

本项目的编译工具是scons，依托AIC SDK进行编译。配置AWTK 参数需要在AIC SDK 根目录使用命令。需要额外注意的是，luban-lite SDK目前仅支持`RTOS`平台。

**scons--menuconfig** 进入可视化界面进行配置。

## 1 配置说明

配置所在路径：在AIC SDK 根目录使用命令scons--menuconfig 进入可视化界面，选择AWTK GUI

```sh
Local packages options -->
    Third-party packages options-->
    [*] AWTK GUI -->
            AWTK common function config -->
            AWTK cropping config -->
            AWTK Select a demo -->
            AWTK thread config -->
            AWTK input device config -->

```
配置可以分为五块：
-   AWTK common function config：配置AWTK 关键功能，包括G2D、图片解码、APP_RES配置、AWTK 开机自启动、LCD 刷新模式选择
-   AWTK cropping config：裁剪AWTK 配置，以便裁剪掉不需要的功能
-   AWTK Select a demo：选择运行AWTK示例，目前提供两个官方的示例
-   AWTK thread config：配置AWTK 相关线程，可根据自己的应用调整线程栈大小避免溢出
-   AWTK input device config：配置触摸设备，目前仅支持GT911 设备输入

除了配置[1.2资源配置](#12-资源配置)中资源配置路径需要改。一般而言，使用`默认配置`即可，不需要修改里面的配置。如果需要修改，相关配置说明可以参考：awtk/docs/porting_common.md 和AWTK 官方文档

### 1.2 资源配置

`注意：在配置使用了文件系统，需要配置读取资源路径，这样应用才能正常启动`，在默认配置中，文件系统选项默认开启。建议使用将资源文件下载到板子的方式，目前从外部SD卡读取文件速度较慢。

安装资源文件到板子配置：资源文件默认安装到板子/rodata 目录。
```sh
Application options -->
    [*] Using File System Image 0 -->
        (packages/third-party/awtk-ui/user_apps/awtk-demo-chart/rtos_res/)
```

如果是从SD 卡读取资源文件，就不需要将资源文件安装到板子：
```sh
Application options -->
    [] Using File System Image 0 -->
```
---

AWTK 资源读取路径配置：
```sh
Local packages options -->
    Third-party packages options-->
    [*] AWTK GUI -->
            AWTK common function config -->
                (/rodata/res) Set app resource root
```

AWTK 资源读取路径配置：从 SD卡读取资源文件配置，SD卡默认挂载到 /sdcard 目录下
```sh
Local packages options -->
    Third-party packages options-->
    [*] AWTK GUI -->
            AWTK common function config -->
                (/sdcard/offical-demo/res) Set app resource root
```

D13x 在默认配置下/rodata 区域最大空间是**6M**，因此可以适当将资源文件进行裁剪。例如提供的 chart demo，rtos_res 安装的资源，仅仅留下默认主题文件（default\raw），image文件夹也仅仅留下xx分辨率的图片（image/xx），然后将其全部打包进板子中。

### 1.3 内存配置

在D13x 平台上共有6M 内存可供用户使用。如果实在分配不出更多的内存，可以参考 "./user_apps/awtk-demo-chart/src/application.c" 中设置图片缓冲大小函数 `image_manager_set_max_mem_size_of_cached_images(image_manager(), 1366430)`。图片缓冲区大小对速度的影响很大，需要根据需要评估需要设置多大的缓冲区。如果缓冲区设置过小，便会频繁的读取，释放资源，速度将受到**极大**的影响。

在D21x 平台上面，可以去配置使用的更大的CMA内存，以供分配更多内存给到AWTK 图片缓冲区：

配置CMA 内存
```sh
Board options -->
    Mem Options  --->
    (0x2000000) CMA mem size
```

特别需要注意的是：在AIC D13x 平台上，内存资源受限比较紧张，默认不适用窗口动画缓存。如果是D21 x平台，内存充足，可以选择打开。

```sh
Local packages options -->
    Third-party packages options-->
    [*] AWTK GUI -->
            AWTK cropping config -->
                （ ）Don't use window animation cache and default open don't use dialog hightlighter

```

### 1.4 额外配置说明

#### 1.4.1 LCD 刷新模式选择

目前支持三种刷新模式，默认使用swap ex模式，也推荐使用这个模式。它们的区别如下：

| LCD 模式 | LCD 旋转功能 | 脏机制 | 高效旋转 |
| -------- | ------------| ------ | ------- |
|  flush   |     支持     |  支持  |  不支持  |
|  swap    |     支持     | 不支持 |  不支持  |
|  swap ex |     支持     |  支持  |  不支持 |

高效旋转目前不支持，主要原因是g2d_rotate_image_ex() 和 g2d_blend_image_rotate() 接口还没进行对接。启用文件系统情况下，不要定义宏 `WITH_FAST_LCD_PORTRAIT` 即可。关于高效旋转更多请看文档：./awtk/docs/how_to_use_fast_lcd_portrait.md。

选择LCD 刷新模式：
```sh
Local packages options -->
    Third-party packages options-->
    [*] AWTK GUI -->
            AWTK common function config -->
                Select lcd refresh mode (swap_ex mode)  --->

```

#### 1.4.2 开机自启动

如果需要开机自启动，需要选择 “Self start funcion upon startup”。如果不需要开机自启动，那需要在命令行，输入命令 test_awtk，手动执行应用。

```sh
Local packages options -->
    Third-party packages options-->
    [*] AWTK GUI -->
            AWTK common function config -->
                [*] Self start funcion upon startup

```

如果需要修改执行命令，参考 "awtk-rtos/awtk-port/rk_run.c" 文件。
```c
#ifdef AWTK_START_UP
INIT_APP_EXPORT(awtk_init); /* 开机启动后执行 awtk_init函数 */
#else
MSH_CMD_EXPORT_ALIAS(awtk_init, test_awtk, awtk chart demo); /* test_awtk 是命令， 后面是说明  */
#endif
```
#### 1.4.3 硬件图片解码格式支持

关于格式描述方式：AWTK和AIC的格式的字节序都是小端，但是格式名称描述不一样，比如：AIC 格式描述为ABGR_8888 对应 AWTK 格式描述为RGBA8888。本文档和配置文件均按照AIC 的格式描述进行描述。

在D21x 平台上，PNG格式不支持**硬件解码**为RGB565格式，JPEG格式仅支持**硬件解码**为YUV格式。在D13x 平台上PNG格式不支持**硬件解码**为RGB565格式。

D21x 平台：
|       图片格式       | RGB565  | RGB888/ARGB8888 |
| --------------------| --------| ----------------|
|        PNG          |  不支持  |       支持      |
|       JEPG/JPG      |  不支持  |      不支持     |

D13x 平台：
|       图片格式       | RGB565  | RGB888/ARGB8888 |
| --------------------| --------| ----------------|
|        PNG          |  不支持  |       支持      |
|       JEPG/JPG      |   支持   |        支持     |


使用图片注意点：
PNG 格式：
- 原图是什么格式，将解码成什么格式。如果想要减少内存的使用，可以将使用原来PNG 24位的图片转换为16位图片
- PNG 图片使用图片格式为16位图片，将默认解码为RGB888。开启动画，则会将RGB888转换为RGB565。硬件不支持解码为RGB565，必须进行转化为RGB565 AWTK才支持动画。

JGP/JGEG 格式：
- D13x 平台默认默认解码成RGB565 格式。关闭动画则解码为RGB888 格式。D21x 解码为YUV 格式，然后将YUV 格式转换为RGB 格式以供AWTK 使用。
- D13x 平台如果没有关闭动画功能，由于动画功能格式不支持RGB888，将会对原图格式为RGB888 的图片进行一次转换，转换为RGB565格式。D13x 平台如果没有图片透明度推荐使用JPEG格式。


推荐图片使用：在默认配置，`动画功能打开`的情况下。如果关闭了动画功能或者内存比较紧张的情况下，请参考上面解码支持和图片使用注意点自行选择。

D21x平台：
推荐使用32位 PNG图片，这样解码效率最高

D13x平台：
JPEG/JPG格式或者32位 PNG图片，这样解码效率最高


#### 1.4.4 demo 选择

目前支持的两个demo，一个是 chart demo 一个是official demo

```sh
Local packages options -->
    Third-party packages options-->
    [*] AWTK GUI -->
            Select a demo (Use the official demo)  --->
                (X) Use the chart demo
                ( ) Use the official demo
```

chart demo : 资源文件比较小，可以放在板子中。资源在：./awtk/user_apps/awtk-demo-chart/rtos_res/
official demo: 资源文件比较大，不建议在D13x 运行此Demo。资源在：./awtk/user_apps/offical-demo/res/，建议将资源文件放到SD卡进行运行。
 D13x 运行official demo 效果非常差，主要是因为内存不够。

#### 1.4.5 日志等级选择

可以选择不同的日志等级，以方便调试。目前支持info 等级和debug 等级，默认为info等级。

```sh
Local packages options -->
    Third-party packages options-->
    [*] AWTK GUI -->
            AWTK cropping config  --->
                Choice log level (log level debug)  --->
                    ( ) log level debug
                    (X) log level info
```

## 2 编译说明

编译用到的宏定义一部分定义在 ./SConscript中，一部分在Kconfig 在可视化界面中进行配置。配置由相关脚本进行解析，得到全局的编译的宏。

相关宏的说明可以参考：awtk/docs/porting_common.md
参与编译的文件可以参考：awtk/docs/porting_common.md 和 ./SConscript

如果需要编译自己的 app，需要修改SConsript。AWTK Designed 生成的资源文件也需要做出一些调整，将不需要的文件可以删掉，只留下需要的，参考 rtos/res路径下的资源文件。参考代码：

```py
# GetDepend 是从Kconfig获取相关宏
if GetDepend('LPKG_AWTK_USING_DEMOS_CHART'):
    awtk_demo_cwd = cwd + '/user_apps/'
    # add user 3rd
    src += find_sources(awtk_demo_cwd + 'awtk-demo-chart/3rd/awtk-widget-chart-view/src/')
    src += find_sources(awtk_demo_cwd + 'awtk-demo-chart/3rd/awtk-widget-chart-view/src/base/')
    src += find_sources(awtk_demo_cwd + 'awtk-demo-chart/3rd/awtk-widget-chart-view/src/chart_view/')
    src += find_sources(awtk_demo_cwd + 'awtk-demo-chart/3rd/awtk-widget-chart-view/src/pie_slice/')

    # add user src
    src += find_sources(awtk_demo_cwd + 'awtk-demo-chart/src/')
    src += find_sources(awtk_demo_cwd + 'awtk-demo-chart/src/pages/')
    src += find_sources(awtk_demo_cwd + 'awtk-demo-chart/src/common/')

    # install needed res
    if GetDepend('WITH_FS_RES'):
        Mkdir(awtk_demo_cwd + 'awtk-demo-chart/rtos_res/res/assets/default/')
        Install(awtk_demo_cwd + 'awtk-demo-chart/rtos_res/res/assets/default/', awtk_demo_cwd + 'awtk-demo-chart/res/assets/default/raw')
```

## 3 优化建议

### 3.1 运行速度优化建议

- 提前加载图片资源到内存中（assets_manager_preload），目前从文件系统读取数据较慢。
- 将图片数据存放到 /rodata区，从SD 卡读取数据较慢
- 增大图片缓冲区。使用函数image_manager_set_max_mem_size_of_cached_images(image_manager(), 1366430)，以增大缓冲区。

### 3.2 优化内存使用

- 将frame buffer格式由RGB888/ARGB8888 改变为RGB565。如果需要开启动画，AWTK 不支持RGB888 格式，需要设置frame buffer 格式为ARGB8888 或者RGB565。
- 使用image_manager_set_max_mem_size_of_cached_images(image_manager(), 1366430) 设置合理的缓冲区大小。
- image_manager_unload_unused(image_manager(), 1); 配置从图片管理器中卸载指定时间内没有使用的图片。
- 关闭窗口动画缓存，默认关闭。

关于图片资源内存的优化建议
- 使用更低分辨率的图片
- 如果不需要使用到透明度，可以将32位图转换为24/16位图，建议转换为16位图。
- JPEG 格式相比PNG格式更省内存，但没有透明度。
- JPEG和PNG 格式选择，可以根据[1.4.3 硬件图片解码格式进行选择](#143-硬件图片解码格式支持)


## 4 对接流程

除了图片解码的流程和AWTK 标准流程不一样，其他流程都是按照AWTK 的软件流程进行对接的。可以参考AWTK 官方的移植文档和这里的对接源码。

对接源码结构：
```sh
    -> aic_g2d   # 加速和解码部分对接代码
        -> aic_dec_asset_frame.c # AIC 图片解码帧内存申请和图片头信息解析
        -> aic_dec_asset.c # AIC 硬件图片解码实现
        -> aic_g2d.c # AIC G2D和图片格式转换实现，G2D 使用GE实现，图片解码使用VE实现。
        -> aic_graphic_buffer.c # AIC graphic buffer实现，AWTK 基于此结构体进行具体图片信息管理
        -> aic_rtos_mem.c # AIC G2D和解码需要用到的CMA 内存管理

    -> input_thread # 触摸对接代码
        -> touch_thread.c # 触摸模块代码，目前仅用到这个
        -> input_dispatcher.c # 触摸调试代码
        -> tslib_thread.c # 目前不支持tslib，未实现

    -> lcd_rtos # lcd 对接代码
        -> fb_disp_info.h # 获取fb 信息实现
        -> lcd_disp.c # lcd 移植具体实现

    -> platform # 平台相关对接代码
        -> date_time.c # 获取时间代码，非必须
        -> fs_os.c # 文件系统接口实现
        -> platform.c # 内存，时钟实现

    -> run.c # AWTK 运行线程实现
```

图片解码流程做了一些调整，因为硬件解码需要CMA 内存，如果CMA 内存不够软件解码。关于CMA 内存也可以理解为资源的一部分，资源分配失败就返回。

**AIC 图片硬件解码流程**：

获取资源 + 硬件解码 -> 图片格式转化 -> 加入管理器进行管理

**AWTK 图片软件解码流程**：

获取资源 -> STB 软件解码 + 图片格式转化 -> 加入管理器进行管理

AIC 硬件解码和AWTK 软件解码的区别就是，在获取资源函数的时候就进行硬件解码。如果解码失败，则交由软件再次进行资源获取和进行下一步的调用STB库进行解码。
