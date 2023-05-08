#include <asm/page.h>
#include <asm/pgtable.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/highmem.h>

#define pte_to_phys(virt_addr, pte_value) ((virt_addr & ~PAGE_MASK) | (pte_value & PAGE_MASK))

#define __phys_to_page(paddr)	(pfn_to_page(PFN_DOWN(paddr)))

static size_t get_min_step_inside_page(size_t addr, size_t size);

static size_t get_process_page_addr(struct mm_struct *mm, size_t virt_addr, pte_t **pte);

static size_t read_pte2phy_addr(size_t virt_addr, char *buf, size_t len, pte_t *pte);

static size_t write_pte2phy_addr(size_t virt_addr, char *buf, size_t len, pte_t *pte);

static inline size_t get_min_step_inside_page(size_t addr, size_t size)
{
	return min(PAGE_SIZE - (addr & (PAGE_SIZE - 1)), size);
}

static size_t get_process_page_addr(struct mm_struct *mm, size_t virt_addr, pte_t **pte)
{
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	
	pgd = pgd_offset(mm, virt_addr);
	if (pgd_none(*pgd))
		return 0;
	
	p4d = p4d_offset(pgd, virt_addr);
	if (p4d_none(*p4d))
		return 0;
		
	pud = pud_offset(p4d, virt_addr);
	if (pud_none(*pud))
		return 0;
		
	pmd = pmd_offset(pud, virt_addr);
	if (pmd_none(*pmd))
		return 0;
	
	*pte = pte_offset_kernel(pmd, virt_addr);
	if (pte_none(**pte))
		return 0;
	
	return pte_val(**pte);
}

static size_t read_pte2phy_addr(size_t virt_addr, char *buf, size_t len, pte_t *pte)
{	
	size_t ret;
	void *kaddr;
	
	if (!pfn_valid(pte_pfn(*pte)))
		return 0;
	
	kaddr = __va(pte_to_phys(virt_addr, pte_val(*pte)) & 0x7fffffffffffffULL);

	
	ret = copy_to_user(buf, kaddr, len);

	return len;
}

static size_t write_pte2phy_addr(size_t virt_addr, char *buf, size_t len, pte_t *pte)
{
	size_t ret;
	void *baseaddr;

	if (!pfn_valid(pte_pfn(*pte)))
		return 0;
	
	baseaddr = __va(pte_to_phys(virt_addr, pte_val(*pte)) & 0x7fffffffffffffULL);

	ret = copy_from_user(baseaddr, buf, len);

	return len;
}
