import os
import sys

# Luban-Lite root directory
AIC_ROOT = os.path.normpath(os.getcwd())

# luban-lite custom scripts
aic_script_path = os.path.join(AIC_ROOT, 'tools/scripts/')
sys.path.append(aic_script_path)
from aic_build import *
chk_prj_config(AIC_ROOT)
PRJ_CHIP,PRJ_BOARD,PRJ_KERNEL,PRJ_APP,PRJ_DEFCONFIG_NAME,PRJ_CUSTOM_LDS,MKIMAGE_POST_ACTION = get_prj_config(AIC_ROOT)
PRJ_NAME = PRJ_DEFCONFIG_NAME.replace('_defconfig','')
PRJ_OUT_DIR = 'output/' + PRJ_NAME + '/images/'
AIC_SCRIPT_DIR = aic_script_path
AIC_COMMON_DIR = os.path.join(AIC_ROOT, 'bsp/artinchip/sys/' + PRJ_CHIP)
AIC_PACK_DIR = os.path.join(AIC_ROOT, 'target/' + PRJ_CHIP + '/' + PRJ_BOARD + '/pack/')

# Var tranfer to SConscript
Export('AIC_ROOT')
Export('AIC_SCRIPT_DIR')
Export('AIC_COMMON_DIR')
Export('AIC_PACK_DIR')
Export('PRJ_CHIP')
Export('PRJ_BOARD')
Export('PRJ_KERNEL')
Export('PRJ_APP')
Export('PRJ_NAME')
Export('PRJ_DEFCONFIG_NAME')
Export('PRJ_OUT_DIR')
# Var tranfer to Kconfig 'option env=xxx'
os.environ["AIC_ROOT"]           = AIC_ROOT
os.environ["AIC_SCRIPT_DIR"]     = AIC_SCRIPT_DIR
os.environ["AIC_COMMON_DIR"]     = AIC_COMMON_DIR
os.environ["AIC_PACK_DIR"]       = AIC_PACK_DIR
os.environ["PRJ_CHIP"]           = PRJ_CHIP
os.environ["PRJ_BOARD"]          = PRJ_BOARD
os.environ["PRJ_KERNEL"]         = PRJ_KERNEL
os.environ["PRJ_APP"]            = PRJ_APP
os.environ["PRJ_NAME"]           = PRJ_NAME
os.environ["PRJ_DEFCONFIG_NAME"] = PRJ_DEFCONFIG_NAME
os.environ["PRJ_OUT_DIR"]        = PRJ_OUT_DIR

# rtconfig
chip_path = os.path.join(AIC_ROOT, 'bsp/artinchip/sys/' + PRJ_CHIP)
sys.path.append(chip_path)
import rtconfig

# RTT_ROOT
if os.getenv('RTT_ROOT'):
    RTT_ROOT = os.getenv('RTT_ROOT')
else:
    RTT_ROOT = os.path.join(AIC_ROOT, 'kernel/rt-thread/')
os.environ["RTT_ROOT"]           = RTT_ROOT
sys.path.append(os.path.join(RTT_ROOT, 'tools'))
from building import *

# ENV_ROOT
if os.getenv('ENV_ROOT') is None:
    ENV_ROOT = RTT_ROOT + '/../../tools/env'
    os.environ["ENV_ROOT"] =  ENV_ROOT

# TARGET
TARGET = PRJ_OUT_DIR + rtconfig.SOC + '.' + rtconfig.TARGET_EXT

# check QEMU
qemu_en = get_config('.config', 'CONFIG_QEMU_RUN')
if qemu_en:
    ld = os.path.join(chip_path, 'link_script', rtconfig.QEMU_LD_SCRIPT)
else:
    if not PRJ_CUSTOM_LDS:
        ld  = os.path.join(chip_path, 'link_script', rtconfig.LD_SCRIPT)
    else:
        ld  = os.path.join(AIC_ROOT, PRJ_CUSTOM_LDS)
    # Compile xxx.ld.S to xxxx.ld
    lds = ld + '.S'
    abs_gcc = os.path.join(rtconfig.EXEC_PATH, rtconfig.CC)
    cmd = abs_gcc + ' -E -P -<' + lds + ' > ' + ld
    os.system(cmd)
rtconfig.LFLAGS += ' -T ' + ld

# add post action
rtconfig.POST_ACTION += MKIMAGE_POST_ACTION

# create env
env  = Environment(tools = ['mingw'],
AS   = rtconfig.AS,   ASFLAGS   = rtconfig.AFLAGS,
CC   = rtconfig.CC,   CFLAGS   = rtconfig.CFLAGS,
CXX  = rtconfig.CXX,  CXXFLAGS  = rtconfig.CXXFLAGS,
AR   = rtconfig.AR,   ARFLAGS   = '-rc',
LINK = rtconfig.LINK, LINKFLAGS = rtconfig.LFLAGS,
RANLIBCOM = '')
env.PrependENVPath('PATH', rtconfig.EXEC_PATH)

# add --start-group and --end-group for GNU GCC
if sys.platform != "win32":
    env['LINKCOM'] = '$LINK -o $TARGET $LINKFLAGS $__RPATH $SOURCES $_LIBDIRFLAGS -Wl,--start-group $_LIBFLAGS -Wl,--end-group'
else:
    env['LINKCOM'] = "${TEMPFILE('$LINK -o $TARGET $LINKFLAGS $__RPATH $SOURCES $_LIBDIRFLAGS -Wl,--start-group $_LIBFLAGS -Wl,--end-group','$LINKCOMSTR')}"
env['ASCOM'] = env['ASPPCOM']

# signature database
env.SConsignFile(PRJ_OUT_DIR + ".sconsign.dblite")

Export('RTT_ROOT')
Export('rtconfig')

if sys.platform == "win32":
    import re
    from SCons.Subst import quote_spaces

    WINPATHSEP_RE = re.compile(r"\\([^\"'\\]|$)")

    def tempfile_arg_esc_func(arg):
        arg = quote_spaces(arg)
        if sys.platform != "win32":
            return arg
        # GCC requires double Windows slashes, let's use UNIX separator
        return WINPATHSEP_RE.sub(r"/\1", arg)

    env["TEMPFILEARGESCFUNC"]    = tempfile_arg_esc_func

# Var tranfer to building.py
env['AIC_ROOT']                  = AIC_ROOT
env['AIC_SCRIPT_DIR']            = AIC_SCRIPT_DIR
env['AIC_COMMON_DIR']            = AIC_COMMON_DIR
env['AIC_PACK_DIR']              = AIC_PACK_DIR
env['PRJ_CHIP']                  = PRJ_CHIP
env['PRJ_BOARD']                 = PRJ_BOARD
env['PRJ_KERNEL']                = PRJ_KERNEL
env['PRJ_NAME']                  = PRJ_NAME
env['PRJ_APP']                   = PRJ_APP
env['PRJ_DEFCONFIG_NAME']        = PRJ_DEFCONFIG_NAME
env['PRJ_OUT_DIR']               = PRJ_OUT_DIR

# prepare building environment
objs = PrepareBuilding(env, RTT_ROOT, has_libcpu=False)

# make a building
DoBuilding(TARGET, objs)
