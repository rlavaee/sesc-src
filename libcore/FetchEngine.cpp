/* 
   SESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Jose Renau
                  Milos Prvulovic
                  Smruti Sarangi
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

#include <alloca.h>

#include "SescConf.h"
#include "FetchEngine.h"
#include "OSSim.h"
#include "LDSTBuffer.h"
#include "GMemorySystem.h"
#include "GMemoryOS.h"
#include "GProcessor.h"
#include "MemRequest.h"
#include "Pipeline.h"

#include <limits.h>

#ifdef SESC_INORDER
#include <time.h>
#include "GEnergy.h"
#include "GProcessor.h"
#endif


long long FetchEngine::nInst2Sim=0;
long long FetchEngine::totalnInst=0;


FetchEngine::FetchEngine(int cId
			 ,int i
			 // TOFIX: GMemorySystem is in GPRocessor too. This is
			 // not consistent. Remove it from GProcessor and be
			 // sure that everybody that uses it gets it from
			 // FetchEngine (note that an SMT may have multiple
			 // FetchEngine)
			 ,GMemorySystem *gmem
			 ,GProcessor *gp
			 ,FetchEngine *fe)
  :Id(i)
  ,cpuId(cId)
  ,gms(gmem)
  ,gproc(gp) 
  ,pid(-1)
  ,flow(cId, i, gmem)
  ,nMispathFetchCycles("FetchEngine(%d):nMispathFetchCycles",i) //NM
  ,avgBranchTime("FetchEngine(%d)_avgBranchTime", i)
  ,avgInstrsFetched("FetchEngine(%d)_avgInstrsFetched",i)
  ,nDelayInst1("FetchEngine(%d):nDelayInst1", i)
  ,nDelayInst2("FetchEngine(%d):nDelayInst2", i) // Not enough BB/LVIDs per cycle 
  ,nFetched("FetchEngine(%d):nFetched",i)
  ,nBTAC("FetchEngine(%d):nBTAC", i) // BTAC corrections to BTB
  ,unBlockFetchCB(this)
{
  // Constraints
  SescConf->isLong("cpucore", "fetchWidth",cId);
  SescConf->isBetween("cpucore", "fetchWidth", 1, 1024, cId);
  FetchWidth = SescConf->getLong("cpucore", "fetchWidth", cId);

  SescConf->isBetween("cpucore", "bb4Cycle",0,1024,cId);

  //Engin:
  const char *instrSourceSection = gms->getInstrSource()->getDescrSection();
  iCacheBlockSizeLog2 = log2i(SescConf->getLong(instrSourceSection,"bsize")) ;

  BB4Cycle = SescConf->getLong("cpucore", "bb4Cycle",cId);

  if( BB4Cycle == 0 )
    BB4Cycle = USHRT_MAX;
  
  const char *bpredSection = SescConf->getCharPtr("cpucore","bpred",cId);
  
  if( fe )
    bpred = new BPredictor(i,bpredSection,fe->bpred);
  else
    bpred = new BPredictor(i,bpredSection);

  SescConf->isLong(bpredSection, "BTACDelay");
  SescConf->isBetween(bpredSection, "BTACDelay", 0, 1024);
  BTACDelay = SescConf->getLong(bpredSection, "BTACDelay");

  missInstID = 0;
#ifdef SESC_MISPATH
  issueWrongPath = SescConf->getBool("","issueWrongPath");
#endif
  nGradInsts  = 0;
  nWPathInsts = 0;
 
  // Get some icache L1 parameters
  enableICache = SescConf->getBool("","enableICache");
  if (enableICache) {
    IL1HitDelay = 0;
  }else{
    const char *iL1Section = SescConf->getCharPtr("cpucore","instrSource", cId);
    if (iL1Section) {
      char *sec = strdup(iL1Section);
      char *end = strchr(sec, ' ');
      if (end) 
        *end=0; // Get only the first word
      // Must be the i-cache
      SescConf->isInList(sec,"deviceType","icache");

      IL1HitDelay = SescConf->getLong(sec,"hitDelay");
    }else{
      IL1HitDelay = 1; // 1 cycle if impossible to find the information required
    }
  }

  gproc = osSim->id2GProcessor(cpuId);

#ifdef SESC_INORDER
  char strTime[16], fileName[64];
  const char *benchName;
  time_t rawtime;
  struct tm * timeinfo;

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  strftime(strTime, 16, "%H%M",timeinfo);
  benchName = OSSim::getBenchName();
  strcpy(fileName,"/home/masc/spkale/SWITCH/deltas_");
  strncat(fileName,benchName,strlen(benchName));
  //  strncat(fileName,strTime,strlen(strTime));

#ifdef SESC_INORDER_ENERGY
  energyInstFile = fopen(fileName, "w");
  if(energyInstFile == NULL){
    printf("Error, could not open file energy_instr file for writing\n");
  }else{
     fprintf(energyInstFile,"#interval\tenergy\ttime\n");
  }
#endif

  instrCount = 0;
  intervalCount = 0;
  previousTotEnergy = 0.0;
  previousClockCount = 0;

#ifdef SESC_INORDER_SWITCH
  printf("Opening switch file\n"); 

  char switchFileName[64];
  strcpy(switchFileName,"/home/masc/spkale/04_04_05/");
  strcat(switchFileName, OSSim::getBenchName());
  strcat(switchFileName,"/eddTally.dat");
  printf("Opening file:%s\n", switchFileName); 
  switchFile = fopen(switchFileName, "r");

  if(switchFile == NULL){
    printf("Error, could not open file energy_instr file for writing\n");
  }else{
    int mode = getNextCoreMode();
    gproc->setMode(mode);	
  }  
#endif
#endif
}

#ifdef SESC_INORDER
int FetchEngine::gatherRunTimeData()
{
#ifdef SESC_INORDER_ENERGY
    instrCount++;

#if 0
    if(intervalCount > 2000 && intervalCount < 3000){
      if(instrCount == 200){

	++subIntervalCount;

	if(subIntervalCount % 10 == 0){
	  ++intervalCount;
	  subIntervalCount = 0;
	}
	
	double energy = GStatsEnergy::getTotalEnergy();
	double delta_energy = energy - previousTotEnergy;
	long delta_time = globalClock - previousClockCount;
	
	fprintf(energyInstFile,"%d\t%.3f\t%ld\n", intervalCount * 10 + subIntervalCount, delta_energy, delta_time);


        previousTotEnergy =  GStatsEnergy::getTotalEnergy();
        previousClockCount = globalClock;

        instrCount = 0;

      }/* End if instrCount == 200 */
    }
#endif

    if(instrCount == 2000) {
      intervalCount++;
     
#ifdef SESC_INORDER_SWITCH
      int mode = 0;
      /* Get next core change */
      // int mode = getNextCoreMode();
      gproc->setMode(mode);
#endif
      
      if(energyInstFile != NULL) {
        double energy =  GStatsEnergy::getTotalEnergy();
        double delta_energy = energy - previousTotEnergy; 
        long  delta_time = globalClock - previousClockCount;

        fprintf(energyInstFile,"%d\t%.3f\t%ld\n", intervalCount * 10, delta_energy, delta_time);
      }
      
      previousTotEnergy =  GStatsEnergy::getTotalEnergy();
      previousClockCount = globalClock;
      instrCount = 0;
    }/* Endif instrcount = 2000 */
}
#endif

#ifdef SESC_INORDER_SWITCH
int FetchEngine::getNextCoreMode()
{
  char line[128];
  char *c, *pch;
  long interval;
  int mode; 

  if(switchFile == NULL)
	  return -1;

  c = fgets(line, 128, switchFile);
  if(c == NULL)
	  return -1;

  pch = strtok(line,"\t");
  interval = atol(pch);

  pch = strtok(NULL, " ");
  mode = atoi(pch);

  return mode;
}
#endif
#endif

FetchEngine::~FetchEngine()
{
  // There should be a RunningProc::switchOut that clears the statistics
  I(nGradInsts  == 0);
  I(nWPathInsts == 0);

#ifdef SESC_INORDER
  if(energyInstFile != NULL)
    fclose(energyInstFile);
#ifdef SESC_INORDER_SWITCH
  if(switchFile != NULL)
     fclose(switchFile);
#endif
#endif
  
  delete bpred;
}


#ifdef TS_PARANOID
void FetchEngine::fetchDebugRegisters(const DInst *dinst)
{
  TaskContext *tc = dinst->getVersionRef()->getTaskContext();

  if(tc == 0)
    return;
  
  RegType src1,src2,dest;
  const Instruction *inst = dinst->getInt();
  src1 = inst->getSrc1();
  src2 = inst->getSrc2();
  dest = inst->getDest();
  if( inst->hasSrc1Register() 
      && !(src1==2 && dinst->getInst()->isBranch()) 
      && src1 < HiReg 
      && tc->bad_reg[src1] 
      && !dinst->getInst()->isStore())
    fprintf(stderr,"Bad problems on Reg[%d]@0x%x:0x%x\n",src1,dinst->getInst()->getAddr(),0);

  if( inst->hasSrc2Register() 
      && !(src2==2 && dinst->getInst()->isBranch()) 
      && src2 < HiReg 
      && tc->bad_reg[src2]
      && !dinst->getInst()->isStore())
    fprintf(stderr,"Bad problems on Reg[%d]@0x%x:0x:%x\n",src2,dinst->getInst()->getAddr(),0);

  if( inst->hasDestRegister() )
    tc->bad_reg[dest] = false;
}
#endif

void FetchEngine::realFetch(IBucket *bucket, int fetchMax)
{
  long n2Fetched=fetchMax > 0 ? fetchMax : FetchWidth;
  maxBB = BB4Cycle; // Reset the max number of BB to fetch in this cycle (decreased in processBranch)
 
  // This method only can be called once per cycle or the restriction of the
  // BB4Cycle would not enforced
  I(pid>=0);
  I(maxBB>0);
  I(bucket->empty());
  
  I(missInstID==0);

  Pid_t myPid = flow.currentPid();
 
  do {
    nGradInsts++; // Before executePC because it can trigger a context switch

    DInst *dinst = flow.executePC();

    if (dinst == 0)
      break;

    const Instruction *inst = dinst->getInst();
 
    bucket->push(dinst);
    n2Fetched--;
  
    if(inst->isBranch()) {
      if (!processBranch(dinst, n2Fetched))
        break;
    }

  }while(n2Fetched>0 && flow.currentPid()==myPid);

  ushort tmp = FetchWidth - n2Fetched;

  totalnInst+=tmp;
  if( totalnInst >= nInst2Sim ) {
    MSG("stopSimulation at %lld (%lld)",totalnInst, nInst2Sim);
    MSG("nGradInsts is %lld", nGradInsts);
    osSim->stopSimulation();
  }

  nFetched.add(tmp);
}

//Fill the given IBuckes with up to fetchMax instructions
void FetchEngine::fetch(IBucket *bucket, int fetchMax)
{
  //If there is an outstanding miss, fetch fake instructions
  if(missInstID) {
    fakeFetch(bucket, fetchMax);
  }
  //Otherwise, fetch real instructions
  else{
    realFetch(bucket, fetchMax);
  }
  //If an icache is modeled and fetch resulted in one or more instructions
  if(enableICache && !bucket->empty()) {
    if (bucket->top()->getInst()->isStoreAddr())
      IMemRequest::create(bucket->topNext(), gms, bucket); 
    else
      IMemRequest::create(bucket->top(), gms, bucket);
  }
  else{
    // Even if there are no inst to fetch, bucket.empty(), it should
    // be markFetched. Otherwise, we would loose count of buckets
    bucket->markFetchedCB.schedule(IL1HitDelay);
  }
}

//Engin: 
/**** This is a more realistic set of implementations that takes alignment into account******/
bool FetchEngine::processBranch(DInst *dinst, ushort n2Fetched)
{
  const Instruction *inst = dinst->getInst();
  InstID oracleID         = flow.getNextID();

  BPredictor *bp = bpred;
 
#ifdef BPRED_UPDATE_RETIRE
  //Get prediction from home core. Returns and calls go through a single RAS
  PredType prediction  = bp->predict(inst, oracleID, false);
  //Mark home core's branch predictor for update at retirement
  if (!dinst->isFake()){
    dinst->setBPred(bp, oracleID);
  }
  //Oracle updates
#else 
  PredType prediction  = bp->predict(inst, oracleID, !dinst->isFake());
#endif

  if(prediction == CorrectPrediction) {
    if( oracleID != inst->calcNextInstID() ) {
      // Only when the branch is taken check maxBB
      maxBB--;
      if( maxBB == 0 ) {
        // No instructions fetched (stall)
        if (missInstID==0)
          nDelayInst2.add(n2Fetched);
        return false;
      }
    }
    return true;
  }
  
#ifdef SESC_MISPATH
  if (missInstID==0 && !dinst->isFake()) { // Only first mispredicted instruction
    I(missFetchTime == 0);
    missFetchTime = globalClock;
    dinst->setFetch(this);
  }

  missInstID = inst->calcNextInstID();
#else
  I(missInstID==0);

  missInstID = inst->currentID();
  missFetchTime = globalClock;

  if( BTACDelay ) {
    if( prediction == NoBTBPrediction && inst->doesJump2Label() ) {
      nBTAC.inc();
      unBlockFetchCB.schedule(BTACDelay);
    }else{
      dinst->setFetch(this); // blocked fetch (awaked in Resources)
    }
  }else{
    dinst->setFetch(this); // blocked fetch (awaked in Resources)
  }
#endif // SESC_MISPATH

  return false;
}

//Fill the given IBucket with up to fetchMax instructions. resume indicates that we are in the 
//same fetch cycle as the last time fetchAligned was called
bool FetchEngine::fetchAligned(IBucket *bucket, bool resume, int fetchMax)
{
  //If there is an outstanding misprediction, fetch fake instructions
  if(missInstID) {
    fakeFetch(bucket, fetchMax);
  } 
  //Otherwise, fetch real instructions
  else{
    realFetchAligned(bucket,resume,fetchMax);
  }
  //If an icache is modeled and fetch resulted in one or more instructions
  if(enableICache && !bucket->empty()) {
    if (bucket->top()->getInst()->isStoreAddr()) 
      IMemRequest::create(bucket->topNext(), gms, bucket);
    else
      IMemRequest::create(bucket->top(), gms, bucket);
    //printf("FetchEngine::fetchAligned(%ld)    IMemRequest::created  bucket=%p  missInstID=%p   @%lld\n",getCPUId(), bucket, missInstID, globalClock); // NMDEBUG
  }
  else{
    // Even if there are no inst to fetch, bucket.empty(), it should
    // be markFetched. Otherwise, we would loose count of buckets
    //printf("FetchEngine::fetchAligned(%ld)   Bucket Empty  bucket=%p   missInstId=%p  @%lld\n",getCPUId(), bucket, missInstID,globalClock); // NMDEBUG
    bucket->markFetchedCB.schedule(IL1HitDelay);
  }
  return true;
}

//Aligned real fetch. Resume indicates that we are resuming the same fetch cycle 
//from where we left with a new IBucket due to an earlier alignment violation
bool FetchEngine::realFetchAligned(IBucket *bucket, bool resume, int fetchMax)
{
  //Reset the max number of instructions to be fetched in this cycle
  n2FetchedAligned = fetchMax > 0 ? fetchMax : FetchWidth;
  //Reset the max number of BB to fetch in this cycle (decreased in processBranch)
  maxBB = BB4Cycle;

  //Flag the beginning of fetch
  lastPC = -1;

  // This method only can be called once per cycle or the restriction of the
  // BB4Cycle would not enforced
  I(pid>=0);
  I(maxBB>0);
  I(bucket->empty());
  I(missInstID==0);
  //Get pid
  Pid_t myPid = flow.currentPid();

  do {

    //Increment the number of graduated instructions
    // Before executePC because it can trigger a context switch
    nGradInsts++; 
    
    //Get the next dinst through the mint interface
    DInst *dinst = flow.executePC();

    //If no instruction obtained, stop fetching
    if (dinst == 0)
      break;

    //Extract the static mips instruction from the dinst
    const Instruction *inst = dinst->getInst();
   
    //Decrement remaining fetch bandwidth for this cycle
    n2FetchedAligned--;
    //Add the dinst to the current fetch bundle
    bucket->push(dinst); 

    //If this instruction can redirect the PC
    if(inst->isBranch()) {
      //If there was a branch misprediction or the max bb's per cycle reached, stop fetching
      if (!processBranch(dinst, n2FetchedAligned)) { break; }
    }

    //If the instruction is not a branch, still must do an alignment check
    if((lastPC != -1) && ((inst->getAddr() >> iCacheBlockSizeLog2) != (lastPC >> iCacheBlockSizeLog2))){
      break;
    }
    //Record the PC for this instruction for the next comparison
    lastPC = inst->getAddr();

  } while(n2FetchedAligned>0 && flow.currentPid()==myPid);
  
  //Statistics
  ushort tmp = FetchWidth - n2FetchedAligned;
  avgInstrsFetched.sample(tmp);

  totalnInst+=tmp;
  if( totalnInst >= nInst2Sim ) {
    MSG("stopSimulation at %lld (%lld)",totalnInst, nInst2Sim);
    osSim->stopSimulation();
  }
  nFetched.add(tmp);
  
  return true;
}
/**************************************************************************************/
void FetchEngine::fakeFetch(IBucket *bucket, int fetchMax)
{
  I(missInstID);

  nMispathFetchCycles.inc(); // NM

#ifdef SESC_MISPATH
  if(!issueWrongPath)
    return;

  ushort n2Fetched = FetchWidth;

  do {
    // Ugly note: 4 as parameter? Anything different than 0 works
    DInst *dinst = DInst::createInst(missInstID, 4, cpuId);
    I(dinst);

    dinst->setFake();
    n2Fetched--;
    bucket->push(dinst);

    const Instruction *fakeInst = Instruction::getInst(missInstID);
    if (fakeInst->isBranch()) {
      if (!processBranch(dinst, n2Fetched))
        break;
    }else{
      missInstID = fakeInst->calcNextInstID();
    }

  }while(n2Fetched);

  nFetched.add(FetchWidth - n2Fetched);
  nWPathInsts += FetchWidth - n2Fetched;
#endif // SESC_MISPATH
}

void FetchEngine::dump(const char *str) const
{
  char *nstr = (char *)alloca(strlen(str) + 20);

  sprintf(nstr, "%s_FE", str);
 
  bpred->dump(nstr);
  flow.dump(nstr);
}

void FetchEngine::unBlockFetch()
{
  I(missInstID);
  missInstID = 0;

  Time_t n = (globalClock-missFetchTime);

  avgBranchTime.sample(n);
  n *= FetchWidth;
  nDelayInst1.add(n);

  missFetchTime=0;
}

void FetchEngine::switchIn(Pid_t i) 
{
  I(pid==-1);
  I(i>=0);
  I(nGradInsts  == 0);
  I(nWPathInsts == 0);
  pid = i;

  bpred->switchIn(i);
  flow.switchIn(i);
}

void FetchEngine::switchOut(Pid_t i) 
{
  I(pid>=0);
  I(pid==i);
  pid = -1;
  flow.switchOut(i);
  bpred->switchOut(i);
}

