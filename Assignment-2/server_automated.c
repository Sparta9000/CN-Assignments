#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <sched.h>
#include <pthread.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>
#include <signal.h>

#define RUNNING_THRESHOLD 5

static char multithreaded = 1;
static volatile sig_atomic_t running = 1;

void signalHandler(int signal) {
    running = 0;
}

#define STAT_PATH_SIZE 64

struct process_info {
    int pid;
    char name[256];
    unsigned long utime;
    unsigned long stime;
    unsigned long total_cpu_time;
};

int is_numeric(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0;
        }
    }
    return 1;
}

int get_process_info(const char *pid, struct process_info *proc) {
    char stat_path[STAT_PATH_SIZE];
    snprintf(stat_path, STAT_PATH_SIZE, "/proc/%s/stat", pid);

    // printf("stat_path: %s\n", stat_path);

    FILE *file = fopen(stat_path, "r");
    if (file == NULL) {
        return -1;
    }

    char buffer[512];
    memset(buffer, 0, sizeof(buffer));

    fread(buffer, sizeof(char), sizeof(buffer), file);
    char *token = strtok(buffer, " ");

    int i = 0;
    while (token != NULL) {
        if (i == 0) {
            proc->pid = atoi(token);
        } else if (i == 1) {
            strncpy(proc->name, token, sizeof(proc->name) - 1);
            proc->name[sizeof(proc->name) - 1] = '\0';
        } else if (i == 13) {
            strtoul(token, NULL, 10);
            proc->utime = strtoul(token, NULL, 10);
        } else if (i == 14) {
            proc->stime = strtoul(token, NULL, 10);
        } else if (i > 14) {
            break;
        }
        i++;
        token = strtok(NULL, " ");
    }

    fclose(file);

    proc->total_cpu_time = proc->utime + proc->stime;

    // printf("PID: %d, Name: %s, User Time (utime): %lu, Kernel Time (stime): %lu, Total CPU Time: %lu\n",
    //        proc->pid, proc->name, proc->utime, proc->stime, proc->total_cpu_time);

    return 0;
}

struct process_info* get_top_processes() {
    struct dirent *entry;

    struct process_info* processes = malloc(2 * sizeof(struct process_info));
    if (processes == NULL) {
        perror("malloc");
        return NULL;
    }

    struct process_info process1;
    struct process_info process2;
    process1.total_cpu_time = 0;
    process2.total_cpu_time = 0;

    process1.pid = -1;
    process2.pid = -1;

    struct process_info temp_process;

    DIR *proc_dir = opendir("/proc");
    if (proc_dir == NULL) {
        perror("opendir");
        return NULL;
    }

    while ((entry = readdir(proc_dir)) != NULL) {
        if (is_numeric(entry->d_name)) {
            if (get_process_info(entry->d_name, &temp_process) == 0) {
                if (temp_process.pid < 1) continue;

                if (temp_process.total_cpu_time > process1.total_cpu_time) {
                    process2 = process1;
                    process1 = temp_process;
                } else if (temp_process.total_cpu_time > process2.total_cpu_time) {
                    process2 = temp_process;
                }
            }
        }
    }

    closedir(proc_dir);

    processes[0] = process1;
    processes[1] = process2;

    return processes;
}

void* handle_client(void *args) {
    int new_socket = *(int *)args;
    free(args);

    char buffer[1024] = {0};

    if (read(new_socket, buffer, 1024) <= 0) {
        perror("read");
        close(new_socket);
        return NULL;
    }
    printf("Request recieved: %s\n", buffer);

    if (strcmp(buffer, "get_top_processes") != 0) {
        char response[15] = "Invalid request";
        printf("Sending response: %s\n", response);
        send(new_socket, response, 15, 0);
        close(new_socket);
        return NULL;
    }

    char message[2048] = {0};

    struct process_info* top_processes = get_top_processes();

    if (top_processes == NULL) {
        return NULL;
    }

    snprintf(message, 2048, "Top 2 CPU-consuming processes on the server are:\nPID: %d, Name: %s, Total CPU Time: %lu\nPID: %d, Name: %s, Total CPU Time: %lu\n",
             top_processes[0].pid, top_processes[0].name, top_processes[0].total_cpu_time,
             top_processes[1].pid, top_processes[1].name, top_processes[1].total_cpu_time);

    // sleep(3); // Self induced delay

    send(new_socket, message, strlen(message), 0);
    printf("top_cpu_processes sent\n");

    free(top_processes);
    close(new_socket);

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        multithreaded = atoi(argv[1]);
    }

    int counter = 0;

    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);

    signal(SIGINT, signalHandler);

    pthread_attr_t attr;
    pthread_attr_init(&attr);

    if (multithreaded) {
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    }
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    

    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket");
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8002);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        return 1;
    }

    pthread_t threads[RUNNING_THRESHOLD];

    while ((running) && (counter < RUNNING_THRESHOLD)) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            return 1;
        }

        pthread_t thread;

        int *new_socket_ptr = malloc(sizeof(int));
        if (new_socket_ptr == NULL) {
            perror("malloc");
            return 1;
        }

        *new_socket_ptr = new_socket;

        if (!running) {
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        }

        pthread_create(&thread, &attr, handle_client, (void *)new_socket_ptr);

        threads[counter] = thread;
        if ((!running) || (!multithreaded)) {
            pthread_join(thread, NULL);
        }
        counter++;
    }

    for (int i = 0; i < counter; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_attr_destroy(&attr);

    close(server_fd);
    return 0;
}