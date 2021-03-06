#ifndef SESCAPI_H
#define SESCAPI_H


/*
 * Flags specification:
 *
 * flags are used in sesc_spawn
 and sesc_sysconf. Both cases share the
 * same flags structure, but some flag parameters are valid in some
 * cases.
 *
 * Since the pid has only 16 bits and flags are 32 bits, the lower 16
 * bits are a parameter to the flags.
 */

/* The created thread is not a allowed to migrate between CPUs on
 * context switchs.
 */
#define SESC_FLAG_NOMIGRATE  0x10000

/* The tid parameter (the lower 16 bits) indicate in which CPU this
 * thread is going to be mapped. If the flag SESC_SPAWN_NOMIGRATE is
 * also provided, the thread would always be executed in the same
 * CPU. (unless sesc_sysconf is modifies the flags status)
 */
#define SESC_FLAG_MAP        0x20000

#define SESC_FLAG_FMASK      0x8fff0000
#define SESC_FLAG_DMASK      0xffff



/* Example of utilization:
 *
 * tid = sesc_spawn(func,0,SESC_FLAG_NOMIGRATE|SESC_FLAG_MAP|7);
 * This would map the thread in the processor 7 for ever.
 *
 * sesc_sysconf(tid,SESC_FLAG_NOMIGRATE|SESC_FLAG_MAP|2);
 * Moves the previous thread (tid) to the processor 2
 *
 * sesc_sysconf(tid,SESC_FLAG_MAP|2); 
 * Keeps the thread tid in the same processor, but it is allowed to
 * migrate in the next context switch.
 *
 * tid = sesc_spawn(func,0,0);
 * Creates a thread and maps it to the processor an iddle processor if
 * possible.
 *
 * tid = sesc_spawn(func,0,SESC_FLAG_NOMIGRATE);
 * The same that the previous, but once assigned to a processor, it
 * never migrates.
 *
 */

enum FetchOpType {
  FetchIncOp = 0,
  FetchDecOp,
  FetchSwapOp
};

#define LOCKED    1
#define UNLOCKED  0

#define MAXLOCKWAITING 1023

/* #define  NOSPIN_DOSUSPEND 1 */

#if defined(SESCAPI_NATIVE_IRIX) && !defined(SESCAPI_NATIVE)
#define SESCAPI_NATIVE 1
#endif

#ifdef SESCAPI_NATIVE
#include <pthread.h>

#define slock_t pthread_mutex_t

#define sesc_lock_init(x) pthread_mutex_init(x,0)
#define sesc_lock(x)      pthread_mutex_lock(x)
#define sesc_unlock(x)    pthread_mutex_unlock(x)

#ifdef SESCAPI_NATIVE_IRIX
#include <sys/pmo.h>
#include <fetchop.h>
#endif

#else
typedef struct {
  volatile long spin;            /* lock spins */
#ifdef NOSPIN_DOSUSPEND
  volatile long waitSpin;
  volatile long waitingPos;
  int waiting[MAXLOCKWAITING+1];
#endif
} slock_t;
#endif  /* !SESCAPI_NATIVE */


typedef struct {
  volatile int gsense;
#ifdef SESCAPI_NATIVE_IRIX
  atomic_var_t *count;
#else /* !SESCAPI_NATIVE_IRIX */
  volatile long count;    /* the count of entered processors */
#ifdef NOSPIN_DOSUSPEND
  /* Only for the enter phase */
  volatile long waitingPos;
  int waiting[MAXLOCKWAITING+1];
#endif
#endif /* SESCAPI_NATIVE_IRIX */
} sbarrier_t;

typedef struct {
  long count;                   /* shared object, the number of arrived processors */
  volatile int gsen;            /* shared object, the global sense */
} sgbarr_t;

typedef struct {
  int lsen;                     /* local object, the local sense */
} slbarr_t;

typedef struct {
  volatile long count;          /* the number of resource available to the semaphore */
} ssema_t;

/* A flag, implemented an in the original ANL macros for Splash-2 */
typedef struct{
  int     flag;
  int     count;
  slock_t lock;
  ssema_t queue;
} sflag_t;

#ifdef __cplusplus
extern "C" {
#endif
  void sesc_preevent_(long vaddr, long type, void *sptr);
  void sesc_preevent(long vaddr, long type, void *sptr);
  void sesc_postevent_(long vaddr, long type, const void *sptr);
  void sesc_postevent(long vaddr, long type, const void *sptr);
  void sesc_memfence_(long vaddr);
  void sesc_memfence(long vaddr);
  void sesc_acquire_(long vaddr);
  void sesc_acquire(long vaddr);
  void sesc_release_(long vaddr);
  void sesc_release(long vaddr);

  void sesc_init(void);
  int  sesc_get_num_cpus(void);
  void sesc_sysconf(int tid, long flags);
  int  sesc_spawn(void (*start_routine) (void *), void *arg, long flags);

  //ENGIN: For adaptive CMP
  int sesc_begin_region(int region);
  int sesc_end_region(int region);
  int sesc_set_num_region(int numRegions);
  void sesc_start_next_application(char *);

  int  sesc_self(void);
  int  sesc_suspend(int tid);
  int  sesc_resume(int tid);
  int  sesc_yield(int tid);

  void sesc_exit(int err);
  void sesc_finish(void);  /* Finish the whole simulation */
  void sesc_wait(void);

  long sesc_fetch_op(enum FetchOpType op, volatile long *addr, long val); 
  void sesc_simulation_mark(void);
  void sesc_fast_sim_begin(void);
  void sesc_fast_sim_begin_(void);
  void sesc_fast_sim_end(void);
  void sesc_fast_sim_end_(void);

#ifndef SESCAPI_NATIVE
  /*
   * LOCK/UNLOCK operation
   * a simple spin lock
   */
  void sesc_lock_init(slock_t * lock);
  void sesc_lock(slock_t * lock);
  void sesc_unlock(slock_t * lock);
#endif

  /*
   * Barrier 
   * a two-phase centralized barrier
   */
  void sesc_barrier_init(sbarrier_t *barr);
  void sesc_barrier(sbarrier_t *barr, long num_proc);

  /*
   * Semaphore
   * busy-wait semaphore
   */
  void sesc_sema_init(ssema_t *sema, int initValue);
  void sesc_psema(ssema_t *sema);
  void sesc_vsema(ssema_t *sema);

  /*
   * Flag
   * using a lock and a semaphore
   */
  void sesc_flag_init(sflag_t *flag);
  void sesc_flag_wait(sflag_t *flag);
  void sesc_flag_set(sflag_t *flag);
  void sesc_flag_clear(sflag_t *flag);

#ifdef VALUEPRED
  int  sesc_get_last_value(int id);
  void sesc_put_last_value(int id, int val);
  int  sesc_get_stride_value(int id);
  void sesc_put_stride_value(int id, int val);
  int  sesc_get_incr_value(int id, int lval);
  void sesc_put_incr_value(int id, int incr);
  void sesc_verify_value(int rval, int pval);
#endif

#ifdef TLS
  /* The thread begins the first epoch in its sequence */
  void sesc_begin_epochs(void);

  /* Creates and begins a new epoch that is the sequential successor of the
     current epoch. Return value is similar to fork - 0 is returned to the
     newly created epoch, the ID of the new epoch is returned to the original
     epoch
  */
  int  sesc_future_epoch(void);

  /* Creates and begins a new epoch that is the sequential successor of the
     current epoch. The successor begins executing instructions from the
     instruction address codeAddr.
  */
  volatile void  sesc_future_epoch_jump(void *codeAddr);

  /* Completes an epoch that has already created its future.  The epoch wait
     until it can commit, then commits.  This call does not return.
  */
  void sesc_commit_epoch(void);

  /* Completes the current epoch and begins its sequential successor.  This
     could be done using sesc_future_epoch followed by csesc_commit_epoch, but
     sesc_change_epoch is more efficient
  */ 
  void sesc_change_epoch(void);

  /* The thread ends the last epoch in its sequence */
  void sesc_end_epochs(void);
  
  void sesc_acquire_begin(void);
  void sesc_acquire_retry(void);
  void sesc_acquire_end(void);

  void sesc_release_begin(void);
  void sesc_release_end(void);


#if ( (defined mips) && (__GNUC__ >= 3) )
  static inline void tls_begin_epochs(void) __attribute__ ((always_inline));
  static inline void tls_change_epoch(void) __attribute__ ((always_inline));
  static inline void tls_end_epochs(void) __attribute__ ((always_inline));

  static inline void tls_acquire_begin(void) __attribute__ ((always_inline));
  static inline void tls_acquire_retry(void) __attribute__ ((noreturn));
  static inline void tls_acquire_end(void) __attribute__ ((always_inline));

  static inline void tls_release_begin(void) __attribute__ ((always_inline));
  static inline void tls_release_end(void) __attribute__ ((always_inline));

  static inline void tls_lock_init(slock_t *lock_ptr) __attribute__ ((always_inline));
  static inline void tls_lock(slock_t *lock_ptr) __attribute__ ((always_inline));
  static inline void tls_unlock(slock_t *lock_ptr) __attribute__ ((always_inline));

  static inline void tls_barrier_init(sbarrier_t *barr_ptr) __attribute__ ((always_inline));
  static inline void tls_barrier(sbarrier_t *barr_ptr, long num_proc) __attribute__ ((always_inline));

  static inline void tls_begin_epochs(void){
    asm volatile ("jal sesc_begin_epochs"
		  :
		  :
		  : "cc", "memory", "a0", "a1", "a2", "a3", "v0", "v1",
		  "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "t8", "t9",
		  "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "ra",
		  "$f0",  "$f1",  "$f2",  "$f3",  "$f4",  "$f5",  "$f6",  "$f7",
		  "$f8",  "$f9",  "$f10", "$f11", "$f12", "$f13", "$f14", "$f15",
		  "$f16", "$f17", "$f18", "$f19", "$f20", "$f21", "$f22", "$f23",
		  "$f24", "$f25", "$f26", "$f27", "$f28", "$f29", "$f30", "$f31",
		  "$fcc0", "$fcc1", "$fcc2", "$fcc3", "$fcc4", "$fcc5", "$fcc6", "$fcc7");
  }
  static inline void tls_change_epoch(void){
    asm volatile ("jal sesc_change_epoch"
		  :
		  : 
		  : "cc", "memory", "a0", "a1", "a2", "a3", "v0", "v1",
		  "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "t8", "t9",
		  "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "ra",
		  "$f0",  "$f1",  "$f2",  "$f3",  "$f4",  "$f5",  "$f6",  "$f7",
		  "$f8",  "$f9",  "$f10", "$f11", "$f12", "$f13", "$f14", "$f15",
		  "$f16", "$f17", "$f18", "$f19", "$f20", "$f21", "$f22", "$f23",
		  "$f24", "$f25", "$f26", "$f27", "$f28", "$f29", "$f30", "$f31",
		  "$fcc0", "$fcc1", "$fcc2", "$fcc3", "$fcc4", "$fcc5", "$fcc6", "$fcc7");
  }
  static inline void tls_end_epochs(void){
    asm volatile ("jal sesc_end_epochs"
		  :
		  : 
		  : "cc", "memory", "a0", "a1", "a2", "a3", "v0", "v1",
		  "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "t8", "t9",
		  "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "ra",
		  "$f0",  "$f1",  "$f2",  "$f3",  "$f4",  "$f5",  "$f6",  "$f7",
		  "$f8",  "$f9",  "$f10", "$f11", "$f12", "$f13", "$f14", "$f15",
		  "$f16", "$f17", "$f18", "$f19", "$f20", "$f21", "$f22", "$f23",
		  "$f24", "$f25", "$f26", "$f27", "$f28", "$f29", "$f30", "$f31",
		  "$fcc0", "$fcc1", "$fcc2", "$fcc3", "$fcc4", "$fcc5", "$fcc6", "$fcc7");
  }
  static inline void tls_acquire_begin(void){
    asm volatile ("jal sesc_change_epoch\njal sesc_acquire_begin"
		  :
		  : 
		  : "cc", "memory", "a0", "a1", "a2", "a3", "v0", "v1",
		  "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "t8", "t9",
		  "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "ra",
		  "$f0",  "$f1",  "$f2",  "$f3",  "$f4",  "$f5",  "$f6",  "$f7",
		  "$f8",  "$f9",  "$f10", "$f11", "$f12", "$f13", "$f14", "$f15",
		  "$f16", "$f17", "$f18", "$f19", "$f20", "$f21", "$f22", "$f23",
		  "$f24", "$f25", "$f26", "$f27", "$f28", "$f29", "$f30", "$f31",
		  "$fcc0", "$fcc1", "$fcc2", "$fcc3", "$fcc4", "$fcc5", "$fcc6", "$fcc7");
  }
  static inline void tls_acquire_retry(void){
    sesc_acquire_retry();
  }
  static inline void tls_acquire_end(void){
    asm volatile ("jal sesc_acquire_end"
		  :
		  : 
		  : "cc", "memory", "a0", "a1", "a2", "a3", "v0", "v1",
		  "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "t8", "t9",
		  "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "ra",
		  "$f0",  "$f1",  "$f2",  "$f3",  "$f4",  "$f5",  "$f6",  "$f7",
		  "$f8",  "$f9",  "$f10", "$f11", "$f12", "$f13", "$f14", "$f15",
		  "$f16", "$f17", "$f18", "$f19", "$f20", "$f21", "$f22", "$f23",
		  "$f24", "$f25", "$f26", "$f27", "$f28", "$f29", "$f30", "$f31",
		  "$fcc0", "$fcc1", "$fcc2", "$fcc3", "$fcc4", "$fcc5", "$fcc6", "$fcc7");
  }
  static inline void tls_release_begin(void){
    asm volatile ("jal sesc_release_begin"
		  :
		  : 
		  : "cc", "memory", "a0", "a1", "a2", "a3", "v0", "v1",
		  "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "t8", "t9",
		  "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "ra",
		  "$f0",  "$f1",  "$f2",  "$f3",  "$f4",  "$f5",  "$f6",  "$f7",
		  "$f8",  "$f9",  "$f10", "$f11", "$f12", "$f13", "$f14", "$f15",
		  "$f16", "$f17", "$f18", "$f19", "$f20", "$f21", "$f22", "$f23",
		  "$f24", "$f25", "$f26", "$f27", "$f28", "$f29", "$f30", "$f31",
		  "$fcc0", "$fcc1", "$fcc2", "$fcc3", "$fcc4", "$fcc5", "$fcc6", "$fcc7");
  }
  static inline void tls_release_end(void){
    asm volatile ("jal sesc_release_end\njal sesc_change_epoch"
		  :
		  : 
		  : "cc", "memory", "a0", "a1", "a2", "a3", "v0", "v1",
		  "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7", "t8", "t9",
		  "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "ra",
		  "$f0",  "$f1",  "$f2",  "$f3",  "$f4",  "$f5",  "$f6",  "$f7",
		  "$f8",  "$f9",  "$f10", "$f11", "$f12", "$f13", "$f14", "$f15",
		  "$f16", "$f17", "$f18", "$f19", "$f20", "$f21", "$f22", "$f23",
		  "$f24", "$f25", "$f26", "$f27", "$f28", "$f29", "$f30", "$f31",
		  "$fcc0", "$fcc1", "$fcc2", "$fcc3", "$fcc4", "$fcc5", "$fcc6", "$fcc7");
  }
  static inline void tls_lock_init(slock_t *lock_ptr){
    lock_ptr->spin=UNLOCKED;
  }
  static inline void tls_lock(slock_t *lock_ptr){
    tls_acquire_begin();
    while(lock_ptr->spin!=UNLOCKED)
      tls_acquire_retry();
    lock_ptr->spin=LOCKED;
    tls_acquire_end();
  }
  static inline void tls_unlock(slock_t *lock_ptr){
    tls_release_begin();
    lock_ptr->spin=UNLOCKED;
    tls_release_end();
  }
  static inline void tls_barrier_init(sbarrier_t *barr_ptr){
    barr_ptr->count=0;
    barr_ptr->gsense=0;
  }
  static inline void tls_barrier(sbarrier_t *barr_ptr, long num_proc){
    register int  lsense=!barr_ptr->gsense;
    register long lcount;
    tls_acquire_begin();
    lcount=barr_ptr->count++;
    tls_acquire_end();
    if(lcount==num_proc-1){
      barr_ptr->count=0;
      tls_release_begin();
      barr_ptr->gsense=lsense;
      tls_release_end();
    }else{
      tls_acquire_begin();
      while(barr_ptr->gsense!=lsense)
	tls_acquire_retry();
      tls_acquire_end();
    }
  }
  static inline void tls_flag_init(sflag_t *flag_ptr){
    flag_ptr->flag=LOCKED;
  }
  static inline void tls_flag_clear(sflag_t *flag_ptr){
    flag_ptr->flag=LOCKED;
  }
  static inline void tls_flag_set(sflag_t *flag_ptr){
    tls_release_begin();
    flag_ptr->flag=UNLOCKED;
    tls_release_end();
  }
  static inline void tls_flag_wait(sflag_t *flag_ptr){
    tls_acquire_begin();
    while(flag_ptr->flag!=UNLOCKED)
      tls_acquire_retry();
    tls_acquire_end();
  }

#endif
#endif

#ifdef TASKSCALAR
  void sesc_begin_versioning(void);
  int  sesc_fork_successor(void);
  void sesc_become_safe(void);
  void sesc_end_versioning(void);
  int  sesc_is_safe(int pid);
  int  sesc_commit();
  int  sesc_prof_commit(int id);
#endif

#ifdef __cplusplus
}
#endif
#endif                          /* SESCAPI_H */
