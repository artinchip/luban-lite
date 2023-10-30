#!/usr/bin/env python3
# -*- coding:utf-8 -*-
# SPDX-License-Identifier: GPL-2.0+
#
# Dehuang.Wu
# Copyright (C) 2021-2023 ArtInChip Technology Co., Ltd

import os
import re
import sys
import platform
import argparse
import subprocess

def run_cmd(cmdstr):
    # print(cmdstr)
    cmd = cmdstr.split(' ')
    ret = subprocess.run(cmd, stdout=subprocess.PIPE)
    if ret.returncode != 0:
        sys.exit(1)

def mkimage_get_mtdpart_size(outfile):
    imgname = os.path.basename(outfile)
    partlist = os.path.join(os.path.dirname(outfile), 'partition_file_list.h')
    size = 0
    if not os.path.exists(partlist):
        return 0
    with open(partlist) as f:
        lines = f.readlines()
        for ln in lines:
            name = ln.split(',')[1].replace('"', '').replace('*', '')
            if imgname == name or imgname in name:
                size = int(ln.split(',')[2])
                return size
    print('Image {} is not used in any partition'.format(imgname))
    print('please check your project\'s image_cfg.json');
    return size

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--inputdir", type=str,
                        help="input directory")
    parser.add_argument("-o", "--outfile", type=str,
                        help="output file")
    parser.add_argument("-p", "--pagesize", type=str,
                        help="page size")
    parser.add_argument("-b", "--blocksize", type=str,
                        help="block size")
    parser.add_argument("-t", "--tooldir", type=str,
                        help="tool directory")
    args = parser.parse_args()

    imgsize = mkimage_get_mtdpart_size(args.outfile)
    cmdstr = args.tooldir
    if platform.system() == 'Linux':
        cmdstr += 'mklittlefs '
    elif platform.system() == 'Windows':
        cmdstr += 'mklittlefs.exe '
    cmdstr += '-c {} '.format(args.inputdir)
    cmdstr += '-b {} '.format(args.blocksize)
    cmdstr += '-p {} '.format(args.pagesize)
    cmdstr += '-s {} '.format(imgsize)
    cmdstr += '{}'.format(args.outfile)
    run_cmd(cmdstr)
