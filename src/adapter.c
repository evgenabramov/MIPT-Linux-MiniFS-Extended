#include "adapter.h"

#include <assert.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "fs.h"

int perform_init(const char* fs_filename, FILE** fs) {
    *fs = fopen(fs_filename, "wb+");
    if (*fs == NULL) {
        fprintf(stdout, "perform_init: cannot open file\n");
        return -1;
    }
    fs_init(*fs);
    fclose(*fs);
    return 0;
}

int perform_open(const char* fs_filename, FILE** fs) {
    if (access(fs_filename, R_OK | W_OK) != 0) {
        return -1;
    }
    *fs = fopen(fs_filename, "rb+");
    fs_open(*fs);
    return 0;
}

int perform_touch(struct tokenizer tokenizer) {
    if (tokenizer.token_count != 2) {
        fprintf(stdout, "Usage: touch <path>\n");
        return -1;
    }

    struct token* second_token = tokenizer.head->next;

    char path[128];
    memset(path, 0, sizeof(path));
    strncpy(path, second_token->start, second_token->len);

    if (create_at(path, REG, NULL) != 0) {
        return -1;
    }

    return 0;
}

int perform_mkdir(struct tokenizer tokenizer) {
    if (tokenizer.token_count != 2) {
        fprintf(stdout, "Usage: mkdir <path>\n");
        return -1;
    }

    struct token* second_token = tokenizer.head->next;

    char path[128];
    memset(path, 0, sizeof(path));
    strncpy(path, second_token->start, second_token->len);

    if (create_at(path, DIR, NULL) != 0) {
        return -1;
    }

    return 0;
}

int perform_rm(struct tokenizer tokenizer) {
    if (tokenizer.token_count != 2) {
        fprintf(stdout, "Usage: rm <file path>\n");
        return -1;
    }

    struct token* second_token = tokenizer.head->next;

    char path[128];
    memset(path, 0, sizeof(path));
    strncpy(path, second_token->start, second_token->len);

    if (remove_at(path) != 0) {
        return -1;
    }

    return 0;
}

int perform_rmdir(struct tokenizer tokenizer) {
    if (tokenizer.token_count != 2) {
        fprintf(stdout, "Usage: rmdir <dir path>\n");
        return -1;
    }

    struct token* second_token = tokenizer.head->next;

    char path[128];
    memset(path, 0, sizeof(path));
    strncpy(path, second_token->start, second_token->len);

    if (remove_at(path) != 0) {
        return -1;
    }
    return 0;
}

int perform_cat(struct tokenizer tokenizer) {
    if (tokenizer.token_count != 2) {
        fprintf(stdout, "Usage: cat <file path>\n");
        return -1;
    }

    struct token* second_token = tokenizer.head->next;

    char path[128];
    memset(path, 0, sizeof(path));
    strncpy(path, second_token->start, second_token->len);

    ssize_t res;
    if ((res = find_file(path)) == -1) {
        fprintf(stdout, "perform_cat: file not found\n");
        return -1;
    }
    int inode_index = res;
    struct inode* inode = get_inode(inode_index);

    if (inode->type != REG) {
        fprintf(stdout, "perform_cat: not a regular file\n");
        return -1;
    }

    char* content = read_file(inode_index);
    fputs(content, stdout);

    free(content);
    return 0;
}

int perform_ls(struct tokenizer tokenizer) {
    if (tokenizer.token_count != 2) {
        fprintf(stdout, "Usage: ls <dir path>\n");
        return -1;
    }

    struct token* second_token = tokenizer.head->next;

    char path[128];
    memset(path, 0, sizeof(path));
    strncpy(path, second_token->start, second_token->len);

    ssize_t res;
    if ((res = find_file(path)) == -1) {
        fprintf(stdout, "perform_ls: directory not found\n");
        return -1;
    }
    int inode_index = res;
    struct inode* inode = get_inode(inode_index);

    if (inode->type != DIR) {
        fprintf(stdout, "perform_ls: not a directory\n");
        return -1;
    }

    int dir_count = inode->file_len / sizeof(struct dir_entry);
    struct dir_entry* dirs = (struct dir_entry*)read_file(inode_index);

    for (int i = 0; i < dir_count; ++i) {
        printf("%s\n", dirs[i].name);
    }

    free(dirs);
    return 0;
}

int perform_get(struct tokenizer tokenizer) {
    if (tokenizer.token_count != 3) {
        fprintf(stdout, "Usage: get <global path> <minifs path>\n");
        return -1;
    }

    struct token* second_token = tokenizer.head->next;

    char global_path[128];
    memset(global_path, 0, sizeof(global_path));
    strncpy(global_path, second_token->start, second_token->len);

    struct token* third_token = second_token->next;

    char minifs_path[128];
    memset(minifs_path, 0, sizeof(minifs_path));
    strncpy(minifs_path, third_token->start, third_token->len);

    FILE* global = fopen(global_path, "r");

    struct stat stat_info;
    fstat(fileno(global), &stat_info);

    char* content = malloc(stat_info.st_size + 1);
    fseek(global, 0, SEEK_SET);
    fread(content, 1, stat_info.st_size, global);
    content[stat_info.st_size] = '\0';
    assert(strlen(content) == stat_info.st_size);

    if (create_at(minifs_path, REG, content) != 0) {
        fprintf(stdout, "perform_get: failed to create regular minifs file\n");
        return -1;
    }

    free(content);
    fclose(global);
    dump_info();
    return 0;
}

int perform_put(struct tokenizer tokenizer) {
    if (tokenizer.token_count != 3) {
        fprintf(stdout, "Usage: put <minifs path> <global path>\n");
        return -1;
    }

    struct token* second_token = tokenizer.head->next;

    char minifs_path[128];
    memset(minifs_path, 0, sizeof(minifs_path));
    strncpy(minifs_path, second_token->start, second_token->len);

    struct token* third_token = second_token->next;

    char global_path[128];
    memset(global_path, 0, sizeof(global_path));
    strncpy(global_path, third_token->start, third_token->len);

    FILE* global = fopen(global_path, "w");
    if (global == NULL) {
        fprintf(stdout, "perform_put: cannot create file at global path\n");
        return -1;
    }

    ssize_t res;
    if ((res = find_file(minifs_path)) == -1) {
        fprintf(stdout, "perform_put: file at minifs_path not found\n");
        return -1;
    }
    int inode_index = res;
    struct inode* inode = get_inode(inode_index);

    if (inode->type != REG) {
        fprintf(stdout, "perform_put: not a regular file\n");
        return -1;
    }

    char* content = read_file(inode_index);
    fputs(content, global);

    free(content);
    fclose(global);
    return 0;
}