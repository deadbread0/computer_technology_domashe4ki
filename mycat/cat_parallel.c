#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdbool.h>

void CopyFromTo(int from, int to);

const int MAX_READ = 1024; //максимальное количество символов для чтения за раз

int main(int ac, char* av[])
{
    bool only_cat_flag = false;
    if (ac == 1)
        only_cat_flag = true;
    
    do {
        char buf[MAX_READ] = {};

        int fds[2] = {};

        if (pipe(fds) < 0)
            perror("pipe");

        int p = fork();

        if (p == 0)
        {
            close(fds[0]);
            for (int i = 1; i < ac; i++)
            {
                char* filee = av[i];
                int fd = open(av[i], O_RDONLY);

                if (fd < 0)
                    perror(av[i]);

                CopyFromTo(fd, fds[1]);
            }
            if (only_cat_flag)
                CopyFromTo(0, fds[1]);
            
            close(fds[1]);
            exit(0);
        }

        close(fds[1]);
        CopyFromTo(fds[0], 1);
        wait(0);
    } while (only_cat_flag);

    return 0;
}

void CopyFromTo(int from, int to)
{
    char buf[MAX_READ] = {};
    int c = 1;
    while (c)
    {
        c = read(from, buf, MAX_READ);
        if (c < 0)
            perror("read");
        int w = write(to, buf, c);
        if (w < 0)
            perror("write");
    }

}
