#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

struct Monitor {
    int sum;
};

long int million = 1000000;

void* Func(int* num);

int main(const int argc, const char* argv[])
{
    int n = atoi(argv[1]);
    struct Monitor m = {0};
    typedef void* ptr(int*);
    ptr* f =  Func;

    pthread_t* threads = calloc(n, sizeof(pthread_t));

    for (int i = 0; i < n; i++)
    {
        if (pthread_create(&(threads[i]), NULL, f, &(m.sum)))
            perror("pthread_create");
    }
    
    for (int i = 0; i < n; i++)
    {
        pthread_join(threads[i], NULL);
    }
    printf("%lld\n", m.sum / million);
}

void* Func(int* num)
{
    for (int j = 0; j < million; j++)
        (*num)++;

    return NULL;
}