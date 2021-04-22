#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct token {
    char* start;
    size_t len;
    struct token* next;
};

struct tokenizer {
    size_t token_count;
    struct token* head;
};

void tokenizer_init(struct tokenizer* tokenizer, char* line);
void tokenizer_free(struct tokenizer* tokenizer);