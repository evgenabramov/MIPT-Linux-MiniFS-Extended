#pragma once

#include "common/net_utils.h"
#include "common/tokenizer.h"
#include "server/fs.h"

int perform_init(int disk_fd, int client_fd);

int perform_touch(struct tokenizer, int client_fd);
int perform_mkdir(struct tokenizer, int client_fd);

int perform_rm(struct tokenizer, int client_fd);
int perform_rmdir(struct tokenizer, int client_fd);

int perform_cat(struct tokenizer, int client_fd);
int perform_ls(struct tokenizer, int client_fd);

int perform_get(struct tokenizer, int client_fd);
int perform_put(struct tokenizer, int client_fd);