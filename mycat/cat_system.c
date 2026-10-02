#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

void SystemCatBody(int fd);
int ReturnfdOfFileOut(int ac, char* av[]);

const int MAX_READ = 1024; //максимальное количество символов для чтения за раз

int main(int ac, char* av[])
{
    int fd_out = ReturnfdOfFileOut(ac, av);

    if (fd_out < 0)
    {
        perror(fd_out);
        return 0;
    }

    if (ac == 1) //только cat
    {
        char buf[MAX_READ] = {};

        while (1)
            SystemCatBody(0);

        return 0;
    }

    for (int i = 1; i < ac; i++)
    {
        int fd = open(av[i], O_RDONLY);
        if (fd < 0)
            perror("open");

        if (fd < 0)
        {
            perror(av[i]);
            return;
        }

        SystemCatBody(fd);
        close(fd);
    }

}

void SystemCatBody(int fd)
{
    int flag_not_eof = 1;
    char buf[MAX_READ] = {};

    while (flag_not_eof)
    {
        flag_not_eof = read(fd, buf, MAX_READ);
        int err = write(1, buf, flag_not_eof);

        if (err < 0)
            perror(err);
    }
}

int ReturnfdOfFileOut(int ac, char* av[]) 
{
    int last_i_of_out = 0;

    for (int i = 1; i < ac; i++)
    {
        if (av[i][0] == '>')
            last_i_of_out = i;
    }

    int fd = open(av[last_i_of_out], O_RDONLY); 
    if (fd < 0)
        perror("open");
    close(fd);

    return fd;
}
