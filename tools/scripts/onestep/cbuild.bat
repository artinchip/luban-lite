@echo off
rem SPDX-License-Identifier: GPL-2.0+
rem
rem Copyright (C) 2023 ArtInChip Technology Co., Ltd

if exist %SDK_PRJ_TOP_DIR%\.config (
	set /P defconfig=<%SDK_PRJ_TOP_DIR%\.defconfig
	set build_dir=%SDK_PRJ_TOP_DIR%\output\%defconfig%
	if exist %build_dir% cd /d %build_dir%
) else (
	echo Not lunch project yet
)
