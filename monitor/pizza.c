
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>

struct Monitor {
    pthread_mutex_t m;

    int p, k;           // количество поваров и курьеров
    int w, wmax;        // занятые места для приготовления и их максимум
    int s, smax;        // пиццы на складе и его вместимость

    pthread_cond_t c;   // ожидание свободного места для готовки
    pthread_cond_t d;   // ожидание изменения состояния склада
};

void* Cook(void* arg);
void* Deliver(void* arg);

int main(const int argc, const char* argv[])
{
    if (argc < 3) {
        printf("enter amount of cooks and delivers\n");
        return 1;
    }

    struct Monitor m = {0};

    m.p = atoi(argv[1]);
    m.k = atoi(argv[2]);

    m.wmax = 5;
    m.smax = 10;

    pthread_mutex_init(&m.m, NULL);
    pthread_cond_init(&m.c, NULL);
    pthread_cond_init(&m.d, NULL);

    pthread_t* thread_cook = calloc(m.p, sizeof(pthread_t));
    pthread_t* thread_deliver = calloc(m.k, sizeof(pthread_t));

    if (!thread_cook || !thread_deliver)
        perror("calloc");

    for (int i = 0; i < m.p; i++) {
        if (pthread_create(&thread_cook[i], NULL, Cook, &m)) {
            perror("pthread_create");
        }
    }

    for (int i = 0; i < m.k; i++) {
        if (pthread_create(&thread_deliver[i], NULL, Deliver, &m)) {
            exit(1);
        }
    }

    // Потоки работают бесконечно
    for (int i = 0; i < m.p; i++)
        pthread_join(thread_cook[i], NULL);

    for (int i = 0; i < m.k; i++)
        pthread_join(thread_deliver[i], NULL);

    free(thread_cook);
    free(thread_deliver);

    return 0;
}


void* Cook(void* arg)
{
    struct Monitor* m = (struct Monitor*)arg;

    while (true) {
        pthread_mutex_lock(&m->m);

        while (m->w == m->wmax) { //ждем пока освободится место для готовки
            pthread_cond_wait(&m->c, &m->m);
        }

        m->w++; //повар занял место

        printf("Cook starts cooking, %d free places\n", m->wmax - m->w);

        pthread_mutex_unlock(&m->m);

        // готовится пицца

        pthread_mutex_lock(&m->m);

        while (m->s == m->smax) { //ждем хотя бы одного свободного места
            pthread_cond_wait(&m->d, &m->m);
        }

        m->s++; //пицца на складе
        m->w--; //место свободно

        printf("Pizza in box, %d free boxes and %d free places\n", m->smax - m->s, m->wmax - m->w);

        pthread_cond_signal(&m->c); //будим повара, тк место освободилось

        pthread_cond_signal(&m->d); //будим курьера, склад непуст

        pthread_mutex_unlock(&m->m);
    }

    return NULL;
}


void* Deliver(void* arg)
{
    struct Monitor* m = (struct Monitor*)arg;

    while (true) {
        pthread_mutex_lock(&m->m);

        while (m->s == 0) { //курьер ждет пиццу
            pthread_cond_wait(&m->d, &m->m);
        }

        m->s--; //берем пиццу со склада
        // проверку сюда добавить надо на правильную сборку пиццы

        printf("deliver takes pizza, %d free boxes\n", m->smax - m->s);

        pthread_cond_signal(&m->d); //будим всех, кто ждал сигнал от склада

        pthread_mutex_unlock(&m->m);
    }

    return NULL;
}
