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


#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "estl.h"

#include "nanassert.h"

#include "icode.h"
#include "ThreadContext.h"

#include "Events.h"
#include "Snippets.h"
#include "callback.h"

// 0 int, 1 FP, 2 none
#define INSTRUCTION_MAX_DESTPOOL 3

#include "InstType.h"

typedef ulong InstID;

enum RegType {
    NoDependence = 0,
    ReturnReg = 31,
    InternalReg,
    IntFPBoundary = 32+1,
    HiReg = 65+1,
    CoprocStatReg,
    InvalidOutput,
    NumArchRegs // = BaseInstTypeReg +MaxInstType old times
};

typedef unsigned char MemDataSize;

//! Static Instruction type
/*! For each assembler in the binary there is an Instruction
 *  associated. It is an expanded version of the MIPS instruction.
 *
 */
class Instruction {
public:
  static icode_ptr LowerLimit;
  static icode_ptr UpperLimit;

private:
  static size_t InstTableSize;
  static int    maxFuncID;
  static Instruction *InstTable;
  
  typedef HASH_MAP<long, Instruction*> InstHash;
  static InstHash instHash;

  static bool inLimits(icode_ptr next) {
    return !(next < LowerLimit || next > UpperLimit);
  }

  static void initializeMINT(int argc,
			     char **argv,
			     char **envp);
  
  static void initializeTrace(int argc,
			      char **argv,
			      char **envp);

  static void MIPSDecodeInstruction(size_t index
				    ,icode_ptr &picode
				    ,InstType &opcode
				    ,InstSubType &subCode
				    ,MemDataSize &dataSize
				    ,RegType &src1
				    ,RegType &dest
				    ,RegType &src2
				    ,EventType &uEvent
				    ,bool &BJCondLikely
				    ,bool &guessTaken
				    ,bool &jumpLabel);

protected:
  InstType opcode;
  RegType src1;
  RegType src2;
  RegType dest;
  // MINT notifies the call and the return of the instrumented
  // events. To avoid the overhead of the call and the return, I
  // mark the first instruction in the event function with the event
  // type (uEvent). Note that this is different from the iEvent
  // instruction type.

  EventType uEvent;
  InstSubType subCode;
  MemDataSize dataSize;

  char src1Pool;  // src1 register is in the FP pool?
  char src2Pool;  // src2 register is in the FP pool?
  char dstPool;   // Destination register is in the FP pool?
  char skipDelay; // 1 when the instruction has delay slot (iBJ ^ !BJCondLikely only)

  bool guessTaken;
  bool condLikely;
  bool jumpLabel; // If iBJ jumps to offset (not register)

#ifdef AIX
  const char *funcName;
#endif

public:
  static void initialize(int argc,
			 char **argv,
			 char **envp);  
  
  static void finalize();

  static const Instruction *getInst(InstID id) {
    // I(id <InstTableSize);
    return &InstTable[id];
  }

  static const Instruction *getInst4Addr(long addr) {
    icode_ptr picode = addr2icode(addr);
    return getInst(picode->instID);
  }

  bool hasDestRegister() const {
    GI(dstPool==2, dest == InvalidOutput);
    return dest != InvalidOutput;
  }
  bool hasSrc1Register() const {
    GI(src1Pool==2, src1 == NoDependence);
    return src1 != NoDependence;
  }
  bool hasSrc2Register() const {
    GI(src2Pool==2, src2 == NoDependence);
    return src2 != NoDependence;
  }
  static unsigned char whichPool(RegType r) {
    unsigned char p;
    if( r < IntFPBoundary && r != NoDependence ) {
      p  = 0;
    }else if( r >= IntFPBoundary && r <= CoprocStatReg ) {
      p  = 1;
    }else{
      I(r == InvalidOutput || r == NoDependence);
      p  = 2; // void null
    }
    I(p<INSTRUCTION_MAX_DESTPOOL);

    return p;
  }
  
  InstID currentID() const {  return (this - InstTable);  }

  long getAddr() const {
    icode_ptr picode = Itext[currentID()];
    I(picode->instID == currentID());
    return picode->addr;
  }

  icode_ptr getICode() const{ 
    icode_ptr picode = Itext[currentID()];
    I(picode->instID == currentID());
    return picode ;  
  }
  
  long getRawInst() const {
    return Itext[currentID()]->instr;
  }

  bool guessAsTaken() const { return guessTaken; }
  bool isBJLikely() const   { return condLikely; }
  bool isBJCond() const     { return opcode == iBJ && subCode == BJCond; }
  bool doesJump2Label() const {
    I(opcode==iBJ);
    return jumpLabel;
  }

  int getSrc1Pool() const { return src1Pool; } //  0 Int , 1 FP, 2 none
  int getSrc2Pool() const { return src2Pool; } //  0 Int , 1 FP, 2 none
  int getDstPool() const  { return dstPool;  } //  0 Int , 1 FP, 2 none

  InstID getTarget() const {
    I(opcode == iBJ);
    icode_ptr picode = Itext[currentID()];
    if( picode->target )
      return picode->target->instID;
    return calcNextInstID();
  }

  InstID calcNextInstID() const { return currentID()+skipDelay;  }

  RegType getSrc2() const { return src2;  }
  RegType getDest() const { return dest;  }
  RegType getSrc1() const { return src1;  }

  bool isBranchTaken() const { return subCode == BJUncond; } // subCode == iBJUncond -> opcode == iBJ
  bool isBranch() const      { return opcode == iBJ; }
  bool isFuncCall() const    { return subCode == BJCall; }   // subCode == iBJCall -> opcode == iBJ
  bool isFuncRet() const     { return subCode == BJRet;  }   // subCode == iBJRet -> opcode == iBJ

  bool isMemory() const {
    // Important: it is not possible to use (picode->opflags & E_MEM_REF)
    // because some memory instructions at the beginning of the libcall are
    // modeled as returns, not as memory
    return (subCode == iMemory || opcode == iFence);
  }

  bool isFence() const { return opcode == iFence; }
  bool isLoad() const  { return opcode == iLoad;  }

  // In the stores the src1 is the address where the data is going to
  // be stored.  src2 is the data to be stored
  bool isStore() const { return opcode == iStore; }
  bool isStoreAddr() const { return subCode == iFake; } // opcode == iALU 

  MemDataSize getDataSize() const { return dataSize; }

  bool isType(InstType t) const { return t == opcode;  }

  bool isEvent() const { return uEvent != NoEvent; }
  EventType getEvent() const { return uEvent;  }
  InstType getOpcode() const { return opcode;  }
  InstSubType getSubCode() const { return subCode;  }

  bool isNOP() const { return subCode == iNop;  }

  // Get the name of a given opcode
  static const char *opcode2Name(InstType op);
  
  static InstID getEventID(EventType ev) { return (InstTableSize - MaxEvent) + ev;  }

  static InstID getTableSize() { return InstTableSize;  }
  static int getMaxFuncID()    { return maxFuncID;  }

  void dump(const char *str) const;

  // This class is useful for STL hash_maps. It is more efficient that
  // the default % size
  class HashAddress{
  public: 
    size_t operator()(const long &addr) const {
      return ((addr>>2) ^ (addr>>18));
    }
  };
};

#endif   // INSTRUCTION_H
