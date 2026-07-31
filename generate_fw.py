import re
import os

c_file = '/home/ethan/.gemini/antigravity-cli/brain/e820fea6-4263-4ad6-98ef-05afb28b57c7/scratch/rtl8852au/phl/hal_g6/mac/fw_ax/rtl8852a/hal8852a_fw.c'

with open(c_file, 'r') as f:
    data = f.read()

match = re.search(r'u8 array_8852a_u2_nic\[\] = \{(.*?)\};', data, re.DOTALL)
if not match:
    print("Array not found!")
    exit(1)

array_content = match.group(1)

out_c = 'drivers/usb/rtl8852a_fw.c'
out_h = 'drivers/usb/rtl8852a_fw.h'

with open(out_h, 'w') as f:
    f.write('#ifndef RTL8852A_FW_H\n#define RTL8852A_FW_H\n\n#include <stdint.h>\n\nextern const uint8_t rtl8852a_fw_u2_nic[];\nextern const uint32_t rtl8852a_fw_u2_nic_len;\n\n#endif\n')

with open(out_c, 'w') as f:
    f.write('#include "rtl8852a_fw.h"\n\nconst uint8_t rtl8852a_fw_u2_nic[] = {')
    f.write(array_content)
    f.write('};\n\nconst uint32_t rtl8852a_fw_u2_nic_len = sizeof(rtl8852a_fw_u2_nic);\n')

print("Generated rtl8852a_fw.c and rtl8852a_fw.h")
