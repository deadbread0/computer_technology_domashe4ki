
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#define MAX_PIZZA 10
#define INGREDIENTS 5

struct Pizza {
    int count;
    char ingredients[INGREDIENTS];
    bool ready;
    bool on_stock;
};

struct Monitor {
    pthread_mutex_t m;

    int p, k;           // количество поваров и курьеров
    int w, wmax;        // занятые места для приготовления и их максимум
    int s, smax;        // пиццы на складе и его вместимость

    struct Pizza pizza[MAX_PIZZA];

    int next_pizza;

    int ready_queue[MAX_PIZZA]; //готовая пицца на складе
    int ready_head;
    int ready_tail;
    int ready_count;

    pthread_cond_t c;   // ожидание свободного места для готовки
    pthread_cond_t d;   // ожидание изменения состояния склада
};

void* Cook(void* arg);
void* Deliver(void* arg);

const char recipe[] = "pizza";

int main(const int argc, const char* argv[])
{
    if (argc < 3) {
        printf("enter amount of cooks and delivers\n");
        return 1;
    }

    struct Monitor m = {0};

    m.p = atoi(argv[1]);
    m.k = atoi(argv[2]);

    if (m.p <= 0 || m.k <= 0) {
        printf("amount of cooks and delivers must be positive\n");
        return 1;
    }

    m.wmax = 5;
    m.smax = 10;

    pthread_mutex_init(&m.m, NULL);
    pthread_cond_init(&m.c, NULL);
    pthread_cond_init(&m.d, NULL);

    for (int i = 0; i < MAX_PIZZA; i++) {
        m.pizza[i].count = 0;
        m.pizza[i].ready = false;
        m.pizza[i].on_stock = false;

        for (int j = 0; j < INGREDIENTS; j++)
            m.pizza[i].ingredients[j] = '\0';
    }

    pthread_t* thread_cook = calloc(m.p, sizeof(pthread_t));
    pthread_t* thread_deliver = calloc(m.k, sizeof(pthread_t));

    if (!thread_cook || !thread_deliver) {
        perror("calloc");
        free(thread_cook);
        free(thread_deliver);
        return 1;
    }

    for (int i = 0; i < m.p; i++) {
        if (pthread_create(&thread_cook[i], NULL, Cook, &m)) {
            perror("pthread_create");
            exit(1);
        }
    }

    for (int i = 0; i < m.k; i++) {
        if (pthread_create(&thread_deliver[i], NULL, Deliver, &m)) {
            perror("pthread_create");
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

        // ищем пиццу, которая ещё не готова и не находится на складе
        int id = -1;

        for (int i = 0; i < MAX_PIZZA; i++) {
            int candidate = (m->next_pizza + i) % MAX_PIZZA;

            if (!m->pizza[candidate].ready &&
                !m->pizza[candidate].on_stock) {
                id = candidate;
                m->next_pizza = (candidate + 1) % MAX_PIZZA;
                break;
            }
        }

        pthread_mutex_unlock(&m->m);

        if (id == -1) { //готовить нечего
            pthread_mutex_lock(&m->m);
            m->w--;
            pthread_cond_signal(&m->c);
            pthread_mutex_unlock(&m->m);
            continue;
        }

        // готовится пицца

        // Повар проверяет, каких ингредиентов не хватает
        if (!m->pizza[id].ready) {
            int count = m->pizza[id].count;

            if (count < INGREDIENTS) {
                int found = -1;

                for (int i = 0; i < INGREDIENTS; i++) {
                    if (m->pizza[id].ingredients[i] == '\0') {
                        found = i;
                        break;
                    }
                }

                if (found != -1) { 

                    m->pizza[id].ingredients[found] = recipe[count];
                    m->pizza[id].count++;
                }
            }

            if (m->pizza[id].count >= INGREDIENTS)
                m->pizza[id].ready = true;
        }

        pthread_mutex_lock(&m->m);

        m->w--;

        if (m->pizza[id].ready && !m->pizza[id].on_stock) {
            while (m->s == m->smax) { //ждем хотя бы одного свободного места
                pthread_cond_wait(&m->d, &m->m);
            }

            m->s++; //пицца на складе

            m->pizza[id].on_stock = true;

            m->ready_queue[m->ready_tail] = id;
            m->ready_tail = (m->ready_tail + 1) % MAX_PIZZA;
            m->ready_count++;

            pthread_cond_signal(&m->d); //будим курьера, склад непуст
        }

        pthread_cond_signal(&m->c); //будим повара, тк место освободилось

        pthread_mutex_unlock(&m->m);
    }

    return NULL;
}


void* Deliver(void* arg)
{
    struct Monitor* m = (struct Monitor*)arg;

    while (true) {
        pthread_mutex_lock(&m->m);

        while (m->ready_count == 0) { //курьер ждет пиццу
            pthread_cond_wait(&m->d, &m->m);
        }

        int id = m->ready_queue[m->ready_head];

        m->ready_head = (m->ready_head + 1) % MAX_PIZZA;
        m->ready_count--;

        m->s--; //берем пиццу со склада
        // проверку сюда добавить надо на правильную сборку пиццы

        char result[INGREDIENTS + 1];

        for (int i = 0; i < INGREDIENTS; i++)
            result[i] = m->pizza[id].ingredients[i];

        result[INGREDIENTS] = '\0';

        if (strcmp(result, "pizza") == 0) {
            // printf("%s ok\n", result);
        }
        else {
            printf("%s - WRONG ORDER!\n", result);
        }

        // Освобождаем пиццу для повторного использования
        m->pizza[id].count = 0;
        m->pizza[id].ready = false;
        m->pizza[id].on_stock = false;

        for (int i = 0; i < INGREDIENTS; i++)
            m->pizza[id].ingredients[i] = '\0';

        pthread_cond_signal(&m->c);
        pthread_cond_signal(&m->d); //будим всех, кто ждал сигнал от склада

        pthread_mutex_unlock(&m->m);
    }

    return NULL;
}
