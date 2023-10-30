@echo off
rem SPDX-License-Identifier: GPL-2.0+
rem
rem Copyright (C) 2023 ArtInChip Technology Co., Ltd

scons --menuconfig -C %SDK_PRJ_TOP_DIR% && scons --save-def -C %SDK_PRJ_TOP_DIR%
