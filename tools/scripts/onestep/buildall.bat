@echo off
rem SPDX-License-Identifier: GPL-2.0+
rem
rem Copyright (C) 2023 ArtInChip Technology Co., Ltd

setlocal EnableDelayedExpansion

set NEED_CLEAN=%1

set LOG_DIR=%SDK_PRJ_TOP_DIR%\.log
if not exist %LOG_DIR% mkdir %LOG_DIR%
del %LOG_DIR%\*.log /f /q

rem First, build all the solution one by one

for /f %%i in ('dir /b "%SDK_PRJ_TOP_DIR%\target\configs\*_defconfig"') do (
	@echo off
	@set SOLUTION=%%i
	@set SOLUTION=!SOLUTION:~0,-10!
	@echo.
	@echo --------------------------------------------------------------
	@echo Build !SOLUTION!
	@echo --------------------------------------------------------------
	call scons --apply-def=%%i -C %SDK_PRJ_TOP_DIR%

	if "%NEED_CLEAN%" == "clean" call scons -c -C %SDK_PRJ_TOP_DIR%

	echo.
	call scons -C %SDK_PRJ_TOP_DIR% 2>&1 | tee %LOG_DIR%\!SOLUTION!.log
)

rem Second, parse the build log one by one and show the result

del %LOG_DIR%\warning.* /f /q

@echo.
@echo --------------------------------------------------------------
@echo The build result of all solution:
@echo --------------------------------------------------------------

@set /a CNT=0
for /f %%i in ('dir /b "%SDK_PRJ_TOP_DIR%\target\configs\*_defconfig"') do (
	@echo off
	@set /a CNT+=1

	@set CNT_FMT="  !CNT!"
	@set CNT_FMT=!CNT_FMT:~-3!
	@set CNT_FMT=!CNT_FMT:~0,2!

	@set SOLUTION=%%i
	@set SOLUTION=!SOLUTION:~0,-10!
	@set SOLUTION_FMT="!SOLUTION!                                   "
	@set SOLUTION_FMT=!SOLUTION_FMT:~1,40!

	find "Luban-Lite is built successfully" %LOG_DIR%\!SOLUTION!.log > nul
	if !errorlevel! equ 0 (
		rem Scan the warning information in log
		find "warning: " %LOG_DIR%\!SOLUTION!.log | find "warning: " /i | find "is shorter than expected" /v /c > %LOG_DIR%\warning.tmp
		for /F %%j in ('type %LOG_DIR%\warning.tmp') do set war_cnt=%%j

		echo !CNT_FMT!. !SOLUTION_FMT! is OK. Warning: !war_cnt!
		if not "!war_cnt!" == "0" (
			echo. >> %LOG_DIR%\warning.log
			echo [!SOLUTION!]: >> %LOG_DIR%\warning.log
			find "warning: " %LOG_DIR%\!SOLUTION!.log | find "warning: " /i | find "is shorter than expected" /v >> %LOG_DIR%\warning.log
		)
	) else (
		echo !CNT_FMT!. !SOLUTION_FMT! is failed!
	)
)

if exist %LOG_DIR%\warning.log (
	@echo.
	@echo --------------------------------------------------------------
	@echo The warning information of all solution:
	@echo --------------------------------------------------------------
	type %LOG_DIR%\warning.log
)
