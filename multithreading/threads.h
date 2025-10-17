#ifndef __THREADS__
#define __THREADS__

enum JBL
  {
   JBL_RBX = 0,
   JBL_RBP = 1,
   JBL_R12 = 2, 
   JBL_R13 = 3,
   JBL_R14 = 4,
   JBL_R15 = 5,
   JBL_RSP = 6,
   JBL_PC  = 7
  };

/* Demangle a jump buffer pointer for linux system used for testing */
static unsigned long int _ptr_demangle(unsigned long int p)
{
    unsigned long int ret;

    asm("movq %1, %%rax;\n"
        "rorq $0x11, %%rax;"
        "xorq %%fs:0x30, %%rax;"
        "movq %%rax, %0;"
    : "=r"(ret)
    : "r"(p)
    : "%rax"
    );
    return ret;
}

/* Mangle a jump buffer pointer for linux system used for testing */
static unsigned long int _ptr_mangle(unsigned long int p)
{
    unsigned long int ret;

    asm("movq %1, %%rax;\n"
        "xorq %%fs:0x30, %%rax;"
        "rolq $0x11, %%rax;"
        "movq %%rax, %0;"
    : "=r"(ret)
    : "r"(p)
    : "%rax"
    );
    return ret;
}

static void set_reg(jmp_buf *buf, enum JBL reg, unsigned long int val)
{
  switch (reg) {
  case JBL_RBX:
  case JBL_RBP:
  case JBL_RSP:
  case JBL_PC:
    (*buf)->__jmpbuf[reg] = _ptr_mangle(val);
    return; 
  case JBL_R12:
  case JBL_R13:
  case JBL_R14:
  case JBL_R15:
    (*buf)->__jmpbuf[reg] = val;
    return;
  }
  assert(0);
}

static unsigned long int get_reg(jmp_buf *buf, enum JBL reg)
{
  switch (reg) {
  case JBL_RBX:
  case JBL_RBP:
  case JBL_RSP:
  case JBL_PC:
    return _ptr_demangle((*buf)->__jmpbuf[reg]);
  case JBL_R12:
  case JBL_R13:
  case JBL_R14:
  case JBL_R15:
    return (*buf)->__jmpbuf[reg];
  }
  assert(0);
  return -1;
}

static void *start_thunk() {
  asm("popq %%rbp;\n"           //clean up the function prologue
      "movq %%r13, %%rdi;\n"    //put arg in $rdi
      "pushq %%r12;\n"          //push &start_routine
      "retq;\n"                 //return to &start_routine
      :
      :
      : "%rdi"
  );
  __builtin_unreachable();
}

static unsigned long int _ptr_mangle(unsigned long int p)__attribute__((unused));
static unsigned long int _ptr_demangle(unsigned long int p)__attribute__((unused));

static void set_reg(jmp_buf *buf, enum JBL reg, unsigned long int val)__attribute__((unused));
static unsigned long int get_reg(jmp_buf *buf, enum JBL reg)__attribute__((unused));
static void *start_thunk() __attribute__((unused));


#endif
