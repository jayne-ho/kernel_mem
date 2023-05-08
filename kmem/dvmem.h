#ifndef DV_MEM_H_
#define DV_MEM_H_

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/sched/mm.h>
#include <linux/kallsyms.h>
#include <linux/kprobes.h>

#define DEV_FILE_NAME "dvmem"

struct mem_tool_device {
	struct cdev cdev;
	struct device *dev;
	int max;
};

struct mem_process_rw {
	pid_t pid;
	size_t virt_addr;
	void *base;
	size_t len;
};

struct vm_area_process {
    unsigned long start;
    unsigned long end;
    char perms[5];
    unsigned long offset;
    unsigned int major;
    unsigned int minor;
    unsigned long inode;
    char path[128];
};

#define MEMIOCTMAPC                                              _IOR('M', 0x2, int)
#define MEMIOCTMAP                                               _IOWR('M', 0x3, void *)

static struct mem_tool_device *memdev;

static struct mem_process_rw process_rw_data[1];

static dev_t mem_tool_dev_t;

static struct class *mem_tool_class;



static inline struct mm_struct *get_task_mm_by_vpid(pid_t nr);

static inline ssize_t mem_tool_rw_core(const char __user *buf, size_t (*mem_fn)(size_t , char *, size_t, pte_t *));



static int mem_tool_open(struct inode *inode, struct file *filp);

static int mem_tool_release(struct inode *inode, struct file *filp);

static ssize_t mem_tool_read(struct file *file, char __user *buf, size_t count, loff_t *pos);

static ssize_t mem_tool_write(struct file *file, const char __user *buf, size_t count, loff_t *pos);

static long mem_tool_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

static const struct file_operations mem_tool_fops = {
	.owner		= THIS_MODULE,
	.open           = mem_tool_open,
	.release        = mem_tool_release,
	.read           = mem_tool_read,
	.write          = mem_tool_write,
	.llseek         = no_llseek,
	.compat_ioctl   = mem_tool_ioctl,
	.unlocked_ioctl = mem_tool_ioctl,
};



static int user_custom_init(void);

static void user_custom_exit(void);

static int __init dev_mem_tool_init(void);

static void __exit dev_mem_tool_exit(void);

#endif // DV_MEM_H_
