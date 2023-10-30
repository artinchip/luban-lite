import os
import platform

# toolchains options
# CPUNAME = e906/e906f/e906fd/e906p/e906fp/e906fdp
# CPUNAME = e907/e907f/e907fd/e907p/e907fp/e907fdp
SOC         ='d13x'
ARCH        ='risc-v'
CPU         ='e9xx'
CPUNAME     ='e907fdp'
VENDOR      ='artinchip'
CROSS_TOOL  ='gcc'

if os.getenv('RTT_CC'):
    CROSS_TOOL = os.getenv('RTT_CC')

if  CROSS_TOOL == 'gcc':
    PLATFORM    = 'gcc'
    fcwd = os.path.abspath(os.path.dirname(__file__))
    if platform.system() == 'Linux':
        EXEC_PATH = os.path.normpath(fcwd + '/../../../../toolchain/bin/')
    elif platform.system() == 'Windows':
        EXEC_PATH = os.path.normpath(fcwd + '/../../../../toolchain/bin/')
else:
    print ('Please make sure your toolchains is GNU GCC!')
    exit(0)

if os.getenv('RTT_EXEC_PATH'):
    EXEC_PATH = os.getenv('RTT_EXEC_PATH')

# BUILD = 'debug'
BUILD = 'release'
if BUILD == 'debug':
    CFLAGS_DBG = ' -O0 -gdwarf-2'
    AFLAGS_DBG = ' -gdwarf-2'
else:
    CFLAGS_DBG = ' -O2 -g2'
    AFLAGS_DBG = ''

prj_out_dir = ''
if os.environ.get('PRJ_OUT_DIR'):
    prj_out_dir = os.environ.get('PRJ_OUT_DIR')

if PLATFORM == 'gcc':
    # toolchains
    PREFIX  = 'riscv64-unknown-elf-'
    CC      = PREFIX + 'gcc'
    CXX     = PREFIX + 'g++'
    AS      = PREFIX + 'gcc'
    AR      = PREFIX + 'ar'
    LINK    = PREFIX + 'g++'
    TARGET_EXT = 'elf'
    SIZE    = PREFIX + 'size'
    OBJDUMP = PREFIX + 'objdump'
    OBJCPY  = PREFIX + 'objcopy'
    STRIP   = PREFIX + 'strip'
    LD_SCRIPT       = 'gcc_aic.ld'
    QEMU_LD_SCRIPT  = 'gcc_qemu.ld'

    if CPUNAME == 'e906fdp' or CPUNAME == 'e907fdp':
        DEVICE = ' -march=rv32imafdcpzpsfoperand_xtheade -mabi=ilp32d'
        M_DEVICE = ' -march=rv32imafdc -mabi=ilp32d'
    if CPUNAME == 'e906fp' or CPUNAME == 'e907fp':
        DEVICE = ' -march=rv32imafcpzpsfoperand_xtheade -mabi=ilp32f'
    if CPUNAME == 'e906p' or CPUNAME == 'e907p':
        DEVICE = ' -march=rv32imacpzpsfoperand_xtheade -mabi=ilp32'
    if CPUNAME == 'e906fd' or CPUNAME == 'e907fd':
        DEVICE = ' -march=rv32imafdc_xtheade -mabi=ilp32d'
    if CPUNAME == 'e906f' or CPUNAME == 'e907f':
        DEVICE = ' -march=rv32imafc_xtheade -mabi=ilp32f'
    if CPUNAME == 'e906' or CPUNAME == 'e907':
        DEVICE = ' -march=rv32imac_xtheade -mabi=ilp32'

    B_CFLAGS  = ' -c -g -ffunction-sections -fdata-sections -Wall -mcmodel=medlow'
    B_AFLAGS  = ' -c' + ' -x assembler-with-cpp' + ' -D__ASSEMBLY__'
    CFLAGS  = DEVICE + B_CFLAGS + CFLAGS_DBG
    AFLAGS  = DEVICE + B_AFLAGS + AFLAGS_DBG
    CXXFLAGS = CFLAGS
    LFLAGS  = DEVICE + ' -nostartfiles -Wl,--no-whole-archive -lm -lc -lgcc -Wl,-gc-sections -Wl,-zmax-page-size=1024 -Wl,-Map=' + prj_out_dir + SOC + '.map'
    CPATH   = ''
    LPATH   = ''

    # module setting
    M_PREFIX  = 'riscv-none-embed-'
    M_CC      = M_PREFIX + 'gcc'
    M_CXX     = M_PREFIX + 'g++'
    M_AS      = M_PREFIX + 'gcc'
    M_AR      = M_PREFIX + 'ar'
    M_LINK    = M_PREFIX + 'g++'
    M_SIZE    = M_PREFIX + 'size'
    M_OBJDUMP = M_PREFIX + 'objdump'
    M_OBJCPY  = M_PREFIX + 'objcopy'
    M_STRIP   = M_PREFIX + 'strip'
    M_CFLAGS  = M_DEVICE + B_CFLAGS + CFLAGS_DBG + ' -fPIC -shared'
    M_AFLAGS  = M_DEVICE + B_AFLAGS + AFLAGS_DBG
    M_CXXFLAGS = M_CFLAGS
    M_LFLAGS  = M_DEVICE + ' -Wl,--gc-sections,-z,max-page-size=0x4 -shared -fPIC -nostartfiles -nostdlib -static-libgcc'
    M_POST_ACTION = M_STRIP + ' -R .hash $TARGET\n' + M_SIZE + ' $TARGET \n'
    M_BIN_PATH = ''

DUMP_ACTION = OBJDUMP + ' -D -S $TARGET > ' + prj_out_dir + SOC + '.asm\n'
POST_ACTION = '@ls -og $TARGET\n@echo\n'
POST_ACTION += OBJCPY + ' -O binary $TARGET ' + prj_out_dir + SOC + '.bin\n'
POST_ACTION += SIZE + ' $TARGET \n'

