Import('AIC_ROOT')
Import('PRJ_CHIP')
Import('PRJ_BOARD')
Import('PRJ_KERNEL')
Import('PRJ_APP')
import os
from building import *

objs = []

# chip
rel_path = os.path.join('bsp/artinchip/sys/' + PRJ_CHIP, 'SConscript')
abs_path = os.path.join(AIC_ROOT, rel_path)
if os.path.isfile(abs_path):
    objs = objs + SConscript(rel_path)

# board
rel_path = os.path.join('target/' + PRJ_CHIP, PRJ_BOARD, 'SConscript')
abs_path = os.path.join(AIC_ROOT, rel_path)
if os.path.isfile(abs_path):
    objs = objs + SConscript(rel_path)

# kernel/xxx
rel_path = os.path.join('kernel', PRJ_KERNEL, 'SConscript')
abs_path = os.path.join(AIC_ROOT, rel_path)
if os.path.isfile(abs_path):
    objs = objs + SConscript(rel_path)
# kernel/common
rel_path = os.path.join('kernel', 'common', 'SConscript')
abs_path = os.path.join(AIC_ROOT, rel_path)
if os.path.isfile(abs_path):
    objs = objs + SConscript(rel_path)

# bsp
rel_path = os.path.join('bsp', 'SConscript')
abs_path = os.path.join(AIC_ROOT, rel_path)
if os.path.isfile(abs_path):
    objs = objs + SConscript(rel_path)

# packages
rel_path = os.path.join('packages', 'SConscript')
abs_path = os.path.join(AIC_ROOT, rel_path)
if os.path.isfile(abs_path):
    objs = objs + SConscript(rel_path)

# app
rel_path = os.path.join('application', PRJ_KERNEL, PRJ_APP, 'SConscript')
abs_path = os.path.join(AIC_ROOT, rel_path)
if os.path.isfile(abs_path):
    objs = objs + SConscript(rel_path)

Return('objs')
