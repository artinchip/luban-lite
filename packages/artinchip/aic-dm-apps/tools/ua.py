# File      : ua.py
#             Tool Script for building User Applications
# This file is part of RT-Thread RTOS
# COPYRIGHT (C) 2006 - 2015, RT-Thread Development Team
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License along
#  with this program; if not, write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#
# Change Logs:
# Date           Author       Notes
# 2015-02-07     Bernard      The firstly version
#

import os
import sys
from SCons.Script import *

Rtt_Root = ''
BSP_Root = ''
Env = None

# toolchain path
if os.environ.has_key("M_EXEC_PATH"):
    M_EXEC_PATH = os.environ["M_EXEC_PATH"]
else:
    M_EXEC_PATH = rtconfig.M_EXEC_PATH

def BuildEnv(BSP_ROOT, RTT_ROOT):
    if BSP_ROOT == None:
        if os.getenv('BSP_ROOT'):
            BSP_ROOT = os.getenv('BSP_ROOT')
        else:
            print 'Please set BSP(board support package) directory!'
            exit(-1)

    if not os.path.exists(BSP_ROOT):
        print 'No BSP(board support package) directory found!'
        exit(-1)

    if RTT_ROOT == None:
        # get RTT_ROOT from BSP_ROOT
        sys.path = sys.path + [BSP_ROOT]
        try:
            import rtconfig
            RTT_ROOT = rtconfig.RTT_ROOT
        except Exception as e:
            print 'Import rtconfig.py in BSP(board support package) failed.'
            print e
            exit(-1)

    global Rtt_Root
    global BSP_Root

    Rtt_Root = RTT_ROOT
    BSP_Root = BSP_ROOT

def BuildHostApplication(TARGET, SConscriptFile):
    import platform

    platform_type = platform.system()
    if platform_type == 'Windows' or platform_type.find('MINGW') != -1:
        TARGET = TARGET.replace('.mo', '.exe')

    sys.path = sys.path + [os.path.join(os.getcwd(), 'tools', 'host')]

    from building import PrepareHostModuleBuilding

    HostRtt = os.path.join(os.getcwd(), 'tools', 'host', 'rtthread')
    Env = Environment()

    if not GetOption('verbose'):
        # override the default verbose command string
        Env.Replace(
            ARCOMSTR = 'AR $TARGET',
            ASCOMSTR = 'AS $TARGET',
            ASPPCOMSTR = 'AS $TARGET',
            CCCOMSTR = 'CC $TARGET',
            CXXCOMSTR = 'CXX $TARGET',
            LINKCOMSTR = 'LINK $TARGET'
        )

    PrepareHostModuleBuilding(Env)

    objs = SConscript(SConscriptFile)
    objs += SConscript(HostRtt + '/SConscript')

    target = Env.Program(TARGET, objs)
    return

def BuildHostLibrary(TARGET, SConscriptFile):
    import platform

    platform_type = platform.system()
    if platform_type == 'Windows' or platform_type.find('MINGW') != -1:
        TARGET = TARGET.replace('.mo', '.exe')

    sys.path = sys.path + [os.path.join(os.getcwd(), 'tools', 'host')]

    from building import PrepareHostModuleBuilding

    HostRtt = os.path.join(os.getcwd(), 'tools', 'host', 'rtthread')
    Env = Environment()

    if not GetOption('verbose'):
        # override the default verbose command string
        Env.Replace(
            ARCOMSTR = 'AR $TARGET',
            ASCOMSTR = 'AS $TARGET',
            ASPPCOMSTR = 'AS $TARGET',
            CCCOMSTR = 'CC $TARGET',
            CXXCOMSTR = 'CXX $TARGET',
            LINKCOMSTR = 'LINK $TARGET'
        )

    PrepareHostModuleBuilding(Env)

    objs = SConscript(SConscriptFile)
    objs += SConscript(HostRtt + '/SConscript')

    target = Env.Program(TARGET, objs)
    return


def BuildApplication(TARGET, SConscriptFile, BSP_ROOT = None, RTT_ROOT = None, CHIP_DIR = None, PRJ_OUT_DIR = None):
    global Env
    global Rtt_Root
    global BSP_Root

    # add comstr option
    AddOption('--verbose',
                dest='verbose',
                action='store_true',
                default=False,
                help='print verbose information during build')

    # build application in host 
    if BSP_ROOT == None and RTT_ROOT == None and not os.getenv('BSP_ROOT'):
        BuildHostApplication(TARGET, SConscriptFile)
        return

    if RTT_ROOT == None and os.getenv('RTT_ROOT'):
        RTT_ROOT = os.getenv('RTT_ROOT')

    # handle BSP_ROOT and RTT_ROOT
    BuildEnv(BSP_ROOT, RTT_ROOT)

    sys.path = sys.path + [os.path.join(Rtt_Root, 'tools'), BSP_Root, CHIP_DIR, PRJ_OUT_DIR]

    # get configuration from BSP 
    import rtconfig 
    from rtua import GetCPPPATH
    from rtua import GetCPPDEFINES
    from building import PrepareModuleBuilding

    linkflags = rtconfig.M_LFLAGS + ' -e main'
    CPPPATH = GetCPPPATH(BSP_Root, Rtt_Root)

    if rtconfig.PLATFORM == 'cl': 
        Env = Environment(TARGET_ARCH='x86')
        Env.Append(CCFLAGS=rtconfig.M_CFLAGS)
        Env.Append(LINKFLAGS=rtconfig.M_LFLAGS)
        Env.Append(CPPPATH=CPPPATH)
        Env.Append(LIBS='rtthread', LIBPATH=BSP_Root)
        Env.Append(CPPDEFINES=GetCPPDEFINES() + ['RTT_IN_MODULE'])
        Env.PrependENVPath('PATH', M_EXEC_PATH)
    else:
        Env = Environment(tools = ['mingw'],
            AS = rtconfig.M_AS, ASFLAGS = rtconfig.M_AFLAGS,
            CC = rtconfig.M_CC, CCFLAGS = rtconfig.M_CFLAGS,
            CPPDEFINES = GetCPPDEFINES(),
            CXX = rtconfig.M_CXX, AR = rtconfig.M_AR, ARFLAGS = '-rc',
            LINK = rtconfig.M_LINK, LINKFLAGS = linkflags,
            CPPPATH = CPPPATH)
        Env.PrependENVPath('PATH', M_EXEC_PATH)

    if not GetOption('verbose'):
        # override the default verbose command string
        Env.Replace(
            ARCOMSTR = 'AR $TARGET',
            ASCOMSTR = 'AS $TARGET',
            ASPPCOMSTR = 'AS $TARGET',
            CCCOMSTR = 'CC $TARGET',
            CXXCOMSTR = 'CXX $TARGET',
            LINKCOMSTR = 'LINK $TARGET'
        )

    PrepareModuleBuilding(Env, Rtt_Root, BSP_Root)

    objs = SConscript(SConscriptFile)

    # build program 
    if rtconfig.PLATFORM == 'cl':
        dll_target = TARGET.replace('.mo', '.dll')
        target = Env.SharedLibrary(dll_target, objs)

        target = Command("$TARGET", dll_target, [Move(TARGET, dll_target)])
        # target = dll_target
    else:
        target = Env.Program(TARGET, objs)

    if hasattr(rtconfig, 'M_POST_ACTION'):
        Env.AddPostAction(target, rtconfig.M_POST_ACTION)

    #if hasattr(rtconfig, 'M_BIN_PATH'):
    #    Env.AddPostAction(target, [Copy(rtconfig.M_BIN_PATH, TARGET)])

def BuildLibrary(TARGET, SConscriptFile, BSP_ROOT = None, RTT_ROOT = None, CHIP_DIR = None, PRJ_OUT_DIR = None):
    global Env
    global Rtt_Root
    global BSP_Root

    # add comstr option
    AddOption('--verbose',
                dest='verbose',
                action='store_true',
                default=False,
                help='print verbose information during build')

    # build application in host 
    if BSP_ROOT == None and RTT_ROOT == None and not os.getenv('BSP_ROOT'):
        BuildHostLibrary(TARGET, SConscriptFile)
        return

    if RTT_ROOT == None and os.getenv('RTT_ROOT'):
        RTT_ROOT = os.getenv('RTT_ROOT')

    # handle BSP_ROOT and RTT_ROOT
    BuildEnv(BSP_ROOT, RTT_ROOT)

    sys.path = sys.path + [os.path.join(Rtt_Root, 'tools'), BSP_Root, CHIP_DIR, PRJ_OUT_DIR]

    # get configuration from BSP 
    import rtconfig 
    from rtua import GetCPPPATH
    from rtua import GetCPPDEFINES
    from building import PrepareModuleBuilding

    linkflags = rtconfig.M_LFLAGS + ' -e 0'
    CPPPATH = GetCPPPATH(BSP_Root, Rtt_Root)

    if rtconfig.PLATFORM == 'cl': 
        Env = Environment(TARGET_ARCH='x86')
        Env.Append(CCFLAGS=rtconfig.M_CFLAGS)
        Env.Append(LINKFLAGS=rtconfig.M_LFLAGS)
        Env.Append(CPPPATH=CPPPATH)
        Env.Append(LIBS='rtthread', LIBPATH=BSP_Root)
        Env.Append(CPPDEFINES=GetCPPDEFINES() + ['RTT_IN_MODULE'])
        Env.PrependENVPath('PATH', M_EXEC_PATH)
    else:
        Env = Environment(tools = ['mingw'],
            AS = rtconfig.M_AS, ASFLAGS = rtconfig.M_AFLAGS,
            CC = rtconfig.M_CC, CCFLAGS = rtconfig.M_CFLAGS,
            CPPDEFINES = GetCPPDEFINES(),
            CXX = rtconfig.M_CXX, AR = rtconfig.M_AR, ARFLAGS = '-rc',
            LINK = rtconfig.M_LINK, LINKFLAGS = linkflags,
            CPPPATH = CPPPATH)
        Env.PrependENVPath('PATH', M_EXEC_PATH)

    if not GetOption('verbose'):
        # override the default verbose command string
        Env.Replace(
            ARCOMSTR = 'AR $TARGET',
            ASCOMSTR = 'AS $TARGET',
            ASPPCOMSTR = 'AS $TARGET',
            CCCOMSTR = 'CC $TARGET',
            CXXCOMSTR = 'CXX $TARGET',
            LINKCOMSTR = 'LINK $TARGET'
        )

    PrepareModuleBuilding(Env, Rtt_Root, BSP_Root)

    objs = SConscript(SConscriptFile)

    # build program 
    if rtconfig.PLATFORM == 'cl':
        dll_target = TARGET.replace('.so', '.dll')
        target = Env.SharedLibrary(dll_target, objs)

        target = Command("$TARGET", dll_target, [Move(TARGET, dll_target)])
        # target = dll_target
    else:
        target = Env.Program(TARGET, objs)

    if hasattr(rtconfig, 'M_POST_ACTION'):
        Env.AddPostAction(target, rtconfig.M_POST_ACTION)
