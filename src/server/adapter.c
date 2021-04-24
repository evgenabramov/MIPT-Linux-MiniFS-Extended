#include <assert.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "server/adapter.h"

int perform_init(int disk_fd, int client_fd) {
    return fs_init(disk_fd, client_fd);
}

int perform_touch(struct tokenizer tokenizer, int client_fd) {
    if (tokenizer.token_count != 2) {
        send_failure("Usage: touch <path>", client_fd);
        return -1;
    }

    struct token* second_token = tokenizer.head->next;

    char path[128];
    memset(path, 0, sizeof(path));
    strncpy(path, second_token->start, second_token->len);

    if (create_at(path, REG, NULL) != 0) {
        return -1;
    }

    send_status(1, client_fd);
    return 0;
}

int perform_mkdir(struct tokenizer tokenizer, int client_fd) {
    if (tokenizer.token_count != 2) {
        send_failure("Usage: mkdir <path>", client_fd);
        return -1;
    }

    struct token* second_token = tokenizer.head->next;

    char path[128];
    memset(path, 0, sizeof(path));
    strncpy(path, second_token->start, second_token->len);

    if (create_at(path, DIR, NULL) != 0) {
        return -1;
    }

    send_status(1, client_fd);
    return 0;
}

int perform_rm(struct tokenizer tokenizer, int client_fd) {
    if (tokenizer.token_count != 2) {
        send_failure("Usage: rm <file path>", client_fd);
        return -1;
    }

    struct token* second_token = tokenizer.head->next;

    char path[128];
    memset(path, 0, sizeof(path));
    strncpy(path, second_token->start, second_token->len);

    if (remove_at(path) != 0) {
        return -1;
    }

    send_status(1, client_fd);
    return 0;
}

// TODO: distinguish regular files and folders for `rm` and `rmdir`
int perform_rmdir(struct tokenizer tokenizer, int client_fd) {
    if (tokenizer.token_count != 2) {
        send_failure("Usage: rmdir <dir path>", client_fd);
        return -1;
    }

    struct token* second_token = tokenizer.head->next;

    char path[128];
    memset(path, 0, sizeof(path));
    strncpy(path, second_token->start, second_token->len);

    if (remove_at(path) != 0) {
        return -1;
    }
    send_status(1, client_fd);
    return 0;
}

int perform_cat(struct tokenizer tokenizer, int client_fd) {
    if (tokenizer.token_count != 2) {
        send_failure("Usage: cat <file path>", client_fd);
        return -1;
    }

    struct token* second_token = tokenizer.head->next;

    char path[128];
    memset(path, 0, sizeof(path));
    strncpy(path, second_token->start, second_token->len);

    ssize_t res;
    if ((res = find_file(path)) == -1) {
        send_failure("perform_cat: file not found", client_fd);
        return -1;
    }
    int inode_index = (int)res;
    struct inode* inode = get_inode(inode_index);

    if (inode->type != REG) {
        send_failure("perform_cat: not a regular file", client_fd);
        return -1;
    }

    char* content = read_file(inode_index);
    
    // Send response to client
    send_success(content, client_fd);
    
    free(content);
    return 0;
}

int perform_ls(struct tokenizer tokenizer, int client_fd) {
    if (tokenizer.token_count != 2) {
        send_failure("Usage: ls <dir path>", client_fd);
        return -1;
    }

    struct token* second_token = tokenizer.head->next;

    char path[128];
    memset(path, 0, sizeof(path));
    strncpy(path, second_token->start, second_token->len);

    ssize_t res;
    if ((res = find_file(path)) == -1) {
        send_failure("perform_ls: directory not found", client_fd);
        return -1;
    }
    int inode_index = (int)res;
    struct inode* inode = get_inode(inode_index);

    if (inode->type != DIR) {
        send_failure("perform_ls: not a directory", client_fd);
        return -1;
    }

    int dir_count = (int)(inode->file_len / sizeof(struct dir_entry));
    struct dir_entry* dirs = (struct dir_entry*)read_file(inode_index);
    
    char response[1024];
    memset(response, 0, sizeof(response));
    int pos = 0;
    for (int i = 0; i < dir_count; ++i) {
        if (i > 0) {
            response[pos++] = '\n';
        }
        sprintf(&response[pos],"%s", dirs[i].name);
        pos += (int)strlen(dirs[i].name);
    }
    
    // Send response to client
    send_success(response, client_fd);

    free(dirs);
    return 0;
}

int perform_get(struct tokenizer tokenizer, int client_fd) {
    // Tokens correctness is checked by client
    struct token* third_token = tokenizer.head->next->next;

    char minifs_path[128];
    memset(minifs_path, 0, sizeof(minifs_path));
    strncpy(minifs_path, third_token->start, third_token->len);
    
    // TODO: use heap to store file content
    char content[4096];
    safe_recv(content, client_fd);

    if (create_at(minifs_path, REG, content) != 0) {
        send_failure("perform_get: failed to create regular minifs file", client_fd);
        return -1;
    }
    
    dump_info();

    send_status(1, client_fd);
    return 0;
}

int perform_put(struct tokenizer tokenizer, int client_fd) {
    // Tokens correctness is checked by client
    struct token* second_token = tokenizer.head->next;

    char minifs_path[128];
    memset(minifs_path, 0, sizeof(minifs_path));
    strncpy(minifs_path, second_token->start, second_token->len);

    ssize_t res;
    if ((res = find_file(minifs_path)) == -1) {
        send_failure("perform_put: file at minifs_path not found", client_fd);
        return -1;
    }
    int inode_index = (int)res;
    struct inode* inode = get_inode(inode_index);

    if (inode->type != REG) {
        send_failure("perform_put: not a regular file", client_fd);
        return -1;
    }

    char* content = read_file(inode_index);
    send_success(content, client_fd);
    
    free(content);
    return 0;
}