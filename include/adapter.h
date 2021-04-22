#pragma once

#include "tokenizer.h"

int perform_init(const char* fs_filename, FILE** fs);
int perform_open(const char* fs_filename, FILE** fs);

int perform_touch(struct tokenizer);
int perform_mkdir(struct tokenizer);

int perform_rm(struct tokenizer);
int perform_rmdir(struct tokenizer);

int perform_cat(struct tokenizer);
int perform_ls(struct tokenizer);

int perform_get(struct tokenizer);
int perform_put(struct tokenizer);