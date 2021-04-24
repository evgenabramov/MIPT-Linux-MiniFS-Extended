#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common/net_utils.h"
#include "common/tokenizer.h"

int conn_fd;

ssize_t get_user_line(char** line, size_t* maxlen) {
    printf(">>> ");
    return getline(line, maxlen, stdin);
}

int read_user_id() {
    while (1) {
        char* input = NULL;
        size_t n = 0;

        printf("User id (for communication with server): ");
        getline(&input, &n, stdin);
        input[n - 1] = '\0';

        errno = 0;
        int user_id = (int)strtol(input, NULL, 10);
        if (errno == 0) {
            return user_id;
        } else {
            puts("Incorrect input");
        }
    }
}

int connect_to_server(const char* ip, int port) {
    conn_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (conn_fd < 0) {
        perror("socket");
        return -1;
    }

    if (setsockopt(conn_fd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0) {
        puts("setsockopt");
        close(conn_fd);
        return -1;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) != 1) {
        perror("inet_pton");
        close(conn_fd);
        return -1;
    }

    if (connect(conn_fd, (struct sockaddr*)(&serv_addr), sizeof(serv_addr)) < 0) {
        perror("connect");
        close(conn_fd);
        return -1;
    }
    return 0;
}

void send_tokens(struct tokenizer tokenizer) {
    struct token* token = tokenizer.head;
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    int pos = 0;
    for (int i = 0; i < tokenizer.token_count; ++i) {
        strncpy(buffer + pos, token->start, (int)token->len);
        buffer[pos + token->len] = ' ';  // delimiter
        pos += ((int)token->len + 1);
        token = token->next;
    }
    safe_send(buffer, conn_fd, pos);
}

int main(int argc, char** argv) {
    setbuf(stdout, NULL);

    printf("Welcome to MiniFS!\n");

    const char* ip = "127.0.0.1";
    const int port = 8080;

    int user_id = read_user_id();
    if (connect_to_server(ip, port) < 0) {
        puts("Cannot connect to server, exit...");
        exit(1);
    }
    char user_id_str[10];
    memset(user_id_str, 0, sizeof(user_id_str));
    sprintf(user_id_str, "%d", user_id);

    safe_send(user_id_str, conn_fd, -1);
    if (recv_status(conn_fd) == 0) {
        puts("Server rejected to work with this user id");
        close(conn_fd);
        exit(1);
    }

    char* line = NULL;
    size_t maxlen = 0;

    ssize_t len = 0;
    struct tokenizer tokenizer;

    while ((len = get_user_line(&line, &maxlen)) > 0) {
        tokenizer_init(&tokenizer, line);

        if (tokenizer.token_count == 0) {
            continue;
        }

        size_t len = tokenizer.head->len;
        char* first = tokenizer.head->start;

        if (strncmp(first, "quit", len) == 0) {
            tokenizer_free(&tokenizer);
            break;
        } else if (strncmp(first, "touch", len) == 0 || strncmp(first, "mkdir", len) == 0 ||
                   strncmp(first, "rm", len) == 0 || strncmp(first, "rmdir", len) == 0) {
            send_tokens(tokenizer);
            recv_response(conn_fd, 0);
        } else if (strncmp(first, "ls", len) == 0 || strncmp(first, "cat", len) == 0) {
            send_tokens(tokenizer);
            recv_response(conn_fd, 1);
        } else if (strncmp(first, "put", len) == 0) {
            if (tokenizer.token_count != 3) {
                puts("Usage: put <minifs path> <global path>");
                tokenizer_free(&tokenizer);
                continue;
            }

            send_tokens(tokenizer);

            struct token* third_token = tokenizer.head->next->next;

            char global_path[128];
            memset(global_path, 0, sizeof(global_path));
            strncpy(global_path, third_token->start, third_token->len);

            FILE* global = fopen(global_path, "w");
            if (global == NULL) {
                puts("cannot create file at global path");
                return -1;
            }

            // TODO: use heap to store file content
            char content[4096];
            memset(content, '\0', sizeof(content));
            if (recv_store_response(conn_fd, content) < 0) {
                fclose(global);
                tokenizer_free(&tokenizer);
                continue;
            }

            fputs(content, global);
            fclose(global);
        } else if (strncmp(first, "get", len) == 0) {
            if (tokenizer.token_count != 3) {
                puts("Usage: get <global path> <minifs path>");
                tokenizer_free(&tokenizer);
                continue;
            }

            send_tokens(tokenizer);

            struct token* second_token = tokenizer.head->next;

            char global_path[128];
            memset(global_path, 0, sizeof(global_path));
            strncpy(global_path, second_token->start, second_token->len);

            FILE* global = fopen(global_path, "r");

            struct stat stat_info;
            fstat(fileno(global), &stat_info);

            char* content = malloc(stat_info.st_size + 1);
            fseek(global, 0, SEEK_SET);
            fread(content, 1, stat_info.st_size, global);
            content[stat_info.st_size] = '\0';
            assert(strlen(content) == stat_info.st_size);

            safe_send(content, conn_fd, -1);

            fclose(global);
            recv_response(conn_fd, 0);
        } else {
            printf("Unexpected input, please retry\n");
            printf("Supported commands:\n");
            printf("touch, mkdir, cat, ls, rm, rmdir, put, get\n");
        }

        tokenizer_free(&tokenizer);
    }

    close(conn_fd);
    return 0;
}
