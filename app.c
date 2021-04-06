#include <stdio.h>
#include <fcntl.h>

int main()
{
    int fd;

    fd = open("/dev/myDev", O_RDWR);
    if (fd < 0)
    {
        perror("open");
    }

    printf("pid = %d", getpid());

    ioctl(fd, getpid(), 12);

    close(fd);

    return 0;
}
