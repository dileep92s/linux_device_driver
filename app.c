#include <stdio.h>
#include <fcntl.h>

int main()
{
    int fd;
    char msg[] = "hello driver!\n";

    fd = open("/dev/myDev", O_RDWR);
    if (fd < 0)
    {
        perror("open");
    }

    write(fd, msg, sizeof(msg));
    perror("write");

    read(fd, msg, sizeof(msg));
    perror("read");
    printf("msg = %s\n", msg);

    printf("pid = %d\n", getpid());
    ioctl(fd, getpid(), 10);

    close(fd);

    return 0;
}
