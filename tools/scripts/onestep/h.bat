@echo off
rem SPDX-License-Identifier: GPL-2.0+
rem
rem Copyright (C) 2023 ArtInChip Technology Co., Ltd

echo.
echo Luban-Lite SDK OneStep commands:
echo   h             : Get this help.
echo   lunch [No.]   : Start with selected defconfig, .e.g. lunch 3
echo   me            : Config SDK with menuconfig
echo   m             : Build all and generate final image
echo   c             : Clean all
echo   croot/cr      : cd to SDK root directory.
echo   cout/co       : cd to build output directory.
echo   cbuild/cb     : cd to build root directory.
echo   ctarget/ct    : cd to target board directory.
echo   list          : List all SDK defconfig.
echo   i             : Get current project's information.
echo   buildall      : Build all the *defconfig in target/configs
echo   rebuildall    : Clean and build all the *defconfig in target/configs
echo.
