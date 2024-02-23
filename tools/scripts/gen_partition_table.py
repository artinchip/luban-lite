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


def aic_create_parts_json(cfg):
    mtd = ""
    ubi = ""
    nftl = ""
    gpt = ""

    part_str = '{\n\t"partitions": {\n'
    part_type = []
    part_size = 0
    part_offs = 0
    total_siz = 0
    media_type = cfg["image"]["info"]["media"]["type"]
    if media_type == "spi-nand" or media_type == "spi-nor":
        total_siz = size_str_to_int(cfg[media_type]["size"])
        partitions = cfg[media_type]["partitions"]
        part_type.append("mtd")
        mtd = "spi{}.0:".format(cfg["image"]["info"]["media"]["device_id"])
        if len(partitions) == 0:
            print("Partition table is empty")
            sys.exit(1)

        for part in partitions:
            itemstr = ""
            if "size" not in partitions[part]:
                print("No size value for partition: {}".format(part))
            part_offs += part_size
            itemstr += partitions[part]["size"]
            part_size = size_str_to_int(partitions[part]["size"])
            if partitions[part]["size"] == "-":
                part_size = total_siz - part_offs
            if "offset" in partitions[part]:
                itemstr += "@{}".format(partitions[part]["offset"])
                part_offs = size_str_to_int(partitions[part]["offset"])
            itemstr += "({})".format(part)

            mtd += itemstr + ","
            if "ubi" in partitions[part]:
                if "ubi" not in part_type:
                    part_type.append("ubi")

                volumes = partitions[part]["ubi"]
                if len(volumes) == 0:
                    print("Volume of {} is empty".format(part))
                    sys.exit(1)
                ubi += "{}:".format(part)
                for vol in volumes:
                    itemstr = ""
                    if "size" not in volumes[vol]:
                        print("No size value for ubi volume: {}".format(vol))
                    itemstr += volumes[vol]["size"]
                    if "offset" in volumes[vol]:
                        itemstr += "@{}".format(volumes[vol]["offset"])
                    itemstr += "({})".format(vol)
                    ubi += itemstr + ","
                ubi = ubi[0:-1] + ";"

            if "nftl" in partitions[part]:
                if "nftl" not in part_type:
                    part_type.append("nftl")
                nftl_volumes = partitions[part]["nftl"]
                if len(nftl_volumes) == 0:
                    print("Volume of {} is empty".format(part))
                    sys.exit(1)
                nftl += "{}:".format(part)
                for vol in nftl_volumes:
                    itemstr = ""
                    if "size" not in nftl_volumes[vol]:
                        print("No size value for nftl volume: {}".format(vol))
                    itemstr += nftl_volumes[vol]["size"]
                    if "offset" in nftl_volumes[vol]:
                        itemstr += "@{}".format(nftl_volumes[vol]["offset"])
                    itemstr += "({})".format(vol)
                    nftl += itemstr + ","
                nftl = nftl[0:-1] + ";"

        mtd = mtd[0:-1]
        part_str += "\t\t\"mtd\" : \"{}\",\n".format(mtd)
        if len(ubi) > 0:
            ubi = ubi[0:-1]
            part_str += "\t\t\"ubi\" : \"{}\",\n".format(ubi)
        if len(nftl) > 0:
            nftl = nftl[0:-1]
            part_str += "\t\t\"nftl\" : \"{}\",\n".format(nftl)

    elif media_type == "mmc":
        part_type.append("gpt")
        partitions = cfg[media_type]["partitions"]
        if len(partitions) == 0:
            print("Partition table is empty")
            sys.exit(1)
        for part in partitions:
            itemstr = ""
            if "size" not in partitions[part]:
                print("No size value for partition: {}".format(part))
            itemstr += partitions[part]["size"]
            if "offset" in partitions[part]:
                itemstr += "@{}".format(partitions[part]["offset"])
            itemstr += "({})".format(part)
            gpt += itemstr + ","
        gpt = gpt[0:-1]
        part_str += "\t\t\"gpt\" : \"{}\",\n".format(gpt)
        # parts_mmc will be deleted later, keep it just for old version AiBurn tool
    else:
        print("Not supported media type: {}".format(media_type))
        sys.exit(1)

    part_str += '\t\t"type": {},\n'.format(str(part_type).replace("'", '"'))
    part_str += "\t}\n}"
    return part_str


def aic_create_parts_string(cfg):
    mtd = ""
    ubi = ""
    nftl = ""
    gpt = ""

    part_str = ""
    fal_cfg = "\n"
    part_size = 0
    part_offs = 0
    total_siz = 0
    media_type = cfg["image"]["info"]["media"]["type"]
    if media_type == "spi-nand" or media_type == "spi-nor":
        total_siz = size_str_to_int(cfg[media_type]["size"])
        partitions = cfg[media_type]["partitions"]
        mtd = "spi{}.0:".format(cfg["image"]["info"]["media"]["device_id"])
        if len(partitions) == 0:
            print("Partition table is empty")
            sys.exit(1)
        if media_type == "spi-nor":
            fal_cfg += "\n#ifdef FAL_PART_HAS_TABLE_CFG\n"
            fal_cfg += "#define FAL_PART_TABLE \\\n{ \\\n"
        for part in partitions:
            itemstr = ""
            if "size" not in partitions[part]:
                print("No size value for partition: {}".format(part))
            part_offs += part_size
            itemstr += partitions[part]["size"]
            part_size = size_str_to_int(partitions[part]["size"])
            if partitions[part]["size"] == "-":
                part_size = total_siz - part_offs
            if "offset" in partitions[part]:
                itemstr += "@{}".format(partitions[part]["offset"])
                part_offs = size_str_to_int(partitions[part]["offset"])
            itemstr += "({})".format(part)
            mtd += itemstr + ","
            if "ubi" in partitions[part]:
                volumes = partitions[part]["ubi"]
                if len(volumes) == 0:
                    print("Volume of {} is empty".format(part))
                    sys.exit(1)
                ubi += "{}:".format(part)
                for vol in volumes:
                    itemstr = ""
                    if "size" not in volumes[vol]:
                        print("No size value for ubi volume: {}".format(vol))
                    itemstr += volumes[vol]["size"]
                    if "offset" in volumes[vol]:
                        itemstr += "@{}".format(volumes[vol]["offset"])
                    itemstr += "({})".format(vol)
                    ubi += itemstr + ","
                ubi = ubi[0:-1] + ";"

            if "nftl" in partitions[part]:
                nftl_volumes = partitions[part]["nftl"]
                if len(nftl_volumes) == 0:
                    print("Volume of {} is empty".format(part))
                    sys.exit(1)
                nftl += "{}:".format(part)
                for vol in nftl_volumes:
                    itemstr = ""
                    if "size" not in nftl_volumes[vol]:
                        print("No size value for ubi volume: {}".format(vol))
                    itemstr += nftl_volumes[vol]["size"]
                    if "offset" in nftl_volumes[vol]:
                        itemstr += "@{}".format(nftl_volumes[vol]["offset"])
                    itemstr += "({})".format(vol)
                    nftl += itemstr + ","
                nftl = nftl[0:-1] + ";"

            if media_type == "spi-nor":
                fal_cfg += "    {}FAL_PART_MAGIC_WORD, \"{}\",".format("{", part)
                fal_cfg += "FAL_USING_NOR_FLASH_DEV_NAME, "
                fal_cfg += "{},{},0{}, \\\n".format(part_offs, part_size, "}")

        mtd = mtd[0:-1]
        part_str = "#define IMAGE_CFG_JSON_PARTS_MTD \"{}\"\n".format(mtd)
        if len(ubi) > 0:
            ubi = ubi[0:-1]
            part_str += "#define IMAGE_CFG_JSON_PARTS_UBI \"{}\"\n".format(ubi)
        if len(nftl) > 0:
            nftl = nftl[0:-1]
            part_str += "#define IMAGE_CFG_JSON_PARTS_NFTL \"{}\"\n".format(nftl)

        if media_type == "spi-nor":
            fal_cfg += "}\n#endif\n"
            part_str += fal_cfg
    elif media_type == "mmc":
        partitions = cfg[media_type]["partitions"]
        if len(partitions) == 0:
            print("Partition table is empty")
            sys.exit(1)
        for part in partitions:
            itemstr = ""
            if "size" not in partitions[part]:
                print("No size value for partition: {}".format(part))
            itemstr += partitions[part]["size"]
            if "offset" in partitions[part]:
                itemstr += "@{}".format(partitions[part]["offset"])
            itemstr += "({})".format(part)
            gpt += itemstr + ","
        gpt = gpt[0:-1]
        part_str = "#define IMAGE_CFG_JSON_PARTS_GPT \"{}\"\n".format(gpt)
        # parts_mmc will be deleted later, keep it just for old version AiBurn tool
    else:
        print("Not supported media type: {}".format(media_type))
        sys.exit(1)

    return part_str


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--config", type=str,
                        help="image configuration file name")
    parser.add_argument("-o", "--outfile", type=str,
                        help="output partition file")
    parser.add_argument("-j", "--json", type=str,
                        help="output partition json file")
    args = parser.parse_args()
    if args.config == None:
        print('Error, option --config is required.')
        sys.exit(1)

    cfg = parse_image_cfg(args.config)
    parts = aic_create_parts_json(cfg)
    if args.json == None:
        print(parts)
    else:
        with open(args.json, "w+") as f:
            f.write(parts)

    parts = aic_create_parts_string(cfg)
    if args.outfile is None:
        print(parts)
    else:
        print("args.outfile: ", args.outfile)
        with open(args.outfile, "w+") as f:
            f.write("/* This is an auto generated file, please don't modify it. */\n\n")
            f.write("#ifndef _AIC_IMAGE_CFG_JSON_PARTITION_TABLE_H_\n")
            f.write("#define _AIC_IMAGE_CFG_JSON_PARTITION_TABLE_H_\n\n")
            f.write(parts)
            f.write("\n#endif\n")


