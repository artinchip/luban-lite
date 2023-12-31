#!/usr/bin/env python3
# -*- coding:utf-8 -*-
# SPDX-License-Identifier: GPL-2.0+
#
# Copyright (C) 2021 ArtInChip Technology Co., Ltd
# Dehuang Wu <dehuang.wu@artinchip.com>

import os, sys, subprocess, math, re, zlib, json, struct, argparse
from collections import namedtuple
from collections import OrderedDict
from Cryptodome.PublicKey import RSA
from Cryptodome.Hash import MD5
from Cryptodome.Hash import SHA256
from Cryptodome.Cipher import AES
from Cryptodome.Signature import PKCS1_v1_5

DATA_ALIGNED_SIZE = 2048
META_ALIGNED_SIZE = 512
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

def get_file_path(path, alternate_dir):
    if os.path.exists(path):
        return path
    if os.path.exists(alternate_dir + path):
        return alternate_dir + path
    return None

def aic_boot_get_resource_file_size(cfg, keydir, datadir):
    """ Get size of all resource files
    """
    files = {}
    filepath = ""
    if "resource" in cfg:
        if "private" in cfg["resource"]:
            filepath = get_file_path(cfg["resource"]["private"], keydir)
            if filepath == None:
                filepath = get_file_path(cfg["resource"]["private"], datadir)
            if filepath == None:
                print("Error, {} is not found.".format(cfg["resource"]["private"]))
                sys.exit(1)
            statinfo = os.stat(filepath)
            files["resource/private"] = statinfo.st_size
            files["round(resource/private)"] = round_up(statinfo.st_size, 4)

        if "pubkey" in cfg["resource"]:
            filepath = get_file_path(cfg["resource"]["pubkey"], keydir)
            if filepath == None:
                filepath = get_file_path(cfg["resource"]["pubkey"], datadir)
            if filepath == None:
                print("Error, {} is not found.".format(cfg["resource"]["pubkey"]))
                sys.exit(1)
            statinfo = os.stat(filepath)
            files["resource/pubkey"] = statinfo.st_size
            files["round(resource/pubkey)"] = round_up(statinfo.st_size, 4)
        if "pbp" in cfg["resource"]:
            filepath = get_file_path(cfg["resource"]["pbp"], datadir)
            if filepath == None:
                print("Error, {} is not found.".format(cfg["resource"]["pbp"]))
                sys.exit(1)
            statinfo = os.stat(filepath)
            files["resource/pbp"] = statinfo.st_size
            files["round(resource/pbp)"] = round_up(statinfo.st_size, 16)
    if "encryption" in cfg:
        if "iv" in cfg["encryption"]:
            filepath = get_file_path(cfg["encryption"]["iv"], keydir)
            if filepath == None:
                filepath = get_file_path(cfg["encryption"]["iv"], datadir)
            if filepath == None:
                print("Error, {} is not found.".format(cfg["encryption"]["iv"]))
                sys.exit(1)
            statinfo = os.stat(filepath)
            files["encryption/iv"] = statinfo.st_size
            files["round(encryption/iv)"] = round_up(statinfo.st_size, 4)
    if "loader" in cfg:
        if "file" in cfg["loader"]:
            filepath = get_file_path(cfg["loader"]["file"], datadir)
            if filepath != None:
                statinfo = os.stat(filepath)
                if statinfo.st_size > (4 * 1024 * 1024):
                    print("Loader size is too large")
                    sys.exit(1)
                files["loader/file"] = statinfo.st_size
                files["round(loader/file)"] = round_up(statinfo.st_size, 256)
            else:
                print("File {} is not exist".format(cfg["loader"]["file"]))
                sys.exit(1)
    return files

def aic_boot_calc_image_length(filesizes, sign):
    """ Calculate the boot image's total length
    """
    total_siz = filesizes["resource_start"]
    if "resource/pubkey" in filesizes:
        total_siz = total_siz + filesizes["round(resource/pubkey)"]
    if "encryption/iv" in filesizes:
        total_siz = total_siz + filesizes["round(encryption/iv)"]
    if "resource/private" in filesizes:
        total_siz = total_siz + filesizes["round(resource/private)"]
    if "resource/pbp" in filesizes:
        total_siz = total_siz + filesizes["round(resource/pbp)"]
    total_siz = round_up(total_siz, 256)
    if sign:
        # Add the length of signature
        total_siz = total_siz + 256
    else:
        # Add the length of md5
        total_siz = total_siz + 16
    return total_siz

def aic_boot_calc_image_length_for_ext(filesizes, sign):
    """ Calculate the boot image's total length
    """
    total_siz = filesizes["resource_start"]
    if "resource/pubkey" in filesizes:
        total_siz = total_siz + filesizes["round(resource/pubkey)"]
    if "resource/private" in filesizes:
        total_siz = total_siz + filesizes["round(resource/private)"]
    total_siz = round_up(total_siz, 256)
    if sign:
        # Add the length of signature
        total_siz = total_siz + 256
    else:
        # Add the length of md5
        total_siz = total_siz + 16
    return total_siz

def check_loader_run_in_dram(cfg):
    if "loader" not in cfg:
        return False
    if "run in dram" in cfg["loader"]:
        if cfg["loader"]["run in dram"].upper() == "FALSE":
            return False
    return True

def aic_boot_get_loader_bytes(cfg, filesizes):
    """ Read the loader's binaray data, and perform encryption if it is needed.
    """

    loader_size = 0
    header_size = 256
    rawbytes = bytearray(0)
    if check_loader_run_in_dram(cfg):
        # No loader in first aicimg
        # Record the information to generate header and resource bytes
        filesizes["resource_start"] = header_size + loader_size
        return rawbytes

    if "round(loader/file)" in filesizes:
        loader_size = filesizes["round(loader/file)"]
        try:
            fpath = get_file_path(cfg["loader"]["file"], cfg["datadir"])
            with open(fpath, "rb") as f:
                rawbytes = f.read(loader_size)
        except IOError:
            print("Failed to open loader file: {}".format(fpath))
            sys.exit(1)

        if len(rawbytes) == 0:
            print("Read loader data failed.")
            sys.exit(1)
        if len(rawbytes) < loader_size:
            rawbytes = rawbytes + bytearray(loader_size - len(rawbytes))

    # Record the information to generate header and resource bytes
    filesizes["resource_start"] = header_size + loader_size

    if "encryption" in cfg and loader_size > 0:
        # Only encrypt loader content, if loader not exist, don't do it
        try:
            fpath = get_file_path(cfg["encryption"]["key"], cfg["keydir"])
            if fpath == None:
                fpath = get_file_path(cfg["encryption"]["key"], cfg["datadir"])
            with open(fpath, "rb") as f:
                keydata = f.read(16)
        except IOError:
            print('Failed to open aes key file')
            sys.exit(1)
        try:
            fpath = get_file_path(cfg["encryption"]["iv"], cfg["keydir"])
            if fpath == None:
                fpath = get_file_path(cfg["encryption"]["iv"], cfg["datadir"])
            with open(fpath, "rb") as f:
                ivdata = f.read(16)
        except IOError:
            print('Failed to open iv file')
            sys.exit(1)
        cipher = AES.new(keydata, AES.MODE_CBC, ivdata)
        enc_bytes = cipher.encrypt(rawbytes)
        return enc_bytes
    else:
        return rawbytes

def aic_boot_get_loader_for_ext(cfg, filesizes):
    """ Read the loader's binaray data, and perform encryption if it is needed.
    """

    loader_size = 0
    rawbytes = bytearray(0)
    if "round(loader/file)" in filesizes:
        loader_size = filesizes["round(loader/file)"]
        try:
            fpath = get_file_path(cfg["loader"]["file"], cfg["datadir"])
            with open(fpath, "rb") as f:
                rawbytes = f.read(loader_size)
        except IOError:
            print("Failed to open loader file: {}".format(fpath))
            sys.exit(1)

        if len(rawbytes) == 0:
            print("Read loader data failed.")
            sys.exit(1)
        if len(rawbytes) < loader_size:
            rawbytes = rawbytes + bytearray(loader_size - len(rawbytes))

    header_size = 256
    # Record the information to generate header and resource bytes
    filesizes["resource_start"] = header_size + loader_size

    return rawbytes

def aic_boot_get_resource_bytes(cfg, filesizes):
    """ Pack all resource data into boot image's resource section
    """
    resbytes = bytearray(0)
    if "resource/private" in filesizes:
        priv_size = filesizes["round(resource/private)"]
        try:
            fpath = get_file_path(cfg["resource"]["private"], cfg["datadir"])
            with open(fpath, "rb") as f:
                privdata = f.read(priv_size)
        except IOError:
            print('Failed to open private file')
            sys.exit(1)
        resbytes = resbytes + privdata + bytearray(priv_size - len(privdata))
    if "resource/pubkey" in filesizes:
        pubkey_size = filesizes["round(resource/pubkey)"]
        try:
            fpath = get_file_path(cfg["resource"]["pubkey"], cfg["keydir"])
            if fpath == None:
                fpath = get_file_path(cfg["resource"]["pubkey"], cfg["datadir"])
            with open(fpath, "rb") as f:
                pkdata = f.read(pubkey_size)
        except IOError:
            print('Failed to open pubkey file')
            sys.exit(1)
        # Add padding to make it alignment
        resbytes = resbytes + pkdata + bytearray(pubkey_size - len(pkdata))
    if "encryption/iv" in filesizes:
        iv_size = filesizes["round(encryption/iv)"]
        try:
            fpath = get_file_path(cfg["encryption"]["iv"], cfg["keydir"])
            if fpath == None:
                fpath = get_file_path(cfg["encryption"]["iv"], cfg["datadir"])
            with open(fpath, "rb") as f:
                ivdata = f.read(iv_size)
        except IOError:
            print('Failed to open iv file')
            sys.exit(1)
        resbytes = resbytes + ivdata + bytearray(iv_size - len(ivdata))
    if "resource/pbp" in filesizes:
        pbp_size = filesizes["round(resource/pbp)"]
        try:
            fpath = get_file_path(cfg["resource"]["pbp"], cfg["datadir"])
            with open(fpath, "rb") as f:
                pbp_data = f.read(pbp_size)
        except IOError:
            print('Failed to open pbp file')
            sys.exit(1)
        resbytes = resbytes + pbp_data + bytearray(pbp_size - len(pbp_data))
    if len(resbytes) > 0:
        res_size = round_up(len(resbytes), 256)
        if len(resbytes) != res_size:
            resbytes = resbytes + bytearray(res_size - len(resbytes))
    return resbytes

def aic_boot_get_resource_for_ext(cfg, filesizes):
    """ Pack all resource data into boot image's resource section
    """
    resbytes = bytearray(0)
    if "resource/private" in filesizes:
        priv_size = filesizes["round(resource/private)"]
        try:
            fpath = get_file_path(cfg["resource"]["private"], cfg["datadir"])
            with open(fpath, "rb") as f:
                privdata = f.read(priv_size)
        except IOError:
            print('Failed to open private file')
            sys.exit(1)
        resbytes = resbytes + privdata + bytearray(priv_size - len(privdata))
    if "resource/pubkey" in filesizes:
        pubkey_size = filesizes["round(resource/pubkey)"]
        try:
            fpath = get_file_path(cfg["resource"]["pubkey"], cfg["keydir"])
            if fpath == None:
                fpath = get_file_path(cfg["resource"]["pubkey"], cfg["datadir"])
            with open(fpath, "rb") as f:
                pkdata = f.read(pubkey_size)
        except IOError:
            print('Failed to open pubkey file')
            sys.exit(1)
        # Add padding to make it alignment
        resbytes = resbytes + pkdata + bytearray(pubkey_size - len(pkdata))
    if len(resbytes) > 0:
        res_size = round_up(len(resbytes), 256)
        if len(resbytes) != res_size:
            resbytes = resbytes + bytearray(res_size - len(resbytes))
    return resbytes

def aic_boot_checksum(bootimg):
    length = len(bootimg)
    offset = 0
    total = 0
    while offset < length:
        val = int.from_bytes(bootimg[offset: offset + 4], byteorder='little', signed=False)
        total = total + val
        offset = offset + 4
    return (~total) & 0xFFFFFFFF

def aic_boot_add_header(h, n):
    return h + n.to_bytes(4, byteorder='little', signed=False)

def aic_boot_gen_header_bytes(cfg, filesizes):
    """ Generate header bytes
    """
    # Prepare header information
    magic = "AIC "
    checksum = 0
    header_ver = int("0x00010001", 16)
    if "head_ver" in cfg:
        header_ver = int(cfg["head_ver"], 16)

    img_len = aic_boot_calc_image_length(filesizes, "signature" in cfg)
    fw_ver = 0
    if "anti-rollback counter" in cfg:
        fw_ver = cfg["anti-rollback counter"]

    loader_length = 0
    if "loader/file" in filesizes:
        loader_length = filesizes["loader/file"]

    loader_ext_offset = 0
    if check_loader_run_in_dram(cfg):
        loader_length = 0
        loader_ext_offset = img_len
        # ensure ext loader start position is aligned to 512
        loader_ext_offset = round_up(img_len, META_ALIGNED_SIZE)

    load_address = 0
    entry_point = 0
    if "loader" in cfg:
        load_address = int(cfg["loader"]["load address"], 16)
        entry_point = int(cfg["loader"]["entry point"], 16)
    sign_algo = 0
    sign_offset = 0
    sign_length = 0
    sign_key_offset = 0
    sign_key_length = 0
    next_res_offset = filesizes["resource_start"]
    priv_data_offset = 0
    priv_data_length = 0
    if "resource" in cfg and "private" in cfg["resource"]:
        priv_data_offset = next_res_offset
        priv_data_length = filesizes["resource/private"]
        next_res_offset = priv_data_offset + filesizes["round(resource/private)"]
    if "signature" in cfg and cfg["signature"]["algo"] == "rsa,2048":
        sign_algo = 1
        sign_length = 256
        sign_offset = img_len - sign_length
    else:
        # Append md5 result to the end
        sign_algo = 0
        sign_length = 16
        sign_offset = img_len - sign_length

    if "resource" in cfg and "pubkey" in cfg["resource"]:
        sign_key_offset = next_res_offset
        # Set the length value equal to real size
        sign_key_length = filesizes["resource/pubkey"]
        # Calculate offset use the size after alignment
        next_res_offset = sign_key_offset + filesizes["round(resource/pubkey)"]
    enc_algo = 0
    iv_data_offset = 0
    iv_data_length = 0
    if "encryption" in cfg and cfg["encryption"]["algo"] == "aes-128-cbc":
        enc_algo = 1
        iv_data_offset = next_res_offset
        iv_data_length = 16
        next_res_offset = iv_data_offset + filesizes["round(encryption/iv)"]
    pbp_data_offset = 0
    pbp_data_length = 0
    if "resource" in cfg and "pbp" in cfg["resource"]:
        pbp_data_offset = next_res_offset
        pbp_data_length = filesizes["resource/pbp"]
        next_res_offset = pbp_data_offset + filesizes["round(resource/pbp)"]
    # Generate header bytes
    header_bytes = magic.encode(encoding="utf-8")
    header_bytes = aic_boot_add_header(header_bytes, checksum)
    header_bytes = aic_boot_add_header(header_bytes, header_ver)
    header_bytes = aic_boot_add_header(header_bytes, img_len)
    header_bytes = aic_boot_add_header(header_bytes, fw_ver)
    header_bytes = aic_boot_add_header(header_bytes, loader_length)
    header_bytes = aic_boot_add_header(header_bytes, load_address)
    header_bytes = aic_boot_add_header(header_bytes, entry_point)
    header_bytes = aic_boot_add_header(header_bytes, sign_algo)
    header_bytes = aic_boot_add_header(header_bytes, enc_algo)
    header_bytes = aic_boot_add_header(header_bytes, sign_offset)
    header_bytes = aic_boot_add_header(header_bytes, sign_length)
    header_bytes = aic_boot_add_header(header_bytes, sign_key_offset)
    header_bytes = aic_boot_add_header(header_bytes, sign_key_length)
    header_bytes = aic_boot_add_header(header_bytes, iv_data_offset)
    header_bytes = aic_boot_add_header(header_bytes, iv_data_length)
    header_bytes = aic_boot_add_header(header_bytes, priv_data_offset)
    header_bytes = aic_boot_add_header(header_bytes, priv_data_length)
    header_bytes = aic_boot_add_header(header_bytes, pbp_data_offset)
    header_bytes = aic_boot_add_header(header_bytes, pbp_data_length)
    header_bytes = aic_boot_add_header(header_bytes, loader_ext_offset)
    header_bytes = header_bytes + bytearray(256 - len(header_bytes))
    return header_bytes

def aic_boot_gen_header_for_ext(cfg, filesizes):
    """ Generate header bytes
    """
    # Prepare header information
    magic = "AIC "
    checksum = 0
    header_ver = int("0x00010001", 16)
    if "head_ver" in cfg:
        header_ver = int(cfg["head_ver"], 16)

    img_len = aic_boot_calc_image_length_for_ext(filesizes, "signature" in cfg)
    fw_ver = 0

    loader_length = 0
    if "loader/file" in filesizes:
        loader_length = filesizes["loader/file"]

    loader_ext_offset = 0

    load_address = 0
    entry_point = 0
    if "loader" in cfg:
        load_address = int(cfg["loader"]["load address"], 16)
        entry_point = int(cfg["loader"]["entry point"], 16)
    sign_algo = 0
    sign_offset = 0
    sign_length = 0
    sign_key_offset = 0
    sign_key_length = 0
    next_res_offset = filesizes["resource_start"]
    priv_data_offset = 0
    priv_data_length = 0
    if "resource" in cfg and "private" in cfg["resource"]:
        priv_data_offset = next_res_offset
        priv_data_length = filesizes["resource/private"]
        next_res_offset = priv_data_offset + filesizes["round(resource/private)"]
    if "signature" in cfg and cfg["signature"]["algo"] == "rsa,2048":
        sign_algo = 1
        sign_length = 256
        sign_offset = img_len - sign_length
    else:
        # Append md5 result to the end
        sign_algo = 0
        sign_length = 16
        sign_offset = img_len - sign_length

    if "resource" in cfg and "pubkey" in cfg["resource"]:
        sign_key_offset = next_res_offset
        # Set the length value equal to real size
        sign_key_length = filesizes["resource/pubkey"]
        # Calculate offset use the size after alignment
        next_res_offset = sign_key_offset + filesizes["round(resource/pubkey)"]
    enc_algo = 0
    iv_data_offset = 0
    iv_data_length = 0
    pbp_data_offset = 0
    pbp_data_length = 0
    # Generate header bytes
    header_bytes = magic.encode(encoding="utf-8")
    header_bytes = aic_boot_add_header(header_bytes, checksum)
    header_bytes = aic_boot_add_header(header_bytes, header_ver)
    header_bytes = aic_boot_add_header(header_bytes, img_len)
    header_bytes = aic_boot_add_header(header_bytes, fw_ver)
    header_bytes = aic_boot_add_header(header_bytes, loader_length)
    header_bytes = aic_boot_add_header(header_bytes, load_address)
    header_bytes = aic_boot_add_header(header_bytes, entry_point)
    header_bytes = aic_boot_add_header(header_bytes, sign_algo)
    header_bytes = aic_boot_add_header(header_bytes, enc_algo)
    header_bytes = aic_boot_add_header(header_bytes, sign_offset)
    header_bytes = aic_boot_add_header(header_bytes, sign_length)
    header_bytes = aic_boot_add_header(header_bytes, sign_key_offset)
    header_bytes = aic_boot_add_header(header_bytes, sign_key_length)
    header_bytes = aic_boot_add_header(header_bytes, iv_data_offset)
    header_bytes = aic_boot_add_header(header_bytes, iv_data_length)
    header_bytes = aic_boot_add_header(header_bytes, priv_data_offset)
    header_bytes = aic_boot_add_header(header_bytes, priv_data_length)
    header_bytes = aic_boot_add_header(header_bytes, pbp_data_offset)
    header_bytes = aic_boot_add_header(header_bytes, pbp_data_length)
    header_bytes = aic_boot_add_header(header_bytes, loader_ext_offset)
    header_bytes = header_bytes + bytearray(256 - len(header_bytes))
    return header_bytes

def aic_boot_gen_signature_bytes(cfg, bootimg):
    """ Generate RSASSA-PKCS1-v1.5 Signature with SHA-256
    """
    if "privkey" not in cfg["signature"]:
        print("RSA Private key is not exist.")
        sys.exit(1)
    try:
        fpath = get_file_path(cfg["signature"]["privkey"], cfg["keydir"])
        if fpath == None:
            fpath = get_file_path(cfg["signature"]["privkey"], cfg["datadir"])
        with open(fpath, 'rb') as frsa:
            rsakey = RSA.importKey(frsa.read())
    except IOError:
        print("Failed to open file: " + cfg["signature"]["privkey"])
        sys.exit(1)
    # Check if it is private key
    if rsakey.has_private() == False:
        print("Should to use RSA private key to sign")
        sys.exit(1)
    keysize = max(1, math.ceil(rsakey.n.bit_length() / 8))
    if keysize != 256:
        print("Only RSA 2048 is supported, please input RSA 2048 Private Key.")
        sys.exit(1)
    # Calculate SHA-256 hash
    sha256 = SHA256.new()
    sha256.update(bootimg)
    # Encrypt the hash, and using RSASSA-PKCS1-V1.5 Padding
    signer = PKCS1_v1_5.new(rsakey)
    sign_bytes = signer.sign(sha256)
    return sign_bytes

def aic_boot_gen_img_md5_bytes(cfg, bootimg):
    """ Calculate MD5 of image to make brom verify image faster
    """
    # Calculate MD5 hash
    md5 = MD5.new()
    md5.update(bootimg)
    md5_bytes = md5.digest()
    return md5_bytes

def aic_boot_check_params(cfg):
    if "encryption" in cfg and cfg["encryption"]["algo"] != "aes-128-cbc":
        print("Only support aes-128-cbc encryption")
        return False
    if "signature" in cfg and cfg["signature"]["algo"] != "rsa,2048":
        print("Only support rsa,2048 signature")
        return False
    # if "loader" not in cfg or "load address" not in cfg["loader"]:
    #     print("load address is not set")
    #     return False
    # if "loader" not in cfg or "entry point" not in cfg["loader"]:
    #     print("entry point is not set")
    #     return False
    return True

def aic_boot_create_image(cfg, keydir, datadir):
    """ Create AIC format Boot Image for Boot ROM
    """
    if aic_boot_check_params(cfg) == False:
        sys.exit(1)
    filesizes = aic_boot_get_resource_file_size(cfg, keydir, datadir)

    loader_bytes = aic_boot_get_loader_bytes(cfg, filesizes)
    resource_bytes = bytearray(0)
    if "resource" in cfg or "encryption" in cfg:
        resource_bytes = aic_boot_get_resource_bytes(cfg, filesizes)
    header_bytes = aic_boot_gen_header_bytes(cfg, filesizes)
    bootimg = header_bytes + loader_bytes + resource_bytes

    head_ver = int("0x00010001", 16)
    if "head_ver" in cfg:
        head_ver = int(cfg["head_ver"], 16)
    if "signature" in cfg:
        signature_bytes = aic_boot_gen_signature_bytes(cfg, bootimg)
        bootimg = bootimg + signature_bytes
        return bootimg

    # Secure boot is not enabled, always add md5 result to the end
    md5_bytes = aic_boot_gen_img_md5_bytes(cfg, bootimg[8:])
    bootimg = bootimg + md5_bytes
    # Calculate checksum.
    # When MD5 is disabled, checksum will be checked by BROM.
    cs = aic_boot_checksum(bootimg)
    cs_bytes = cs.to_bytes(4, byteorder='little', signed=False)
    bootimg = bootimg[0:4] + cs_bytes + bootimg[8:]
    # Verify the checksum value
    cs = aic_boot_checksum(bootimg)
    if cs != 0:
        print("Checksum is error: {}".format(cs))
        sys.exit(1)
    return bootimg

def aic_boot_create_ext_image(cfg, keydir, datadir):
    """ Create AIC format Boot Image for Boot ROM
    """

    filesizes = aic_boot_get_resource_file_size(cfg, keydir, datadir)
    loader_bytes = aic_boot_get_loader_for_ext(cfg, filesizes)
    resource_bytes = bytearray(0)
    if "resource" in cfg:
        resource_bytes = aic_boot_get_resource_for_ext(cfg, filesizes)
    header_bytes = aic_boot_gen_header_for_ext(cfg, filesizes)
    bootimg = header_bytes + loader_bytes + resource_bytes

    head_ver = int("0x00010001", 16)
    if "head_ver" in cfg:
        head_ver = int(cfg["head_ver"], 16)
    if "signature" in cfg:
        signature_bytes = aic_boot_gen_signature_bytes(cfg, bootimg)
        bootimg = bootimg + signature_bytes
        return bootimg

    # Secure boot is not enabled, always add md5 result to the end
    md5_bytes = aic_boot_gen_img_md5_bytes(cfg, bootimg[8:])
    bootimg = bootimg + md5_bytes
    # Calculate checksum.
    # When MD5 is disabled, checksum will be checked by BROM.
    cs = aic_boot_checksum(bootimg)
    cs_bytes = cs.to_bytes(4, byteorder='little', signed=False)
    bootimg = bootimg[0:4] + cs_bytes + bootimg[8:]
    # Verify the checksum value
    cs = aic_boot_checksum(bootimg)
    if cs != 0:
        print("Checksum is error: {}".format(cs))
        sys.exit(1)
    return bootimg

def itb_create_image(itsname, itbname, keydir, dtbname, script_dir):
    mkcmd = os.path.join(script_dir, "mkimage")
    if os.path.exists(mkcmd) == False:
        mkcmd = "mkimage"
    if sys.platform == "win32":
        mkcmd += ".exe"
    # If the key exists, generate image signature information and write it to the itb file.
    # If the key exists, write the public key to the dtb file.
    if keydir != None and dtbname != None:
        cmd = [mkcmd, "-E", "-f", itsname, "-k", keydir, "-K", dtbname, "-r", itbname]
    else:
        cmd = [mkcmd, "-E", "-f", itsname, itbname]

    ret = subprocess.run(cmd, stdout=subprocess.PIPE)
    if ret.returncode != 0:
        sys.exit(1)

def img_gen_fw_file_name(cfg):
    # Image file name format:
    # <platform>_<product>_v<version>_c<anti-rollback counter>.img
    img_file_name = cfg["image"]["info"]["platform"];
    img_file_name += "_"
    img_file_name += cfg["image"]["info"]["product"];
    img_file_name += "_v"
    img_file_name += cfg["image"]["info"]["version"];
    if "anti-rollback" in cfg["image"]["info"]:
        img_file_name += "_c"
        img_file_name += cfg["image"]["info"]["anti-rollback"];
    img_file_name += ".img"
    return img_file_name.replace(" ", "_")

def calc_crc32(fname, size):
    """Calculate crc32 for a file
    Args:
        fname: file path
    """
    hash = 0
    step = 16 * 1024
    if size > 0:
        step = size

    if os.path.exists(fname) == False:
        return 0

    with open(fname, 'rb') as fp:
        while True:
            s = fp.read(step)
            if not s:
                break
            hash = zlib.crc32(s, hash)
            if size > 0:
                # only need to calc first 'size' byte
                break
    return hash & 0xffffffff

def str_to_nbytes(s, n):
    """ String to n bytes
    """
    ba = bytearray(s, encoding="utf-8")
    nzero = n - len(ba)
    if nzero > 0:
        ba.extend([0] * nzero)
    return bytes(ba)

def int_to_uint32_bytes(n):
    """ Int value to uint32 bytes
    """
    return n.to_bytes(4, byteorder='little', signed=False)

"""
struct artinchip_fw_hdr{
    char magic[8];
    char platform[64];
    char product[64];
    char version[64];
    char media_type[64];
    u32  media_dev_id;
    u8   nand_array_org[64];/* NAND Array Organization */
    u32  meta_offset; /* Meta Area start offset */
    u32  meta_size;   /* Meta Area size */
    u32  file_offset; /* File data Area start offset */
    u32  file_size;   /* File data Area size */
};
"""
def img_write_fw_header(imgfile, cfg, meta_area_size, file_area_size):
    """ Generate Firmware image's header data
    Args:
        cfg: Dict from JSON
        meta_area_size: size of meta data area
        file_area_size: size of file data area
    """
    array_org_len = 64
    nand_array_org = ""
    if "array_organization" in cfg["image"]["info"]["media"]:
        array_orgval = cfg["image"]["info"]["media"]["array_organization"]
        if not isinstance(array_orgval, list):
            print("Error, nand array organization should be a list.")
            return -1
        param_str = ""
        for item in array_orgval:
            param_str += "P={},B={};".format(item["page"].upper(), item["block"].upper())
        param_str = param_str[0:-1]
        nand_array_org = param_str
    dev_id = 0
    if "device_id" in cfg["image"]["info"]["media"]:
        val = cfg["image"]["info"]["media"]["device_id"]
        if isinstance(val, str):
            dev_id = int(val)
        else:
            dev_id = val

    magic = "AIC.FW"
    platform = str(cfg["image"]["info"]["platform"])
    product = str(cfg["image"]["info"]["product"])
    version = str(cfg["image"]["info"]["version"])
    media_type = str(cfg["image"]["info"]["media"]["type"])
    media_dev_id = dev_id
    meta_offset = DATA_ALIGNED_SIZE
    meta_size = meta_area_size
    file_offset = DATA_ALIGNED_SIZE + meta_area_size
    file_size = file_area_size

    buff = str_to_nbytes("AIC.FW", 8)
    buff = buff + str_to_nbytes(platform, 64)
    buff = buff + str_to_nbytes(product, 64)
    buff = buff + str_to_nbytes(version, 64)
    buff = buff + str_to_nbytes(media_type, 64)
    buff = buff + int_to_uint32_bytes(media_dev_id)
    buff = buff + str_to_nbytes(nand_array_org, 64)
    buff = buff + int_to_uint32_bytes(meta_offset)
    buff = buff + int_to_uint32_bytes(meta_size)
    buff = buff + int_to_uint32_bytes(file_offset)
    buff = buff + int_to_uint32_bytes(file_size)
    imgfile.seek(0, 0)
    imgfile.write(buff)
    imgfile.flush()
    if VERBOSE:
        print("\tImage header is generated.")
    return 0

"""
struct artinchip_fwc_meta {
    char magic[8];
    char name[64];
    char partition[64];
    u32  offset;
    u32  size;
    u32  crc;
    u32  ram;
    char attr[64]
};
"""
def img_gen_fwc_meta(name, part, offset, size, crc, ram, attr):
    """ Generate Firmware component's meta data
    Args:
        cfg: Dict from JSON
        datadir: working directory for image data
    """
    buff = str_to_nbytes("META", 8)
    buff = buff + str_to_nbytes(name, 64)
    buff = buff + str_to_nbytes(part, 64)
    buff = buff + int_to_uint32_bytes(offset)
    buff = buff + int_to_uint32_bytes(size)
    buff = buff + int_to_uint32_bytes(crc)
    buff = buff + int_to_uint32_bytes(ram)
    buff = buff + str_to_nbytes(attr, 64)

    if VERBOSE:
        print("\t\tMeta for {:<25} offset {:<10} size {} ({})".format(name,
            hex(offset), hex(size), size))
    return buff

def check_partition_exist(table, partval):
    if isinstance(partval, list):
        for item in partval:
            if item.find(":") > 0: # UBI Volume
                parts = item.split(":")
                part = parts[0]
                vol = parts[1]
                if part not in table:
                    print("{} not in table {}".format(part, table))
                    return False
                if vol not in table[part]["ubi"]:
                    print("{} not in ubi {}".format(vol, table[part]["ubi"]))
                    return False
            else:
                if item not in table:
                    print("{} not in table {}".format(partval, table))
                    return False
    else:
        if partval not in table:
            print("{} not in table {}".format(partval, table))
            return False
    return True

def img_write_fwc_meta_section(imgfile, cfg, sect, meta_off, file_off, datadir):
    fwcset = cfg["image"][sect]
    media_type = cfg["image"]["info"]["media"]["type"]

    if media_type not in cfg:
        print("Cannot find partitions for {}".format(media_type))
        return (-1, -1)
    partitions = cfg[media_type]["partitions"]
    for fwc in fwcset:
        file_size = fwcset[fwc]["filesize"]
        if file_size <= 0:
            continue
        imgfile.seek(meta_off, 0)
        path = str(datadir + fwcset[fwc]["file"])
        crc = calc_crc32(path, 0)
        if "ram" in fwcset[fwc]:
            ram = int(fwcset[fwc]["ram"], 16)
        else:
            ram = 0xFFFFFFFF
        attrval = fwcset[fwc]["attr"]
        if isinstance(attrval, list):
            attr = str(";".join(attrval))
        else:
            attr = str(attrval)
        attr = attr.replace(' ', '')
        name = str("image." + sect + "." + fwc)

        if "part" in fwcset[fwc]:
            partval = fwcset[fwc]["part"]
            if check_partition_exist(partitions, partval) == False:
                print("Partition {} not exist".format(partval))
                return (-1, -1)
            if isinstance(partval, list):
                part = str(";".join(partval))
            else:
                part = str(partval)
        else:
            part = ""
        meta = img_gen_fwc_meta(name, part, file_off, file_size, crc, ram, attr)
        imgfile.write(meta)
        fwcset[fwc]["meta_off"] = meta_off
        fwcset[fwc]["file_off"] = file_off
        # Update for next item
        meta_off += META_ALIGNED_SIZE
        file_size = round_up(file_size, DATA_ALIGNED_SIZE)
        file_off += file_size
    return (meta_off, file_off)

def img_write_fwc_meta_to_imgfile(imgfile, cfg, meta_start, file_start, datadir):
    """ Generate and write FW component's meta data
    Args:
        imgfile: Image file handle
        cfg: Dict from JSON
        meta_start: meta data area start offset
        file_start: file data area start offset
        datadir: working directory
    """
    meta_offset = meta_start
    file_offset = file_start
    if VERBOSE:
        print("\tMeta data for image components:")
    # 1, FWC of updater
    meta_offset, file_offset = img_write_fwc_meta_section(imgfile, cfg, "updater",
                                                      meta_offset, file_offset,
                                                      datadir)
    if meta_offset < 0:
        return -1
    # 2, Image Info(The same with image header)
    imgfile.seek(meta_offset, 0)
    img_fn = datadir + img_gen_fw_file_name(cfg);
    crc = calc_crc32(img_fn, DATA_ALIGNED_SIZE)
    info_offset = 0 # Image info is the image header, start from 0
    info_size = DATA_ALIGNED_SIZE
    part = ""
    name = "image.info"
    ram = 0xFFFFFFFF
    attr = "required"
    meta = img_gen_fwc_meta(name, part, info_offset, info_size, crc, ram, attr)
    imgfile.write(meta)
    # Only meta offset increase
    meta_offset += META_ALIGNED_SIZE
    # 3, FWC of target
    meta_offset, file_offset = img_write_fwc_meta_section(imgfile, cfg, "target",
                                                      meta_offset, file_offset,
                                                      datadir)
    if meta_offset < 0:
        return -1
    imgfile.flush()
    return 0

def img_write_fwc_file_to_imgfile(imgfile, cfg, file_start, datadir):
    """ Write FW component's file data
    Args:
        imgfile: Image file handle
        cfg: Dict from JSON
        file_start: file data area start offset
        datadir: working directory
    """
    file_offset = file_start
    if VERBOSE:
        print("\tPacking file data:")
    for section in ["updater", "target"]:
        fwcset = cfg["image"][section]
        for fwc in fwcset:
            path = get_file_path(fwcset[fwc]["file"], datadir)
            if path == None:
                continue
            if VERBOSE:
                print("\t\t" + os.path.split(path)[1])
            # Read fwc file content, and write to image file
            imgfile.seek(file_offset, 0)
            step = 16 * 1024
            with open(path, "rb") as fwcfile:
                while True:
                    bindata = fwcfile.read(step)
                    if not bindata:
                        break
                    imgfile.write(bindata)
            # Update for next file
            filesize = fwcset[fwc]["filesize"]
            filesize = round_up(filesize, DATA_ALIGNED_SIZE)
            file_offset += filesize
    imgfile.flush()
    return 0

def img_get_fwc_file_size(cfg, datadir):
    """ Scan directory and get Firmware component's file size, update to cfg
    Args:
        cfg: Dict from JSON
        datadir: working directory for image data
    """
    for section in ["updater", "target"]:
        fwcset = cfg["image"][section]
        for fwc in fwcset:
            path = get_file_path(fwcset[fwc]["file"], datadir)
            if path == None:
                attr = fwcset[fwc]["attr"]
                if "required" in attr:
                    print("Error, file {} is not exist".format(fwcset[fwc]["file"]))
                    return -1
                else:
                    # FWC file is not exist, but it is not necessary
                    fwcset[fwc]["filesize"] = 0
                    continue
            statinfo = os.stat(path)
            fwcset[fwc]["filesize"] = statinfo.st_size
    return 0

def round_up(x, y):
    return int((x + y - 1) / y) * y

def aic_create_parts_for_env(cfg):
    mtd = ""
    ubi = ""
    gpt = ""

    part_str = ""
    media_type = cfg["image"]["info"]["media"]["type"]
    if media_type == "spi-nand" or media_type == "spi-nor":
        partitions = cfg[media_type]["partitions"]
        mtd = "spi{}.0:".format(cfg["image"]["info"]["media"]["device_id"])
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
        mtd = mtd[0:-1]
        part_str = "MTD={}\n".format(mtd)
        if len(ubi) > 0:
            ubi = ubi[0:-1]
            part_str += "UBI={}\n".format(ubi)
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
        part_str = "GPT={}\nparts_mmc={}\n".format(gpt, gpt)
        # parts_mmc will be deleted later, keep it just for old version AiBurn tool
    else:
        print("Not supported media type: {}".format(media_type))
        sys.exit(1)

    return part_str

def uboot_env_create_image(srcfile, outfile, size, part_str, redund, script_dir):
    tmpfile = srcfile + ".part.tmp"
    fs = open(srcfile, "r+")
    envstr = fs.readlines()
    fs.close()
    fp = open(tmpfile, "w+")
    fp.write(part_str)
    fp.writelines(envstr)
    fp.close()

    mkenvcmd = os.path.join(script_dir, "mkenvimage")
    if os.path.exists(mkenvcmd) == False:
        mkenvcmd = "mkenvimage"
    if sys.platform == "win32":
        mkenvcmd += ".exe"
    if "enable" in redund:
        cmd = [mkenvcmd, "-r","-s", str(size), "-o", outfile, tmpfile]
    else:
        cmd = [mkenvcmd, "-s", str(size), "-o", outfile, tmpfile]
    ret = subprocess.run(cmd, subprocess.PIPE)
    if ret.returncode != 0:
        sys.exit(1)

def firmware_component_preproc(cfg, datadir, keydir, bindir):
    """ Perform firmware component pre-process
    Args:
        cfg: Dict from JSON
        datadir: working directory for image data
        keydir: key material directory for image data
    """

    preproc_cfg = cfg["temporary"]
    if "itb" in preproc_cfg:
        # Need to generate FIT image
        imgnames = preproc_cfg["itb"].keys()
        for itbname in imgnames:
            itsname = preproc_cfg["itb"][itbname]["its"]
            outfile = datadir + itbname
            dtbfile = None
            keypath = None

            if VERBOSE:
                print("\tCreating {} ...".format(outfile))
            srcfile = get_file_path(itsname, datadir)
            if srcfile == None:
                print("File {} is not exist".format(itsname))
                sys.exit(1)
            if "dtb" in preproc_cfg["itb"][itbname].keys():
                dtbname = preproc_cfg["itb"][itbname]["dtb"]
                dtbfile = get_file_path(dtbname, datadir)
                if dtbfile == None:
                    print("File {} is not exist".format(dtbname))
                    sys.exit(1)
            if "keydir" in preproc_cfg["itb"][itbname].keys():
                keydir = preproc_cfg["itb"][itbname]["keydir"]
                keypath = get_file_path(keydir, datadir)
                if keypath == None:
                    print("File {} is not exist".format(keydir))

            itb_create_image(srcfile, outfile, keypath, dtbfile, bindir)

            # Generate a spl image with spl dtb file
            if "bin" in preproc_cfg["itb"][itbname].keys():
                srcbin = preproc_cfg["itb"][itbname]["bin"]["src"]
                dstbin = preproc_cfg["itb"][itbname]["bin"]["dst"]
                srcfile = get_file_path(srcbin, datadir)
                if srcfile == None:
                    print("File {} is not exist".format(srcbin))
                    sys.exit(1)
                dstfile = get_file_path(dstbin, datadir)
                if dstfile == None:
                    print("File {} is not exist".format(dstbin))
                    sys.exit(1)
                cmd = ["cat {} {} > {}".format(srcfile, dtbfile, dstfile)]
                ret = subprocess.run(cmd, shell=True, stdout=subprocess.PIPE)
                if ret.returncode != 0:
                      sys.exit(1)
    if "uboot_env" in preproc_cfg:
        # Need to generate uboot env bin
        imgnames = preproc_cfg["uboot_env"].keys()
        part_str = aic_create_parts_for_env(cfg)
        envredund = "disable"
        for binname in imgnames:
            envfile = preproc_cfg["uboot_env"][binname]["file"]
            envsize = preproc_cfg["uboot_env"][binname]["size"]
            if "redundant" in preproc_cfg["uboot_env"][binname]:
                envredund = preproc_cfg["uboot_env"][binname]["redundant"]
            outfile = datadir + binname
            if VERBOSE:
                print("\tCreating {} ...".format(outfile))
            srcfile = get_file_path(envfile, datadir)
            if srcfile == None:
                print("File {} is not exist".format(envfile))
                sys.exit(1)
            uboot_env_create_image(srcfile, outfile, envsize, part_str,
                    envredund, bindir)

    if "aicboot" in preproc_cfg:
        # Need to generate aicboot image
        imgnames = preproc_cfg["aicboot"].keys()
        for name in imgnames:
            imgcfg = preproc_cfg["aicboot"][name]
            imgcfg["keydir"] = keydir
            imgcfg["datadir"] = datadir
            outname = datadir + name
            if VERBOSE:
                print("\tCreating {} ...".format(outname))
            imgbytes = aic_boot_create_image(imgcfg, keydir, datadir)

            if check_loader_run_in_dram(imgcfg):
                extimgbytes = aic_boot_create_ext_image(imgcfg, keydir, datadir)
                padlen = round_up(len(imgbytes), META_ALIGNED_SIZE) - len(imgbytes)
                if padlen > 0:
                    imgbytes += bytearray(padlen)
                imgbytes += extimgbytes
                # For Debug
                # with open(outname + ".ext", "wb") as f:
                #     f.write(extimgbytes)

            with open(outname, "wb") as f:
                f.write(imgbytes)


def generate_bootcfg(bcfgfile, cfg):
    comments = ["# Boot configuration file\n",
                "# Used in SD Card FAT32 boot and USB Disk upgrade.\n",
                "# Format:\n",
                "# protection=part1 name,part2 name,part3 name\n",
                "#   Protects partitions from being overwritten when they are upgraded.\n"
                "# boot0=size@offset\n",
                "#   boot0 size and location offset in 'image' file, boot rom read it.\n"
                "# boot0=example.bin\n",
                "#   boot0 image is file example.bin, boot rom read it.\n"
                "# boot1=size@offset\n",
                "#   boot1 size and location offset in 'image' file, boot0 read it.\n"
                "# boot1=example.bin\n",
                "#   boot1 image is file example.bin, boot0 read it.\n"
                "# image=example.img\n",
                "#   Packed image file is example.img, boot1 use it.\n",
                "\n\n",
                ]
    bcfgfile.writelines(comments)

    fwcset = cfg["image"]["updater"]
    fwckeys = cfg["image"]["updater"].keys()
    if "spl" in fwckeys:
        fwcname = "spl"
        linestr = "# {}\n".format(fwcset[fwcname]["file"])
        bcfgfile.write(linestr)
        linestr = "boot0={}@{}\n".format(hex(fwcset[fwcname]["filesize"]),
                                        hex(fwcset[fwcname]["file_off"]))
        bcfgfile.write(linestr)

    if "uboot" in fwckeys:
        fwcname = "uboot"
        linestr = "# {}\n".format(fwcset[fwcname]["file"])
        bcfgfile.write(linestr)
        linestr = "boot1={}@{}\n".format(hex(fwcset[fwcname]["filesize"]),
                                        hex(fwcset[fwcname]["file_off"]))
        bcfgfile.write(linestr)

    if "env" in fwckeys:
        fwcname = "env"
        linestr = "# {}\n".format(fwcset[fwcname]["file"])
        bcfgfile.write(linestr)
        linestr = "env={}@{}\n".format(hex(fwcset[fwcname]["filesize"]),
                                       hex(fwcset[fwcname]["file_off"]))
        bcfgfile.write(linestr)

    imgfn = img_gen_fw_file_name(cfg)
    linestr = "image={}\n".format(imgfn)
    bcfgfile.write(linestr)
    bcfgfile.flush()

def get_spinand_image_list(cfg, datadir):
    imglist = []
    orglist = cfg["image"]["info"]["media"]["array_organization"]
    for item in orglist:
        paramstr = "_page_{}_block_{}".format(item["page"], item["block"])
        paramstr = paramstr.lower()
        status_ok = True
        for fwcname in cfg["image"]["target"]:
            if "ubi" not in cfg["image"]["target"][fwcname]["attr"]:
                # Not UBI partition
                continue
            # UBI component
            filepath = cfg["image"]["target"][fwcname]["file"]
            if filepath.find("*") <= 0:
                # No need to check
                continue
            filepath = filepath.replace("*", paramstr)
            filepath = get_file_path(filepath, datadir)
            if filepath == None and "optional" not in cfg["image"]["target"][fwcname]["attr"]:
                # FWC file not exist
                status_ok = False
                print("{} is not found".format(cfg["image"]["target"][fwcname]["file"]))
                break
            backup = cfg["image"]["target"][fwcname]["file"]
            # Backup the original file path string, because it will be modified
            # when generating image
            cfg["image"]["target"][fwcname]["file.backup"] = backup
        if status_ok:
            imglist.append(paramstr)
    backup = cfg["image"]["info"]["product"]
    cfg["image"]["info"]["product.backup"] = backup
    backup = cfg["image"]["bootcfg"]
    cfg["image"]["bootcfg.backup"] = backup
    return imglist

def fixup_spinand_ubi_fwc_name(cfg, paramstr):
    for fwcname in cfg["image"]["target"]:
        if "ubi" not in cfg["image"]["target"][fwcname]["attr"]:
            # Not UBI partition
            continue
        # UBI component
        filepath = cfg["image"]["target"][fwcname]["file.backup"]
        if filepath.find("*") <= 0:
            # No need to fixup
            continue
        cfg["image"]["target"][fwcname]["file"] = filepath.replace("*", paramstr)
    # fixup others
    backup = cfg["image"]["info"]["product.backup"]
    cfg["image"]["info"]["product"] = backup + paramstr
    backup = cfg["image"]["bootcfg.backup"]
    cfg["image"]["bootcfg"] = "{}({})".format(backup, paramstr[1:])

def build_firmware_image(cfg, datadir):
    """ Build firmware image
    Args:
        cfg: Dict from JSON
        datadir: working directory for image data
    """
    # Step1: Get all FWC file's size
    ret = img_get_fwc_file_size(cfg, datadir)
    if ret != 0:
        return ret

    # Step2: Calculate Meta Area's size, one FWC use DATA_ALIGNED_SIZE bytes
    meta_area_size = 0
    for s in ["updater", "target"]:
        fwcset = cfg["image"][s]
        for fwc in fwcset:
            if fwcset[fwc]["filesize"] > 0:
                meta_area_size += META_ALIGNED_SIZE
    # Image header is also one FWC, it need one FWC Meta
    meta_area_size += META_ALIGNED_SIZE

    # Step3: Calculate the size of FWC File Data Area
    file_area_size = 0
    for s in ["updater", "target"]:
        if s in cfg["image"] == False:
            return -1
        for fwc in cfg["image"][s]:
            if "filesize" in cfg["image"][s][fwc] == False:
                return -1
            filesize = cfg["image"][s][fwc]["filesize"]
            if filesize > 0:
                filesize = round_up(filesize, DATA_ALIGNED_SIZE)
                file_area_size += filesize

    # Step4: Create empty image file
    img_fn = datadir + img_gen_fw_file_name(cfg)
    img_total_size = DATA_ALIGNED_SIZE # Header
    img_total_size += meta_area_size
    img_total_size += file_area_size
    with open(img_fn, 'wb') as imgfile:
        imgfile.truncate(img_total_size)
        # Step5: Write header
        ret = img_write_fw_header(imgfile, cfg, meta_area_size, file_area_size)
        if ret != 0:
            return ret
        # Step6: Write FW Component meta to image
        meta_start = DATA_ALIGNED_SIZE
        file_start = meta_start + meta_area_size
        ret = img_write_fwc_meta_to_imgfile(imgfile, cfg, meta_start,
                                            file_start, datadir)
        if ret != 0:
            return ret
        # Step7: Write FW Component file data to image
        ret = img_write_fwc_file_to_imgfile(imgfile, cfg, file_start, datadir)
        if ret != 0:
            return ret
        imgfile.flush()

    abspath = "{}".format(img_fn)
    (img_path, img_name) = os.path.split(abspath)
    if VERBOSE:
        print("\tImage file is generated: {}/{}\n\n".format(img_path, img_name))

    bootcfg_fn = datadir + cfg["image"]["bootcfg"]
    with open(bootcfg_fn, 'w') as bcfgfile:
        generate_bootcfg(bcfgfile, cfg)
        bcfgfile.flush()
    return 0

if __name__ == "__main__":
    default_bin_root = os.path.dirname(sys.argv[0])
    parser = argparse.ArgumentParser()
    parser.add_argument("-c", "--config", type=str,
                        help="image configuration file name")
    parser.add_argument("-d", "--datadir", type=str,
                        help="input image data directory")
    parser.add_argument("-k", "--keydir", type=str,
                        help="key material directory")
    parser.add_argument("-v", "--verbose", action='store_true',
                        help="show detail information")
    args = parser.parse_args()
    if args.config == None:
        print('Error, option --config is required.')
        sys.exit(1)
    # If user not specified data directory, use current directory as default
    if args.datadir == None:
        args.datadir = './'
    if args.datadir.endswith('/') == False:
        args.datadir = args.datadir + '/'
    if args.keydir == None:
        args.keydir = args.datadir
    if args.keydir.endswith('/') == False:
        args.keydir = args.keydir + '/'
    if args.verbose:
        VERBOSE = True

    cfg = parse_image_cfg(args.config)
    # Pre-process here, e.g: signature, encryption, ...
    if "temporary" in cfg:
        firmware_component_preproc(cfg, args.datadir, args.keydir,
                default_bin_root)

    cfg["image"]["bootcfg"] = "bootcfg.txt"
    # Finally build the firmware image
    imglist = []
    if cfg["image"]["info"]["media"]["type"] == "spi-nand":
        imglist = get_spinand_image_list(cfg, args.datadir)
    if len(imglist) > 0:
        # SPI-NAND UBI case
        for item in imglist:
            # fixup file path
            fixup_spinand_ubi_fwc_name(cfg, item)
            ret = build_firmware_image(cfg, args.datadir)
            if ret != 0:
                sys.exit(1)
    else:
        # Just create image, no need to fixup anything
        ret = build_firmware_image(cfg, args.datadir)
        if ret != 0:
            sys.exit(1)

