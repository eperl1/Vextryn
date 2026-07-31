#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
int main() {
    int fd = open("/dev/bus/usb/002/017", O_WRONLY);
    if (fd < 0) { perror("open"); return 1; }
    if (ioctl(fd, USBDEVFS_RESET, 0) < 0) { perror("ioctl"); return 1; }
    close(fd);
    printf("Reset successful\n");
    return 0;
}
