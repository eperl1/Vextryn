import hashlib
import re

c_file = '/home/ethan/.gemini/antigravity-cli/brain/e820fea6-4263-4ad6-98ef-05afb28b57c7/scratch/rtl8852au/phl/hal_g6/mac/fw_ax/rtl8852a/hal8852a_fw.c'

with open(c_file, 'r') as f:
    data = f.read()

match = re.search(r'u8 array_8852a_u2_nic\[\] = \{(.*?)\};', data, re.DOTALL)
if not match:
    print("Array not found!")
    exit(1)

array_content = match.group(1)
byte_strings = re.findall(r'0x[0-9a-fA-F]{2}', array_content)
bytes_data = bytes(int(b, 16) for b in byte_strings)

print(f"Size: {len(bytes_data)}")
sha256 = hashlib.sha256(bytes_data).hexdigest()
print(f"SHA256: {sha256}")
