#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <sys/stat.h>

int GetSize(char* filename);

int main()
{
    const char* filesrc = "filesrc";
    const char* filedst = "filedst";

    int src = open(filesrc, O_RDONLY);
    int dst = open(filedst, O_RDWR|O_CREAT);
    int size = GetSize(filesrc);

    ftruncate(dst, size);
    char* s = mmap(NULL, size, PROT_READ, MAP_PRIVATE, src, 0);
    char* d = mmap(NULL, size, PROT_WRITE, MAP_PRIVATE, dst, 0);
    memcpy(d, s, size);
}

int GetSize(char* filename)
{
    struct stat st;
    int err = stat(filename, &st);
    if (err < 0) 
        perror("stat");
    return st.st_size;
}