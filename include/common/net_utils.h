#pragma once

#include <arpa/inet.h>

// Base functions for information transmission
void safe_send(const char* buf, int conn_fd, int len);
void safe_recv(char* buf, int conn_fd);

void send_status(int status, int client_fd);

int recv_status(int conn_fd);

// Error messages
void send_failure(const char* buf, int conn_fd);

// Correct response
void send_success(const char* buf, int conn_fd);

// Receive response and print message
int recv_response(int conn_fd, int has_result);

// Receive response and store it into buffer from outer scope
int recv_store_response(int conn_fd, char* buf);