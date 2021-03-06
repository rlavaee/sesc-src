/*
 * Routines for reading and parsing the text section and managing the
 * memory for an address space.
 *
 * Copyright (C) 1993 by Jack E. Veenstra (veenstra@cs.rochester.edu)
 * 
 * This file is part of MINT, a MIPS code interpreter and event generator
 * for parallel programs.
 * 
 * MINT is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 1, or (at your option)
 * any later version.
 * 
 * MINT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with MINT; see the file COPYING.  If not, write to
 * the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <sys/types.h>
#include <sys/utsname.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#if defined(__svr4__) || defined(_SVR4_SOURCE)
#include <sys/times.h>
#endif
#include <limits.h>

#define MAIN

#include "icode.h"
#include "ThreadContext.h"
#include "opcodes.h"
#include "globals.h"
#include "symtab.h"
#include "mendian.h"

#include "nanassert.h"

#ifdef DARWIN
#include <fenv.h>
#endif

#include <assert.h>

/* Note: the debugger dbx gets line numbers confused because of embedded
 * newlines inside the double quotes in the following macro.
 */
#define USAGE \
"\nUsage: %s [mint options] [-- simulator options] objfile [objfile options]\n\
\n\
mint options:\n\
	[-h heap_size]		heap size in bytes, default: %d (0x%x)\n\
	[-k stack_size]		stack size in bytes, default: %d (0x%x)\n\
	[-n nice_value]	        \'nice\' MINT process, default: 0 if 'nice' used, 4 if not\n\
	[-p procs]		number of per-process regions, default: %d\n"

FILE *Fobj=0;		/* file descriptor for object file */
static int Nicode=0;	/* number of free icodes left */

/* imported functions */
void init_main_thread();
void upshot_init(char *fname);
void subst_init();
void subst_functions();

/* exported functions */
void mint_init(int argc, char **argv, char **envp);
void allocate_fixed(long addr, long nbytes);

/* private functions */
static void parse_args(int argc, char **argv);
static void copy_argv(int arg_start, int argc, char **argv, char **envp);
static void mint_stats();
static void create_addr_space();
static void read_text();
static void usage();

void mint_init(int argc, char **argv, char **envp)
{
  extern int optind;
  int next_arg;
  FILE *fd;
  
  Mint_output = stderr;
  Simname = argv[0];
  parse_args(argc, argv);
  ThreadContext::staticConstructor();

  next_arg = optind;
  Objname = argv[next_arg];

  read_hdrs(Objname);
  subst_init();
  read_text();
  create_addr_space();
  close_object();
  copy_argv(next_arg, argc, argv, envp);
  subst_functions();

  ThreadContext::initMainThread();
  

#ifdef DARWIN
  feclearexcept(FE_ALL_EXCEPT);
#endif

  close_object();
}

static void usage()
{
  fprintf(stderr, USAGE, Simname,
	  HEAP_SIZE, HEAP_SIZE,
	  STACK_SIZE, STACK_SIZE,
	  MAXPROC-1);
}

/* parse the command line arguments */
static void parse_args(int argc, char **argv)
{
  int errflag;
  int c;
  extern char *optarg;

  /* Value of command line option -n, -321 means 'not specified' */
  long int NiceValue=-321;
  
  /* set up the default values */
  Stack_size = STACK_SIZE;
  Heap_size = HEAP_SIZE;
  Max_nprocs = MAXPROC-1;
  
  errflag = 0;
  while((c=getopt(argc, argv, "n:h:k:P:p:s:W"))!=-1){
    switch (c) {
    case 'n':
      NiceValue=strtol(optarg,NULL,0);
      break;
    case 'h':
      Heap_size = strtol(optarg, NULL, 0);
      break;
    case 'k':
      Stack_size = strtol(optarg, NULL, 0);
      break;
    case 'p':
      Max_nprocs = strtol(optarg, NULL, 0);
      if (Max_nprocs < 1 || Max_nprocs >= MAXPROC) {
	errflag = 1;
	fprintf(stderr, "Number of processes must be in the range: 1 to %d\n",MAXPROC);
      }
      break;
    default:
      errflag = 1;
      break;
    }
  }

  /* If nice value is set using the -n command line option, use it */
  if(NiceValue!=-321){
    if(nice(NiceValue)==-1){
      char msg[] = "Invalid nice value (option -n)";
      error(msg);
      exit(1);
    }
  }

  if (errflag) {
    usage();
    exit(1);
  }
}

/* copy application args onto the simulated stack for the main process */
static void
copy_argv(int arg_start, int argc, char **argv, char **envp)
{
    int i, size, nenv;
    long *sp;
    int argc_obj;
    char **argv_obj, **eptr, **envp_obj;
    ThreadContext *pthread;
    int sizeofptr;

    pthread = ThreadContext::getMainThreadContext();
    sizeofptr = sizeof(char *);

    /* add up sizes of all the arguments */
    size = 0;
    for (i = arg_start; i < argc; i++) {
	unsigned long val =size + strlen(argv[i]) + 1;
	val = (val + 0x1f) & ~0x1f;
	size = val;
    }

    /* Add in the sizes of all the environment variables, and count
     * the number of environment variable pointers that we need.
     */
    nenv = 0;
    for (eptr = envp; *eptr; eptr++) {
	unsigned long val =size + strlen(*eptr) + 1;
	val = (val + 0x1f) & ~0x1f;
	size = val;
        nenv++;
    }

    /* get the number of arguments to the simulated object program */
    argc_obj = argc - arg_start;

    /* add in space for the argv and envp pointers, including
     * two NULL pointers
     */
    size += (argc_obj + nenv + 2) * sizeofptr;

    /* add in space for argc */
    size += sizeof(int);

    /* Originaly round the size up to a double word boundary, for
     * performance reasons is much better to have a bigger alignment
     **/
    size = (size + 0x1f) & ~0x1f;

    /* allocate stack space for all the args */
    pthread->setREGNUM(29, pthread->getREGNUM(29) - size);

    I(pthread->getREGNUM(29) >= Stack_start && pthread->getREGNUM(29) <= Stack_end);

    /* get the real address */
    sp = (long *) pthread->virt2real(pthread->getREGNUM(29));

    /* the first stack item is the number of args (argc) */
    *sp++ = SWAP_WORD(argc_obj);
	
    /* the next stack items are the array of pointers argv */
    argv_obj = (char **) sp;

    /* leave space for the argv array, including the NULL pointer */
    sp += argc_obj + 1;

    /* next come the pointers for the environment variables */
    envp_obj = (char **) sp;
    sp = &sp[nenv + 1];

    /* copy the args to the stack of the main thread */
    for (i = arg_start; i < argc; i++) {
        strcpy((char *) sp, argv[i]);
	argv_obj[i - arg_start] = (char *) SWAP_WORD(pthread->real2virt((long)sp));
	unsigned long val =(unsigned long) sp + strlen(argv[i]) + 1;
	/* Align word */
	val = (val + 0x1f) & ~0x1f;
        sp = (long *)(val);
	
    }
    argv_obj[argc - arg_start] = NULL;

    /* copy the environment variables to the stack of the main thread */
    for (i = 0, eptr = envp; i < nenv; i++, eptr++) {
        strcpy((char *) sp, *eptr);
	envp_obj[i] = (char *) SWAP_WORD(pthread->real2virt((long)sp));
	unsigned long val =(unsigned long) sp + strlen(*eptr) + 1;
	/* Align word */
	val = (val + 0x1f) & ~0x1f;
        sp = (long *)(val);
    }
    envp_obj[nenv] = NULL;
}

/*
 * logbase2() returns the log (base 2) of its argument, rounded up.
 * It also rounds up its argument to the next higher power of 2.
 */
static int
logbase2(long *pnum)
{
    unsigned int logsize;
    long exp;

    for (logsize = 0, exp = 1; exp < *pnum; logsize++)
        exp *= 2;
    
    /* round pnum up to nearest power of 2 */
    *pnum = exp;

    return logsize;
}

void *allocate2(long nbytes)
{
  void *ptr;
  long size2;
  int status = 0;
  /* round nbytes up to the next power of 2 */
  size2 = nbytes;
  logbase2(&size2);
#ifdef POSIX_MEMALIGN
  /* MCH: memalign is obsolete, but on iacoma3 posix_memalign is not yet supported */
  // Milos: align to a 16-megabyte boundary so small
  // changes to the simulator are unlikely to change the
  // starting address of the allocated block of memory
  status = posix_memalign(&ptr,0x1000000,nbytes);
#else
  ptr = malloc(size2);
  status = (ptr == NULL);
#endif

  if(status) {
    fprintf(stderr,"allocate2: %x\n",status);
    fprintf(stderr, "allocate_aligned: cannot allocate 0x%x bytes at a 0x%x boundary.\n",
	    (unsigned) nbytes, (unsigned) size2);
    exit(-1);
  }
  /* Slow as hell  bzero(ptr, size2); */

  return ptr;
}

/* Allocate memory for the shared memory region and all the per-process
 * private memory regions and initialize the address space for the main
 * process.
 */
static void create_addr_space()
{
  int size, multiples;
  unsigned i;
  unsigned dwords;
  long *addr;
  thread_ptr pthread;
  unsigned long min_seek, total_size;
  signed long heap_start;
  unsigned long heap_size;

  /* this assumes that the bss is located higher in memory than the data */

  /* Segments order:
   *
   * (Rdata|Data)...Bss...Heap...Stacks
   *
   */

  if (Rdata_start < Data_start) {
    Rsize_round = Rdata_size;
    if (Rsize_round != (Data_start - Rdata_start)) {
      char msg[] = "Rdata is not contiguous with Data";
      fatal(msg);
    }
  }else{
    // Data section includes rdata section (if it exists)
    Rsize_round = 0;
  }

  /* DB_size points to the beginning of Heap */
  DB_size  = Rsize_round + Bss_start + Bss_size - Data_start;

  Mem_size = DB_size;
  Mem_size = (Mem_size + M_ALIGN - 1) & ~(M_ALIGN - 1);     // Page align
  unsigned long Heap_start_rel = Mem_size;

  Mem_size = Mem_size+Heap_size;
  Mem_size = (Mem_size + M_ALIGN - 1) & ~(M_ALIGN - 1);     // Page align
  unsigned long Stack_start_rel = Mem_size;

  Stack_size = (Stack_size + M_ALIGN - 1) & ~(M_ALIGN - 1); // Page align

  Mem_size = Mem_size+Stack_size*Max_nprocs;

  // Data_end includes Bss (may or may not include Rdata)
  Data_end  = Data_start  + Heap_start_rel;
  Rdata_end = Rdata_start + Rdata_size;

  Private_start = (long)allocate2(Mem_size);
  Private_end   = Private_start + Mem_size;

  if (((ulong)Private_end > (ulong)Data_start  && (ulong)Private_end < (ulong)Data_end)
      || ((ulong)Private_start > (ulong)Data_start  && (ulong)Private_start < (ulong)Data_end)) {
    long oldSpace = Private_start;
    Private_start = (long)allocate2(Mem_size);
    fprintf(stderr,"Overlap: Shifting address space [0x%x] -> [0x%x]\n",(unsigned)oldSpace, (unsigned)Private_start);
    free((void *)oldSpace);
    Private_end   = Private_start + Mem_size;
  }

  pthread = ThreadContext::getMainThreadContext();

  signed long addrSpace = Private_start;
  signed long rdataMap  = Private_start - Rdata_start;
 

  addr = (long *) addrSpace;

  if (Rdata_start < Data_start) {
    /* Read in the .rdata section first since the .rdata section is not
     * contiguous with the rest of the data in the address space or object
     * file.
     */
    if (Rdata_seek != 0 && Rdata_size > 0) {
      fseek(Fobj, Rdata_seek, SEEK_SET);

      /* read in the .rdata section from the object file */
      if (fread(addr, sizeof(char), Rdata_size, Fobj) < Rdata_size){
	char msg[] = "create_addr_space: end-of-file reading rdata section\n"; 
	fatal(msg);
      }

      /* move "addr" to prepare for reading in the data and bss */
      addr = (long *) (Private_start + Rsize_round);
      addrSpace = (signed long) addr;
    }
    rdataMap = Private_start - Data_start;
  }

  /* Figure out which data section comes first. The SGI COFF files have
   * .data first. DECstations have .rdata first except sometimes the
   * .rdata section is before the .text section.
   */
  min_seek = ULONG_MAX;
  if (Rdata_start >= Data_start && Rdata_seek != 0)
    min_seek = Rdata_seek;
  if (Data_seek != 0 && Data_seek < min_seek)
    min_seek = Data_seek;
  if (Sdata_seek != 0 && Sdata_seek < min_seek)
    min_seek = Sdata_seek;
  if (min_seek == ULONG_MAX){
    char msg[] = "create_addr_space: no .rdata or .data section\n"; 
    fatal(msg);
  }
  fseek(Fobj, min_seek, SEEK_SET);
  dwords = Data_size / 4;

  /* read in the initialized data from the object file */
  if (fread(addr, sizeof(long), dwords, Fobj) < dwords){
    char msg[] = "create_addr_space: end-of-file reading data section\n";
    fatal(msg);
  }

  /* zero out the bss section */
  addr = (long *) (addrSpace + Bss_start - Data_start);
  dwords = Bss_size / 4;
  for (i = 0; i < dwords; i++)
    *addr++ = 0;

  heap_start = Private_start + Heap_start_rel;
  heap_size = Stack_start_rel - Heap_start_rel; // Next to heap is the stack
  if (heap_size < HEAP_SIZE_MIN) {
    fprintf(stderr, "Not enough memory for private malloc: %ld (0x%lx)\n",
	    heap_size,heap_size);
    fprintf(stderr, "Try increasing it using the \"-h\" option.\n");
    usage();
    exit(1);
  }
  Heap_start = heap_start;
  Heap_end   = heap_start+heap_size;
  /* initialize the private memory allocator for this thread */
  pthread->setHeapManager(HeapManager::create(heap_start,heap_size));
  // malloc_init(pthread, heap_start, heap_size);

  /* point the sp to the top of the allocated space */
  /* (The stack grows down toward lower memory addresses.) */
  Stack_start = Private_start + Stack_start_rel;
  Stack_end   = Private_end;
  pthread->setREGNUM(29, Stack_start + Stack_size);

  /* Change the sp so that mapping will work for sp-relative addresses. */
  pthread->setREGNUM(29, pthread->real2virt(pthread->getREGNUM(29)));

  pthread->initAddressing(rdataMap  // RDataMap
			  ,addrSpace - Data_start  // memMap
			  ,pthread->getREGNUM(29) - Stack_size // Stack Top
			  );
}


/* Share the parent's address space with the child. This is used to
 * support the sproc() system call on the SGI. This also sets up
 * all the mapping fields in the child's thread structure.
 */

/* create a new copy of an icode */
icode_ptr newcopy_icode(icode_ptr picode){
  int i;
  icode_ptr inew;
  static icode_ptr Icode_free;
  
  /* reduce calls to malloc and make more efficient use of space
   * by allocating several icodes at once
   */
  if (Nicode == 0) {
    Icode_free = (icode_ptr) malloc(1024 * sizeof(struct icode));
    if (Icode_free == NULL){
      char msg[] = "newcopy_icode: out of memory\n";
      fatal(msg);
    }
    Nicode = 1024;
  }
  inew = &Icode_free[--Nicode];
  inew->func = picode->func;
  inew->addr = picode->addr;
  inew->instr = picode->instr;
  for (i = 0; i < 4; i++)
    inew->args[i] = picode->args[i];
  inew->immed = picode->immed;
  inew->next = picode->next;
  inew->target = picode->target;
  inew->not_taken = picode->not_taken;
  inew->is_target = picode->is_target;
  inew->opnum = picode->opnum;
  inew->opflags = picode->opflags;
  
  inew->instID = picode->instID;
#if (defined TLS)
  inew->setReplayClass(picode->getReplayClass());
#endif
  return inew;
}

icode_ptr icodeArray;
size_t    icodeArraySize;

/* Reads the text section of the object file and creates the linked list
 * of icode structures.
 */
static void
read_text()
{
  int make_copy, voffset, err;
  unsigned i;
  unsigned num_pointers;
  long instr, opflags, iflags;
  icode_ptr picode, prev_picode, dslot, pcopy, *pitext, pcopy2;
  struct op_desc *pdesc;
  unsigned int addr;
  int opnum;
  int regfield[4], target;
  int prev_was_branch;
  signed short immed;	/* immed must be a signed short */
  PFPI *pfunc;
#ifdef DEBUG_READ_TEXT
  static int Debug_addr = 0;
#endif
  int ud_defined=0;
  int ud_params[MAX_UD_PARAMS];
  int ud_size=0;
  int ud_i;
  unsigned int ud_addr;
  icode_ptr ud_picode;


  /* Allocate space for pointers to icode, plus the SGI function pointers
   * for the _lock routines, plus some for special function
   * pointers that have no other place to go, plus 1 for a NULL pointer.
   * Text_size is the number of instructions.
   */
  num_pointers = Text_size + EXTRA_ICODES;
  Itext = (icode_ptr *) calloc((num_pointers + 1), sizeof(icode_ptr));
  if (Itext == NULL){
    char msg[] = "read_text: cannot allocate enough bytes for Itext.\n";
    fatal(msg);
  }
  /* Allocate space for the icode structures */
  picode = (icode_ptr) calloc(num_pointers, sizeof(struct icode));
  icodeArray=picode;
  icodeArraySize=num_pointers;
  if (picode == NULL){
    char msg[] = "read_text: cannot allocate 0x%x bytes for icode structs.\n"; 
    fatal(msg);
  }

  /* Assign each pointer to its corresponding icode, and link each
   * icode to point to the next one in the array.
   */
  pitext = Itext;
  for (i = 0; i < num_pointers; i++) {
    *pitext++ = picode;
    picode->instID = i;
#if (defined TLS)
    picode->setReplayClass(OpInternal);
#endif
    picode->next = picode + 1;
    picode++;
  }
  *pitext = NULL;

  fseek(Fobj, Text_seek, SEEK_SET);
  /*    ldnsseek(Ldptr, ".text"); */

#ifdef notdef
  for (i = 0; i < max_opnum; i++)
    printf("%d %s\n", i, desc_table[i]);
#endif

  prev_was_branch = 0;
  prev_picode = NULL;
  picode = Itext[0];
  addr = Text_start;
  for (i = 0; i < Text_size; i++, addr += 4, picode++) {
    err = fread(&instr, sizeof(long), 1, Fobj);
    if (err < 1){
      char msg[] = "read_text: end-of-file reading text section\n"; 
      fatal(msg);
    }
    instr = SWAP_WORD((unsigned)instr);

    picode->addr = addr;
    regfield[RS] = (instr >> 21) & 0x1f;
    regfield[RT] = (instr >> 16) & 0x1f;
    regfield[RD] = (instr >> 11) & 0x1f;
    regfield[SA] = (instr >> 6) & 0x1f;
    immed = instr & 0xffff;

    ud_defined = ( ((instr >> 26) & 0x3f) == 60 );

    opnum = decode_instr(picode, instr);
    pdesc = &desc_table[opnum];
    opflags = pdesc->opflags;
    iflags = pdesc->iflags;

    picode->opflags = opflags;
    pfunc = pdesc->func;

    /* replace instructions that use r0 with faster equivalent ones */
    switch(opnum) {
    case beq_opn:
      if (picode->args[RT] == 0) {
	if (picode->args[RS] == 0) {
	  pfunc = b_op;
	  opnum = b_opn;
	} else
	  pfunc = beq0_op;
      }
      break;
    case bne_opn:
      if (picode->args[RT] == 0)
	pfunc = bne0_op;
      break;
    case addiu_opn:
      if (picode->args[RS] == 0) {
	pfunc = li_op;
	opnum = li_opn;
      } else if (picode->args[RS] == picode->args[RT]) {
	pfunc = addiu_xx_op;
      }
      break;
    case addu_opn:
      if (picode->args[RT] == 0) {
	if (picode->args[RS] == 0) {
	  pfunc = move0_op;
	  opnum = move_opn;
	} else {
	  pfunc = move_op;
	  opnum = move_opn;
	}
      }
      break;
    }
    picode->opnum = opnum;

    /* Change instructions that modify the stack pointer to
     * include an overflow check. A stack underflow cannot happen.
     */
    if (opnum == addiu_opn && regfield[RT] == 29) {
      if (immed < 0) {
	/* zero the opnum so that the optimizer won't inline this
	 * instruction
	 */
	picode->opnum = 0;
	pfunc = sp_over_op;
      }
    }

    /* Precompute the branch and jump targets */
    if (iflags & IS_BRANCH) {
      if (immed == -1) {
	//	warning("branch to itself at addr 0x%x. jump to next instruction if executed\n",
	//		picode->addr);
	picode->target = picode + 1;
      } else {
	picode->target = picode + immed + 1;
      }
      picode->target->is_target = 1;
    } else if ((iflags & IS_JUMP) && !ud_defined ) {
      /* for jump instrns, the target address is: */
      target = ((instr & 0x03ffffff) << 2) | ((addr + 4) & 0xf0000000);
      /* target == 0 if it has not been relocated yet */
      if (target > 0) {
	/* if the target address is out of range, then this
	 * is probably a jump to a shared library function.
	 */
	if ((unsigned) target > Text_start + 4 * (Text_size + 10)) {
	  char msg[] = "target address of jump instruction is past end of text.\n";
	  fatal(msg);
	}
	picode->target = addr2icode(target);
	picode->target->is_target = 1;
      }
    } else if (opnum == lui_opn)
      /* precompute the shift, use the target field */
      picode->target = (icode_ptr) (immed << 16);
    picode->not_taken = picode + 2;

    /* Find user define opnums and handle them appropriately
       In particular, handle parameters so that decoder does not
       get confused */
    ud_size = 0;

    if ( ud_defined ) {

      ud_size = picode->args[RD];
      if(ud_size > MAX_UD_PARAMS){
	char msg[] = "read_text: illegal user-defined opcode.\n"; 
	fatal(msg);
      }
      ud_addr = addr+4;
      ud_picode = picode+1;
	
      for(ud_i=0; ud_i < ud_size; ud_i++,ud_addr+=4,ud_picode++) {

	err = fread(&ud_params[ud_i], sizeof(long), 1, Fobj);
	if (err < 1){
	  char msg[] = "read_text: end-of-file reading text section\n"; 
	  fatal(msg);
	}
	ud_params[ud_i] = SWAP_WORD((unsigned)ud_params[ud_i]);
	ud_picode->instr = ud_params[ud_i];
	ud_picode->instID = i+ud_i+1;
	ud_picode->addr = ud_addr;
      }
      picode->next = ud_picode;

      switch((ud_class_t)picode->args[SA]) {
      case ud_call:
	/*FIXME: check for proper arguments*/
	if(ud_size > 0) {
	  picode->target = addr2icode(ud_params[0]);
	  picode->target->is_target = 1;
	}
	break;
      case ud_spawn:
	break;
      case ud_fork:
	break;
      case ud_regsynch:
	break;
      case ud_compute:
	break;
      }
    }

    voffset = 2;

    /* Either not tracing any memory references or
     * not a memory reference; use normal version.
     */
    if( opflags & E_MEM_REF )
      picode->func = pfunc[voffset];
    else
      picode->func = pfunc[0];
	
    dslot = NULL;
    if (prev_was_branch) {

      dslot = newcopy_icode(picode);
      /* link in the new copy to the "next" field of the previous icode */
      prev_picode->next = dslot;

      /* The "next" field of the dslot icode should never be used
       * since the current value of pthread->target is used instead.
       * So set the "next" field to NULL to catch bugs.
       */
      dslot->next = NULL;

      /* Either not tracing any memory references or
       * not a memory reference; use branch delay slot version.
       */
      if( opflags & E_MEM_REF )
	dslot->func = pfunc[voffset+1];
      else
	dslot->func = pfunc[1];
    }

    /* Set or clear the "prev_was_branch" flag so that the next instruction
     * knows which version of the function to call.
     */
    prev_was_branch = iflags & BRANCH_OR_JUMP;

    /* prev_picode is needed only for branch delay slot instructions */
    prev_picode = picode;

    /*  If ud_opnums were found, then adjust these variables
	accordingly 
    */
    if( ud_size > 0 ) {
      i+=ud_size;
      picode+=ud_size;
      addr+=4*ud_size;
    }

  }
}

/* Performs the work for decoding an instruction word, filling in the
 * fields of an icode structure, and replacing instructions that use
 * register r0 (always zero) with simpler, faster equivalent ones.
 * The register indices are pre-shifted here so that register value
 * lookups at execution time will be faster.
 *
 * Returns: opnum, the index into the opcode description table.
 */
int
decode_instr(icode_ptr picode, int instr)
{
    int iflags;
    struct op_desc *pdesc;
    int j, opcode, bits31_28, bits20_16, bits5_0, fmt, opnum;
    int coproc, cofun;
    int regfield[4];
    signed short immed;	/* immed must be a signed short */
    
    picode->instr = instr;
    opcode = (instr >> 26) & 0x3f;
    bits31_28 = (instr >> 28) & 0xf;
    bits20_16 = (instr >> 16) & 0x1f;
    bits5_0 = instr & 0x3f;
    regfield[RS] = (instr >> 21) & 0x1f;
    regfield[RT] = (instr >> 16) & 0x1f;
    regfield[RD] = (instr >> 11) & 0x1f;
    regfield[SA] = (instr >> 6) & 0x1f;
    immed = instr & 0xffff;
    picode->immed = immed;
    
    if (opcode == 0) {
        /* special instruction */
        opnum = special_opnums[bits5_0];
    } else if (opcode == 1) {
        /* regimm instruction */
        opnum = regimm_opnums[bits20_16];
    } else if (opcode == 60) {
        /* user defined instructions */
        opnum = user_opnums[bits5_0];
    } else if (bits31_28 != 4) {
        /* normal operation */
        opnum = normal_opnums[opcode];
    } else {
        /* coprocessor instruction */
        coproc = (instr >> 26) & 0x3;
        
        fmt = regfield[FMT];
        if ((instr >> 25) & 1) {
            /* general coprocessor operation, uses cofun */
            if (coproc == 1) {
                cofun = instr & 0x03f;
                if (fmt == 16)		/* single precision format */
                    opnum = cop1func_opnums[0][cofun];
                else if (fmt == 17)		/* double precision format */
                    opnum = cop1func_opnums[1][cofun];
                else			/* fixed-point format */
                    opnum = cop1func_opnums[2][cofun];
                
            } else {
                /* coprocessor other than 1 */
                opnum = normal_opnums[opcode];
            }
        } else {
            switch (fmt) {
              case 0:
                /* mfc1, move register from coprocessor */
                opnum = mfc_opnums[coproc];
                break;
              case 2:
                /* cfc1, move control from coprocessor */
                opnum = cfc_opnums[coproc];
                break;
              case 4:
                /* mtc1, move register to coprocessor */
                opnum = mtc_opnums[coproc];
                break;
              case 6:
                /* ctc1, move control to coprocessor */
                opnum = ctc_opnums[coproc];
                break;
              case 8:
                /* coprocessor branch */
                if (regfield[FT] < 4)
                    opnum = bc_opnums[coproc][regfield[FT]];
                else
                    opnum = reserved_opn;
                break;
              default:
                opnum = reserved_opn;
                break;
            }
        }
    }
    pdesc = &desc_table[opnum];
#if 0
    opflags = pdesc->opflags;
#endif
    iflags = pdesc->iflags;

    /* for coprocessor instructions, the fmt field should not be shifted */
    picode->args[FMT] = regfield[FMT];
    
    /* 1. Pre-shift the register indices.
     * 2. Modify instructions that write to r0 so that they write
     *    to r32 instead (so that r0 remains zero).
     * 3. Modify instructions that access the floating point
     *    registers so that they use the correct index into the fp[] array.
     *    This requires flipping the low order bit.
     */
    for (j = 0; j < 4; j++) {
        if (pdesc->regflags[j] & MOD0)
	  if (regfield[j] == 0) {
                if (iflags & MOD0_IS_NOP) {
                    /* replace this instruction with a nop */
                    opnum = nop_opn;
                } else
                    regfield[j] = 32;
	  }
        /* shift the register values so they can be used
         * as indices directly */
        if ((pdesc->regflags[j] & REG0) || (pdesc->regflags[j] & DREG1))
            picode->args[j] = regfield[j] << 2;
        else if (pdesc->regflags[j] & REG1) {
	  /* ENDIANA TRASH */
#ifdef LENDIAN
	  picode->args[j] = regfield[j] << 2;
#else
	  /* flip the low order bit of single precision fp regs */
	  picode->args[j] = (regfield[j] ^ 1) << 2;
#endif
        } else
            picode->args[j] = regfield[j];
    }
    return opnum;
}
