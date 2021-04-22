#include "tokenizer.h"

void tokenizer_init(struct tokenizer* tokenizer, char* line) {
    tokenizer->token_count = 0;
    tokenizer->head = NULL;

    char* next = line;
    struct token* last_token = NULL;

    for (;;) {
        while (*next == ' ' || *next == '\t' || *next == '\n') {
            ++next;
        }

        if (*next == '\0') {
            break;
        }

        char* start = next;
        while (*next != ' ' && *next != '\t' && *next != '\n') {
            ++next;
        }

        struct token* token = (struct token*)malloc(sizeof(struct token));
        token->start = start;
        token->len = next - start;
        token->next = NULL;

        if (last_token) {
            last_token->next = token;
        } else {
            tokenizer->head = token;
        }

        last_token = token;
        ++tokenizer->token_count;
    }
}

void tokenizer_free(struct tokenizer* tokenizer) {
    struct token* next = tokenizer->head;
    while (next) {
        struct token* prev = next;
        next = next->next;
        free(prev);
    }
}