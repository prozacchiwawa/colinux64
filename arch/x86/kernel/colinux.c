#include <linux/fuse.h> // stdint types
#include <linux/irqflags.h>
#include <linux/cooperative.h>

long co_passage_page_holding_count;

static inline void co_passage_page_ref_up(void)
{
#ifdef ENABLE_PASSAGE_HOLDING_CHECK
  co_passage_page_holding_count++;
#endif
}

static inline void co_passage_page_ref_down(void)
{
#ifdef ENABLE_PASSAGE_HOLDING_CHECK
  co_passage_page_holding_count--;
#endif
}

static inline void co_passage_page_acquire(unsigned long *flags)
{
  local_irq_save(flags);
  co_passage_page_ref_up();
}

static inline void co_passage_page_release(unsigned long flags)
{
  co_passage_page_ref_down();
  local_irq_restore(flags);
}

#define cpu_has_fxsr 1
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
		  "mov %1, %%rdx\n"
		  "mov %2, %%r8\n"
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
  char *target = (char*)&co_passage_page->params[4], *end = target + len + 1;
  unsigned long flags;

  co_passage_page_acquire(&flags);
  co_passage_page->operation = CO_OPERATION_TERMINATE;
  co_passage_page->params[0] = CO_TERMINATE_PANIC;
  co_passage_page->params[1] = 0;
  co_passage_page->params[2] = 0;
  co_passage_page->params[3] = len;
  while (target < end) *target++ = *text++;
  co_switch_wrapper();
  while(1);
}
