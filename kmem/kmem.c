#include "kmem.h"
#include "mem.h"
#include "maps.h"

static inline struct mm_struct *get_task_mm_by_vpid(pid_t nr)
{
	struct task_struct *task;

	task = pid_task(find_vpid(nr), PIDTYPE_PID);
	if (!task)
		return NULL;
	
	return get_task_mm(task);
}

static inline ssize_t mem_tool_rw_core(const char __user *buf, size_t (*mem_fn)(size_t , char *, size_t, pte_t *))
{
	struct task_struct *task;
	struct mm_struct *mm;
	size_t max_page;
	size_t page_addr;
	size_t copied = 0;
	
	char *buffer;
	size_t virt_addr;
	size_t len;
	pte_t *pte = NULL;

	if (copy_from_user(process_rw_data, buf, sizeof(struct mem_process_rw)) != 0)
		return -EFAULT;
	
	task = pid_task(find_vpid(process_rw_data->pid), PIDTYPE_PID);
	if (!task)
		return -EFAULT;	

	mm = get_task_mm(task);
	if (!mm)
		return -EFAULT;
	
	virt_addr = process_rw_data->virt_addr;
	buffer = process_rw_data->base;
	len = process_rw_data->len;

	while (len > 0) {
		page_addr = get_process_page_addr(mm, virt_addr, &pte);
		max_page = get_min_step_inside_page(virt_addr, min(len, PAGE_SIZE));

		if (page_addr == 0)
			goto err_phy_addr_none;
	
		copied += mem_fn(virt_addr, buffer, max_page, pte);
		
	err_phy_addr_none:
		len -= max_page;
		buffer += max_page;
		virt_addr += max_page;
	
	}

	mmput(mm);
	return copied;
}

static int mem_tool_open(struct inode *inode, struct file *filp)
{
	filp->private_data = memdev;
	printk("success open file\n");
	return 0;
}

static int mem_tool_release(struct inode *inode, struct file *filp)
{
	printk("success close file\n");
	return 0;
}

static ssize_t mem_tool_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	return mem_tool_rw_core(buf, read_pte2phy_addr);
}

static ssize_t mem_tool_write(struct file *file, const char __user *buf, size_t count, loff_t *pos)
{	
	return mem_tool_rw_core(buf, write_pte2phy_addr);
}

static long mem_tool_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	switch (cmd) {
	
	case MEMIOCTMAPC:
		return get_process_maps_count(arg);

	case MEMIOCTMAP:
		if (copy_from_user(process_rw_data, (void *)arg, sizeof(struct mem_process_rw)) != 0)
			return -EFAULT;

		return get_process_maps(process_rw_data->pid, process_rw_data->base, process_rw_data->len);

	default:
		break;
	
	}
	return 0;
}

static int user_custom_init(void)
{
	return 1;
}

static void user_custom_exit(void)
{
}

static int __init dev_mem_tool_init(void)
{
	int ret;
		
	ret = alloc_chrdev_region(&mem_tool_dev_t, 0, 1, DEV_FILE_NAME);
	if (ret < 0) {
		printk("failed to allocate device numbers: %d\n", ret);
		return ret;
	}
	
	memdev = kmalloc(sizeof(struct mem_tool_device), GFP_KERNEL);
	if (!memdev) {
		printk("failed to allocate memory: %d\n", ret);
		goto done;
	}		
	
	mem_tool_class = class_create(THIS_MODULE, DEV_FILE_NAME);
	if (IS_ERR(mem_tool_class)) {
		printk("failed to create class: %d\n", ret);
		goto done;
	}
	memset(memdev, 0, sizeof(struct mem_tool_device));

	cdev_init(&memdev->cdev, &mem_tool_fops);
	memdev->cdev.owner = THIS_MODULE;
	memdev->cdev.ops = &mem_tool_fops;

	ret = cdev_add(&memdev->cdev, mem_tool_dev_t, 1);
	if (ret) {
		printk("failed to create device: %d\n", ret);
		goto done;
	}
	
	memdev->dev = device_create(mem_tool_class, NULL, mem_tool_dev_t, NULL, "%s", DEV_FILE_NAME);
	if (IS_ERR(memdev->dev)) {
		printk("failed to create device: %d\n", ret);
		goto done;
	}
	
	if (!user_custom_init()) {
		printk("failed to allocate page memory: %d\n", ret);
		goto done;
	}	
	
	printk("device create success %s\n", DEV_FILE_NAME);
	return 0;

done:
	return ret;
}

module_init(dev_mem_tool_init);

static void __exit dev_mem_tool_exit(void)
{
	device_destroy(mem_tool_class, mem_tool_dev_t);
	class_destroy(mem_tool_class);
	
	cdev_del(&memdev->cdev);
	kfree(memdev);
	unregister_chrdev_region(mem_tool_dev_t, 1);
	
	user_custom_exit();

	printk("device destory success %s\n", DEV_FILE_NAME);
}


module_exit(dev_mem_tool_exit);
MODULE_AUTHOR("Jayne");
MODULE_DESCRIPTION("通过内核读取物理内存");
MODULE_LICENSE("GPL");
