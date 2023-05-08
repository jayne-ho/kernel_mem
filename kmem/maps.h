#include <linux/mm.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/fs_struct.h>
#include <linux/mount.h>
#include <linux/ptrace.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/sched/mm.h>

#include "kmem.h"

const char *__arch_vma_name(struct vm_area_struct *vma)
{
	if (vma->vm_mm && (vma->vm_start == (long)vma->vm_mm->context.vdso))
		return "[vdso]";
	if (vma->vm_mm && (vma->vm_start ==
			   (long)vma->vm_mm->context.vdso + PAGE_SIZE))
		return "[vdso_data]";
	return NULL;
}

static const char __user *__vma_get_anon_name(struct vm_area_struct *vma)
{
	if (vma->vm_file)
		return NULL;

	return vma->anon_name;
}

static void __strcat_s(char *__dest, const char *__src, size_t *p)
{
	int __n;

	__n = min(strlen(__src), *p);
	strncat(__dest, __src, __n);
	*p -= __n;
}

static void __strncat_s(char *__dest, const char *__src, size_t __n, size_t *p)
{
	__n = min(__n, *p);
	strncat(__dest, __src, __n);
	*p -= __n;
}

static void __get_vma_name(char *path, size_t n, struct vm_area_struct *vma)
{
	const char __user *name = __vma_get_anon_name(vma);
	struct mm_struct *mm = vma->vm_mm;

	unsigned long page_start_vaddr;
	unsigned long page_offset;
	unsigned long num_pages;
	unsigned long max_len = NAME_MAX;
	int i;

	page_start_vaddr = (unsigned long)name & PAGE_MASK;
	page_offset = (unsigned long)name - page_start_vaddr;
	num_pages = DIV_ROUND_UP(page_offset + max_len, PAGE_SIZE);
	n -= 1;
	__strcat_s(path, "[anon:", &n);

	for (i = 0; i < num_pages; i++) {
		int len;
		int write_len;
		const char *kaddr;
		long pages_pinned;
		struct page *page;

		pages_pinned = get_user_pages_remote(current, mm,
				page_start_vaddr, 1, 0, &page, NULL, NULL);
		if (pages_pinned < 1) {
			__strcat_s(path, "<fault>]", &n);
			return;
		}

		kaddr = (const char *)kmap(page);
		len = min(max_len, PAGE_SIZE - page_offset);
		write_len = strnlen(kaddr + page_offset, len);

		__strncat_s(path, kaddr + page_offset, write_len, &n);
		kunmap(page);
		put_page(page);

		// if strnlen hit a null terminator then we're done 
		if (write_len != len)
			break;

		max_len -= len;
		page_offset = 0;
		page_start_vaddr += PAGE_SIZE;
	}

	__strcat_s(path, "]", &n);
} // copy from mi kernel.... and edit something
 // i donot care size

static int is_stack(struct vm_area_struct *vma);

static void get_process_map_vma(struct vm_area_struct *vma, struct vm_area_process *area);

static size_t get_process_maps(pid_t nr, struct vm_area_process *start, long len);

static size_t get_process_maps_count(pid_t nr);

static int is_stack(struct vm_area_struct *vma)
{
	return vma->vm_start <= vma->vm_mm->start_stack &&
		vma->vm_end >= vma->vm_mm->start_stack;
}

static void get_process_map_vma(struct vm_area_struct *vma, struct vm_area_process *area)
{
	struct mm_struct *mm = vma->vm_mm;
	struct file *file = vma->vm_file;
	vm_flags_t flags = vma->vm_flags;
	unsigned long ino = 0;
	unsigned long long pgoff = 0;
	unsigned long start, end;
	dev_t dev = 0;
	const char *name = NULL;
	
	start = vma->vm_start;
	end = vma->vm_end;
	memset(area->path, 0, sizeof(area->path));
	
	if (file) {
		struct inode *inode = file_inode(vma->vm_file);
		dev = inode->i_sb->s_dev;
		ino = inode->i_ino;
		pgoff = ((loff_t)vma->vm_pgoff) << PAGE_SHIFT;
		name = d_path(&file->f_path, area->path, sizeof(area->path));
		goto done;
	}
	
	if (vma->vm_ops && vma->vm_ops->name) {
		name = vma->vm_ops->name(vma);
		if (name)
			goto done;
	}

	name = __arch_vma_name(vma);
	if (!name) {
		if (!mm) {
			name = "[vdso]";
			goto done;
		}

		if (vma->vm_start <= mm->brk &&
		    vma->vm_end >= mm->start_brk) {
			name = "[heap]";
			goto done;
		}

		if (is_stack(vma)) {
			name = "[stack]";
			goto done;
		}
		
		if (__vma_get_anon_name(vma)) {
			__get_vma_name(area->path, sizeof(area->path), vma);
			goto done;
		}
	}
	
done:
	if (name)
		memcpy(area->path, name, sizeof(area->path));
	
	area->start = start;
	area->end = end;
	area->perms[0] = flags & VM_READ ? 'r' : '-';
	area->perms[1] = flags & VM_WRITE ? 'w' : '-',
	area->perms[2] = flags & VM_EXEC ? 'x' : '-';
	area->perms[3] = flags & VM_MAYSHARE ? flags & VM_SHARED ? 'S' : 's' : 'p';
	area->perms[4] = 0;
	area->offset = pgoff;
	area->major = MAJOR(dev);
	area->minor = MINOR(dev);
	area->inode = ino;
}

static size_t get_process_maps(pid_t nr, struct vm_area_process *start, long len)
{
	size_t count = 0;
	struct mm_struct *mm;
	struct vm_area_struct *vma;
	struct vm_area_process vmp;

	mm = get_task_mm_by_vpid(nr);
	if (!mm)
		return 0;
	
	down_read(&mm->mmap_sem);  // mmap_lock
	for (vma = mm->mmap; vma && len > 0; vma = vma->vm_next, len--) {
		get_process_map_vma(vma, &vmp);
		if (copy_to_user((void *)(start + count), &vmp, sizeof(struct vm_area_process)) == 0)
			count++;
	}
	up_read(&mm->mmap_sem);

	mmput(mm);
	return count;
}

static size_t get_process_maps_count(pid_t nr)
{
	int count;
	struct mm_struct *mm;

	mm = get_task_mm_by_vpid(nr);
	if (!mm)
		return 0;
	
	down_read(&mm->mmap_sem);
	count = mm->map_count;
	up_read(&mm->mmap_sem);

	mmput(mm);
	return count;
}
