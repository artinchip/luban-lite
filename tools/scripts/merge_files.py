#!/usr/bin/env python3
# -*- coding:utf-8 -*-
# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (C) 2021 ArtInChip Technology Co., Ltd
# Mingfeng Li <mingfeng.li@artinchip.com>

import argparse

def merge_files(input_files, output_file):
    with open(output_file, 'wb') as outfile:
        for input_file in input_files:
            with open(input_file, 'rb') as infile:
                outfile.write(infile.read())


if __name__ == "__main__":
    # Creat param parse
    parser = argparse.ArgumentParser()

    #for merge_files
    parser.add_argument("-i", "--input_files", nargs="+", help="input files for merge")
    parser.add_argument("-o", "--output_file", help="merge file")

    # parse cmd param
    args = parser.parse_args()

    merge_files(args.input_files, args.output_file)



