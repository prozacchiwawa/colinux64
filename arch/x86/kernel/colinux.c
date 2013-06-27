#include <linux/fuse.h> // stdint types
#include <linux/slab.h>
#include <asm/pgtable.h>
#include <linux/irqflags.h>
#include <linux/cooperative.h>
#include <linux/cooperative_internal.h>

int co_passage_page_holding_count;
extern void colinux_prim_set_flags(void *address, unsigned long mask, unsigned long value);

void co_passage_page_ref_up(void)
{
#ifdef ENABLE_PASSAGE_HOLDING_CHECK
  co_passage_page_holding_count++;
#endif
}

void co_passage_page_ref_down(void)
{
#ifdef ENABLE_PASSAGE_HOLDING_CHECK
  co_passage_page_holding_count--;
#endif
}

void co_passage_page_acquire(unsigned long *flags)
{
  local_irq_save(*flags);
  co_passage_page_ref_up();
}

void co_passage_page_release(unsigned long flags)
{
  co_passage_page_ref_down();
  local_irq_restore(flags);
}

co_message_t *co_send_message_save(unsigned long *flags)
{
	//co_passage_page_assert_valid();

    // unlock passage page
    colinux_prim_set_flags(co_passage_page, _PAGE_RW, _PAGE_RW);

	co_passage_page_acquire(flags);

	if (co_io_buffer->messages_waiting) {
		co_passage_page_release(*flags);
		return NULL;
	}

	co_passage_page->operation = CO_OPERATION_MESSAGE_TO_MONITOR;
	co_io_buffer->messages_waiting = 1;
	return ((co_message_t *)co_io_buffer->buffer);
}

void co_send_message_restore(unsigned long flags)
{
	co_switch_wrapper();
	co_passage_page_release(flags);
    // lock passage page
    colinux_prim_set_flags(co_passage_page, _PAGE_RW, 0);
}

int co_host_fpu_saved;
char co_host_fpu[0x200];

#define CO_FPU_SAVE(x)
#if 0
do \
  { \
 if (cpu_has_fxsr) \
   asm("fxsave " #x " ; fnclex"); \
 else \
   asm("fnsave " #x " ; fwait"); \
 } \
 while (0)
#endif

#define CO_FPU_RESTORE(x)
#if 0
do \
  { \
 if (cpu_has_fxsr) \
   asm("fxrstor " #x); \
 else \
   asm("frstor " #x); \
 } \
 while (0)
#endif

static void co_switch_wrapper_protected(void)
{
  if (co_host_fpu_saved) {
    CO_FPU_RESTORE(co_host_fpu);
    co_host_fpu_saved = 0;
  }

  /* And switch... */
  // Assume registers we modify in there and in our argument passing
  // convention are non-volatile.  I'll optimize later.
  __asm__("pushq %%rbx\n"
          "mov %0, %%rcx\n"
		  "mov %1, %%rsi\n"
		  "mov %2, %%rdi\n"
		  "callq *%%rcx\n"
		  "popq %%rbx\n"
		  : 
		  : 
		  "r"(co_passage_page->code),
		  "r"(&co_passage_page->linuxvm_state),
		  "r"(&co_passage_page->host_state)
		  : "%rsi", "%rdi", "%rcx", "%rdx", "%r8");
}

#define THREAD_SIZE 8192
#define STACK_WARN 0x100

void co_switch_wrapper(void)
{
  /* taken from irq.c: debugging check for stack overflow */
  long rsp;

  __asm__ __volatile__("and %%rsp,%0" : "=r" (rsp) : "0" (THREAD_SIZE - 1));
  co_switch_wrapper_protected();
}

void co_terminate_panic(const char *text, int len)
{
    unsigned long flags;
    co_message_t *msg = co_send_message_save(&flags);
    char *target = (char*)&co_passage_page->params[4], *end = target + len + 1;

    co_passage_page->operation = CO_OPERATION_TERMINATE;
    co_passage_page->params[0] = CO_TERMINATE_PANIC;
    co_passage_page->params[1] = 0;
    co_passage_page->params[2] = 0;
    co_passage_page->params[3] = len;
    while (target < end) *target++ = *text++;
    co_send_message_restore(flags);
    while (1);
}
