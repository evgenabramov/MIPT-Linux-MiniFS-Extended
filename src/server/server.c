#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "server/adapter.h"

int disk_fd;
_Thread_local int client_fd;
_Thread_local int user_id;

FILE* log_fp;

void write_to_log(const char* msg) {
    fprintf(log_fp, "%s\n", msg);
    fflush(log_fp);
}

void daemonize() {
    pid_t pid;
    
    if ((pid = fork()) < 0) {
        exit(1);
    } else if (pid > 0) {
        exit(0);
    }
    
    if (setsid() < 0) {
        exit(1);
    }
    
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    
    if ((pid = fork()) < 0) {
        exit(1);
    } else if (pid > 0) {
        exit(0);
    }
    umask(0);
}

void create_disk(const char* path) {
    disk_fd = open(path, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (disk_fd == -1) {
        write_to_log("cannot create disk by path to character device");
        exit(1);
    }
    
    // TODO: use perform_open to restore filesystem from previous session
    perform_init(disk_fd, client_fd);
}

int setup_server(int port) {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        write_to_log("cannot create socket");
        exit(1);
    }
    
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        write_to_log("setsockopt error");
    }
    
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(sock_fd, (struct sockaddr*)(&serv_addr), sizeof(serv_addr)) < 0) {
        write_to_log("bind error");
        exit(1);
    }
    
    listen(sock_fd, 1);
    return sock_fd;
}

int recv_tokens(struct tokenizer* tokenizer) {
    char line[1024];
    memset(line, 0, sizeof(line));
    
    safe_recv(line, client_fd);
    tokenizer_init(tokenizer, line);
    return (int)tokenizer->token_count;
}

void* process_client(void* new_client_fd) {
    client_fd = *((int*)(new_client_fd));
    free((int*)new_client_fd);

    char user_id_str[1024];
    memset(user_id_str, 0, sizeof(user_id_str));
    safe_recv(user_id_str, client_fd);
    
    user_id = (int)strtol(user_id_str, 0, 10);
    send_status(1, client_fd);
    
    struct tokenizer tokenizer;
    
    while (recv_tokens(&tokenizer) > 0) {
        size_t len = tokenizer.head->len;
        char* first = tokenizer.head->start;
        
        // Response to client is sent in `perform_` commands
        if (strncmp(first, "quit", len) == 0) {
            tokenizer_free(&tokenizer);
            break;
        } else if (strncmp(first, "touch", len) == 0) {
            perform_touch(tokenizer, client_fd);
        } else if (strncmp(first, "mkdir", len) == 0) {
            perform_mkdir(tokenizer, client_fd);
        } else if (strncmp(first, "cat", len) == 0) {
            perform_cat(tokenizer, client_fd);
        } else if (strncmp(first, "ls", len) == 0) {
            perform_ls(tokenizer, client_fd);
        } else if (strncmp(first, "rm", len) == 0) {
            perform_rm(tokenizer, client_fd);
        } else if (strncmp(first, "rmdir", len) == 0) {
            perform_rmdir(tokenizer, client_fd);
        } else if (strncmp(first, "put", len) == 0) {
            perform_put(tokenizer, client_fd);
        } else if (strncmp(first, "get", len) == 0) {
            perform_get(tokenizer, client_fd);
        }
        
        tokenizer_free(&tokenizer);
    }
    return NULL;
}

// TODO: test working with multiple clients
int main(int argc, char** argv) {
    daemonize();
    log_fp = fopen("log.txt", "a");
    create_disk("/dev/minifs");
    int sock_fd = setup_server(argc >= 2 ? (int)strtol(argv[1], 0, 10) : 8080);
    while (1) {
        int* new_client_fd = malloc(sizeof(int));
        *new_client_fd = accept(sock_fd, NULL, NULL);
        pthread_t thread;
        pthread_create(&thread, NULL, process_client, new_client_fd);
    }
    close(disk_fd);
    return 0;
}