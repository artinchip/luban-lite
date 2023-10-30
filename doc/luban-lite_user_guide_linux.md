
# 1. 环境安装

Luban-Lite SDK 采用了 `scons` 作为编译框架的基础语言，对开发环境的依赖：

- Python2，需安装插件：
  - scons
- Python3，需安装插件：
  - pycryptodomex



已验证可用的操作系统环境：

- Ubuntu 20.04
- Windows 10



# 2. 使用方法

## 2.1 方案加载

在编译一个方案之前，首先需要加载方案的现有配置：

```
$ cd luban-lite
$ scons --list-def                  // 列出当前可选的方案配置
$ scons --apply-def=xxx_defconfig   // 应用上述列表其中一条方案配置
$ scons --info                      // 查看当前方案的基本配置
```

## 2.2 Menuconfig 配置

在加载完方案配置后，可以使用 menuconfig 命令来修改当前配置：

```
$ cd luban-lite
$ scons --menuconfig                // Linux 命令行下启动 Menuconfig
$ ....                              // Menuconfig 配置过程
```

在修改 `Project options` 中的配置时需要注意以下要求：

![](images/lbl_menuconfig_prj.png)

## 2.3 编译

配置完成后，可以使用以下的命令进行编译：

```
$ cd luban-lite
$ scons                             // 编译当前方案
$ scons --verbose                   // 编译当前方案，显示更多详细信息（如GCC命令行参数）
$ scons -c                          // clean当前方案
$ ls output/$chip_$board_$kernel_$app/images/$soc.elf   // 编译生成的目标文件
```

## 2.4 其他命令

```
$ cd luban-lite
$ scons --run-qemu           // 运行当前编译出来的qemu目标文件，需要先打开chip->QEMU配置
$ scons --list-size          // size 命令列出所有 .o 文件的 text/data/bss 各个 section 大小
$ scons --pkgs-update        // 下载选择的在线 packages
```

如果需要 Windows IDE 中编译，首先在命令行下使用命令生成当前方案对应的 IDE 方案文件：

```
$ scons --target=eclipse            // 生成当前方案对应的 eclipse 方案文件
```

然后使用 Eclipse IDE 打开方案文件进行调试。

# 3. OneStep 增强命令

Luban-Lite 中对命令行中的scons工具进行了封装，将一些高频命令行操作定义了一组快捷命令，统称为OneStep命令。

OneStep命令的设计目标是：任意目录，只需一步。

使用方法：

- 需要先导入脚本onestep.sh
- 然后在该shell中就可以从任意目录执行以下命令，包括:
  	- lunch - 选择方案
  	- m - 编译SDK
  	- c - clean SDK
  	- cr - 跳转到SDK根目录等

```
$ cd luban-lite
$ source tools/onestep.sh
$ h
Luban-Lite SDK OneStep commands:
  hmm|h                     : Get this help.
  lunch          [keyword]  : Start with selected defconfig.e.g. lunch mmc
  menuconfig|me             : Config SDK with menuconfig
  m                         : Build all and generate final image
  c                         : Clean all
  croot|cr                  : cd to SDK root directory.
  cout|co                   : cd to build output directory.
  cbuild|cb                 : cd to build root directory.
  ctarget|ct                : cd to target board directory.
  godir|gd       [keyword]  : Go/jump to selected directory.
  list                      : List all SDK defconfig.
  i                         : Get current project's information.
```

