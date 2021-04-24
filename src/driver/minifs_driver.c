#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

#define MINIFS_BLOCK_SIZE 1024
#define DISK_SIZE 73632

static int minifs_open(struct inode*, struct file*);
static ssize_t minifs_read(struct file*, char __user*, size_t, loff_t*);
static ssize_t minifs_write(struct file*, const char __user*, size_t, loff_t*);
static loff_t minifs_llseek(struct file*, loff_t, int);

static const char disk_path[] = "minifs_disk";

static const struct file_operations minifs_fops = {.owner = THIS_MODULE,
                                                   .open = minifs_open,
                                                   .read = minifs_read,
                                                   .write = minifs_write,
                                                   .llseek = minifs_llseek};

static struct class* minifs_class;
static struct cdev minifs_cdev;
static dev_t minifs_dev;

static int minifs_uevent(struct device* dev, struct kobj_uevent_env* env) {
    add_uevent_var(env, "DEVMODE=%#o", 0666);
    return 0;
}

static int __init minifs_init(void) {
    minifs_class = class_create(THIS_MODULE, "minifs");
    minifs_class->dev_uevent = minifs_uevent;
    alloc_chrdev_region(&minifs_dev, 0, 1, "minifs");
    cdev_init(&minifs_cdev, &minifs_fops);
    minifs_cdev.owner = THIS_MODULE;
    cdev_add(&minifs_cdev, minifs_dev, 1);
    device_create(minifs_class, NULL, minifs_dev, NULL, "minifs");
    return 0;
}

static void __exit minifs_exit(void) {
    device_destroy(minifs_class, minifs_dev);
    class_unregister(minifs_class);
    unregister_chrdev_region(minifs_dev, 1);
}

static int minifs_open(struct inode* inode, struct file* file) {
    struct file* filp;
    char buf[MINIFS_BLOCK_SIZE];
    loff_t offset;
    int i;
    
    for (i = 0; i < MINIFS_BLOCK_SIZE; ++i) {
        buf[i] = 0;
    }

    filp = filp_open(disk_path, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
    for (i = 0; i * MINIFS_BLOCK_SIZE < DISK_SIZE; ++i) {
        offset = i * MINIFS_BLOCK_SIZE;
        kernel_write(filp, buf, MINIFS_BLOCK_SIZE, &offset);
    }
    filp_close(filp, NULL);

    return 0;
}

static ssize_t minifs_read(struct file* file, char __user* user, size_t count, loff_t* offset) {
    int bytes_read;
    struct file* filp;
    char buf[count];
    loff_t old_offset = *offset;
    
    if (*offset + count > DISK_SIZE) {
        count = DISK_SIZE - *offset;
    }
    filp = filp_open(disk_path, O_RDONLY, 0);
    bytes_read = kernel_read(filp, buf, count, offset);
    if (copy_to_user(user, buf, bytes_read)) {
        filp_close(filp, NULL);
        return -EFAULT;
    }
    filp_close(filp, NULL);

    *offset = old_offset + bytes_read;
    return bytes_read;
}

static ssize_t minifs_write(struct file* file, const char __user* user, size_t count,
                            loff_t* offset) {
    int bytes_written;
    struct file* filp;
    char buf[count];
    loff_t old_offset = *offset;
    
    if (*offset + count > DISK_SIZE) {
        count = DISK_SIZE - *offset;
    }
    if (copy_from_user(buf, user, count)) {
        return -EFAULT;
    }
    filp = filp_open(disk_path, O_WRONLY, 0);
    bytes_written = kernel_write(filp, buf, count, offset);
    filp_close(filp, NULL);

    *offset = old_offset + bytes_written;
    return bytes_written;
}

static loff_t minifs_llseek(struct file* filp, loff_t off, int whence) {
    loff_t newpos;
    switch (whence) {
        case 0: /* SEEK_SET */
            newpos = off;
            break;
        case 1: /* SEEK_CUR */
            newpos = filp->f_pos + off;
            break;
        case 2: /* SEEK_END */
            newpos = DISK_SIZE + off;
            break;
        default: /* can't happen */
            return -EINVAL;
    }
    if (newpos < 0) {
        return -EINVAL;
    }
    filp->f_pos = newpos;
    return newpos;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("justsolveit@yandex.ru");
MODULE_DESCRIPTION("Module for minifs");

module_init(minifs_init);
module_exit(minifs_exit);