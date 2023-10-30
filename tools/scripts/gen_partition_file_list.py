#!/usr/bin/env python3
# -*- coding:utf-8 -*-
# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (C) 2023 ArtInChip Technology Co., Ltd
# Dehuang Wu <dehuang.wu@artinchip.com>

import os, sys, re, subprocess, json, argparse
from collections import namedtuple
from collections import OrderedDict

VERBOSE = False

def parse_image_cfg(cfgfile):
    """ Load image configuration file
    Args:
        cfgfile: Configuration file name
    """
    with open(cfgfile, "r") as f:
        lines = f.readlines()
        jsonstr = ""
        for line in lines:
            sline = line.strip()
            if sline.startswith("//"):
                continue
            slash_start = sline.find("//")
            if slash_start > 0:
                jsonstr += sline[0:slash_start]
            else:
                jsonstr += sline
        # Use OrderedDict is important, we need to iterate FWC in order.
        jsonstr = jsonstr.replace(",}", "}").replace(",]", "]")
        cfg = json.loads(jsonstr, object_pairs_hook=OrderedDict)
    return cfg

def size_str_to_int(size_str):
    if "k" in size_str or "K" in size_str:
        numstr = re.sub(r"[^0-9]", "", size_str)
        return (int(numstr) * 1024)
    if "m" in size_str or "M" in size_str:
        numstr = re.sub(r"[^0-9]", "", size_str)
        return (int(numstr) * 1024 * 1024)
    if "g" in size_str or "G" in size_str:
        numstr = re.sub(r"[^0-9]", "", size_str)
        return (int(numstr) * 1024 * 1024 * 1024)
    if "0x" in size_str or "0X" in size_str:
        return int(size_str, 16)
    if "-" in size_str:
        return 0
    return int(size_str, 10)

def aic_create_part_file_string(cfg, start_offs):
    part_str = ""
    part_size = 0
    part_offs = start_offs
    total_siz = 0
    media_type = cfg["image"]["info"]["media"]["type"]
    total_siz = size_str_to_int(cfg[media_type]["size"])
    partitions = cfg[media_type]["partitions"]
    if len(partitions) == 0:
        print("Partition table is empty")
        sys.exit(1)
    for part in partitions:
        if "size" not in partitions[part]:
            print("No size value for partition: {}".format(part))
        part_offs += part_size
        if partitions[part]["size"] == "-":
            part_size = total_siz - part_offs
        else:
            part_size = size_str_to_int(partitions[part]["size"])
        if "offset" in partitions[part]:
            part_offs = size_str_to_int(partitions[part]["offset"])
        partitions[part]["part_size"] = part_size
        partitions[part]["part_offs"] = part_offs

    target = cfg["image"]["target"]
    for name in target:
        fwc = target[name]
        part_list = fwc["part"]
        for idx in range(len(part_list)):
            part = part_list[idx]
            if part not in partitions.keys():
                print("Partition {} is not exist in partition table".format(part))
                sys.exit(1)
            part_str += "{},{},{}\n".format(part, fwc["file"], partitions[part]["part_size"])
    return part_str

def aic_create_nand_part_file_string(cfg, start_offs):
    part_str = ""
    part_size = 0
    part_offs = start_offs
    total_siz = 0
    media_type = cfg["image"]["info"]["media"]["type"]
    organization = cfg["image"]["info"]["media"]["array_organization"]
    total_siz = size_str_to_int(cfg[media_type]["size"])
    partitions = cfg[media_type]["partitions"]

    if len(partitions) == 0:
        print("Partition table is empty")
        sys.exit(1)
    nands = ""
    if len(organization):
        nands = "nands="
    for idx in range(len(organization)):
        pagesiz = 0
        if "page" in organization[idx]:
            pagesiz = size_str_to_int(organization[idx]["page"])
        blocksiz = 0
        if "block" in organization[idx]:
            blocksiz = size_str_to_int(organization[idx]["block"])
        oobsiz = 0
        if "oob" in organization[idx]:
            oobsiz = size_str_to_int(organization[idx]["oob"])
        nands += "{},{},{};".format(pagesiz, blocksiz, oobsiz)
    if len(nands) > 0:
        part_str += nands[0:-1]
        part_str += "\n"
    for part in partitions:
        if "size" not in partitions[part]:
            print("No size value for partition: {}".format(part))
        part_offs += part_size
        part_size = size_str_to_int(partitions[part]["size"])
        if partitions[part]["size"] == "-":
            part_size = total_siz - part_offs
        if "offset" in partitions[part]:
            part_offs = size_str_to_int(partitions[part]["offset"])
        partitions[part]["part_size"] = part_size
        partitions[part]["part_offs"] = part_offs

    target = cfg["image"]["target"]
    for name in target:
        fwc = target[name]
        part_list = fwc["part"]
        for idx in range(len(part_list)):
            part = part_list[idx]
            if part not in partitions.keys():
                print("Partition {} is not exist in partition table".format(part))
                sys.exit(1)
            part_str += "{},{},{}\n".format(part, fwc["file"], partitions[part]["part_size"])
    return part_str

def aic_create_parts_string(cfg):
    part_str = ""
    media_type = cfg["image"]["info"]["media"]["type"]
    if media_type == "spi-nor":
        part_str += aic_create_part_file_string(cfg, 0)
    if media_type == "spi-nand":
        part_str += aic_create_nand_part_file_string(cfg, 0)
    if media_type == "mmc":
        part_str += aic_create_part_file_string(cfg, 17*1024)

    return part_str


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--config", type=str,
                        help="image configuration file name")
    parser.add_argument("-o", "--outfile", type=str,
                        help="output partition file")
    args = parser.parse_args()
    if args.config == None:
        print('Error, option --config is required.')
        sys.exit(1)

    cfg = parse_image_cfg(args.config)
    parts = aic_create_parts_string(cfg)
    if args.outfile == None:
        print(parts)
    else:
        with open(args.outfile, "w+") as f:
            f.write(parts)
            f.close()


