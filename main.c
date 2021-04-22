#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "adapter.h"
#include "tokenizer.h"

ssize_t get_user_line(char** line, size_t* maxlen) {
    printf(">>> ");
    return getline(line, maxlen, stdin);
}

FILE* main_fs;

int main(int argc, char** argv) {
    printf("Welcome to MiniFS!\n");

    char* line = NULL;
    size_t maxlen = 0;

    ssize_t len = 0;
    struct tokenizer tokenizer;

    const char* fs_filename = (argc == 1) ? "filesystem" : argv[1];
    if (perform_open(fs_filename, &main_fs) != 0) {
        perform_init(fs_filename, &main_fs);
        perform_open(fs_filename, &main_fs);
    }

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
        } else if (strncmp(first, "touch", len) == 0) {
            perform_touch(tokenizer);
        } else if (strncmp(first, "mkdir", len) == 0) {
            perform_mkdir(tokenizer);
        } else if (strncmp(first, "cat", len) == 0) {
            perform_cat(tokenizer);
        } else if (strncmp(first, "ls", len) == 0) {
            perform_ls(tokenizer);
        } else if (strncmp(first, "rm", len) == 0) {
            perform_rm(tokenizer);
        } else if (strncmp(first, "rmdir", len) == 0) {
            perform_rmdir(tokenizer);
        } else if (strncmp(first, "put", len) == 0) {
            perform_put(tokenizer);
        } else if (strncmp(first, "get", len) == 0) {
            perform_get(tokenizer);
        } else {
            printf("Unexpected input, please retry\n");
            printf("Supported commands:\n");
            printf("touch, mkdir, cat, ls, rm, rmdir, put, get\n");
        }

        tokenizer_free(&tokenizer);
    }

    fclose(main_fs);
    return 0;
}