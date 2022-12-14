#include "franklin/apic.h"
#include "d.h"
#include "franklin/69.h"
#include "franklin/acpi.h"
#include "franklin/interrupt.h"
#include "franklin/io.h"
#include "franklin/proc.h"
#include "franklin/spinlock.h"
#include "franklin/switch.h"
#include "franklin/time.h"

static void
init_timer(uint32_t*);

// right now its only getting the LUINT32_T pin that is connected to NMI
void
walk_madt(MADT* madt)
{

  uint8_t lapicId[100], x = 1;
  for (uint32_t i = 0; i < madt->h.length;) {
    if (madt->entry[i] == 0)
      lapicId[x++] = madt->entry[i + 3];
    if (madt->entry[i] == 4)
      NMI_LINT = madt->entry[i + 5];
    i += madt->entry[i + 1];
  }
}

void
init_apic(uint32_t* lapic)
{
  // set the correct LUINT32_T pin for NMI
  if (NMI_LINT == 1) {
    write32(lapic, LINT1, 1 << 10);
  } else {
    write32(lapic, LINT0, 1 << 10);
  };
  EOI = read32(lapic, EOI_REG);
  write32(lapic, TPR_REG, 0);
  write32(lapic, SPURIOUS_VECTOR, 0x1FF);

  init_timer(lapic);
}

static void
init_timer(uint32_t* lapic)
{
  static uint32_t ticks;
  static uint32_t configured;

  if (configured) // 1st CPU will configure the timer
    goto startimer;

  ticks = 100 * configure_timer(
                  lapic); // return ticks in 1ms, multiply by 100 to get 100ms
  configured = 1;

startimer:
  print("start lapic timer\n");
  write32(lapic, TIMER_REG, 34 | 1 << 17); // vector 34 and periodic mode
  write32(lapic, DIVIDE_REG, 0);
  write32(lapic, INITCOUNT, ticks);
}

extern void
scheduler(void);
extern struct proc* curproc;

void
apic_timer(struct regs* regs)
{

  struct proc* current;
  *EOI = 0;

  // lock needs to be held, because
  // the CPU might set state to runnable,
  // and then another CPU might run the process
  // this would result in 2 CPUs running on the same stack

  acq();
  current = get_current_proc();
  current->state = RUNNABLE;
  scheduler();
  r();

  /* release(&spinlock); */
}
