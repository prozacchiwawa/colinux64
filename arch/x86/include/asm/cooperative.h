/*
 *  linux/include/asm/cooperative.h
 *
 *  Copyright (C) 2004 Dan Aloni
 *
 *  This file defines the lower level interfaces between the Cooperative Linux
 *  kernel and the host OS driver. It's for both external inclusion from the
 *  and internal inclusion in the kernel sources.
 */

#ifndef __LINUX_ASM_COOPERATIVE_H__
#define __LINUX_ASM_COOPERATIVE_H__

#pragma pack(1)
typedef struct {
	unsigned short size;
	struct x86_idt_entry *table;
} x86_idt_t;

typedef struct {
	unsigned short limit;
	struct x86_dt_entry *base;
} x86_gdt_t;
#pragma pack()

typedef struct {
    unsigned long phys;
    void *virt;
} colinux_revmap_t;

#pragma pack(1)
#ifndef __x86_64__
typedef struct {
	unsigned char border2[0x4];

	uint32_t cs;
        #define CO_ARCH_STATE_STACK_CS "0x04"

	uint32_t ds;
        #define CO_ARCH_STATE_STACK_DS "0x08"

	uint32_t es;
        #define CO_ARCH_STATE_STACK_ES "0x0C"

	uint32_t cr3;
        #define CO_ARCH_STATE_STACK_CR3 "0x10"

	uint32_t cr4;
        #define CO_ARCH_STATE_STACK_CR4 "0x14"

	uint32_t cr2;
        #define CO_ARCH_STATE_STACK_CR2 "0x18"

	uint32_t cr0;
        #define CO_ARCH_STATE_STACK_CR0 "0x1C"

	x86_gdt_t gdt;
        #define CO_ARCH_STATE_STACK_GDT "0x20"

	uint32_t fs;
        #define CO_ARCH_STATE_STACK_FS  "0x26"

	uint32_t gs;
        #define CO_ARCH_STATE_STACK_GS  "0x2A"

	unsigned short ldt;
        #define CO_ARCH_STATE_STACK_LDT "0x2E"

	x86_idt_t idt;
        #define CO_ARCH_STATE_STACK_IDT "0x30"

	unsigned short tr;
        #define CO_ARCH_STATE_STACK_TR  "0x36"

	uint32_t return_eip;
        #define CO_ARCH_STATE_STACK_RETURN_EIP  "0x38"

	uint32_t flags;
        #define CO_ARCH_STATE_STACK_FLAGS "0x3C"

	uint32_t esp;
        #define CO_ARCH_STATE_STACK_ESP "0x40"

	uint32_t ss;
        #define CO_ARCH_STATE_STACK_SS "0x44"

	uint32_t dr0;
        #define CO_ARCH_STATE_STACK_DR0 "0x48"

	uint32_t dr1;
        #define CO_ARCH_STATE_STACK_DR1 "0x4C"

	uint32_t dr2;
        #define CO_ARCH_STATE_STACK_DR2 "0x50"

	uint32_t dr3;
        #define CO_ARCH_STATE_STACK_DR3 "0x54"

	uint32_t dr6;
        #define CO_ARCH_STATE_STACK_DR6 "0x58"

	uint32_t dr7;
        #define CO_ARCH_STATE_STACK_DR7 "0x5C"

	union {
		uint32_t temp_cr3;
		uint32_t other_map;
	};

        #define CO_ARCH_STATE_STACK_TEMP_CR3 "0x60"
        #define CO_ARCH_STATE_STACK_OTHERMAP "0x60"

	uint32_t relocate_eip;
        #define CO_ARCH_STATE_STACK_RELOCATE_EIP "0x64"

	uint32_t pad1;
        #define CO_ARCH_STATE_STACK_RELOCATE_EIP_AFTER "0x68"

	uint32_t va;
        #define CO_ARCH_STATE_STACK_VA "0x6C"

	uint32_t sysenter_cs;
        #define CO_ARCH_STATE_SYSENTER_CS "0x70"

	uint32_t sysenter_esp;
        #define CO_ARCH_STATE_SYSENTER_ESP "0x74"

	uint32_t sysenter_eip;
        #define CO_ARCH_STATE_SYSENTER_EIP "0x78"
} co_arch_state_stack_t;
#else
typedef struct {
	unsigned char border2[0x4];

	uint32_t cs;
        #define CO_ARCH_STATE_STACK_CS "0x04"

	uint64_t efer;
	#define CO_ARCH_STATE_MSR_EFER "0x08"

	uint64_t cr3;
        #define CO_ARCH_STATE_STACK_CR3 "0x10"

	uint64_t cr4;
        #define CO_ARCH_STATE_STACK_CR4 "0x18"

	uint64_t cr2;
        #define CO_ARCH_STATE_STACK_CR2 "0x20"

	uint64_t cr0;
        #define CO_ARCH_STATE_STACK_CR0 "0x28"

	uint64_t fs_base;
	#define CO_ARCH_STATE_STACK_FSBASE "0x30"

	uint64_t return_eip;
        #define CO_ARCH_STATE_STACK_RETURN_EIP  "0x38"

	uint64_t flags;
        #define CO_ARCH_STATE_STACK_FLAGS "0x40"

	uint64_t esp;
        #define CO_ARCH_STATE_STACK_ESP "0x48"

	uint64_t gs_base;
        #define CO_ARCH_STATE_STACK_GSBASE "0x50"

	uint64_t dr0;
        #define CO_ARCH_STATE_STACK_DR0 "0x58"

	uint64_t dr1;
        #define CO_ARCH_STATE_STACK_DR1 "0x60"

	uint64_t dr2;
        #define CO_ARCH_STATE_STACK_DR2 "0x68"

	uint64_t dr3;
        #define CO_ARCH_STATE_STACK_DR3 "0x70"

	uint64_t dr6;
        #define CO_ARCH_STATE_STACK_DR6 "0x78"

	uint64_t dr7;
        #define CO_ARCH_STATE_STACK_DR7 "0x80"

	uint64_t temp_cr3;
        #define CO_ARCH_STATE_STACK_TEMP_CR3 "0x88"
	uint64_t other_map;
        #define CO_ARCH_STATE_STACK_OTHERMAP "0x90"

	uint64_t relocate_eip;
        #define CO_ARCH_STATE_STACK_RELOCATE_EIP "0x98"
        #define CO_ARCH_STATE_STACK_RELOCATE_EIP_AFTER "0xA0"

	uint64_t va;
        #define CO_ARCH_STATE_STACK_VA "0xA0"

	uint32_t sysenter_cs;
        #define CO_ARCH_STATE_SYSENTER_CS "0xA8"

	uint32_t sysenter_esp;
        #define CO_ARCH_STATE_SYSENTER_ESP "0xAC"

	uint32_t sysenter_eip;
        #define CO_ARCH_STATE_SYSENTER_EIP "0xB0"

	uint32_t gs;
	#define CO_ARCH_STATE_STACK_GS "0xB4"

	uint64_t star;
	#define CO_ARCH_STATE_MSR_STAR "0xB8"

	uint64_t lstar;
	#define CO_ARCH_STATE_MSR_LSTAR "0xC0"

	uint64_t cstar;
	#define CO_ARCH_STATE_MSR_CSTAR "0xC8"

	uint64_t sfmask;
        #define CO_ARCH_STATE_MSR_SFMASK "0xD0"

	uint64_t kernelgsbase;
	#define CO_ARCH_STATE_STACK_KERNELGSBASE "0xD8"

	uint32_t ss;
	#define CO_ARCH_STATE_STACK_SS "0xE0"

	uint32_t fs;
	#define CO_ARCH_STATE_STACK_FS "0xE4"

	x86_gdt_t gdt;
        #define CO_ARCH_STATE_STACK_GDT "0xE8"

	unsigned short ldt;
        #define CO_ARCH_STATE_STACK_LDT "0xF2"

	x86_idt_t idt;
        #define CO_ARCH_STATE_STACK_IDT "0xF4"

	unsigned short tr;
        #define CO_ARCH_STATE_STACK_TR  "0xFE"

} co_arch_state_stack_t;
#endif
#pragma pack()

#define CO_MAX_PARAM_SIZE 0x400

typedef void (*win_panic_fun_t)(uint32_t code, uint64_t a, uint64_t b, uint64_t c, uint64_t d);

typedef struct co_arch_passage_page {
	union {
		struct {
			void *self_physical_address;
			uint64_t temp_pgd_physical;
			unsigned char code[0x400];
			
			/* Machine states */
			
			/*
			 * NOTE: *_state fields must be aligned at 16 bytes boundary since
			 * the fxsave/fxload instructions expect an aligned arugment.
			 */
			
			co_arch_state_stack_t host_state;
			co_arch_state_stack_t linuxvm_state;
			
			/* Control parameters */
			unsigned long operation;
			void *params[];
		};
		char first_page[0x1000];
	};
} co_arch_passage_page_t;

/*
 * Address space layout:
 */

#ifdef __x86_64__
#define CO_VPTR_PHYS                         (0xffff880000000000ull)
#define CO_VPTR_BASE                         (0xffffffff80000000ull)
#else
#define CO_VPTR_BASE                         (0xffc00000UL)
#endif
#define CO_VPTR_IO_AREA_SIZE                 (0x10000UL)
#define CO_VPTR_IO_AREA_START                (CO_VPTR_BASE - 0x1200000UL)

#define CO_LOWMEMORY_MAX_MB 984

#pragma pack(1)
typedef struct {
	uint32_t kernel_cs;
	uint32_t kernel_ds;
} co_arch_info_t;

typedef struct {
	void *idt_entries[16];
} co_arch_basic_idt_t;
#pragma pack()
#endif
