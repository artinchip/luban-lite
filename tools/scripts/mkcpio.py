import os
import sys

aic_system = sys.argv[1]
aic_root = sys.argv[2]
aic_pack_dir = sys.argv[3]
prj_out_dir = sys.argv[4]

os.chdir(prj_out_dir)

if aic_system == 'Linux':
    os.system('cat ota-subimgs.cfg | cpio -ov -H crc > ota.cpio')
elif aic_system == 'Windows':
    cpio_tool_dir = os.path.join(aic_root, 'tools\scripts\cpio.exe')
    os.system('type ota-subimgs.cfg | ' + cpio_tool_dir + ' -ov -H crc > ota.cpio')

os.chdir(aic_root)
