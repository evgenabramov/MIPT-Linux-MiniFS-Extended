#include <string.h>
#include <stdio.h>

#include "common/net_utils.h"

void safe_send(const char* buf, int conn_fd, int len) {
    if (len == -1) {
        len = (int)strlen(buf);
    }
    int pos = 0;
    
    send(conn_fd, &len, sizeof(int), 0);
    
    if (len == 0) {
        return;
    }
    
    while (1) {
        int send_bytes = (int)send(conn_fd, buf + pos, len - pos, 0);
        if (send_bytes < 0) {
            perror("safe send");
            return;
        }
        pos += send_bytes;
        if (pos == len) {
            break;
        }
    }
}

void safe_recv(char* buf, int conn_fd) {
    int len = 0;
    int pos = 0;
    
    recv(conn_fd, &len, sizeof(int),0);
    
    if (len == 0) {
        return;
    }

    while (1) {
        int recv_bytes = (int)recv(conn_fd, buf + pos, len - pos, 0);
        if (recv_bytes < 0) {
            perror("safe recv");
            return;
        }
        pos += recv_bytes;
        if (pos == len) {
            break;
        }
    }
}

void send_status(int status, int client_fd) {
    char status_str[2];
    memset(status_str, '\0', sizeof(status_str));
    status_str[0] = (status == 1) ? '1' : '0';
    safe_send(status_str, client_fd, -1);
}

int recv_status(int conn_fd) {
    char status_str[2];
    memset(status_str, '\0', sizeof(status_str));
    safe_recv(status_str, conn_fd);
    return (status_str[0] == '1') ? 1 : 0;
}

void send_failure(const char* buf, int conn_fd) {
    send_status(0, conn_fd);
    safe_send(buf, conn_fd, -1);
}

void send_success(const char* buf, int conn_fd) {
    send_status(1, conn_fd);
    safe_send(buf, conn_fd, -1);
}

int recv_response(int conn_fd, int has_result) {
    int status = recv_status(conn_fd);
    if (status == 1 && !has_result) {
        return 0;
    }
    
    char response[1024];
    memset(response, 0, sizeof(response));
    
    safe_recv(response, conn_fd);
    if (status == 1) {
        printf("%s\n", response);
        return 0;
    } else {
        fprintf(stderr, "%s\n", response);
        return -1;
    }
}

int recv_store_response(int conn_fd, char* buf) {
    int status = recv_status(conn_fd);
    
    char response[4096];
    memset(response, '\0', sizeof(response));
    
    safe_recv(response, conn_fd);
    if (status == 1) {
        strcpy(buf, response);
        return 0;
    } else {
        fprintf(stderr, "%s\n", response);
        return -1;
    }
}