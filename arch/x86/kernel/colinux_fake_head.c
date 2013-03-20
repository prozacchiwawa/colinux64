#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/memblock.h>
#include <linux/module.h>

#include <asm/setup.h>
#include <asm/tlbflush.h>
#include <asm/bios_ebda.h>
#include <asm/pgtable.h>

unsigned long alloc_start = 1 << 26;

unsigned long colinux_boot_alloc_page(void) {
    unsigned long result;
    alloc_start -= PAGE_SIZE;
    result = alloc_start;
    memset((void *)(result + __PAGE_OFFSET), 0, PAGE_SIZE);
    return result;
}

/* Set up the address space as it would be in colinux */
void colinux_create_mapping
(unsigned long phys, 
 unsigned long virt, 
 unsigned long cr3) {
    unsigned long *pml4_map, *pml3_map, *pml2_map, *pte_map;
    unsigned long pml4_entry = (virt >> (PAGE_SHIFT + (9 * 3))) & 0x1ff;
    unsigned long pml3_entry = (virt >> (PAGE_SHIFT + (9 * 2))) & 0x1ff;
    unsigned long pml2_entry = (virt >> (PAGE_SHIFT + 9)) & 0x1ff;
    unsigned long pt_entry = (virt >> PAGE_SHIFT) & 0x1ff;
    pml4_map = (unsigned long *)(cr3 + __PAGE_OFFSET);
    if (!pml4_map[pml4_entry]) {
        unsigned long new_pml3 = colinux_boot_alloc_page();
        pml4_map[pml4_entry] = new_pml3 | _PAGE_REALPHYS | _PAGE_PRESENT | _PAGE_RW;
    }
    pml3_map = (unsigned long *)((pml4_map[pml4_entry] & ~(PAGE_SIZE - 1)) + __PAGE_OFFSET);
    if (!pml3_map[pml3_entry]) {
        unsigned long new_pml2 = colinux_boot_alloc_page();
        pml3_map[pml3_entry] = new_pml2 | _PAGE_REALPHYS | _PAGE_PRESENT | _PAGE_RW;
    }
    pml2_map = (unsigned long *)((pml3_map[pml3_entry] & ~(PAGE_SIZE - 1)) + __PAGE_OFFSET);
    if (!pml2_map[pml2_entry]) {
        unsigned long new_pml1 = colinux_boot_alloc_page();
        pml2_map[pml2_entry] = new_pml1 | _PAGE_REALPHYS | _PAGE_PRESENT | _PAGE_RW;
    }
    pte_map = (unsigned long *)((pml2_map[pml2_entry] & ~(PAGE_SIZE - 1)) + __PAGE_OFFSET);
    pte_map[pt_entry] = phys | _PAGE_REALPHYS | _PAGE_PRESENT | _PAGE_RW;
}

void colinux_fake_head(void *early_boot_info) {
    int i;
    // Start from 1mb
    unsigned long pml4, cr3;
    unsigned long *pml4_map = (unsigned long *)swapper_pg_dir, *cr3_map;
    struct colinux_revmap_t *revmap;
    unsigned long direct_map_target;
    unsigned long remaining_map;
    unsigned long pml4_entry;
    unsigned long kernel_map;
    unsigned long local_apic_page;
    int kernel_pml4_entry = (KERNEL_IMAGE_START >> (PAGE_SHIFT + (9 + 3))) & 0x1ff;
    int direct_pml4_entry = (__PAGE_OFFSET >> (PAGE_SHIFT + (9 * 3))) & 0x1ff;
    struct boot_params *bootparams = (struct boot_params *)((char *)early_boot_info + __PAGE_OFFSET);
    struct e820entry *mapent = bootparams->e820_map;

    __asm__("mov %%cr3, %0" : "=r"(cr3));
    cr3_map = (unsigned long *)(__PAGE_OFFSET + cr3);
    
    pml4 = colinux_boot_alloc_page();
    pml4_map = (unsigned long *)(__PAGE_OFFSET + pml4);

    // Premap ffff880000000000 as a direct mapping of all physical memory
    direct_map_target = __PAGE_OFFSET;    
    remaining_map = 1 << 26;

    while (remaining_map) {
        colinux_create_mapping
            (direct_map_target - __PAGE_OFFSET, 
             direct_map_target, 
             pml4);
        direct_map_target += PAGE_SIZE;
        remaining_map -= PAGE_SIZE;
    }

    cr3_map[direct_pml4_entry] = pml4_map[direct_pml4_entry];

    // Make list of used pages
    // It's trivial here
    page_revmap_size = (1 << 26) >> PAGE_SHIFT;
    page_revmap = revmap = ((struct colinux_revmap_t *)(__PAGE_OFFSET + alloc_start)) - page_revmap_size;
    for (i = 0; i < 1 << 26; i += PAGE_SIZE, revmap++)
    {
        revmap->phys = i;
        revmap->virt = i + __PAGE_OFFSET;
    }
    
    alloc_start = (unsigned long)page_revmap - __PAGE_OFFSET;

    __flush_tlb_all();

    // Premap ffffffff80000000 as vmlinux
    for (kernel_map = 0;
         kernel_map < KERNEL_IMAGE_SIZE;
         kernel_map += PAGE_SIZE) {
        colinux_create_mapping
            (kernel_map,
             KERNEL_IMAGE_START + kernel_map,
             pml4);
    }

    cr3_map[kernel_pml4_entry] = pml4_map[kernel_pml4_entry];
    // Page table self-map
    cr3_map[kernel_pml4_entry+1] = cr3 | _PAGE_REALPHYS | _PAGE_RW | _PAGE_PRESENT;

    // local apic mapping
    local_apic_page = colinux_boot_alloc_page();
    colinux_create_mapping
        (local_apic_page, __PAGE_OFFSET + 0xfee00000, cr3);

    __flush_tlb_all();

    // Give the kernel only space we haven't used for kernel page tables
    // This is a bunch of extra memory, but it's a sunk cost in colinux
    // since we just want ordinary page space from windows.
    bootparams->e820_entries = 3;
    mapent->addr = alloc_start;
    mapent->size = (1 << 26) - alloc_start;
    mapent->type = E820_RESERVED;
    mapent++;
    mapent->addr = 0xa0000;
    mapent->size = 0x100000 - 0xa0000;
    mapent->type = E820_RESERVED;
    mapent++;
    mapent->addr = 0;
    mapent->size = alloc_start;
    mapent->type = E820_RAM;
    mapent++;
}
