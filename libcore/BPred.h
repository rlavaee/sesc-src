/* 
   SESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Jose Renau
                  Smruti Sarangi

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

#ifndef BPRED_H
#define BPRED_H

/*
 * The original Branch predictor code was inspired in simplescalar-3.0, but
 * heavily modified to make it more OOO. Now, it supports more branch predictors
 * that the standard simplescalar distribution.
 *
 * Supported branch predictors models:
 *
 * Oracle, NotTaken, Taken, 2bit, 2Level, 2BCgSkew, Hybrid
 *
 */

#include "nanassert.h"
#include "estl.h"

#include "GStats.h"
#include "Instruction.h"        // InstID
#include "CacheCore.h"
#include "EnergyMgr.h"
#include "SCTable.h"

#define RAP_T_NT_ONLY   1

enum PredType {
  CorrectPrediction = 0,
  NoPrediction,
  NoBTBPrediction,
  MissPrediction
};

class BPred {
public:
  typedef unsigned long long HistoryType;
  class Hash4HistoryType {
  public: 
    size_t operator()(const HistoryType &addr) const {
      return ((addr) ^ (addr>>16));
    }
  };
  
protected:
  const int id;

  GStatsCntr nHit;  // N.B. predictors should not update these counters directly
  GStatsCntr nMiss; // in their predict() function.

  GStatsEnergy *bpredEnergy;

  HistoryType calcInstID(const Instruction *inst) const {
    HistoryType cid = inst->currentID();
    return (cid >> 17) ^ (cid); // randomize it
    //    return cid;
  }
protected:
public:
  BPred(int i, const char *section, const char *name);
  virtual ~BPred();

  // If oracleID is not passed, the predictor is not updaed
  virtual PredType predict(const Instruction *inst, InstID oracleID, bool doUpdate) = 0;

  PredType doPredict(const Instruction *inst, InstID oracleID, bool doUpdate) {
    PredType pred = predict(inst, oracleID, doUpdate);
    if (!doUpdate || pred == NoPrediction)
      return pred;

    nHit.cinc(pred == CorrectPrediction);
    nMiss.cinc(pred != CorrectPrediction);

    return pred;
  }
  
  virtual void switchIn(Pid_t pid)  = 0;
  virtual void switchOut(Pid_t pid) = 0;
};


class BPRas:public BPred {
private:
  const ushort RasSize;

  InstID *stack;
  int index;
  GStatsEnergy *rasEnergy;
protected:
public:
  BPRas(int i, const char *section);
  ~BPRas();
  PredType predict(const Instruction * inst, InstID oracleID, bool doUpdate);

  void switchIn(Pid_t pid);
  void switchOut(Pid_t pid);
};

class BPBTB:public BPred {
private:
  GStatsEnergy *btbEnergy;

  class BTBState : public StateGeneric<> {
  public:
    BTBState() {
      inst = 0;
    }

    InstID inst;

    bool operator==(BTBState s) const {
      return inst == s.inst;
    }
  };

  typedef CacheGeneric<BTBState, ulong, false> BTBCache;
  
  BTBCache *data;
  
protected:
public:
  BPBTB(int i, const char *section, const char *name=0);
  ~BPBTB();

  PredType predict(const Instruction * inst, InstID oracleID, bool doUpdate);
  void updateOnly(const Instruction * inst, InstID oracleID);

  void switchIn(Pid_t pid);
  void switchOut(Pid_t pid);
};

class BPOracle:public BPred {
private:
  BPBTB btb;

protected:
public:
  BPOracle(int i, const char *section)
    :BPred(i, section, "Oracle")
    ,btb(i,section) {
  }

  PredType predict(const Instruction * inst, InstID oracleID, bool doUpdate);

  void switchIn(Pid_t pid);
  void switchOut(Pid_t pid);
};

class BPNotTaken:public BPred {
private:
protected:
public:
  BPNotTaken(int i, const char *section)
    :BPred(i, section, "NotTaken") {
    // Done
  }

  PredType predict(const Instruction * inst, InstID oracleID, bool doUpdate);

  void switchIn(Pid_t pid);
  void switchOut(Pid_t pid);
};

class BPTaken:public BPred {
private:
  BPBTB btb;

protected:
public:
  BPTaken(int i, const char *section)
    :BPred(i, section, "Taken") 
    ,btb(i,section) {
    // Done
  }

  PredType predict(const Instruction * inst, InstID oracleID, bool doUpdate);

  void switchIn(Pid_t pid);
  void switchOut(Pid_t pid);
};

class BPStatic:public BPred {
private:
  BPBTB btb;

protected:
public:
  BPStatic(int i, const char *section)
    :BPred(i, section, "Static") 
    ,btb(i,section) {
    // Done
  }

  PredType predict(const Instruction * inst, InstID oracleID, bool doUpdate);

  void switchIn(Pid_t pid);
  void switchOut(Pid_t pid);
};

class BP2bit:public BPred {
private:
  BPBTB btb;

  SCTable table;
protected:
public:
  BP2bit(int i, const char *section);

  PredType predict(const Instruction * inst, InstID oracleID, bool doUpdate);

  void switchIn(Pid_t pid);
  void switchOut(Pid_t pid);
};

class BP2level:public BPred {
private:
  BPBTB btb;

  const ushort l1Size;
  const ulong  l1SizeMask;

  const ushort historySize;
  const HistoryType historyMask;

  SCTable globalTable;

  HistoryType *historyTable; // LHR
protected:
public:
  BP2level(int i, const char *section);
  ~BP2level();

  PredType predict(const Instruction * inst, InstID oracleID, bool doUpdate);

  void switchIn(Pid_t pid);
  void switchOut(Pid_t pid);
};

class BPHybrid:public BPred {
private:
  BPBTB btb;

  //GAg: GHR size and mask
  const HistoryType gHistSize;
  const HistoryType gHistMask;

  //SAg: BHR size and mask
  const HistoryType lHistSize;
  const HistoryType lHistMask;

  //SAg: Level 1 size and mask
  const HistoryType bhtSize; 
  const HistoryType bhtMask;
  
  //Meta Table Mask
  const HistoryType mMask;

  //GHR and BHTs

  HistoryType *ghr;  // Global History Register

HistoryType *bht; // Branch History Table (for SAg component)

  //Local, global and meta tables
  SCTable localTable;  //SAg PHT
  SCTable globalTable; //GAg PHT
  SCTable metaTable;   //Meta PHT

protected:
public:

  BPHybrid(int i, const char *section);
  ~BPHybrid();
  
  
  PredType predict(const Instruction * inst, InstID oracleID, bool doUpdate);

  void switchIn(Pid_t pid);
  void switchOut(Pid_t pid);
};

//Engin: A21264
/*******************************************************************/
class BPA21264:public BPred {
private:

  //BTB
  BPBTB btb;

  //GAg: GHR size and mask
  const ushort gHistSize;
  const HistoryType gHistMask;

  //SAg: BHR size and mask
  const ushort lHistSize;
  const HistoryType lHistMask;

  //SAg: Level 1 size and mask
  const ushort bhtSize;
  const ushort bhtMask;
  
  //Meta Table Mask
  const HistoryType mMask;

  //GHR and BHTs
  HistoryType ghr;  // Global History Register
  HistoryType *bht; // Branch History Table (for SAg component)

  //Local, global and meta tables
  SCTable localTable;  //SAg PHT
  SCTable globalTable; //GAg PHT
  SCTable metaTable;   //Meta PHT


protected:
public:
  BPA21264(int i, const char *section);
  ~BPA21264();

  PredType predict(const Instruction * inst, InstID oracleID, bool doUpdate);

  void switchIn(Pid_t pid);
  void switchOut(Pid_t pid);
};
/*******************************************************************/

class BP2BcgSkew : public BPred {
private:
  BPBTB btb;

  SCTable BIM;

  SCTable G0;
  const ushort       G0HistorySize;
  const HistoryType  G0HistoryMask;

  SCTable G1;
  const ushort       G1HistorySize;
  const HistoryType  G1HistoryMask;

  SCTable metaTable;
  const ushort       MetaHistorySize;
  const HistoryType  MetaHistoryMask;

  HistoryType history;
protected:
public:
  BP2BcgSkew(int i, const char *section);
  ~BP2BcgSkew();
  
  PredType predict(const Instruction * inst, InstID oracleID, bool doUpdate);

  void switchIn(Pid_t pid);
  void switchOut(Pid_t pid);
};

class BPyags : public BPred {
private:
  BPBTB btb;

  const ushort historySize;
  const HistoryType historyMask;

  SCTable table;
  SCTable ctableTaken;
  SCTable ctableNotTaken;

  HistoryType ghr; // Global History Register

  uchar *CacheTaken;
  HistoryType CacheTakenMask;
  HistoryType CacheTakenTagMask;

  uchar *CacheNotTaken;
  HistoryType CacheNotTakenMask;
  HistoryType CacheNotTakenTagMask;

protected:
public:
  BPyags(int i, const char *section);
  ~BPyags();
  
  PredType predict(const Instruction * inst, InstID oracleID, bool doUpdate);

  void switchIn(Pid_t pid);
  void switchOut(Pid_t pid);
};

class BPredictor {
private:
  const int id;
  const bool SMTcopy;

  BPRas ras;
  BPred *pred;
  
  GStatsCntr nBranches;
  GStatsCntr nTaken;
  GStatsCntr nMiss;           // hits == nBranches - nMiss

  char *section;

protected:
public:
  //Engin:

  BPredictor(int i, const char *section, BPredictor *bpred=0);
  ~BPredictor();

  static BPred *getBPred(int id, const char *sec);

  PredType predict(const Instruction *inst, InstID oracleID, bool doUpdate) {
    I(inst->isBranch());

    nBranches.cinc(doUpdate);
    nTaken.cinc(inst->calcNextInstID() != oracleID);
  
    PredType p= ras.doPredict(inst, oracleID, doUpdate);
    if( p != NoPrediction ) {
      nMiss.cinc(p != CorrectPrediction && doUpdate);
      return p;
    }

    p = pred->doPredict(inst, oracleID, doUpdate);

    nMiss.cinc(p != CorrectPrediction && doUpdate);

    return p;
  }

  void dump(const char *str) const;

  void switchIn(Pid_t pid) {
    pred->switchIn(pid);
  }

  void switchOut(Pid_t pid) {
    pred->switchOut(pid);
  }
};

#endif   // BPRED_H
