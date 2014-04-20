/* 
   SESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Jose Renau
                  Luis Ceze

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
#ifndef DINST_H
#define DINST_H

#include "pool.h"
#include "nanassert.h"

#include "ThreadContext.h"
#include "Instruction.h"
#include "callback.h"

#include "Snippets.h"
#include "GStats.h"

class Resource;
class FetchEngine;
class FReg;
class BPredictor;


// FIXME: do a nice class. Not so public
class DInstNext {
 private:
  DInst *dinst;

 public:
  DInstNext() {
    dinst = 0;
  }

  DInstNext *nextDep;
  bool       isUsed; // true while non-satisfied RAW dependence
  const DInstNext *getNext() const { return nextDep; }
  DInstNext *getNext() { return nextDep; }
  void setNextDep(DInstNext *n) {
    nextDep = n;
  }

  void init(DInst *d) {
    I(dinst==0);
    dinst = d;
  }

  DInst *getDInst() const { return dinst; }

  void setParentDInst(DInst *d) { }

};

class DInst {
 public:
  // In a typical RISC processor MAX_PENDING_SOURCES should be 2
  static const int MAX_PENDING_SOURCES=2;
  
  //DDR2
  int sequenceNum;
  int coreID;

private:

  static pool<DInst> dInstPool;

  DInstNext pend[MAX_PENDING_SOURCES];
  DInstNext *last;
  DInstNext *first;
  
  int cId;

  // BEGIN Boolean flags
  bool loadForwarded;
  bool issued;
  bool executed;
  bool depsAtRetire;
  bool deadStore;
  bool deadInst;

  bool resolved; // For load/stores when the address is computer, for
		 // the rest of instructions when it is executed


#ifdef SESC_MISPATH
  bool fake;
#endif
  // END Boolean flags

  // BEGIN Time counters
  Time_t wakeUpTime;
  // END Time counters

#ifdef BPRED_UPDATE_RETIRE
  BPredictor *bpred;
  InstID oracleID;
  long nextPC;
#endif

  const Instruction *inst;
  VAddr vaddr;
  Resource    *resource;
  DInst      **RATEntry;

 private:

  FetchEngine *fetch;

  CallbackBase *pendEvent;

  char nDeps;              // 0, 1 or 2 for RISC processors

#ifdef DEBUG
 public:
  static long currentID;
  long ID; // static ID, increased every create (currentID). pointer to the
  // DInst may not be a valid ID because the instruction gets recycled
#endif

 protected:
 public:
  DInst();

  void doAtSimTime();
  StaticCallbackMember0<DInst,&DInst::doAtSimTime>  doAtSimTimeCB;

  void doAtSelect();
  StaticCallbackMember0<DInst,&DInst::doAtSelect>  doAtSelectCB;

  DInst *clone();

  void doAtExecuted();
  StaticCallbackMember0<DInst,&DInst::doAtExecuted> doAtExecutedCB;


  static DInst *createInst(InstID pc, VAddr va, int cId);
  static DInst *createDInst(const Instruction *inst, VAddr va, int cId);

  void killSilently();
  void scrap(); // Destroys the instruction without any other effects
  void destroy();

  void setResource(Resource *res) {
    I(!resource);
    resource = res;
  }
  Resource *getResource() const { return resource; }

  void setRATEntry(DInst **rentry) {
    I(!RATEntry);
    RATEntry = rentry;
  }

#ifdef BPRED_UPDATE_RETIRE
  void setBPred(BPredictor *bp, InstID oid) {
    I(oracleID==0);
    I(bpred == 0);
    bpred     = bp;
    oracleID = oid;
    nextPC  = Instruction::getInst(oracleID)->getAddr();
  }
#endif

  void setFetch(FetchEngine *fe) {
    I(!isFake());

    fetch = fe;
  }

  FetchEngine *getFetch() const {
    return fetch;
  }

  void addEvent(CallbackBase *cb) {
    I(inst->isEvent());
    I(pendEvent == 0);
    pendEvent = cb;
  }

  CallbackBase *getPendEvent() {
    return pendEvent;
  }

  DInst *getNextPending() {
    I(first);
    DInst *n = first->getDInst();

    I(n);

    I(n->nDeps > 0);
    n->nDeps--;

    first->isUsed = false;
    first->setParentDInst(0);
    first = first->getNext();

    return n;
  }


  void addSrc1(DInst * d) {
    I(d->nDeps < MAX_PENDING_SOURCES);
    d->nDeps++;
    
    DInstNext *n = &d->pend[0];
    I(!n->isUsed);
    n->isUsed = true;
    n->setParentDInst(this);

    I(n->getDInst() == d);
    if (first == 0) {
      first = n;
    } else {
      last->nextDep = n;
    }
    n->nextDep = 0;
    last = n;
  }

  void addSrc2(DInst * d) {
    I(d->nDeps < MAX_PENDING_SOURCES);
    d->nDeps++;
    
    DInstNext *n = &d->pend[1];
    I(!n->isUsed);
    n->isUsed = true;
    n->setParentDInst(this);

    I(n->getDInst() == d);
    if (first == 0) {
      first = n;
    } else {
      last->nextDep = n;
    }
    n->nextDep = 0;
    last = n;
  }

  void addFakeSrc(DInst * d) {
    I(d->nDeps < MAX_PENDING_SOURCES);
    d->nDeps++;
    
    DInstNext *n = &d->pend[1];
    I(!n->isUsed);
    n->isUsed = true;
    n->setParentDInst(this);

    I(n->getDInst() == d);
    if (first == 0) {
      first = n;
    } else {
      last->nextDep = n;
    }
    n->nextDep = 0;
    last = n;
  }

  char getnDeps() const { return nDeps; }

  bool isStallOnLoad() const {  return false; }

  bool isSrc1Ready() const { return !pend[0].isUsed; }
  bool isSrc2Ready() const { return !pend[1].isUsed; }
  bool hasDeps()     const { 
    GI(!pend[0].isUsed && !pend[1].isUsed, nDeps==0);
    return nDeps!=0;
  }
  bool hasPending()  const { return first != 0;  }
  const DInst *getFirstPending() const { return first->getDInst(); }
  const DInstNext *getFirst() const { return first; }

  const Instruction *getInst() const { return inst; }

  VAddr getVaddr() const { return vaddr;  }

  int getContextId() const { return cId; }

  void dump(const char *id) const;

  // methods required for LDSTBuffer
  bool isLoadForwarded() const { return loadForwarded; }
  void setLoadForwarded() {
    I(!loadForwarded);
    loadForwarded=true;
  }

  bool isIssued() const { return issued; }
  void markIssued() {
    I(!issued);
    I(!executed);
    issued = true;
  }

  bool isExecuted() const { return executed; }
  void markExecuted() {
    I(issued);
    I(!executed);
    executed = true;
  }

  bool isDeadStore() const { return deadStore; }
  void setDeadStore() { 
    I(!deadStore);
    I(!hasPending());
    deadStore = true; 
  }

  void setDeadInst() { deadInst = true; }
  bool isDeadInst() const { return deadInst; }

  
  bool hasDepsAtRetire() const { return depsAtRetire; }
  void setDepsAtRetire() { 
    I(!depsAtRetire);
    depsAtRetire = true;
  }
  void clearDepsAtRetire() { 
    I(depsAtRetire);
    depsAtRetire = false;
  }

  bool isResolved() const { return resolved; }
  void markResolved() { 
    resolved = true; 
  }

  bool isEarlyRecycled() const { return false; }

#ifdef SESC_MISPATH
  void setFake() { 
    I(!fake);
    fake = true; 
  }
  bool isFake() const  { return fake; }
#else
  bool isFake() const  { return false; }
#endif

  void awakeRemoteInstructions();

  void setWakeUpTime(Time_t t)  { 
    // ??? FIXME: Why fails?I(wakeUpTime <= t); // Never go back in time
    //I(wakeUpTime <= t);
    wakeUpTime = t;
  }

  Time_t getWakeUpTime() const { return wakeUpTime; }

#ifdef DEBUG
  long getID() const { return ID; }
#endif
};

class Hash4DInst {
 public: 
  size_t operator()(const DInst *dinst) const {
    return (size_t)(dinst);
  }
};

#endif   // DINST_H
