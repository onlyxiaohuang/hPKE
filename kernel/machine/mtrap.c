#include "kernel/riscv.h"
#include "kernel/process.h"
#include "spike_interface/spike_utils.h"
#include <string.h>

static void handle_instruction_access_fault() { panic("Instruction access fault!"); }

static void handle_load_access_fault() { panic("Load access fault!"); }

static void handle_store_access_fault() { panic("Store/AMO access fault!"); }


//lab1_challenge2
static void handle_illegal_instruction() { 
  sprint("");
  panic("Illegal instruction!"); 

}

static void handle_misaligned_load() { panic("Misaligned Load!"); }

static void handle_misaligned_store() { panic("Misaligned AMO!"); }

// added @lab1_3
static void handle_timer() {
  int cpuid = 0;
  // setup the timer fired at next time (TIMER_INTERVAL from now)
  *(uint64*)CLINT_MTIMECMP(cpuid) = *(uint64*)CLINT_MTIMECMP(cpuid) + TIMER_INTERVAL;

  // setup a soft interrupt in sip (S-mode Interrupt Pending) to be handled in S-mode
  write_csr(sip, SIP_SSIP);
}

//added @lab1_challenge2
char path[150],error_code[100010];
struct stat mystat;
void print_line(addr_line *line){
  int len = strlen(current -> dir[current -> file[line -> file].dir]);
  strcpy(path,current -> dir[current -> file[line -> file].dir]);
  path[len] = '/';
  strcpy(path + len + 1,current -> file[line->file].file);

//read the unique content from path
  spike_file_t *f = spike_file_open(path,O_RDONLY,0);
  spike_file_stat(f,&mystat);
  spike_file_read(f,error_code,mystat.st_size);
  spike_file_close(f);
  
  int offset = 0,cnt = 0;
  while(offset < mystat.st_size){
    int x = offset;
    while(x < mystat.st_size && error_code[x] != '\n') x ++;
    if(cnt == line -> line - 1){
      error_code[x] = '\0';
      sprint("Runtime error at %s:%d\n%s\n",path,line->line,error_code + offset);
      break;
    }
    else cnt ++,offset = x + 1;
  }

}

void print_error(){
  uint64 mepc = read_csr(mepc);
  for(int i = 0;i < current -> line_ind;i ++)
    if(mepc < current -> line[i].addr){
      print_line(current -> line + i - 1);
      break;
    }
}

//
// handle_mtrap calls a handling function according to the type of a machine mode interrupt (trap).
//
void handle_mtrap() {
  uint64 mcause = read_csr(mcause);
  switch (mcause) {
    case CAUSE_MTIMER:
      handle_timer();
      break;
    case CAUSE_FETCH_ACCESS:
      print_error();
      handle_instruction_access_fault();
      break;
    case CAUSE_LOAD_ACCESS:
      print_error();
      handle_load_access_fault();
    case CAUSE_STORE_ACCESS:
      print_error();
      handle_store_access_fault();
      break;
    case CAUSE_ILLEGAL_INSTRUCTION:
      // TODO (lab1_2): call handle_illegal_instruction to implement illegal instruction
      // interception, and finish lab1_2.
      //panic( "call handle_illegal_instruction to accomplish illegal instruction interception for lab1_2.\n" );
      print_error();
      handle_illegal_instruction();
      break;
    case CAUSE_MISALIGNED_LOAD:
      print_error();
      handle_misaligned_load();
      break;
    case CAUSE_MISALIGNED_STORE:
      print_error();
      handle_misaligned_store();
      break;

    default:
      sprint("machine trap(): unexpected mscause %p\n", mcause);
      sprint("            mepc=%p mtval=%p\n", read_csr(mepc), read_csr(mtval));
      panic( "unexpected exception happened in M-mode.\n" );
      break;
  }
}
