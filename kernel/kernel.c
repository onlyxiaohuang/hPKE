/*
 * Supervisor-mode startup codes
 */

#include "riscv.h"
#include "string.h"
#include "elf.h"
#include "process.h"

#include "spike_interface/spike_utils.h"

// process is a structure defined in kernel/process.h
process user_app[NCPU];

//
// load the elf, and construct a "process" (with only a trapframe).
// load_bincode_from_host_elf is defined in elf.c
//
void load_user_program(process *proc) {
  // USER_TRAP_FRAME is a physical address defined in kernel/config.h
  proc -> hartid = read_tp();
  if(proc -> hartid == 0){
      proc->trapframe = (trapframe *)USER_TRAP_FRAME;
      memset(proc->trapframe, 0, sizeof(trapframe));
      // USER_KSTACK is also a physical address defined in kernel/config.h
      proc ->kstack = USER_KSTACK ;
      proc -> trapframe -> regs.sp = USER_STACK;
  }
  else {
      proc->trapframe = (trapframe *)0x85300000;
      memset(proc->trapframe, 0, sizeof(trapframe));
      // USER_KSTACK is also a physical address defined in kernel/config.h
      proc->kstack = 0x85100000;
      proc->trapframe->regs.sp = 0x85200000;
  }


  // changed @lab1_challenge3
//  proc->dtb = dtb;
  //proc->hartid = hartid;
  //proc->trapframe->epc = dtb;
  //sprint("%lx\n",proc -> trapframe -> epc);
  // load_bincode_from_host_elf() is defined in kernel/elf.c
  load_bincode_from_host_elf(proc);
}

//
// s_start: S-mode entry point of riscv-pke OS kernel.
//
int s_start(void) {
  
  // Note: we use direct (i.e., Bare mode) for memory mapping in lab1.
  // which means: Virtual Address = Physical Address
  // therefore, we need to set satp to be 0 for now. we will enable paging in lab2_x.
  // 
  // write_csr is a macro defined in kernel/riscv.h

  //changed @ lab1_challenge3 :
  //use the code to compute hartid and dtb;
  unsigned hartid = read_tp();
  //unsigned long long dtb = 0x0000000081000000ull + hartid * 0x04000000ull;
  //sprint("%x %llx\n",hartid,dtb);
  sprint("hartid = %d: Enter supervisor mode...\n",hartid);
  write_csr(satp, 0);

  // the application code (elf) is first loaded into memory, and then put into execution
  load_user_program(&user_app[hartid]);

  sprint("hartid = %d: Switch to user mode...\n",hartid);
  // switch_to() is defined in kernel/process.c
  switch_to(&user_app[hartid]);

  // we should never reach here.
  return 0;
}
