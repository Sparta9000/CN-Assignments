#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sched.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

void* client_task(void *arg) {
    int thread_num = *(int*)arg;
    int sock = 0;
    struct sockaddr_in serv_addr;
    const char *message = "get_top_processes";
    char buffer[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return NULL;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8002);

    if (inet_pton(AF_INET, "10.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("inet");
        return NULL;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        return NULL;
    }

    send(sock, message, strlen(message), 0);
    printf("Thread %d: Request Sent\n", thread_num);
    read(sock, buffer, 1024);
    printf("Thread %d: Message recieved: %s\n", thread_num, buffer);

    close(sock);
}

int main(int argc, char const *argv[]) {
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(1, &mask);
    sched_setaffinity(1, sizeof(mask), &mask);

    int n_threads = 1;
    if (argc > 1) {
        n_threads = atoi(argv[1]);
    }

    pthread_t threads[n_threads];

    for (int i = 0; i < n_threads; i++) {
        void *arg = (void *)&i;
        pthread_create(&threads[i], NULL, client_task, arg);
    }

    for (int i = 0; i < n_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}