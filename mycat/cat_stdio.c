#include <stdio.h>

void StdioCatBody(FILE* file);

const int MAX_READ = 1024; //максимальное количество символов для чтения за раз

int main(int ac, char* av[])
{
    if (ac == 1) //только cat
    {
        char buf[MAX_READ] = {};

        while (1)
            StdioCatBody(stdout);

        return 0;
    }

    for (int i = 1; i < ac; i++)
    {
        FILE* filee = fopen(av[i], "r");

        if (filee == 0)
        {
            perror("fopen");
            return 0;
        }

        StdioCatBody(filee);
    }

}

void StdioCatBody(FILE* filee)
{
    int flag_not_eof = 1;
    char buf[MAX_READ] = {};

    while (flag_not_eof)
    {
        flag_not_eof = fread(buf, sizeof(char), MAX_READ, filee);
        int err = fwrite(buf, sizeof(char), flag_not_eof, stdout);

        if (err < 0)
            perror("fwrite");
    }
}
