import urllib.request
url = "https://raw.githubusercontent.com/torvalds/linux/master/drivers/net/wireless/realtek/rtw89/fw.c"
try:
    content = urllib.request.urlopen(url).read().decode('utf-8')
    for line in content.split('\n'):
        if "FWCMD_H2C" in line or "fwhdr_dl" in line:
            print(line)
except Exception as e:
    print(e)
