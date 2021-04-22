#include "fs.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

FILE* fs;

// Для удобства эти данные хранятся и в файле, и в виде структур
// главное, что в оперативной памяти не хранятся сами блоки
struct superblock sb;

int inode_bitmap[INODE_COUNT];
struct inode inode_table[INODE_COUNT];

int block_bitmap[BLOCK_COUNT];

struct inode* get_inode(int inode_index) {
    return &inode_table[inode_index];
}

ssize_t get_free_inode_index() {
    for (int i = 0; i < sb.inode_count; ++i) {
        if (inode_bitmap[i] == 0) {
            inode_bitmap[i] = 1;
            --sb.free_inode_count;
            return i;
        }
    }
    return -1;
}

void free_inode(int inode_index) {
    struct inode* inode = &inode_table[inode_index];

    // Clear blocks
    int block_count = inode->file_len / sb.block_count;
    if ((inode->file_len != 0) && (inode->file_len % sb.block_count == 0)) {
        --block_count;
    }
    for (int i = 0; i < block_count; ++i) {
        free_block(inode->blocks_addr[i]);
    }
    // Clear inode in array
    memset(&inode_table[inode_index], 0, sizeof(struct inode));

    inode_bitmap[inode_index] = 0;
    ++sb.free_inode_count;
}

ssize_t get_free_block_index() {
    for (int i = 0; i < sb.block_count; ++i) {
        if (block_bitmap[i] == 0) {
            block_bitmap[i] = 1;
            --sb.free_block_count;
            return i;
        }
    }
    return -1;
}

void free_block(int block_index) {
    // Clear block in memory
    fseek(fs, DATA_OFFSET + block_index * sb.block_size, SEEK_SET);
    char values[BLOCK_SIZE];
    memset(values, 0, sizeof(values));
    fwrite(&values[0], 1, sizeof(values), fs);

    block_bitmap[block_index] = 0;
    ++sb.free_block_count;
}

// Пишу data в конец файла, ассоциированного с inode_index
// Если не хватает блоков, добавляю
int write_to_file(char* data, int len, int inode_index) {
    struct inode* inode = &inode_table[inode_index];

    if (inode->file_len + len > sb.block_size * ADDR_COUNT) {
        fprintf(stdout, "write_to_file: inode address capacity is too small\n");
        return -1;
    }

    int remain = len;
    int addr_index = inode->file_len / sb.block_size;
    if (inode->file_len == 0) {
        ssize_t res;
        if ((res = get_free_block_index()) < 0) {
            fprintf(stdout, "write_to_file: no free blocks\n");
            return -1;
        }
        int new_block = res;
        inode->blocks_addr[0] = new_block;
    }
    if (inode->file_len % sb.block_size == 0 && inode->file_len != 0) {
        --addr_index;
    }

    while (remain > 0) {
        int block_filled_space = inode->file_len - addr_index * sb.block_size;
        int block_free_space = sb.block_size - block_filled_space;

        if (block_free_space == 0) {
            ssize_t res;
            if ((res = get_free_block_index()) < 0) {
                fprintf(stdout, "write_to_file: no free blocks\n");
                return -1;
            }
            int new_block = res;
            inode->blocks_addr[addr_index + 1] = new_block;
            ++addr_index;
            continue;
        }

        int to_write = ((remain < block_free_space) ? remain : block_free_space);

        int block_index = inode->blocks_addr[addr_index];

        fseek(fs, DATA_OFFSET + sb.block_size * block_index + block_filled_space, SEEK_SET);
        fwrite(data, 1, to_write, fs);

        data += to_write;
        remain -= to_write;
        inode->file_len += to_write;
    }

    return 0;
}

// Create root directory in first inode
int create_root() {
    struct dir_entry root_dir;
    strcpy(root_dir.name, "/");

    ssize_t res;
    if ((res = get_free_inode_index()) < 0) {
        fprintf(stdout, "create_root: no free inodes\n");
        return -1;
    }
    root_dir.inode_index = (uint16_t)res;
    assert(root_dir.inode_index == 0);

    struct inode* inode = &inode_table[root_dir.inode_index];
    inode->type = DIR;
    inode->file_len = 0;

    write_to_file((char*)&root_dir, sizeof(struct dir_entry), root_dir.inode_index);
    return 0;
}

int dump_info() {
    fseek(fs, SUPERBLOCK_OFFSET, SEEK_SET);
    // superblock
    fwrite(&sb, sizeof(struct superblock), 1, fs);

    fwrite(&block_bitmap[0], sizeof(int), sb.block_count, fs);
    fwrite(&inode_bitmap[0], sizeof(int), sb.inode_count, fs);
    fwrite(&inode_table[0], sb.inode_size, sb.inode_count, fs);
    return 0;
}

int load_info() {
    fseek(fs, SUPERBLOCK_OFFSET, SEEK_SET);
    fread(&sb, sizeof(struct superblock), 1, fs);
    fread(&block_bitmap[0], sizeof(int), sb.block_count, fs);
    fread(&inode_bitmap[0], sizeof(int), sb.inode_count, fs);
    fread(&inode_table[0], sb.inode_size, sb.inode_count, fs);
    return 0;
}

int fs_init(FILE* stream) {
    fs = stream;
    setbuf(fs, NULL);

    if (ftruncate(fileno(fs), DATA_OFFSET + BLOCK_COUNT * BLOCK_SIZE) != 0) {
        fprintf(stdout, "fs_init: unable to truncate file\n");
        return -1;
    }

    fseek(fs, 0, SEEK_END);
    assert(ftell(fs) == DATA_OFFSET + BLOCK_COUNT * BLOCK_SIZE);
    fseek(fs, 0, SEEK_SET);

    sb.block_count = BLOCK_COUNT;
    sb.inode_count = INODE_COUNT;
    sb.free_inode_count = INODE_COUNT;
    sb.free_block_count = BLOCK_COUNT;
    sb.block_size = BLOCK_SIZE;
    sb.inode_size = sizeof(struct inode);
    sb.magic_number = MAGIC_NUMBER;

    int buffer[SUPERBLOCK_OFFSET];
    memset(buffer, 0, sizeof(buffer));

    fseek(fs, 0, SEEK_SET);
    fwrite(&buffer[0], sizeof(int), sizeof(buffer) / sizeof(int), fs);

    memset(&block_bitmap[0], 0, sb.block_count * sizeof(int));
    memset(&inode_bitmap[0], 0, sb.inode_count * sizeof(int));
    memset(&inode_table[0], 0, sb.inode_size * sb.inode_count);

    create_root();

    dump_info();
    return 0;
}

int fs_open(FILE* stream) {
    fs = stream;
    setbuf(fs, NULL);
    load_info();
    return 0;
}

char* read_file(int inode_index) {
    struct inode* inode = &inode_table[inode_index];
    char* content = malloc(inode->file_len + 1);
    char* position = content;

    int remain = inode->file_len;
    for (int i = 0; remain > 0 && i < ADDR_COUNT; ++i) {
        int block_index = inode->blocks_addr[i];

        int to_read = (int)((remain < sb.block_size) ? remain : sb.block_size);

        fseek(fs, DATA_OFFSET + block_index * sb.block_size, SEEK_SET);
        fread(position, 1, to_read, fs);

        position += to_read;
        remain -= to_read;
    }

    content[inode->file_len] = '\0';
    return content;
}

ssize_t find_file(char* path) {
    int inode_index = 0;  // root inode index
    for (char* next = strtok(path, "/"); next != NULL; next = strtok(NULL, "/")) {
        struct dir_entry* dirs = (struct dir_entry*)read_file(inode_index);

        int dir_count = inode_table[inode_index].file_len / sizeof(struct dir_entry);

        int found = 0;
        for (int i = 0; i < dir_count; ++i) {
            if (strcmp(dirs[i].name, next) == 0) {
                inode_index = dirs[i].inode_index;
                found = 1;
            }
        }
        free(dirs);
        if (found) {
            continue;
        }
        return -1;  // not found
    }
    return inode_index;
}

int separate_path(const char* path) {
    size_t len = strlen(path);
    int result = -1;
    for (size_t i = 0; i < len; ++i) {
        if (path[i] == '/' && i + 1 != len) {
            result = i;
        }
    }
    return result;
}

int create_at(char* path, int type, char* content) {
    size_t len = strlen(path);
    int sep_index = separate_path(path);

    if (sep_index == -1) {
        fprintf(stdout, "create: wrong path\n");
        return -1;
    }

    char basepath[128];
    memset(basepath, 0, sizeof(basepath));
    if (sep_index != 0) {
        strncpy(basepath, path, sep_index);
    } else {
        // Start from root
        strcpy(basepath, "/");
    }

    char name[128];
    memset(name, 0, sizeof(name));
    strncpy(name, path + sep_index + 1, len - (sep_index + 1));

    ssize_t res;
    if ((res = find_file(basepath)) < 0) {
        fprintf(stdout, "create_at: basepath not found\n");
        return -1;
    }
    int parent_inode = res;

    if (inode_table[parent_inode].type != DIR) {
        fprintf(stdout, "create_at: not a directory\n");
        return -1;
    }

    // New directory entry (file or other directory)
    struct dir_entry new_entry;
    strcpy(new_entry.name, name);
    if ((res = get_free_inode_index()) < 0) {
        fprintf(stdout, "create_at: no free inodes\n");
        return -1;
    }
    new_entry.inode_index = res;

    struct inode* inode = &inode_table[new_entry.inode_index];
    inode->type = type;
    inode->file_len = 0;

    if (content != NULL) {
        write_to_file(content, (int)strlen(content), new_entry.inode_index);
    }

    // Save info about child to parent
    write_to_file((char*)&new_entry, sizeof(struct dir_entry), parent_inode);
    dump_info();
    return 0;
}

int remove_inode(int inode_index) {
    struct inode* inode = &inode_table[inode_index];

    if (inode->type == DIR) {
        struct dir_entry* dirs = (struct dir_entry*)read_file(inode_index);
        int dir_count = inode->file_len / sizeof(struct dir_entry);

        for (int i = 0; i < dir_count; ++i) {
            remove_inode(dirs[i].inode_index);
        }

        free(dirs);
    }

    free_inode(inode_index);
    return 0;
}

int remove_at(char* path) {
    size_t len = strlen(path);
    int sep_index = separate_path(path);

    if (sep_index == -1) {
        fprintf(stdout, "create: wrong path\n");
        return -1;
    }

    char basepath[128];
    memset(basepath, 0, sizeof(basepath));
    strncpy(basepath, path, sep_index);

    char name[128];
    memset(name, 0, sizeof(name));
    strncpy(name, path + sep_index + 1, len - (sep_index + 1));

    ssize_t res;
    if ((res = find_file(basepath)) < 0) {
        fprintf(stdout, "mkdir: basepath not found\n");
        return -1;
    }
    int parent_inode = res;

    struct inode* parent = &inode_table[parent_inode];

    if (parent->type != DIR) {
        fprintf(stdout, "remove: basepath is not a directory\n");
        return -1;
    }

    struct dir_entry* dirs = (struct dir_entry*)read_file(parent_inode);
    int dir_count = parent->file_len / sizeof(struct dir_entry);

    uint16_t inode_remove;
    int index = -1;
    for (int i = 0; i < dir_count; ++i) {
        if (strcmp(dirs[i].name, name) == 0) {
            inode_remove = dirs[i].inode_index;
            index = i;
            break;
        }
    }
    if (index == -1) {
        fprintf(stdout, "remove: file not found");
        return -1;
    }

    // Remove from parent list of records
    uint32_t new_len = parent->file_len - sizeof(struct dir_entry);

    memmove(&dirs[index], &dirs[index + 1],
            parent->file_len - (index + 1) * sizeof(struct dir_entry));
    memset(&dirs[dir_count - 1], 0, sizeof(struct dir_entry));

    int pos_in_block = parent->file_len % sb.block_size;
    if (pos_in_block < sizeof(struct dir_entry)) {
        int addr_index = parent->file_len / sb.block_size;
        int block_index = parent->blocks_addr[addr_index];
        free_block(block_index);
    }
    parent->file_len = 0;  // Write from beginning
    write_to_file((char*)dirs, new_len, parent_inode);
    parent->file_len = new_len;
    free(dirs);

    // Remove inode
    if (remove_inode(inode_remove) != 0) {
        fprintf(stdout, "remove: failed to remove inode\n");
        return -1;
    }

    dump_info();
    return 0;
}