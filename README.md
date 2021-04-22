# minifs - simple file system

Tiny model of ext2 file system. You can specify file to store minifs data by script argument. Default value - `filesystem`. If script stops, all data is saved in this file and can be restored by next execution. 

**Note:** all paths should start with `'/'`. Initially only `'/'` path exists.

## Available commands:

- `touch <path>` — creates empty file at path
- `mkdir <path>` — creates empty directory at path
- `ls <path>` — lists all files in directory at path
- `rm <path>` — remove file at path
- `rmdir <path>` — remove directory at path
- `get <global_path> <minifs_path>` — copy file from outer file system to minifs
- `put <minifs_path> <global_path>` — copy file from minifs to outer file system
- `cat <path>` — output file at path

## How to build & execute

CMake/Pure Makefile

```bash
mkdir build; cd build
cmake ..
make
./minifs
```

## Examples:

(script started at build directory)

```bash
Welcome to MiniFS!
>>> touch /file1
>>> touch /file2
>>> mkdir /dir1
>>> ls /
/
file1
file2
dir1
>>> mkdir /dir1/dir2
>>> touch /dir1/dir2/file3
>>> mkdir /dir1/dir2/dir3
>>> ls /dir1/dir2
file3
dir3
>>> rm /dir1
>>> ls /
/
file1
file2
>>> get ../test_get_put.txt /file3
>>> ls /
/
file1
file2
file3
>>> cat /file3
#include <stdio.h>
#include <stdint.h>

struct dir_enitry {
    uint16_t inode_index;
    char name[14];
} __attribute__((__packed__));

int main() {
    printf("%ld\n", sizeof(struct dir_entry));
    printf("0x%01X\n", 'a');
    printf("0x%01X\n", 'b');
    return 0;
}>>> put /file3 outer.c
```

