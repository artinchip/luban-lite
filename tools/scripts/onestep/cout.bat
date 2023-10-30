@echo off
rem SPDX-License-Identifier: GPL-2.0+
rem
rem Copyright (C) 2023 ArtInChip Technology Co., Ltd

if exist %SDK_PRJ_TOP_DIR%\.config (
	set /P defconfig=<%SDK_PRJ_TOP_DIR%\.defconfig
	set out_dir=%SDK_PRJ_TOP_DIR%\output\%defconfig%
	if exist %out_dir% cd /d %out_dir% && cd images
) else (
	echo Not lunch project yet
)
