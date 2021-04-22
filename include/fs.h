#pragma once

#include <stdint.h>
#include <stdio.h>

#define ADDR_COUNT 6
#define NAME_LEN 12

#define BLOCK_COUNT 64
#define BLOCK_SIZE 1024
#define INODE_COUNT 16
#define MAGIC_NUMBER 0xEF53

#define SUPERBLOCK_OFFSET 1024
#define BLOCK_BITMAP_OFFSET 1056
#define INODE_BITMAP_OFFSET 1120
#define INODE_TABLE_OFFSET 1136
#define DATA_OFFSET 8096

#define DIR 0
#define REG 1

struct inode {
    int type;
    int file_len;
    int blocks_addr[ADDR_COUNT];
};

struct inode* get_inode(int inode_index);

struct dir_entry {
    int inode_index;
    char name[NAME_LEN];
} __attribute__ ((aligned (16)));

struct superblock {
    uint32_t block_count;
    uint32_t inode_count;
    uint32_t free_block_count;
    uint32_t free_inode_count;
    uint32_t block_size;
    uint32_t inode_size;
    uint64_t magic_number;
};

ssize_t get_free_inode_index();
void free_inode(int inode_index);
ssize_t get_free_block_index();
void free_block(int block_index);

int create_root();

int dump_info();

int load_info();

int fs_init(FILE* stream);

int fs_open(FILE* stream);

// Пишу data в конец файла, ассоциированного с inode_index
// Если не хватает блоков, добавляю
int write_to_file(char* data, int len, int inode_index);

char* read_file(int inode_index);

ssize_t find_file(char* path);

int create_at(char* path, int type, char* content);

int remove_inode(int inode_index);

int remove_at(char* path);