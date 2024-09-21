#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sched.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

void client_task(int n_requests) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char message[1024] = "get_top_processes\0";
    char buffer[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8002);

    if (inet_pton(AF_INET, "10.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("inet");
        return;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        return;
    }

    printf("Connected to server\n");

    for(int i = 0; i < n_requests; i++) {        
        send(sock, message, strlen(message), 0);
        printf("Request Sent\n");
        int valread = read(sock, buffer, 1024);
        if (valread > 0) {
            buffer[valread] = '\0';
            printf("Message recieved: %s\n", buffer);
        }
    }

    send(sock, message, strlen(message), 0);
    printf("Request Sent\n");
    read(sock, buffer, 1024);
    printf("Message recieved: %s\n", buffer);

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

    client_task(n_threads);

    return 0;
}