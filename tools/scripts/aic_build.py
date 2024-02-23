#!/usr/bin/env python2
# -*- coding:utf-8 -*-
# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (C) 2021-2023 ArtInChip Technology Co., Ltd

import os
import re
import sys
import platform
import shutil
from SCons.Script import *

COLOR_BEGIN = "\033["
COLOR_RED = COLOR_BEGIN + "41;37m"
COLOR_YELLOW = COLOR_BEGIN + "43;30m"
COLOR_WHITE = COLOR_BEGIN + "47;30m"
COLOR_END = "\033[0m"

def pr_err(string):
    print(COLOR_RED + '*** '+ string + COLOR_END)

def pr_info(string):
    print(COLOR_WHITE + '>>> ' + string + COLOR_END)

def pr_warn(string):
    print(COLOR_YELLOW + '!!! ' + string + COLOR_END)

def do_system(cmd):
    print('{}'.format(cmd))
    return os.system(cmd)

def do_pipe(cmd):
    p = os.popen(cmd)
    return p.readlines()[0]

def get_config(filename, key):
    if not os.path.exists(filename):
        return None
    for line in open(filename):
        line = line.strip()
        if not line or line[0] == '#':
            continue
        v =  line.split('=')
        if v[0] == key:
            return v[1]
    return None

def get_prj_defconfig(aic_root):
    config_file = os.path.join(aic_root, '.config')
    if not os.path.exists(config_file):
        return None
    key_defconfig = 'CONFIG_PRJ_DEFCONFIG_FILENAME'
    key_chip      = 'CONFIG_PRJ_CHIP'
    key_board     = 'CONFIG_PRJ_BOARD'
    key_kernel    = 'CONFIG_PRJ_KERNEL'
    key_app       = 'CONFIG_PRJ_APP'
    defconfig = get_config(config_file, key_defconfig)
    if defconfig:
        defconfig = defconfig.replace('"','')
    else:
        prj_chip = get_config(config_file, key_chip)
        prj_board = get_config(config_file, key_board)
        prj_kernel = get_config(config_file, key_kernel)
        prj_app = get_config(config_file, key_app)
        defconfig = prj_chip + '_' + prj_board + '_'  + prj_kernel + '_'  + prj_app + '_defconfig'
    return defconfig

def list_defconfig(aic_root):
    v = []
    maxlen = 0
    path = os.path.join(aic_root, 'target', 'configs')
    for root, dirs, files in os.walk(path):
        if root != path:
            break
        for f in files:
            if 'defconfig' in f:
                v.append(f)
                if len(f) > maxlen:
                    maxlen = len(f)
    v.sort()
    return v, maxlen

def apply_defconfig(aic_root, defconfig):
    src = os.path.join(aic_root, 'target', 'configs', defconfig)
    if not os.path.exists(src):
        print('defconfig file {} is not exists!'.format(src))
        sys.exit(110)

    print('Load config from {}'.format(os.path.join('target', 'configs', defconfig)))
    os.system('echo ' + defconfig.replace('_defconfig', '') + ' > ' + os.path.join(aic_root, '.defconfig'))

    dst = os.path.join(aic_root, '.config')
    shutil.copy(src, dst)
    sys.path.append(os.path.join(aic_root, 'kernel/rt-thread/tools'))
    from menuconfig import mk_rtconfig
    mk_rtconfig(".config")

def save_defconfig(aic_root):
    src = os.path.join(aic_root, '.config')
    if not os.path.exists(src):
        print('defconfig file {} is not exists!'.format(src))
        sys.exit(110)
    defconfig = get_prj_defconfig(aic_root)
    if defconfig:
        dst = os.path.join(aic_root, 'target', 'configs', defconfig)
        shutil.copy(src, dst)

def update_defconfig(aic_root):
    defconfig = get_prj_defconfig(aic_root)
    if defconfig:
        src = os.path.join(aic_root, 'target', 'configs', defconfig)
        if not os.path.exists(src):
            sys.exit(110)
        dst = os.path.join(aic_root, '.config')
        shutil.copy(src, dst)

# cmd-option: list defconfig
def list_def_cmd(aic_root):
    AddOption('--list-def',
              dest = 'list_defconfig',
              action = 'store_true',
              default = False,
              help = 'to list all board defconfigs')
    list_def = GetOption('list_defconfig')
    if list_def:
        configs,maxlen = list_defconfig(aic_root)
        index = 0
        print('Built-in configs:')
        for c in configs:
            d = c.replace('_defconfig','')
            print('%3d. %-*s - Build for %s' % (index, maxlen, c, d))
            index += 1
        exit(0)

# cmd-option: apply defconfig
def apply_def_cmd(aic_root):
    AddOption('--apply-def',
              dest = 'apply_defconfig',
              type='string',
              nargs=1,
              action = 'store',
              default = "",
              help = 'to apply one board defconfig')
    apply_def = GetOption('apply_defconfig')
    if apply_def:
        if apply_def.isdigit():
            i = int(apply_def, 10)
            configs,maxlen = list_defconfig(aic_root)
            if i < len(configs):
                apply_def = configs[i]
        apply_defconfig(aic_root, apply_def)
        exit(0)

# cmd-option: save defconfig
def save_def_cmd(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    AddOption('--save-def',
              dest = 'save_defconfig',
              action = 'store_true',
              default = False,
              help = 'to save current board defconfig')
    save_def = GetOption('save_defconfig')
    if save_def:
        save_defconfig(aic_root)
        exit(0)

# cmd-option: call 'pkgs --update'
def pkgs_update_cmd(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    AddOption('--pkgs-update',
              dest = 'pkgs_update',
              action = 'store_true',
              default = False,
              help = 'to update packages online')
    update_def = GetOption('pkgs_update')
    if update_def:
        if platform.system() == 'Linux':
            env_py_path = os.path.join(aic_root, './tools/env/tools/scripts/env.py')
            os.system('python ' + env_py_path + ' package --update')
        elif platform.system() == 'Windows':
            os.system('pkgs --update')
        exit(0)

# cmd-option: project info
def show_info_cmd(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    config_file = os.path.join(aic_root, '.config')
    AddOption('--info',
              dest = 'prj_info',
              action = 'store_true',
              default = False,
              help = 'to show current project info')
    prj_info = GetOption('prj_info')
    if prj_info:
        # arch
        arch = 'unknown'
        if get_config(config_file, 'CONFIG_ARCH_RISCV32'):
            arch = 'riscv32'
        if get_config(config_file, 'CONFIG_ARCH_RISCV64'):
            arch = 'riscv64'
        if get_config(config_file, 'CONFIG_ARCH_CSKY'):
            arch = 'csky'
        # out dir
        prj_name = prj_defconfig.replace('_defconfig', '')
        prj_out_dir = 'output/' + prj_name
        # toolchain
        toolchain = 'toolchain/bin'
        if os.path.exists('toolchain/.ready'):
            chip_path = os.path.join(aic_root, './bsp/artinchip/sys/' + prj_chip)
            sys.path.append(chip_path)
            import rtconfig
            toolchain = os.path.join(toolchain, rtconfig.PREFIX)

        # summary
        print('    Target app: application/{}/{}'.format(prj_kernel, prj_app))
        print('   Target chip: {}'.format(prj_chip))
        print('   Target arch: {}'.format(arch))
        print('  Target board: target/{}/{}'.format(prj_chip, prj_board))
        print(' Target kernel: kernel/{}'.format(prj_kernel))
        print('Defconfig file: target/configs/{}'.format(prj_defconfig))
        print('Root directory: {}'.format(aic_root))
        print(' Out directory: {}'.format(prj_out_dir))
        print('     Toolchain: {}'.format(toolchain))
        exit(0)

# cmd-option: add board
def add_board_cmd(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    AddOption('--add-board',
              dest = 'add_board',
              action = 'store_true',
              default = False,
              help = 'add new board')
    add_board = GetOption('add_board')
    if add_board:
        path = os.path.join(aic_root, 'target')
        # select chip
        chips = get_all_chip(path)
        while 1:
            index = 0
            print("Chip list:")
            for c in chips:
                print('%3d: %s' % (index, c))
                index += 1
            i = raw_input("Select chip for new board(number):")
            if i == 'q':
                exit(0)
            if not i.isdigit():
                print("Please enter the number!")
                continue
            i = int(i ,10)
            if i >= len(chips):
                print("Input number is invalid!")
                continue
            break
        chip = chips[i]
        print("{} \n".format(chip))
        # select reference board
        configs,maxlen = list_defconfig(aic_root)
        configs = list(filter(lambda x: chip in x and 'bootloader' not in x, configs))
        while 1:
            index = 0
            print("Reference defconfig:(Create new board base on selected defconfig):")
            for c in configs:
                print('%3d: %s' % (index, c))
                index += 1
            i = raw_input("Select reference defconfig for new board(number):")
            if i == 'q':
                exit(0)
            if not i.isdigit():
                print("Please enter the number!")
                continue
            i = int(i ,10)
            if i >= len(configs):
                print("Input number is invalid!")
                continue
            break
        ref_cfg = configs[i]
        print("{} \n".format(ref_cfg))
        def_elements = ref_cfg.split('_')
        if len(def_elements) != 5:
            print("Error: {} is not conform to 'CHIP_BOARD_KERNEL_APP_defconfig' format!".format(ref_cfg))
            exit(0)
        board = def_elements[1]
        kernel = def_elements[2]
        app = def_elements[3]
        # input new board name
        while 1:
            i = raw_input("Input new board's name:")
            i = i.strip()
            if i == 'q':
                exit(0)
            if '_' in i or ' ' in i:
                print("Input name can't contain '_' or ' ' !")
                continue
            if i != '':
                break
        new_board = i
        print("{} \n".format(new_board))
        # input new app name
        while 1:
            i = raw_input("Input new app's name: (default {})".format(app))
            i = i.strip()
            if i == 'q':
                exit(0)
            if '_' in i or ' ' in i:
                print("Input name can't contain '_' or ' ' !")
                continue
            if i == '':
                i = app
            break
        new_app = i
        print("{} \n".format(new_app))
        # input manufacturer name
        while 1:
            i = raw_input("Input manufacturer's name:")
            i = i.strip()
            if i == 'q':
                exit(0)
            break
        manuf_name = i
        print("{} \n".format(manuf_name))
        # copy xxx_defconfig
        src_def = os.path.join(aic_root, 'target', 'configs', ref_cfg)
        def_elements[1] = new_board
        def_elements[3] = new_app
        des_cfg = '_'.join(def_elements)
        des_def = os.path.join(aic_root, 'target', 'configs', des_cfg)
        print("Copy {} to {}".format(src_def, des_def))
        shutil.copy(src_def, des_def)
        file_data = ''
        with open(des_def, "r") as f:
            for line in f:
                if src_def in line:
                    line = line.replace(src_def, des_def)
                if board in line:
                    line = line.replace(board, new_board)
                if app in line:
                    line = line.replace(app, new_app)
                if 'CONFIG_AIC_CONSOLE_SYSNAME' in line:
                    line = 'CONFIG_AIC_CONSOLE_SYSNAME="{}"\n'.format(manuf_name)
                if 'CONFIG_FINSH_PROMPT_NAME' in line:
                    line = 'CONFIG_FINSH_PROMPT_NAME="{}"\n'.format(manuf_name)
                file_data += line
        with open(des_def, "w") as f:
            f.write(file_data)
        # copy xxx_baremetal_bootloader_defconfig
        def_elements[1] = board
        def_elements[2] = 'baremetal'
        def_elements[3] = 'bootloader'
        src_cfg = '_'.join(def_elements)
        src_def = os.path.join(aic_root, 'target', 'configs', src_cfg)
        def_elements[1] = new_board
        des_cfg = '_'.join(def_elements)
        des_def = os.path.join(aic_root, 'target', 'configs', des_cfg)
        if not os.path.exists(src_def):
            print("Warning: {} file is not exists \n".format(src_def))
        else:
            print("Copy {} to {}".format(src_def, des_def))
            shutil.copy(src_def, des_def)
            file_data = ''
            with open(des_def, "r") as f:
                for line in f:
                    if src_def in line:
                        line = line.replace(src_def, des_def)
                    if board in line:
                        line = line.replace(board, new_board)
                    file_data += line
            with open(des_def, "w") as f:
                f.write(file_data)
        # copy board
        src_d = os.path.join(aic_root, 'target', chip, board)
        des_d = os.path.join(aic_root, 'target', chip, new_board)
        src_d = os.path.normpath(src_d);
        des_d = os.path.normpath(des_d);
        if not os.path.exists(des_d):
            print("Copy {} to {}".format(src_d, des_d))
            if platform.system() == 'Linux':
                shutil.copytree(src_d, des_d)
            elif platform.system() == 'Windows':
                shutil.copytree('\\\\?\\' + src_d, '\\\\?\\' + des_d)
        des_file = os.path.join(des_d, 'pack', 'image_cfg.json')
        file_data = ''
        with open(des_file, "r") as f:
            for line in f:
                if board in line:
                    line = line.replace(board, new_board)
                file_data += line
        with open(des_file, "w") as f:
            f.write(file_data)
        # copy app
        if app != new_app:
            app_os = kernel
            if app_os != 'baremetal':
                app_os = 'os'
            src_d = os.path.join(aic_root, 'application', app_os, app)
            des_d = os.path.join(aic_root, 'application', app_os, new_app)
            src_d = os.path.normpath(src_d);
            des_d = os.path.normpath(des_d);
            if not os.path.exists(des_d):
                print("Copy {} to {}".format(src_d, des_d))
                if platform.system() == 'Linux':
                    shutil.copytree(src_d, des_d)
                elif platform.system() == 'Windows':
                    shutil.copytree('\\\\?\\' + src_d, '\\\\?\\' + des_d)
        print("Add board {} for {} succeed!".format(new_board, manuf_name))
        exit(0)

# cmd-option: windows menuconfig
def win_menuconfig_cmd(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    if platform.system() == 'Windows':
        AddOption('--menuconfig',
                  dest = 'win_menuconfig',
                  action = 'store_true',
                  default = False,
                  help='make menuconfig for luban-lite')
        win_menuconfig = GetOption('win_menuconfig')
        if win_menuconfig:
            with open(".Kconfig.prj", "w") as f:
                f.write('source "./bsp/artinchip/sys/{}/Kconfig.chip"\n'.format(prj_chip))
                f.write('source "target/{}/{}/Kconfig.board"\n'.format(prj_chip, prj_board))
                f.write('source "kernel/{}/Kconfig"\n'.format(prj_kernel))
                f.write('source "application/{}/{}/Kconfig"\n'.format(prj_kernel, prj_app))
                if prj_kernel == 'rt-thread':
                    f.write('source "$PKGS_DIR/Kconfig"\n')
            os.system('menuconfig')
            save_defconfig(aic_root)
            exit(0)

# check qemu tool
def chk_qemu_tool(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    if platform.system() == 'Linux':
        qemu_name = 'qemu-linux'
        qemu_tar_name = 'xuantie-qemu-x86_64'
    elif platform.system() == 'Windows':
        qemu_name = 'qemu-win'
        qemu_tar_name = 'xuantie-qemu-win'
    qemu_ppath = os.path.join(aic_root, 'tools/env/tools/qemu/')
    qemu_path = os.path.join(qemu_ppath, qemu_name)
    qemu_path = os.path.normpath(qemu_path)
    # check
    if os.path.exists(qemu_path):
        return
    # prepare
    for root, dirs, files in os.walk(qemu_ppath):
        if root != qemu_ppath:
            break
        for f in files:
            if f.startswith(qemu_tar_name) and f.find('.tar') >= 0:
                abs_f = os.path.join(qemu_ppath, f)
                os.mkdir(qemu_path)

                pr_info('Extract ' + qemu_tar_name + ' ...')
                os.system('tar -xzf ' + abs_f + ' -C ' + qemu_path)
                break

# cmd-option: run qemu
def run_qemu_cmd(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    AddOption('--run-qemu',
              dest = 'run_qemu',
              action = 'store_true',
              default = False,
              help = 'use qemu run target')
    run_qemu = GetOption('run_qemu')
    if run_qemu:
        # check qemu tool
        chk_qemu_tool(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig)
        # qemu
        chip_path = os.path.join(aic_root, './bsp/artinchip/sys/' + prj_chip)
        sys.path.append(chip_path)
        import rtconfig
        if rtconfig.DEVICE.find('rv32') >= 0:
            qemu_name = 'qemu-system-riscv32'
            machine = 'smartl'
        else:
            qemu_name = 'qemu-system-riscv64'
            machine = 'smarth'
        if platform.system() == 'Linux':
            qemu_path = os.path.join(aic_root, 'tools/env/tools/qemu/qemu-linux/bin', qemu_name)
        elif platform.system() == 'Windows':
            qemu_path = os.path.join(aic_root, 'tools/env/tools/qemu/qemu-win', qemu_name)
        # target
        prj_name = prj_defconfig.replace('_defconfig','')
        prj_out_dir = 'output/' + prj_name + '/images/'
        target = os.path.join(aic_root, prj_out_dir, rtconfig.SOC + '.' + rtconfig.TARGET_EXT)
        # complete cmd
        cmd = qemu_path + ' -cpu ' + rtconfig.CPUNAME + ' -M ' + machine + ' -kernel ' + target + ' -nographic'
        os.system(cmd)
        exit(0)

# cmd-option: list size
def list_size_cmd(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    AddOption('--list-size',
              dest = 'list_size',
              action = 'store_true',
              default = False,
              help = 'list all obj file size')
    list_size = GetOption('list_size')
    if list_size:
        # size
        gcc_size = os.path.join(aic_root, 'toolchain/bin/riscv64-unknown-elf-size')
        # obj foler
        prj_name = prj_defconfig.replace('_defconfig','')
        prj_out_dir = os.path.join(aic_root, 'output/' + prj_name)
        # target
        chip_path = os.path.join(aic_root, './bsp/artinchip/sys/' + prj_chip)
        sys.path.append(chip_path)
        import rtconfig
        target = os.path.join(prj_out_dir, 'images', rtconfig.SOC + '.' + rtconfig.TARGET_EXT)
        # cmd
        p = []
        for root, dirs, files in os.walk(prj_out_dir):
            if root.endswith('images'):
                continue
            for file in files:
                path = os.path.join(root, file)
                p.append(path)
        p.sort()
        paths = target + ' ' + ' '.join(p)
        cmd = gcc_size + ' ' + paths
        os.system(cmd)
        exit(0)

# cmd-option: distclean
def distclean_cmd(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    AddOption('--distclean',
              dest = 'distclean',
              action = 'store_true',
              default = False,
              help = 'clean the toolchain and the output folder')
    distclean = GetOption('distclean')
    if distclean:
        prj_name = prj_defconfig.replace('_defconfig','')
        os.system('rm -rf ' + aic_root + '/output/' + prj_name)
        os.system('rm -rf ' + aic_root + '/toolchain')
        os.system('rm -f ' + aic_root + '/.config')
        exit(0)

# check & prepare toolchain
def chk_prepare_toolchain(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):
    config_file = os.path.join(aic_root, '.config')
    if get_config(config_file, 'CONFIG_ARCH_CSKY'):
        if platform.system() == 'Linux':
            toolchain_name = 'csky-elf-noneabiv2-tools-x86_64-newlib'
        elif platform.system() == 'Windows':
            toolchain_name = 'csky-elf-noneabiv2-tools-mingw-newlib'
    else:
        if platform.system() == 'Linux':
            toolchain_name = 'Xuantie-900-gcc-elf-newlib-x86_64'
        elif platform.system() == 'Windows':
            toolchain_name = 'Xuantie-900-gcc-elf-newlib-mingw'

    os.chdir(aic_root)
    toolchain_ppath = 'tools/toolchain/'
    toolchain_bpath = 'toolchain'
    # check
    ready_f = os.path.join(toolchain_bpath, '.ready')
    if os.path.exists(ready_f):
        with open(ready_f, 'r') as f:
            toolchain_os = f.readline().strip()
            old_toolchain_name = f.readline().strip()
            f.close()
            if platform.system() != toolchain_os:
                shutil.rmtree(toolchain_bpath)
            else:
                if old_toolchain_name != toolchain_name:
                    shutil.rmtree(toolchain_bpath)
                else:
                    return
    # prepare
    for root, dirs, files in os.walk(toolchain_ppath):
        if root != toolchain_ppath:
            break
        for f in files:
            if f.startswith(toolchain_name) and f.find('.tar') >= 0:
                abs_f = os.path.join(toolchain_ppath, f)
                pr_info('Extract toolchain ' + toolchain_name + ' ...')
                if not os.path.exists(toolchain_bpath):
                    os.mkdir(toolchain_bpath)

                os.system('tar -xzf ' + abs_f \
                    + ' --strip-components 1 -C ' + toolchain_bpath)
                with open(ready_f, 'w') as f:
                    f.write(platform.system() + "\n")
                    f.write(toolchain_name)
                break

def mkimage_get_mtdpart_size(imgname):
    partlist = prj_out_dir + 'partition_file_list.h'
    size = 0
    if not os.path.exists(partlist):
        return 0
    with open(partlist) as f:
        lines = f.readlines()
        for ln in lines:
            name = ln.split(',')[1].replace('"', '').replace('*', '')
            if imgname == re.sub(".sparse", "", name):
                size = int(ln.split(',')[2])
                return size
    print('Image {} is not used in any partition'.format(imgname))
    print('please check your project\'s image_cfg.json');
    return size

def mkimage_get_nand_params():
    partlist = prj_out_dir + 'partition_file_list.h'
    if not os.path.exists(partlist):
        return None
    nands = ""
    with open(partlist) as f:
        lines = f.readlines()
        for ln in lines:
            if "nands=" in ln:
                nands=ln[6:]
                break
    if len(nands) == 0:
        return None
    nands_list = nands.strip().split(';')
    ret_list = []
    for nandparam in nands_list:
        ret_list.append(tuple(nandparam.split(',')))
    return ret_list

def mkimage_gen_uffs_script(srcdir, script):
    fs = open(script, 'w+')
    root_path =srcdir
    for root, dirs, files in os.walk(srcdir):
        for sub in dirs:
            dirpath = os.path.join(root, sub).replace(root_path, '').replace('\\', '/')
            if not dirpath.startswith('/'):
                dirpath = '/' + dirpath
            fs.write('mkdir {}\n'.format(dirpath))
        for fn in files:
            fpath = os.path.join(root, fn)
            srcpath = '::' + fpath
            dstpath = fpath.replace(root_path, '').replace('\\', '/')
            if not dstpath.startswith('/'):
                dstpath = '/' + dstpath
            fs.write('cp {} {}\n'.format(srcpath, dstpath))
    fs.close()

def mkimage_check_file_exist(pathdir, filename):
    restr = filename.replace('.', '\\.').replace('*', '.*')
    ents = os.listdir(pathdir)
    for ent in ents:
        matchobj=re.match(restr, ent)
        if matchobj:
            return True
    return False

def mkimage_gen_empty_img(outimg, size):
    f = open(outimg, 'wb')
    f.write(bytearray(size))
    f.close()

def mkimage_gen_mkfs_action(img_id):
    img_enable = 'CONFIG_AIC_USING_FS_IMAGE_{}'.format(img_id)
    if get_config(prj_root_dir + ".config", img_enable) != 'y':
        return ''
    fatfs_enable = 'CONFIG_AIC_USING_FS_IMAGE_TYPE_FATFS_FOR_{}'.format(img_id)
    littlefs_enable = 'CONFIG_AIC_USING_FS_IMAGE_TYPE_LITTLEFS_FOR_{}'.format(img_id)
    uffs_enable = 'CONFIG_AIC_USING_FS_IMAGE_TYPE_UFFS_FOR_{}'.format(img_id)
    mkfscmd = ''
    if get_config(prj_root_dir + '.config', fatfs_enable) == 'y':
        imgname_opt = 'CONFIG_AIC_FS_IMAGE_NAME_{}'.format(img_id)
        imgname = get_config(prj_root_dir + '.config', imgname_opt).replace('"', '')
        imgsize = mkimage_get_mtdpart_size(imgname)
        auto_opt = 'CONFIG_AIC_FATFS_AUTO_SIZE_FOR_{}'.format(img_id)
        srcdir_opt = 'CONFIG_AIC_FS_IMAGE_DIR_{}'.format(img_id)
        srcdir = get_config(prj_root_dir + '.config', srcdir_opt).replace('"', '')
        os.environ["aic_fs_image_dir"] = srcdir
        srcdir = prj_root_dir + srcdir
        if srcdir.endswith('/'):
            srcdir = srcdir[0:len(srcdir) - 1]
        outimg = prj_out_dir + imgname
        data_siz = 0

        # when CONFIG_AIC_FATFS_AUTO_SIZE_FOR_0 is not set, should be check None
        if get_config(prj_root_dir + '.config', auto_opt) != None:
            auto_siz = get_config(prj_root_dir + '.config', auto_opt).replace('"', '')
        else:
            auto_siz = 'n'

        cluster_size = 'CONFIG_AIC_USING_FS_IMAGE_TYPE_FATFS_CLUSTER_SIZE'
        cluster = int(get_config(prj_root_dir + '.config', cluster_size))
        if auto_siz == 'y':
            sector_siz = 512
            cmdstr = 'python3 ' + aic_script_dir + 'makefatfs.py '
            cmdstr += '--auto '
            cmdstr += '--cluster {} '.format(cluster)
            cmdstr += '--sector {} '.format(sector_siz)
            cmdstr += '--tooldir {} '.format(aic_script_dir)
            cmdstr += '--inputdir {} '.format(srcdir)
            cmdstr += '--outfile {}\n'.format(outimg)
            mkfscmd += cmdstr
        else:
            sector_opt = 'CONFIG_AIC_FATFS_SECTOR_SIZE_FOR_{}'.format(img_id)
            sector_siz = int(get_config(prj_root_dir + '.config', sector_opt))
            sectcnt_opt = 'CONFIG_AIC_FATFS_SECTOR_COUNT_FOR_{}'.format(img_id)
            sector_cnt = int(get_config(prj_root_dir + '.config', sectcnt_opt))
            data_siz = sector_siz * sector_cnt
            cmdstr = 'python3 ' + aic_script_dir + 'makefatfs.py '
            cmdstr += '--imgsize {} '.format(data_siz)
            cmdstr += '--cluster {} '.format(cluster)
            cmdstr += '--sector {} '.format(sector_siz)
            cmdstr += '--tooldir {} '.format(aic_script_dir)
            cmdstr += '--inputdir {} '.format(srcdir)
            cmdstr += '--outfile {}\n'.format(outimg)
            mkfscmd += cmdstr
        if data_siz > imgsize:
            print('data_size:{}'.format(data_siz))
            print('imgsize:{}'.format(imgsize))
            print('Error, the image size will larger than partition size')
            print('  -> {}'.format(outimg))
            sys.exit(0)
    if get_config(prj_root_dir + '.config', littlefs_enable) == 'y':
        imgname_opt = 'CONFIG_AIC_FS_IMAGE_NAME_{}'.format(img_id)
        imgname = get_config(prj_root_dir + '.config', imgname_opt).replace('"', '')
        imgsize = mkimage_get_mtdpart_size(imgname)
        blksiz_opt = 'CONFIG_AIC_LITTLEFS_BLOCK_SIZE_FOR_{}'.format(img_id)
        blksiz = get_config(prj_root_dir + '.config', blksiz_opt)
        pagesiz_opt = 'CONFIG_AIC_LITTLEFS_PAGE_SIZE_FOR_{}'.format(img_id)
        pagesiz = get_config(prj_root_dir + '.config', pagesiz_opt)
        srcdir_opt = 'CONFIG_AIC_FS_IMAGE_DIR_{}'.format(img_id)
        srcdir = get_config(prj_root_dir + '.config', srcdir_opt).replace('"', '')
        os.environ["aic_fs_image_dir"] = srcdir
        srcdir = prj_root_dir + srcdir
        outimg = prj_out_dir + imgname
        cmdstr = 'python3 ' + aic_script_dir + 'makelittlefs.py '
        cmdstr += '--pagesize {} '.format(pagesiz)
        cmdstr += '--blocksize {} '.format(blksiz)
        cmdstr += '--tooldir {} '.format(aic_script_dir)
        cmdstr += '--inputdir {} '.format(srcdir)
        cmdstr += '--outfile {}\n'.format(outimg)
        mkfscmd += cmdstr
    if get_config(prj_root_dir + '.config', uffs_enable) == 'y':
        nand_list = mkimage_get_nand_params()
        if nand_list == None:
            return ''
        imgname_opt = 'CONFIG_AIC_FS_IMAGE_NAME_{}'.format(img_id)
        imgname = get_config(prj_root_dir + '.config', imgname_opt).replace('"', '')
        imgsize = mkimage_get_mtdpart_size(imgname)
        srcdir_opt = 'CONFIG_AIC_FS_IMAGE_DIR_{}'.format(img_id)
        srcdir = get_config(prj_root_dir + '.config', srcdir_opt).replace('"', '')
        os.environ["aic_fs_image_dir"] = srcdir
        srcdir = prj_root_dir + srcdir
        for param in nand_list:
            (pagesiz, blksiz, oobsiz) = param
            pagesiz = int(pagesiz)
            blksiz = int(blksiz)
            oobsiz = int(oobsiz)
            blkpagecnt = int(blksiz / pagesiz)
            partblkcnt = int(imgsize / blksiz)
            prefix = 'page_{}k_block_{}k_oob_{}_'.format(int(pagesiz / 1024),
                    int(blksiz / 1024), oobsiz)
            outimg = os.path.join(prj_out_dir,prefix + imgname)
            if os.path.exists(outimg):
                os.remove(outimg)
            cmdstr = 'python3 ' + aic_script_dir + 'makeuffs.py '
            cmdstr += '--pagesize {} '.format(pagesiz)
            cmdstr += '--oobsize {} '.format(oobsiz)
            cmdstr += '--blockpagecount {} '.format(blkpagecnt)
            cmdstr += '--tooldir {} '.format(aic_script_dir)
            cmdstr += '--inputdir {} '.format(srcdir)
            cmdstr += '--outfile {}\n'.format(outimg)
            mkfscmd += cmdstr
    os.environ["img{}_srcdir".format(img_id)] = srcdir
    return mkfscmd


def get_post_build_bat(aic_root_n, post_objcopy, post_build_cmd):
    des_file = os.path.join(prj_out_dir, './post_build.bat')
    with open(des_file, "w") as f:
        f.write('@echo off\n')
        f.write('setlocal EnableDelayedExpansion\n')
        f.write(post_objcopy)
        f.write(post_build_cmd)
        f.write('echo success\n')
        f.write('pause\n')


def mkimage_prebuild(aic_root, prj_chip, prj_board, prj_kernel, prj_app, prj_defconfig):

    global prj_root_dir
    global aic_script_dir
    global aic_common_dir
    global aic_pack_dir
    global prj_out_dir
    prj_root_dir = aic_root + '/'
    aic_script_dir = os.path.join(aic_root, 'tools/scripts/')
    aic_common_dir = os.path.join(aic_root, 'bsp/artinchip/sys/' + prj_chip)
    aic_pack_dir = os.path.join(aic_root, 'target/' + prj_chip + '/' + prj_board + '/pack/')
    prj_name = prj_defconfig.replace('_defconfig','')
    prj_out_dir = os.path.join(aic_root, 'output/' + prj_name + '/images/')
    chip_path = os.path.join(aic_root, './bsp/artinchip/sys/' + prj_chip)
    os.environ["PRJ_OUT_DIR"] = prj_out_dir
    sys.path.append(chip_path)
    ota_enable = 'CONFIG_LPKG_USING_OTA_DOWNLOADER'
    env_enable = 'CONFIG_AIC_ENV_INTERFACE'
    burner_enable = 'CONFIG_GENERATE_BURNER_IMAGE'
    import rtconfig

    POST_ACTION = ''

    if prj_kernel != 'baremetal' and "bootloader" != prj_app:
        if get_config(prj_root_dir + '.config', env_enable) == 'y':
            os.system('cp ' + aic_pack_dir + 'env.txt ' + prj_out_dir)

    if not os.path.exists(prj_out_dir):
        os.makedirs(prj_out_dir)
    # Gen partition table
    CMD = 'python3 ' + aic_script_dir + 'gen_partition_table.py'
    CMD += ' -c ' + aic_pack_dir +  'image_cfg.json'
    CMD += ' -o ' + prj_root_dir + 'partition_table.h'
    CMD += ' -j ' + prj_out_dir + 'partition.json'
    os.system(CMD)
    PRE_ACTION = CMD + ';'

    # Gen partition-file-list table
    CMD = 'python3 ' + aic_script_dir + 'gen_partition_file_list.py'
    CMD += ' -c ' + aic_pack_dir +  'image_cfg.json'
    CMD += ' -o ' + prj_out_dir + 'partition_file_list.h'
    os.system(CMD)
    PRE_ACTION += CMD + ';'

    MKFS_ACTION = ''
    MKFS_ACTION += mkimage_gen_mkfs_action(0)
    MKFS_ACTION += mkimage_gen_mkfs_action(1)

    # save bootloader
    POST_ACTION += 'python3 ' + aic_script_dir + 'linked_size.py' + ' -m ' + prj_out_dir + rtconfig.SOC + '.map\n'
    SOURCE_BIN = prj_out_dir + rtconfig.SOC + '.bin'
    TARGET_BIN = aic_pack_dir + 'bootloader.bin'
    if prj_kernel == 'baremetal' and "bootloader" == prj_app:
        POST_ACTION += '@cp -r ' + SOURCE_BIN + ' ' + TARGET_BIN + '\n'
    else:
        if len(MKFS_ACTION):
            POST_ACTION += MKFS_ACTION
        # packaged ddr files
        POST_ACTION += '@cp -r ' + aic_pack_dir + '* ' + prj_out_dir + '\n'
        if platform.system() == 'Linux':
            MAKE_DDR_TOOL = 'python3 ' + aic_script_dir + 'mk_private_resource.py'
        elif platform.system() == 'Windows':
            MAKE_DDR_TOOL = aic_script_dir + 'mk_private_resource.exe'
        DDR_JSON = prj_out_dir + 'ddr_init.json'
        PART_JSON = prj_out_dir + 'partition.json'
        DDR_BIN = prj_out_dir + 'ddr_init.bin'
        MAKE_DDR_ACTION = MAKE_DDR_TOOL + ' -v -l {},{} -o {}\n'.format(DDR_JSON, PART_JSON, DDR_BIN)
        if os.path.exists(aic_pack_dir + 'ddr_init.json'):
            POST_ACTION += MAKE_DDR_ACTION
        PBP_JSON = prj_out_dir + 'pbp_cfg.json'
        PBP_BIN = prj_out_dir + 'pbp_cfg.bin'
        MAKE_PBP_ACTION = MAKE_DDR_TOOL + ' -v -l {},{} -o {}\n'.format(PBP_JSON, PART_JSON, PBP_BIN)
        if os.path.exists(aic_pack_dir + 'pbp_cfg.json'):
            POST_ACTION += MAKE_PBP_ACTION
        # pack image files
        if mkimage_check_file_exist(aic_pack_dir, '*.pbp'):
            POST_ACTION += '@cp -r ' + aic_pack_dir + '/*.pbp ' + prj_out_dir + '\n'
        elif mkimage_check_file_exist(aic_common_dir, '*.pbp'):
            POST_ACTION += '@cp -r ' + aic_common_dir + '/*.pbp ' + prj_out_dir + '\n'

        ELF_PARSE_TOOL = 'python3 ' + aic_script_dir + 'elf_parse.py'
        ELF_PARSE_ACTION = ELF_PARSE_TOOL + ' $TARGET ' + prj_out_dir + ' ' + rtconfig.PREFIX + '\n'
        POST_ACTION += ELF_PARSE_ACTION

        if platform.system() == 'Linux':
            MAKE_IMG_TOOL = aic_script_dir + 'mk_image.py'
        elif platform.system() == 'Windows':
            MAKE_IMG_TOOL = aic_script_dir + 'mk_image.exe'
        IMG_JSON = prj_out_dir + 'image_cfg.json'
        if get_config(prj_root_dir + '.config', burner_enable) == 'y':
            MAKE_IMG_ARGS = ' -v -b -c '
        else:
            MAKE_IMG_ARGS = ' -v -c '
        MAKE_IMG_ACTION = MAKE_IMG_TOOL + MAKE_IMG_ARGS + IMG_JSON + ' -d ' + prj_out_dir + '\n'
        POST_ACTION += MAKE_IMG_ACTION

    if get_config(prj_root_dir + '.config', ota_enable) == 'y':
        POST_ACTION += mkimage_cpio(aic_root, prj_chip, prj_kernel, prj_app, prj_defconfig, rtconfig.OBJCPY)

    # eclipse pre/post build cmd
    aic_root_n = os.path.normpath(aic_root)
    prj_out_dir_n = os.path.normpath(prj_out_dir)
    aic_script_dir_n = os.path.normpath(aic_script_dir)

    eclipse_pre_build = PRE_ACTION
    eclipse_pre_build = eclipse_pre_build.replace('python3', '${ProjDirPath}/../../../tools/env/tools/Python39/python3')
    eclipse_pre_build = eclipse_pre_build.replace(prj_out_dir, '${ProjDirPath}/Debug/')
    eclipse_pre_build = eclipse_pre_build.replace(prj_out_dir_n, '${ProjDirPath}/Debug')
    eclipse_pre_build = eclipse_pre_build.replace(aic_root, '${ProjDirPath}/../../..')
    eclipse_pre_build = eclipse_pre_build.replace(aic_root_n, '${ProjDirPath}/../../..')
    eclipse_pre_build = eclipse_pre_build.replace('\\', '/')

    eclipse_sdk_pre_build = PRE_ACTION
    eclipse_sdk_pre_build = eclipse_sdk_pre_build.replace('python3', '${ProjDirPath}/tools/Python39/python3')
    eclipse_sdk_pre_build = eclipse_sdk_pre_build.replace(prj_out_dir, '${ProjDirPath}/Debug/')
    eclipse_sdk_pre_build = eclipse_sdk_pre_build.replace(prj_out_dir_n, '${ProjDirPath}/Debug')
    eclipse_sdk_pre_build = eclipse_sdk_pre_build.replace(aic_script_dir, '${ProjDirPath}/tools/scripts/')
    eclipse_sdk_pre_build = eclipse_sdk_pre_build.replace(aic_script_dir_n, '${ProjDirPath}/tools/scripts')
    eclipse_sdk_pre_build = eclipse_sdk_pre_build.replace(aic_root, '${ProjDirPath}')
    eclipse_sdk_pre_build = eclipse_sdk_pre_build.replace(aic_root_n, '${ProjDirPath}')
    eclipse_sdk_pre_build = eclipse_sdk_pre_build.replace('\\', '/')

    eclipse_post_build = POST_ACTION
    eclipse_post_build = eclipse_post_build.replace('\n',';')
    eclipse_post_build = eclipse_post_build.replace('python3', '${ProjDirPath}/../../../tools/env/tools/Python39/python3')
    eclipse_post_build = eclipse_post_build.replace('@cp', '${ProjDirPath}/../../../tools/env/tools/bin/cp')
    eclipse_post_build = eclipse_post_build.replace('@echo', '${ProjDirPath}/../../../tools/env/tools/bin/echo')
    eclipse_post_build = eclipse_post_build.replace(prj_out_dir, '${ProjDirPath}/Debug/')
    eclipse_post_build = eclipse_post_build.replace(prj_out_dir_n, '${ProjDirPath}/Debug')
    eclipse_post_build = eclipse_post_build.replace(aic_root, '${ProjDirPath}/../../..')
    eclipse_post_build = eclipse_post_build.replace(aic_root_n, '${ProjDirPath}/../../..')
    eclipse_post_build = eclipse_post_build.replace('$TARGET', '${ProjDirPath}/Debug/${ProjName}.elf')
    eclipse_post_build = eclipse_post_build.replace('\\', '/')

    eclipse_sdk_post_build = POST_ACTION
    eclipse_sdk_post_build = eclipse_sdk_post_build.replace('\n',';')
    eclipse_sdk_post_build = eclipse_sdk_post_build.replace('python3', '${ProjDirPath}/tools/Python39/python3')
    eclipse_sdk_post_build = eclipse_sdk_post_build.replace('@cp', '${ProjDirPath}/tools/bin/cp')
    eclipse_sdk_post_build = eclipse_sdk_post_build.replace('@echo', '${ProjDirPath}/tools/bin/echo')
    eclipse_sdk_post_build = eclipse_sdk_post_build.replace(prj_out_dir, '${ProjDirPath}/Debug/')
    eclipse_sdk_post_build = eclipse_sdk_post_build.replace(prj_out_dir_n, '${ProjDirPath}/Debug')
    eclipse_sdk_post_build = eclipse_sdk_post_build.replace(aic_root, '${ProjDirPath}')
    eclipse_sdk_post_build = eclipse_sdk_post_build.replace(aic_root_n, '${ProjDirPath}')
    eclipse_sdk_post_build = eclipse_sdk_post_build.replace('$TARGET', '${ProjDirPath}/Debug/${ProjName}.elf')
    eclipse_sdk_post_build = eclipse_sdk_post_build.replace('\\', '/')

    post_objcopy = "${cross_prefix}${cross_objcopy}${cross_suffix} -O binary ${ProjName}.elf ${ProjName}.bin;"
    os.environ["eclipse_pre_build"] = eclipse_pre_build
    os.environ["eclipse_post_build"] = post_objcopy + eclipse_post_build
    os.environ["eclipse_sdk_pre_build"] = eclipse_sdk_pre_build
    os.environ["eclipse_sdk_post_build"] = post_objcopy + eclipse_sdk_post_build

    get_post_build_bat(prj_out_dir, post_objcopy, eclipse_sdk_post_build)

    # complete flag
    POST_ACTION += '@echo \n@echo Luban-Lite is built successfully \n@echo \n'
    return POST_ACTION

noprj_msg = '''
No project information
You can start one project by following commands

    scons --list-def
    scons --apply-def=<chip>_<board>_<kernel>_<app>_defconfig
    scons
'''

def chk_prj_config(aic_root):
    # cmd-option: list defconfig
    list_def_cmd(aic_root)

    # cmd-option: apply defconfig
    apply_def_cmd(aic_root)

    # check '.config'
    path = os.path.join(aic_root, '.config')
    if not os.path.exists(path):
        print(noprj_msg)
        exit(0)

    # update '.config'
    update_defconfig(aic_root)

    # update 'rtconfig.h'
    sys.path.append(os.path.join(aic_root, 'kernel/rt-thread/tools'))
    from menuconfig import mk_rtconfig
    mk_rtconfig(".config")

def get_all_chip(tdir):
    v = []
    for root, dirs, files in os.walk(tdir):
        if root != tdir:
            break
        for d in dirs:
            if d != 'configs':
                v.append(d)
    v.sort()
    return v

def get_all_board(tdir):
    v = []
    for root, dirs, files in os.walk(tdir):
        if root != tdir:
            break
        for d in dirs:
            v.append(d)
    v.sort()
    return v

def get_all_kernel(tdir):
    v = []
    for root, dirs, files in os.walk(tdir):
        if root != tdir:
            break
        for d in dirs:
            if d != 'common':
                v.append(d)
    v.sort()
    return v

def get_all_app(tdir):
    v = []
    for root, dirs, files in os.walk(tdir):
        if root != tdir:
            break
        for d in dirs:
            v.append(d)
    v.sort()
    return v

def mkimage_xip_postaction(aic_root, prj_chip, prj_kernel, prj_app, prj_defconfig, rtconfig_objcpy):
    config_file = os.path.join(aic_root, '.config')
    key_xip    = 'CONFIG_AIC_XIP'
    global XIP_POST_ACTION
    XIP_POST_ACTION = ''

    # get CONFIG_AIC_XIP
    AIC_XIP = get_config(config_file, key_xip)
    if AIC_XIP is None:
        return XIP_POST_ACTION

    global prj_out_dir
    prj_name = prj_defconfig.replace('_defconfig','')
    prj_out_dir = os.path.join(aic_root, 'output/' + prj_name + '/images/')

    # XIP image post action
    XIP_POST_ACTION += '@echo ' + prj_kernel + "-" + prj_app + ': make XIP image begin...\n'
    XIP_POST_ACTION += rtconfig_objcpy + ' -O binary -j .text -j .eh_frame_hdr -j .eh_frame -j .gcc_except_table -j .rodata $TARGET ' + prj_out_dir + prj_chip + '.text.bin\n'
    XIP_POST_ACTION += rtconfig_objcpy + ' -O binary -j .ram.code -j .data $TARGET ' + prj_out_dir + prj_chip+ '.data.bin\n'

    XIP_POST_ACTION += 'python3 '+ aic_script_dir + 'merge_files.py ' + '-i ' + prj_out_dir + prj_chip + '.text.bin' + ' ' + prj_out_dir + prj_chip + '.data.bin' + ' -o ' + prj_out_dir + prj_chip + '.bin\n'
    XIP_POST_ACTION += '@echo ' + prj_kernel + "-" + prj_app + ': make XIP image done... \n'

    return XIP_POST_ACTION

def mkimage_cpio(aic_root, prj_chip, prj_kernel, prj_app, prj_defconfig, rtconfig_objcpy):
    global CPIO_POST_ACTION
    CPIO_POST_ACTION = ''

    if not os.path.exists(aic_pack_dir + 'ota-subimgs.cfg'):
        CPIO_POST_ACTION += '@echo ' + 'Warning: mkimage_cpio not find ' + aic_pack_dir + 'ota-subimgs.cfg' + ' \n'
        return CPIO_POST_ACTION

    os.system('cp ' + aic_pack_dir + 'ota-subimgs.cfg ' + prj_out_dir)
    aic_system = platform.system()

    CPIO_POST_ACTION += '@echo ' + 'make CPIO image begin...\n'
    CPIO_POST_ACTION += 'python3 ' + aic_script_dir + 'mkcpio.py ' + aic_system + ' ' + aic_root + ' ' + aic_pack_dir + ' ' + prj_out_dir + '\n'

    CPIO_POST_ACTION += '@echo ' + 'make CPIO image done...\n'
    return CPIO_POST_ACTION

def get_prj_config(aic_root):
    config_file = os.path.join(aic_root, '.config')
    key_chip    = 'CONFIG_PRJ_CHIP'
    key_board   = 'CONFIG_PRJ_BOARD'
    key_kernel  = 'CONFIG_PRJ_KERNEL'
    key_app     = 'CONFIG_PRJ_APP'
    key_lds     = 'CONFIG_PRJ_CUSTOM_LDS'

    # get PRJ_CHIP
    PRJ_CHIP = get_config(config_file, key_chip)
    if PRJ_CHIP is None:
        print('Get {} in {} fail, please check your configuration!!!'.format(key_chip, config_file))
        exit(0)
    PRJ_CHIP = PRJ_CHIP.replace('"','')
    # check PRJ_CHIP
    path = os.path.join(aic_root, 'target')
    chips = get_all_chip(path)
    if PRJ_CHIP not in chips:
        print('Chip name {} is invalid, please check your configuration!!!'.format(PRJ_CHIP))
        exit(0)

    # get PRJ_BOARD
    PRJ_BOARD = get_config(config_file, key_board)
    if PRJ_BOARD is None:
        print('Get {} in {} fail, please check your configuration!!!'.format(key_board, config_file))
        exit(0)
    PRJ_BOARD = PRJ_BOARD.replace('"','')
    # check PRJ_BOARD
    path = os.path.join(aic_root, 'target', PRJ_CHIP)
    boards = get_all_board(path)
    if PRJ_BOARD not in boards:
        print('Board name {} is invalid, please check your configuration!!!'.format(PRJ_BOARD))
        exit(0)

    # get PRJ_KERNEL
    PRJ_KERNEL = get_config(config_file, key_kernel)
    if PRJ_KERNEL is None:
        print('Get {} in {} fail, please check your configuration!!!'.format(key_kernel, config_file))
        exit(0)
    PRJ_KERNEL = PRJ_KERNEL.replace('"','')
    # check PRJ_KERNEL
    path = os.path.join(aic_root, 'kernel')
    kernels = get_all_kernel(path)
    if PRJ_KERNEL not in kernels:
        print('Kernel name {} is invalid, please check your configuration!!!'.format(PRJ_KERNEL))
        exit(0)

    # get PRJ_APP
    PRJ_APP = get_config(config_file, key_app)
    if PRJ_APP is None:
        print('Get {} in {} fail, please check your configuration!!!'.format(key_app, config_file))
        exit(0)
    PRJ_APP = PRJ_APP.replace('"', '')
    # check PRJ_APP
    path = os.path.join(aic_root, 'application', PRJ_KERNEL)
    apps = get_all_app(path)
    if PRJ_APP not in apps:
        print('App name {} is invalid, please check your configuration!!!'.format(PRJ_APP))
        exit(0)

    # get PRJ_NAME
    PRJ_DEFCONFIG_NAME = get_prj_defconfig(aic_root)

    # get PRJ_CUSTOM_LDS
    custom_lds = get_config(config_file, key_lds)
    if custom_lds:
        custom_lds = custom_lds.replace('"','')
    else:
        custom_lds = ''
    PRJ_CUSTOM_LDS = custom_lds

    # cmd-option: save defconfig
    save_def_cmd(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    # cmd-option: project info
    show_info_cmd(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    # cmd-option: call 'pkgs --update'
    pkgs_update_cmd(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    # cmd-option: windows menuconfig
    win_menuconfig_cmd(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    # cmd-option: run qemu target
    run_qemu_cmd(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    # cmd-option: list size
    list_size_cmd(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    # cmd-option: add board
    add_board_cmd(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    # cmd-option: distclean
    distclean_cmd(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    # check & prepare toolchain
    chk_prepare_toolchain(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    # make image pre_build cmd
    MKIMAGE_POST_ACTION = mkimage_prebuild(aic_root, PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME)

    return PRJ_CHIP, PRJ_BOARD, PRJ_KERNEL, PRJ_APP, PRJ_DEFCONFIG_NAME, PRJ_CUSTOM_LDS, MKIMAGE_POST_ACTION
