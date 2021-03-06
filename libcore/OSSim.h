/* 
   SESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.
   
   Contributed by Jose Renau
                  Basilio Fraguela
                  James Tuck
                  Wei Liu
                  Milos Prvulovic
                  Luis Ceze
                  Smruti Sarangi
                  Paul Sack


This file is part of SESC.

SESC is free software; you can redistribute it and/or modify it under the terms
of the GNU General Public License as published by the Free Software Foundation;
either version 2, or (at your option) any later version.

SESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should  have received a copy of  the GNU General  Public License along with
SESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef _OSSim_H_
#define _OSSim_H_

#include <sys/time.h>

#include <list>
#include <vector>
#include <map>
#include <queue>

#include "nanassert.h"
#include "Snippets.h"

#include "callback.h"

#include "ProcessId.h"
#include "RunningProcs.h"


#include "Events.h"

#include "GStats.h"

#include "DDR2.h"

// Interface that the backend should extend for intercepting the user events. It
// also must be extended for changing the mapping policies for the threads. For
// example, SMT and CMP may have different mapping policies.

class GProcessor;

class OSSim {
private:

  static char *benchName;
  char *reportFile;
  char *traceFile;

  // added 11/19/04 DVDB
  char *thermFile;

  timeval stTime;
  double  frequency;

  char *benchRunning;
  bool justTest;

  bool NoMigration; // Configuration option that dissables migration (optional)
  // Number of instructions to skip passed as parameter when the
  // simulation is invoked. -w10000 would skip the first 10000
  // instructions. A cheap way to implement FastSimBegin
  long long nInst2Skip;
  long long nInst2Sim;
  long long nInstCommited2Sim;

  unsigned long long snapshotGlobalClock;

/********************/
  long    paramThreads;

  int nextProcToSched;

  bool       * parallelizeRegion;
  const char * parallelSection;

  Time_t regionBeginTime;

  GStatsHist ** confTimingHist;
  GStatsHist totalParallelTime;
  GStatsCntr ** nRegionOccur;
/********************/

  typedef struct { 
    Pid_t pid;
    ulong total;
    ulong begin;
    ulong end;
    bool  mtMarks;
  } SimulationMark_t;
  
  SimulationMark_t simMarks;
  std::map<int,SimulationMark_t> idSimMarks;
  
  long numIdSimMarks;
  long waitBeginIdSimMarks;
  long waitEndIdSimMarks;
  
  
protected:
  StaticCallbackMember0<RunningProcs, &RunningProcs::finishWorkNow> finishWorkNowCB;

  void processParams(int argc, char **argv, char **envp);

public:
  
  //DDR2 sub-system with multiple channels
  static DDR2 **DRAM;

  RunningProcs cpus;
  OSSim(int argc, char **argv, char **envp);
  virtual ~OSSim();

  void report(const char *str);

  GProcessor *pid2GProcessor(Pid_t pid);
  ProcessIdState getState(Pid_t pid);
  GProcessor *id2GProcessor(CPU_t cpu);

  bool trace() const { return (traceFile != 0); }



  virtual void preEvent(Pid_t pid, long vaddr, long type, void *sptr) {
    MSG("preevent(%ld, %ld, %p) pid %d", vaddr, type, sptr, pid);
  }

  virtual void postEvent(Pid_t pid, long vaddr, long type, const void *sptr) {
    // Notice that the sptr is a const. This is because the postEvent
    // is notified much latter than it was really executed. If the
    // backend tryies to modify (give data back) to the application
    // through the postEvent, it would NOT WORK. Instead use preEvent
    // to pass data from the backend to the application.
    MSG("postevent(%ld, %ld, %p) pid %d @%lld", vaddr, type, sptr, pid, (long long)globalClock);
  }

  virtual void memBarrierEvent(Pid_t pid, long vaddr, long type, const void *sptr) {
    MSG("membarrier(%ld, %ld, %p) pid %d @%lld", vaddr, type, sptr, pid, (long long)globalClock);
  }

  // Those functions are only callable through the events. Not
  // directly inside the simulator.

  virtual void eventSpawnOpcode(int pid, const int *params, int nParams) {
    MSG("spawnOpcode(%p, %d) pid %d", params, nParams, pid);
  }

  // Spawns a new process newPid with given flags
  // If stopped is true, the new process will not be made runnable
  void eventSpawn(Pid_t curPid, Pid_t newPid, long flags, bool stopped=false);
  void eventSysconf(Pid_t curPid, Pid_t targPid, long flags);
  long eventGetconf(Pid_t curPid, Pid_t targPid);
  void eventExit(Pid_t cpid, int err);
  void tryWakeupParent(Pid_t cpid);
  void eventWait(Pid_t cpid);
  int eventSuspend(Pid_t cpid, Pid_t tid);
  int eventResume(Pid_t cpid, Pid_t tid);
  int eventYield(Pid_t cpid, Pid_t yieldID);
  void eventSaveContext(Pid_t pid);
  void eventLoadContext(Pid_t pid);
  void eventSetInstructionPointer(Pid_t pid, icode_ptr picode);
  icode_ptr eventGetInstructionPointer(Pid_t pid);
  void eventSetPPid(Pid_t pid, Pid_t ppid);
  Pid_t eventGetPPid(Pid_t pid);

#ifdef OLDMARKS
  void eventSimulationMark() {
    simulationMarks++;
  }
  ulong getSimulationMark() const { return simulationMarks; }
  ulong getSimulationMark1() const { return simulationMark1; }
  ulong getSimulationMark2() const { return simulationMark2; }
  bool enoughMarks1() const { return simulationMarks > simulationMark1; }
  bool enoughMarks2() const { return simulationMarks > simulationMark2; }
#else
  void eventSimulationMark() {
    simMarks.total++;
  }
  void eventSimulationMark(int id,Pid_t pid) {
    if(idSimMarks.find(id)==idSimMarks.end()) {
      idSimMarks[id].total = 0;
      idSimMarks[id].begin = 0;
      idSimMarks[id].end = (~0UL)-1;
    }

    idSimMarks[id].total++;
    idSimMarks[id].pid=pid;
    idSimMarks[id].mtMarks=true;
  }
  ulong getSimulationMark() const { return simMarks.total; }
  ulong getSimulationMark1() const { return simMarks.begin; }
  ulong getSimulationMark2() const { return simMarks.end; }
  // swang: change following > to >=
  bool enoughMarks1() const { return simMarks.total >= simMarks.begin; }
  bool enoughMarks2() const { return simMarks.total > simMarks.end; }
#endif

#ifndef OLDMARKS
  ulong getSimulationMark(int id) const { 
    std::map<int,SimulationMark_t>::const_iterator it = idSimMarks.find(id);
    return (*it).second.total; 
  }
  ulong getSimulationMark1(int id) const { 
    std::map<int,SimulationMark_t>::const_iterator it = idSimMarks.find(id);
    return (*it).second.begin; 
  }
  ulong getSimulationMark2(int id) const { 
    std::map<int,SimulationMark_t>::const_iterator it = idSimMarks.find(id);
    return (*it).second.end;
  }
  bool enoughMarks1(int id) const {
    std::map<int,SimulationMark_t>::const_iterator it = idSimMarks.find(id);
    return (*it).second.total > (*it).second.begin; 
  }
  bool enoughMarks2(int id) const {
    std::map<int,SimulationMark_t>::const_iterator it = idSimMarks.find(id); 
    return (*it).second.total > (*it).second.end; 
  }
  bool enoughMTMarks1() const {
    std::map<int,SimulationMark_t>::const_iterator it = idSimMarks.begin();

    bool ret=true;
    for(it=idSimMarks.begin(); it!=idSimMarks.end(); it++) {
      ret = (ret && enoughMarks1( (*it).first ));
    }

    return ret;
  }
  bool enoughMTMarks1(int pid,bool justMe) const {
    std::map<int,SimulationMark_t>::const_iterator it = idSimMarks.begin();
    bool me=false;
    bool ret=true;
    for(it=idSimMarks.begin(); it!=idSimMarks.end(); it++) {
      if( (*it).second.pid != pid )
	ret = (ret && enoughMarks1( (*it).first ));
      else if( (*it).second.mtMarks )
	me = enoughMarks1( (*it).first );
    }
    if(justMe)
      ret=me;

    return ret;
  }
#endif

#ifdef TS_PROFILING
  Profile *getProfiler() const {
    return profiler;
  }
  int getProfPhase() const {
    return profPhase;
  }
  const char *getProfSectionName() const {
    return profSectionName;
  }
#endif


#ifdef TS_RISKLOADPROF
  RiskLoadProf *getRiskLoadProf() const {
    return riskLoadProf;
  }  
#endif

  ThreadContext *getContext(Pid_t pid);

  long getContextRegister(Pid_t pid, int regnum);

  void suspend(Pid_t pid) {
    eventSuspend(-1,pid);
  }

  void resume(Pid_t pid) {
    eventResume(-1,pid);
  }

  // Makes a runnable process stopped
  void stop(Pid_t pid);
  // Makes a stopped process runnable
  void unstop(Pid_t pid);

  // Sets the priority of a process 
  void setPriority(Pid_t pid, int newPrio);

  // Returns the current priority of a process 
  int getPriority(Pid_t pid);
    
  // Removes from cpu a running thread (only if necessary), and
  // activates the pid thread.
  Pid_t contextSwitch(CPU_t cpu, Pid_t nPid);

  // Currently, a CPU can never stop to exist (unregister). Maybe,
  // some fault tolerance freak needs this feature. In that case, he/she
  // should implemented it.
  void registerProc(GProcessor *core);
  void unRegisterProc(GProcessor *core);

  static const char *getBenchName(){
    return benchName;
  }
  const char *getReportFileName(){
    return reportFile;
  }

  void initBoot();
  void preBoot();
  void postBoot();

  void simFinish();
  
  // Boot the whole simulator. Restart is set to true by the exception
  // handler. This may happen in a misspeculation, the simulator would be
  // restarted.
  virtual void boot() {
    initBoot();
    preBoot();
    if(!justTest)
      postBoot();
  }

  void reportOnTheFly(const char *file=0);

  double getFrequency() const { return frequency; }

  size_t getNumCPUs() const { return cpus.size(); }

  void stopSimulation() { cpus.finishWorkNow(); }
  void switchOut(CPU_t id, ProcessId *procId) { cpus.switchOut(id, procId); }

  long long getnInst2Sim() const { return nInst2Sim;  }
  long long getnInstCommited2Sim() const { return nInstCommited2Sim; }

  bool hasWork() const { return cpus.hasWork(); }
 
  void pseudoReset() {snapshotGlobalClock = globalClock;} 

public:

  void initializeReconfiguration(int numRegions);
  int  beginRegion(int regionNumber);
  void endRegion(int regionNumber);
  void startNextApplication(char* appName);
//************
};

typedef CallbackMember4<OSSim, Pid_t, long, long, const void *, &OSSim::postEvent> postEventCB;

extern OSSim *osSim;
extern double etop(double energy);

#endif   // OSSim_H
