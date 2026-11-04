
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <string.h>

#define MAX_READ 1024
#define N 5

// одна ячейка кольцевого буфера
struct Slot
{
    char data[MAX_READ];
    int size;
};

void WriteAll(int fd, char* buf, int size);
void Producer(int ac, char* av[], struct Slot* buffer, sem_t* full, sem_t* empty, sem_t* mutex, int* wi);
void Consumer(struct Slot* buffer, sem_t* full, sem_t* empty, sem_t* mutex, int* ri);


int main(int ac, char* av[])
{
    struct Slot* buffer = mmap(NULL, sizeof(struct Slot) * N,
              PROT_READ | PROT_WRITE,
              MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    int* wi = mmap(NULL, sizeof(int),
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    int* ri = mmap(NULL, sizeof(int),
            PROT_READ | PROT_WRITE,
            MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    *wi = 0;
    *ri = 0;

    sem_t* mutex = sem_open("/cat_mutex", O_CREAT, 0600, 1);
    sem_t* full  = sem_open("/cat_full",  O_CREAT, 0600, N);
    sem_t* empty = sem_open("/cat_empty", O_CREAT, 0600, 0);

    int p = fork();

    if (p < 0)
        perror("fork");

    if (p == 0)
    {
        Producer(ac, av, buffer, full, empty, mutex, wi);
        exit(0);
    }

    Consumer(buffer, full, empty, mutex, ri);

    wait(0);

    sem_close(mutex);
    sem_close(full);
    sem_close(empty);

    sem_unlink("/cat_mutex");
    sem_unlink("/cat_full");
    sem_unlink("/cat_empty");

    return 0;
}

void WriteAll(int fd, char* buf, int size)
{
    int written = 0;

    while (written < size)
    {
        int w = write(fd, buf + written, size - written);

        if (w <= 0)
            perror("write");

        written += w;
    }
}

void Producer(int ac, char* av[], struct Slot* buffer, sem_t* full, sem_t* empty, sem_t* mutex, int* wi)
{
    char buf[MAX_READ];

    //stdin
    if (ac == 1)
    {
        int c = 0;

        while (1)
        {
            c = read(0, buf, MAX_READ);

            if (c < 0)
                perror("read");

            if (c == 0)
                break;

            if (sem_wait(full) < 0)
                perror("sem_wait");

            // кр секция
            if (sem_wait(mutex) < 0)
                perror("sem_wait");

            struct Slot* slot = &buffer[*wi];

            memcpy(slot->data, buf, c);
            slot->size = c;

            *wi = (*wi + 1) % N;

            // конец кр секции
            sem_post(mutex);

            sem_post(empty);
        }
    }
    else
    {
        for (int i = 1; i < ac; i++)
        {
            int fd = open(av[i], O_RDONLY);

            if (fd < 0)
            {
                perror(av[i]);
                continue;
            }

            int c;

            while (1)
            {
                c = read(fd, buf, MAX_READ);

                if (c < 0)
                {
                    perror("read");
                    break;
                }

                if (c == 0)
                    break;

                // ждем свободную ячейку
                if (sem_wait(full) < 0)
                    perror("sem_wait");

                //кр секция
                if (sem_wait(mutex) < 0)
                    perror("sem_wait");

                struct Slot* slot = &buffer[*wi];

                memcpy(slot->data, buf, c);
                slot->size = c;

                *wi = (*wi + 1) % N;

                // конец кр секции
                sem_post(mutex);

                sem_post(empty);
            }

            close(fd);
        }
    }

    //сообщаем Consumer о том, что закончили
    if (sem_wait(full) < 0)
        perror("sem_wait");
    if (sem_wait(mutex) < 0)
        perror("sem_wait");

    buffer[*wi].size = 0;
    *wi = (*wi + 1) % N;

    sem_post(mutex);
    sem_post(empty);
}


void Consumer(struct Slot* buffer, sem_t* full, sem_t* empty, sem_t* mutex, int* ri)
{
    while (1)
    {
        //ждем заполненную ячейку
        if (sem_wait(empty) < 0)
            perror("sem_wait");

        // кр секция
        if (sem_wait(mutex) < 0)
            perror("sem_wait");


       struct Slot* slot = &buffer[*ri];

        int size = slot->size;
        char buf[MAX_READ];

        if (size > 0)
            memcpy(buf, slot->data, size);

        *ri = (*ri + 1) % N;

        // конец кр секции
        sem_post(mutex);

        sem_post(full);

        if (size == 0)
            break;

        WriteAll(1, buf, size);
    }
}

