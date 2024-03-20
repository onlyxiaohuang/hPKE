/*
 * contains the implementation of all syscalls.
 */

#include <stdint.h>
#include <errno.h>

#include "memlayout.h"
#include "util/types.h"
#include "syscall.h"
#include "string.h"
#include "process.h"
#include "util/functions.h"
#include "pmm.h"
#include "vmm.h"
#include "spike_interface/spike_utils.h"

//
// implement the SYS_user_print syscall
//
ssize_t sys_user_print(const char* buf, size_t n) {
  // buf is now an address in user space of the given app's user stack,
  // so we have to transfer it into phisical address (kernel is running in direct mapping).
  assert( current );
  char* pa = (char*)user_va_to_pa((pagetable_t)(current->pagetable), (void*)buf);
  sprint(pa);
  return 0;
}

//
// implement the SYS_user_exit syscall
//
ssize_t sys_user_exit(uint64 code) {
  sprint("User exit with code:%d.\n", code);
  // in lab1, PKE considers only one app (one process). 
  // therefore, shutdown the system when the app calls exit()
  shutdown(code);
}

//
// maybe, the simplest implementation of malloc in the world ... added @lab2_2
//
int init = 1;//xinhao
uint64 sys_user_allocate_page(uint64 n) {
  //changed @ lab2_challenge2
  //the linklist of allocating memory
  if(init){
    current -> heap_size = USER_FREE_ADDRESS_START;
    user_vm_malloc(current -> pagetable,current -> heap_size,sizeof(HMCB) + current -> heap_size);
    current -> heap_size += sizeof(HMCB);
    HMCB *head = (HMCB *) PTE2PA(*page_walk(current -> pagetable,current -> heap_size,0));
    current -> heap_head = current -> heap_tail = (uint64) head;
    head -> size = 0,head -> ne = head;
    init = 0;
  }

  HMCB *u = (HMCB *)current -> heap_head;
  do{
    if(!u -> busy && u -> size >= n){
      u -> busy = 1;
      return u -> offset + sizeof(HMCB);
    }
    u = u -> ne;//search for the next
  }while(u != (HMCB *) current -> heap_tail);

  uint64 allocn = (uint64) sizeof(HMCB) + n + 8;
  uint64 size_ = current -> heap_size;
  user_vm_malloc(current -> pagetable,current -> heap_size,allocn + current -> heap_size);
  current -> heap_size += allocn;
  HMCB *now = (HMCB *) (PTE2PA (*page_walk(current -> pagetable,size_,0)) + (size_ & 0xfff));
  now = (HMCB *)((uint64)now + (8 - ((uint64)now % 8))% 8);

  now -> busy = 1;
  now -> offset = size_;
  now -> size = n;
  now -> ne = u -> ne;

  u -> ne = now;
  u = (HMCB *) current -> heap_head;
  return size_ + sizeof(HMCB);
  /*
  void* pa = alloc_page();
  uint64 va = g_ufree_page;
  g_ufree_page += PGSIZE;
  user_vm_map((pagetable_t)current->pagetable, va, PGSIZE, (uint64)pa,
         prot_to_type(PROT_WRITE | PROT_READ, 1));
  */
//  return va;
}

//
// reclaim a page, indicated by "va". added @lab2_2
//
uint64 sys_user_free_page(uint64 va) {
  pte_t *pte = page_walk(current -> pagetable,va - sizeof(HMCB),0);
  HMCB *now = (HMCB *)(PTE2PA(*pte) + ((va - sizeof(HMCB)) & 0xfff));
  now = (HMCB *) ((uint64) now + (8 - ((uint64) now % 8)) % 8);
  now -> busy = 0;
//  user_vm_unmap((pagetable_t)current->pagetable, va, PGSIZE, 1);
  return 0;
}

//
// [a0]: the syscall number; [a1] ... [a7]: arguments to the syscalls.
// returns the code of success, (e.g., 0 means success, fail for otherwise)
//
long do_syscall(long a0, long a1, long a2, long a3, long a4, long a5, long a6, long a7) {
  switch (a0) {
    case SYS_user_print:
      return sys_user_print((const char*)a1, a2);
    case SYS_user_exit:
      return sys_user_exit(a1);
    // added @lab2_2
    case SYS_user_allocate_page:
      return sys_user_allocate_page(a1);
    case SYS_user_free_page:
      return sys_user_free_page(a1);
    default:
      panic("Unknown syscall %ld \n", a0);
  }
}
