#ifndef DARWIN

#include <g2c.h>
#include "sescapi.h"


struct Gmem{
  slock_t idlock;
  sbarrier_t start;
} *Gmem;

extern void sesc_reg_com() {
         __asm__ __volatile__(".word 0":::
	                           //"$c3r31", "$c3r30", "$c3r29",
	                           //"$c3r28", "$c3r27", "$c3r26", "$c3r25", "$c3r24", "$c3r23", "$c3r22",
	                           //"$c3r21", "$c3r20", "$c3r19", "$c3r18", "$c3r17", "$c3r16", "$c3r15",
	                           //"$c3r14", "$c3r13", "$c3r12", "$c3r11", "$c3r10", "$c3r9", "$c3r8",
	                           //"$c3r7", "$c3r6", "$c3r5", "$c3r4", "$c3r3", "$c3r2", "$c3r1", "$c3r0",
	                           //"$c2r31", "$c2r30", "$c2r29", "$c2r28", "$c2r27", "$c2r26", "$c2r25",
	                           //"$c2r24", "$c2r23", "$c2r22", "$c2r21", "$c2r20", "$c2r19", "$c2r18",
	                           //"$c2r17", "$c2r16", "$c2r15", "$c2r14", "$c2r13", "$c2r12", "$c2r11",
	                           //"$c2r10", "$c2r9", "$c2r8", "$c2r7", "$c2r6", "$c2r5", "$c2r4",
	                           //"$c2r3", "$c2r2", "$c2r1", "$c2r0", "$c0r31", "$c0r30", "$c0r29",
	                           //"$c0r28", "$c0r27", "$c0r26", "$c0r25", "$c0r24", "$c0r23", "$c0r22",
	                           //"$c0r21", "$c0r20", "$c0r19", "$c0r18", "$c0r17", "$c0r16", "$c0r15",
	                           //"$c0r14", "$c0r13", "$c0r12", "$c0r11", "$c0r10", "$c0r9", "$c0r8",
	                           //"$c0r7", "$c0r6", "$c0r5", "$c0r4", "$c0r3", "$c0r2", "$c0r1", "$c0r0",
	                           "$fcc7", "$fcc6", "$fcc5", "$fcc4", "$fcc3", "$fcc2", "$fcc1", "$fcc0",
				   //"accum", "lo", "hi", "$f31", "$f30", "$f29", "$f28", "$f27", "$f26",
				"lo", "hi", "$f31", "$f30", "$f29", "$f28", "$f27", "$f26",
				"$f25", "$f24", "$f23", "$f22", "$f21", "$f20", "$f19", "$f18", "$f17",
				"$f16", "$f15", "$f14", "$f13", "$f12", "$f11", "$f10", "$f9", "$f8",
				"$f7", "$f6", "$f5", "$f4", "$f3", "$f2", "$f1", "$f0", "$31", "$30",
				"$fp", "$sp", "$27", "$26", "$25", "$24", "$23", "$22", "$21", "$20",
 				"$19", "$18", "$17", "$16", "$15", "$14", "$13", "$12", "$11", "$10",
				"$9", "$8", "$7", "$6", "$5", "$4", "$3", "$2", "$1", "$0", "ra", "cc",
				"memory");
}

/* // Fortran */
/* void w_sesc_begin_versioning_() {sesc_begin_versioning();} */
/* int  w_sesc_fork_successor_(int t) {return t=sesc_fork_successor();} */
/* void w_sesc_become_safe_() {sesc_become_safe();} */
/* void w_sesc_end_versioning_() {sesc_end_versioning();} */
/* int  w_sesc_is_safe_(int pid) {return sesc_is_safe(pid);} */
/* int  w_sesc_commit_() {return sesc_commit();} */
/* int  w_sesc_prof_commit_(int id) {return sesc_prof_commit(id);} */
/* void w_sesc_reg_com_() {sesc_reg_com();} */

/* // Fortran 2 */
/* void w_sesc_begin_versioning__() {sesc_begin_versioning();} */
/* int  w_sesc_fork_successor__(int t) {return t=sesc_fork_successor();} */
/* void w_sesc_become_safe__() {sesc_become_safe();} */
/* void w_sesc_end_versioning__() {sesc_end_versioning();} */
/* int  w_sesc_is_safe__(int pid) {return sesc_is_safe(pid);} */
/* int  w_sesc_commit__() {return sesc_commit();} */
/* int  w_sesc_prof_commit__(int id) {return sesc_prof_commit(id);} */
/* void w_sesc_reg_com__() {sesc_reg_com();} */

/* // C */
/* void w_sesc_begin_versioning() {sesc_begin_versioning();} */
/* int  w_sesc_fork_successor(int t) {return t=sesc_fork_successor();} */
/* void w_sesc_become_safe() {sesc_become_safe();} */
/* void w_sesc_end_versioning() {sesc_end_versioning();} */
/* int  w_sesc_is_safe(int pid) {return sesc_is_safe(pid);} */
/* int  w_sesc_commit() {return sesc_commit();} */
/* int  w_sesc_prof_commit(int id) {return sesc_prof_commit(id);} */
/* void w_sesc_reg_com() {sesc_reg_com();} */



/************* ENGIN's WRAPPERS BEGIN HERE *****************/
void w_mem_init__(){
  Gmem = (struct Gmem *) malloc(sizeof(struct Gmem));
}

void w_sesc_lock__() {sesc_lock(&(Gmem->idlock));}
void w_sesc_lock_init__() {sesc_lock_init(&(Gmem->idlock));}
void w_sesc_unlock__() {sesc_unlock(&(Gmem->idlock));}

void w_sesc_barrier_init__() {sesc_barrier_init(&(Gmem->start));}
void w_sesc_barrier__(int *numCpus){sesc_barrier(&(Gmem->start),*numCpus);}

void w_sesc_spawn__(void (*start_routine) (void *)){sesc_spawn(start_routine,0,0);}

//For the adaptive CMP
void w_sesc_set_num_region__(int *numRegions){
  sesc_set_num_region(*numRegions);
}
void w_sesc_begin_region__(int *regionNumber, int *numThreads){
  *numThreads = sesc_begin_region(*regionNumber);
}
void w_sesc_end_region__(int *regionNumber){
  sesc_end_region(*regionNumber);
}

//For spawning threads in swim

extern void engin_func4__();
extern void engin_func3__();
extern void engin_func2__();
extern void engin_func1__();


//extern void main_bzip2();
//extern void applu_();

void w_sesc_func4__(int *numCpus){
  int i;
  for(i=1;i<*numCpus;i++){
    sesc_spawn(engin_func4__,0,0);
  }
}
void w_sesc_func3__(int *numCpus){
  int i;
  for(i=1;i<*numCpus;i++){
    sesc_spawn(engin_func3__,0,0);
  }
}
void w_sesc_func2__(int *numCpus){
  int i;
  for(i=1;i<*numCpus;i++){
    sesc_spawn(engin_func2__,0,0);
  }
}
void w_sesc_func1__(int *numCpus){
  int i;
  for(i=1;i<*numCpus;i++){
    sesc_spawn(engin_func1__,0,0);
  }
}

void w_sesc_wait_for_end__(int *numCpus){
  int i;
  for (i = 1; i < *numCpus; i++){
    sesc_wait();
  }
}

void w_sesc_fast_sim_begin__(){
  sesc_fast_sim_begin();
}

void w_sesc_fast_sim_end__(){
  sesc_fast_sim_end();
}

//void w_sesc_bzip2__(){
//  sesc_spawn(main_bzip2,0,0);
//}

//void w_sesc_applu__(){
//  sesc_spawn(applu_,0,0);
//}

void w_sesc_wait_for_end1__() {
  sesc_wait();
}

#endif
