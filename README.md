# minifs extended - simple file system

Tiny model of ext2 file system (version 2.0). 
All file system data is stored in character device. 
Driver for this device is a Linux kernel module that defines `open`, 
`read`, `write` and `seek` operations. Its implementation
can be found in [`src/driver`](src/driver/minifs_driver.c).

**minifs** has a client-server architecture. 
Code for both parts is placed in corresponding folders 
under `/include` and `/src`.

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

Build and install kernel module:

```bash
cd src/driver
make all
sudo insmod minifs_driver.ko
```

Check module is installed:
```bash
lsmod
```

Uninstall module:
```bash
sudo rmmod minifs_driver.ko
```

Build client and server applications:

```bash
mkdir build; cd build
cmake ..
make
```

Run server:
```bash
./server [port = 8080]
```

Run client:
```bash
./client [ip = 127.0.0.1] [port = 8080]
```

## Examples:

(both server and client scripts are executed from `build` directory)

```bash
./server
```

```bash
./client
Welcome to MiniFS!
User id (for communication with server): 1
>>> ls /
/
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
>>> rmdir /dir1
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
}
>>> put /file3 outer.c
```

To kill server which is implemented as a daemon program, 
follow these steps:

1. List listening ports
```bash
ss -ltp
```
2. Find pid of `server` process in this list
3. Kill corresponding process
```bash
kill -INT <pid>
```
