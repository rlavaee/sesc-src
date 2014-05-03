/*  
    SESC: Super ESCalar simulator
    Copyright (C) 2003 University of Illinois.
    
    Contributed by Engin Ipek
    
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

#include "SescConf.h"
#include "Snippets.h"
#include "DDR2.h"
#include "Cache.h"

#include <limits>
#include <cstdio>
#include <sstream>

#include <iostream>
#include <fstream>
#include <algorithm>
#include <deque> 

GStatsAvg *IntervalMemoryRAccessTime = new GStatsAvg("Interval_memory_RAccessTime"); //Omid
GStatsAvg *IntervalMemoryWAccessTime = new GStatsAvg("Interval_memory_RAccessTime"); //Omid
GStatsCntr *IntervalNReads = new GStatsCntr("interval_number_of_reads");
GStatsCntr *IntervalNWrites = new GStatsCntr("interval_number_of_writes");
GStatsAvg *wrPopulation=new GStatsAvg("write_population");
const long long Interval = 10*1000*1000;
long long IntervalCounter = 1;
//rlavaee{
GStatsAvg *rdPopulation=new GStatsAvg("read_population");
std::ofstream wrThinkTimeStream("wrThinkTime.log");
std::ofstream wrJobCountStream("wrJobCount.log");
std::ofstream rdJobCountStream("rdJobCount.log");
std::ofstream rdThinkTimeStream("rdThinkTime.log");
std::ofstream wrAccessTimeStream("wrAccessTime.log");
std::ofstream rdAccessTimeStream("rdAccessTime.log");
std::ofstream wrNumStream("wrNum.log");
std::ofstream rdNumStream("rdNum.log");
//rlavaee}

//rlavaee{
int rdOccupancy;
int wrOccupancy;
std::deque<Time_t> wrCompQ;
std::deque<Time_t> rdCompQ;
//~rlavaee}

//apareek{
GStatsAvg *readServRate = new GStatsAvg("avg_memory_read_service_time");  
GStatsAvg *writeServRate = new GStatsAvg("avg_memory_write_service_time");  
std::ofstream wrServTimeStream("wrServTime.log");
std::ofstream rdServTimeStream("rdServTime.log");
//apareek}

//DRAM clock, scaled wrt globalClock
Time_t DRAMClock; 

// Elastic Refresh
// const int DDR2::ELASTIC_REFRESH_THRESHOLD = 4;
// const int DDR2::OVERFLOW_16BITS = ((1<<16) - 1);
// const int DDR2::MASK_16BITS = ((1<<16) - 1);
// const int DDR2::MASK_17BITS = ((1<<17) - 1);

//Memory reference constructor
MemRef::MemRef()
  : timeStamp(0), 
    servTimeStamp(0), 
    servTimeStampSet(false), 
    mReq(NULL),
    rankID(-1),
    bankID(-1),
    rowID(-1),
    prechargePending(true), 
    activatePending(true),
    readPending(true),
    writePending(true),
    ready(false),
    valid(false),
    read(true),
    loadMiss(false),
    loadMissSeqNum(-1),
    loadMissCoreID(-1)
{
  //Nothing to do
} 

//Complete this transaction
void MemRef::complete(Time_t when)
{

  //Reset all state
  rankID = -1;
  bankID = -1;
  rowID = -1;
  prechargePending = true;
  activatePending = true;
  readPending = true;
  writePending = true;
  ready = false;
  valid = false;
  read = true;
  loadMiss = false;
  loadMissSeqNum = -1;
  loadMissCoreID = -1;
  
  //Return the memory request to higher levels of the memory hierarchy
  mReq->goUpAbs(when); 
  
  //Reset request
  mReq = NULL;
  
} 
 
//Rank constructor
DDR2Rank::DDR2Rank(const char *section)
  : powerState(IDD2N), 
    emr(SLOW),
    cke(true), 
    lastPrecharge(0),
    lastActivate(0),
    lastRead(0),
    lastWrite(0),
    timeRefresh(0),
    lastRefresh(0),
    urgentRefresh(false),
    numActive(0)

{
  
  //Check energy parameters
  // SescConf->isLong(section, "eIDD2P");
  // SescConf->isLong(section, "eIDD2N");
  // SescConf->isLong(section, "eIDD3N");
  // SescConf->isLong(section, "eIDD3PF");
  // SescConf->isLong(section, "eIDD3PS");
  // SescConf->isLong(section, "eActivate");
  // SescConf->isLong(section, "ePrecharge");
  // SescConf->isLong(section, "eRead");
  // SescConf->isLong(section, "eWrite");
  SescConf->isLong(section, "VDD");
  SescConf->isLong(section, "VDDMAX");
  SescConf->isLong(section, "IDD0");
  SescConf->isLong(section, "IDD2P0");
  SescConf->isLong(section, "IDD2P1");  
  SescConf->isLong(section, "IDD2N");  
  SescConf->isLong(section, "IDD3P");  
  SescConf->isLong(section, "IDD3N");  
  SescConf->isLong(section, "IDD4R");  
  SescConf->isLong(section, "IDD4W");
  SescConf->isLong(section, "IDD5");  
  SescConf->isLong(section, "numChipsPerRank");  
  SescConf->isLong(section, "cycleTime");  

  //Check timing parameters
  SescConf->isLong(section, "tRCD");
  SescConf->isLong(section, "tCL");
  SescConf->isLong(section, "tWL");
  SescConf->isLong(section, "tCCD");
  SescConf->isLong(section, "tWTR");
  SescConf->isLong(section, "tWR");
  SescConf->isLong(section, "tRTP");
  SescConf->isLong(section, "tRP");
  SescConf->isLong(section, "tRRD");
  SescConf->isLong(section, "tRAS");
  SescConf->isLong(section, "tRC");
  SescConf->isLong(section, "tRTRS");
  SescConf->isLong(section, "tOST");
  SescConf->isLong(section, "BL");
  SescConf->isLong(section, "tRFC");
  SescConf->isLong(section, "tREFI");  

  
  //Check bank count
  SescConf->isLong(section, "numBanks");
  

  //Read timing parameters
  tRCD            = SescConf->getLong(section, "tRCD");
  tCL             = SescConf->getLong(section, "tCL");
  tWL             = SescConf->getLong(section, "tWL");
  tCCD            = SescConf->getLong(section, "tCCD");
  tWTR            = SescConf->getLong(section, "tWTR");
  tWR             = SescConf->getLong(section, "tWR");
  tRTP            = SescConf->getLong(section, "tRTP");
  tRP             = SescConf->getLong(section, "tRP");
  tRRD            = SescConf->getLong(section, "tRRD");
  tRAS            = SescConf->getLong(section, "tRAS");
  tRC             = SescConf->getLong(section, "tRC");
  tRTRS           = SescConf->getLong(section, "tRTRS");
  tOST            = SescConf->getLong(section, "tOST"); 
  BL              = SescConf->getLong(section, "BL");

  tRFC            = SescConf->getLong(section, "tRFC");
  tREFI            = SescConf->getLong(section, "tREFI");  

  //Read energy parameters
  // eActivate       = SescConf->getLong(section, "eActivate");
  // ePrecharge      = SescConf->getLong(section, "ePrecharge");
  // eRead           = SescConf->getLong(section, "eRead");
  // eWrite          = SescConf->getLong(section, "eWrite");  
  // eIDD2P          = SescConf->getLong(section, "eIDD2P");
  // eIDD2N          = SescConf->getLong(section, "eIDD2N"); 
  // eIDD3N          = SescConf->getLong(section, "eIDD3N"); 
  // eIDD3PF         = SescConf->getLong(section, "eIDD3PF");
  // eIDD3PS         = SescConf->getLong(section, "eIDD3PS"); 

  long vdd    = SescConf->getLong(section, "VDD");
  long vddMax = SescConf->getLong(section, "VDDMAX");
  long idd0   = SescConf->getLong(section, "IDD0");
  long idd2p0 = SescConf->getLong(section, "IDD2P0");
  long idd2p1 = SescConf->getLong(section, "IDD2P1");  
  long idd2n  = SescConf->getLong(section, "IDD2N");  
  long idd3p  = SescConf->getLong(section, "IDD3P");  
  long idd3n  = SescConf->getLong(section, "IDD3N");  
  long idd4r  = SescConf->getLong(section, "IDD4R");  
  long idd4w  = SescConf->getLong(section, "IDD4W");
  long idd5   = SescConf->getLong(section, "IDD5");  
  long numChipsPerRank = SescConf->getLong(section, "numChipsPerRank");  
  long cycleTime =  SescConf->getLong(section, "cycleTime");  

  // calculate the energy numbers
  // voltage*cycletime: vt
  // TODO: in fact vdd should be derated, no freq derating now
  double vt		= (vddMax/(double)1000.0) * cycleTime * numChipsPerRank / (double) 1000.0 * 
    (vdd/(double)vddMax) * (vdd/(double)vddMax);

  //printf("DRAM vt:%lf\n", vt);
  // double vt		= vdd/(double)1000.0 * cycleTime * numChipsPerRank / (double) 1000.0;
  eIDD2P0		= idd2p0 * vt;
  eIDD2P1		= idd2p1 * vt;
  eIDD2N		= idd2n * vt;
  eIDD3N		= idd3n * vt;
  eIDD3P		= idd3p * vt;
  eActivate		= ((idd0 * tRC) - (idd3n * tRAS + idd2n * (tRC-tRAS))) * vt; 
  ePrecharge		= 0; // included by eActivate
  eRead			= (idd4r - idd3n) * BL/2 * vt;
  eWrite		= (idd4w - idd3n) * BL/2 * vt;
  eRefresh		= (idd5 - idd3n) * tRFC * vt;

/*
  printf("DRAM eIDD3N:%ld\n", eIDD3N);
  printf("DRAM read pj/bit:%ld, %ld\n", eRead/512, eRead);
  printf("DRAM activate + precharge %ld\n", eActivate);
  printf("DRAM read pj/bit:%ld, %ld\n", eRead/512, eRead);
  printf("DRAM write pj/bit:%ld, %ld\n", eWrite/512, eWrite);
  printf("DRAM refresh %ld\n", eRefresh);
*/
  // NOTE: ODT energy no need of voltage derating, because it's assumed.
  // Check micron power calculator excel sheet
  // for 2 ranks system, in mW

  // TOCHECK, need numChipsPerRank or not??
  double tODT = cycleTime * numChipsPerRank/(double) 1000.0;
  //double tODT = cycleTime/(double) 1000.0;
  double pdqRD		= 4.6;
  double pdqWR		= 5.7;
  double pdqRDoth	= 20.4;
  double pdqWRoth	= 21.9;

  eDQRD = pdqRD * BL/2 * tODT;
  eDQWR = pdqWR * BL/2 * tODT;
  eDQRDoth = pdqRDoth * BL/2 * tODT;
  eDQWRoth = pdqWRoth * BL/2 * tODT;

/*
  printf("************* ODT is not counted now !!! ****************\n");
  printf("DRAM ODT read pj/bit:%lf\n", eDQRD/512.0);
  printf("DRAM ODT write pj/bit:%lf\n", eDQWR/512.0);
  printf("DRAM ODT readterm pj/bit:%lf\n", eDQRDoth/512.0);
  printf("DRAM ODT writeterm pj/bit:%lf\n", eDQWRoth/512.0);
*/
 
  //Read bank count
  numBanks        = SescConf->getLong(section, "numBanks");
  
  //Rank energy
  rankEnergy = new GStatsCntr("ddr_rank_energy");
  
  //Calculate log2 of bank count
  numBanksLog2 = log2i(numBanks);
  
  //Allocate banks
  // yanwei: not quite efficient
  banks = *(new std::vector<DDR2Bank *>(numBanks));
  for(int i=0; i<numBanks; i++){
    banks[i] = new DDR2Bank(i); //apareek //setting bank IDs
  } 

  //Initialize Elastic Refresh
  // consDelay = tRFC;
  // propDelay = tRFC;

  // maxDelayAccum = 0;
  // idleAccum = 0;
  // maxDelayCount = 0;
  // high = low = 0;
  // prevStat = false; 		// idle
}

//Bank constructor
DDR2Bank::DDR2Bank(int bankID)
  :state(IDLE),
   openRowID(-1),
   lastPrecharge(0),
   lastActivate(0),
   lastRead(0),
   lastWrite(0),
   casHit(false),
   timeRefresh(0),
   lastRefresh(0)
{
  //Nothing to do
  rdBankCnt    = new GStatsCntr("bank_read_count(%d)", bankID);
  wrBankCnt    = new GStatsCntr("bank_write_count(%d)", bankID);
  thisBankID = bankID;
  std::ostringstream bankNum;
  bankNum << bankID;
//  std::string bankNum = itoa(bankID);
  std::string bankWrIo = "wrCntBank" + bankNum.str() + ".log";
  std::string bankRdIo = "rdCntBank" + bankNum.str() + ".log";
  wrBankCountStream = new std::ofstream(bankWrIo.c_str());
  rdBankCountStream = new std::ofstream(bankRdIo.c_str());
  //std::ofstream rdBankCountStream(bankRdIo.c_str());
}

//apareek{
void DDR2Bank::resetStats()
{

  *wrBankCountStream <<  wrBankCnt->getDouble() << std::endl;
  *rdBankCountStream <<  rdBankCnt->getDouble() << std::endl;
  wrBankCountStream->flush();
  rdBankCountStream->flush();
  //std::cout << "wr[" <<thisBankID << "] " << wrBankCnt->getDouble() << std::endl;
  //std::cout << "rd[" <<thisBankID << "] " << rdBankCnt->getDouble() << std::endl;

  rdBankCnt -> resetStat();
  wrBankCnt -> resetStat();
}
//apareek}
//Precharge bank
void DDR2Bank::precharge()
{
  state = IDLE;
  lastPrecharge = DRAMClock;
  casHit = false;
}

//Activate row
void DDR2Bank::activate(int rowID)
{
  state = ACTIVE;
  openRowID = rowID;
  lastActivate = DRAMClock;
  casHit = false;
}

//Read from open row
void DDR2Bank::read()
{
  lastRead = DRAMClock;
  casHit = true;
//apareek{
	  //logging stats for bank reads
	  rdBankCnt->inc();
//apareek}
}

//Write to open row
void DDR2Bank::write()
{
  lastWrite = DRAMClock;
  casHit = true;
//apareek{
	  //logging stats for bank writes
	  wrBankCnt->inc();
//apareek}
}

void DDR2Rank::dumpBankStats(){//apareek
  for(int i = 0;i< numBanks; i++){
    banks[i]->resetStats();
  }
}
//Can a precharge issue now to the given bank ?
bool DDR2Rank::canIssuePrecharge(int bankID)
{

  //Enforce tWR
  //if(DRAMClock - (banks[bankID]->getLastWrite() + tWL + (BL/2)) < tWR){ //fixed rlavaee
	if(DRAMClock - banks[bankID]->getLastWrite() < tWR+tWL+(BL/2)){
    return false;
  }
  
  //Enforce tRTP
  if(DRAMClock - banks[bankID]->getLastRead() < tRTP){
    return false;
  }
  
  //Enforce tRAS
  if(DRAMClock - banks[bankID]->getLastActivate() < tRAS){
    return false;
  }

  return true;
}

//Can an activate issue now to the given bank ?
bool DDR2Rank::canIssueActivate(int bankID)
{
  //Enforce tRFC
  if(DRAMClock - lastRefresh < tRFC){
    return false;
  }

  //Enforce tRP
  if(DRAMClock - banks[bankID]->getLastPrecharge() < tRP){
    return false;
  }

  //Enforce tRRD
  for(int i=0; i<numBanks; i++){
    if(i!=bankID){
      if(DRAMClock - banks[i]->getLastActivate() < tRRD){
	return false;
      }
    }
  }

  //Enforce tRC
  if(DRAMClock - banks[bankID]->getLastActivate() < tRC){
    return false;
  } 
  
  return true;
}

//Can a read issue to the given bank and row ?
bool DDR2Rank::canIssueRead(int bankID, int rowID)
{  
  
  //Enforce page misses
  if(banks[bankID]->getOpenRowID() != rowID){
    return false;
  }
  
  //Enforce tRCD
  if(DRAMClock - banks[bankID]->getLastActivate() < tRCD){
    return false;
  } 
  
  //Enforce tCCD
  if(DRAMClock - lastRead < tCCD || DRAMClock - lastWrite < tCCD){
    return false;
  }
  
  //Enforce tWTR
  //if(DRAMClock - (lastWrite + tWL + (BL/2)) < tWTR){ rlavaee fixed
	if(DRAMClock - lastWrite < tWTR + tWL + (BL/2)){
    return false;
  }
  
  return true;
}

//Can a write issue to the given bank and row ?
bool DDR2Rank::canIssueWrite(int bankID, int rowID)
{

  //Enforce page misses
  if(banks[bankID]->getOpenRowID() != rowID){
    return false;
  }
  
  //Enforce tRCD
  if(DRAMClock - banks[bankID]->getLastActivate() < tRCD){
    return false;
  }
  
  //Enforce tCCD
  if(DRAMClock - lastRead < tCCD || DRAMClock - lastWrite < tCCD){
    return false;
  }

  return true;
}


//Precharge given bank
void DDR2Rank::precharge(int bankID) 
{
  numActive--;
  //Timing
  lastPrecharge = DRAMClock;
  banks[bankID]->precharge();
  
  //Energy
  rankEnergy->add(ePrecharge);
  
} 

//Activate given row in given bank
void DDR2Rank::activate(int bankID, int rowID)
{
  
  numActive++;
  //Timing 
  lastActivate = DRAMClock;
  banks[bankID]->activate(rowID);

  //Energy
  rankEnergy->add(eActivate);

}

//Read from given row in given bank
void DDR2Rank::read(int bankID, int rowID)
{

  //Timing
  lastRead = DRAMClock;
  banks[bankID]->read();

  //Energy
  rankEnergy->add(eRead);

}

//Write to given row in given bank
void DDR2Rank::write(int bankID, int rowID)
{

  //Timing 
  lastWrite = DRAMClock;
  banks[bankID]->write();

  //Energy
  rankEnergy->add(eWrite);

}

//Get DDR2 rank's background energy
long DDR2Rank::getBackgroundEnergy()
{
  //Return energy for current power state
  switch(powerState)
    {

    case IDD2P0:
      return eIDD2P0;
      break;

    case IDD2P1:
      return eIDD2P1;
      break;

    case IDD3P:
      return eIDD3P;
      break;

    case IDD2N:
      return eIDD2N;
      break;
      
    case IDD3N:
      return eIDD3N;
      break;
      
    default:
      printf("ILLEGAL POWER STATE\n");
      fflush(stdout);
      exit(1);
      break;
    }
}

//Update DDR2 power state
void DDR2Rank::updatePowerState()
{
  
  //Flag indicating all banks are precharged
  // bool allBanksPrecharged = true;
  // for(int i=0; i<numBanks; i++){
  //   if(banks[i]->getState() == ACTIVE){
  //     allBanksPrecharged = false;
  //   }
  // }

  bool allBanksPrecharged = numActive == 0;

  //If CKE is low
  if(cke == false){
    //If all banks are precharged
    if(allBanksPrecharged){
      powerState = (emr == FAST) ? IDD2P1:IDD2P0;
    }
    //Otherwise
    else{
      powerState = IDD3P;
    }
  }
  //Otherwise
  else{
    //If all banks are precharged
    if(allBanksPrecharged){
      powerState = IDD2N;
    }
    //Otherwise
    else{
      powerState = IDD3N;
    }
  }
}

//DDR2 Constructor
DDR2::DDR2(const char *section, int _channelID)
  : channelID(_channelID)
{
  //Check all parameters
  SescConf->isLong(section, "multiplier");
  SescConf->isLong(section, "numCores");
  SescConf->isLong(section, "numChannels");
  SescConf->isLong(section, "numRanks");
  SescConf->isLong(section, "numBanks");
  SescConf->isLong(section, "pageSize");
  SescConf->isLong(section, "queueSize");
  SescConf->isLong(section, "tRCD");
  SescConf->isLong(section, "tCL");
  SescConf->isLong(section, "tWL");
  SescConf->isLong(section, "tCCD");
  SescConf->isLong(section, "tWTR");
  SescConf->isLong(section, "tWR");
  SescConf->isLong(section, "tRTP");
  SescConf->isLong(section, "tRP");
  SescConf->isLong(section, "tRRD");
  SescConf->isLong(section, "tRAS");
  SescConf->isLong(section, "tRC");  
  SescConf->isLong(section, "tRTRS");
  SescConf->isLong(section, "tOST");
  SescConf->isLong(section, "BL");

  SescConf->isLong(section, "tRFC");
  SescConf->isLong(section, "tREFI");  

  //Read all parameters
  multiplier      = SescConf->getLong(section, "multiplier");
  numCores        = SescConf->getLong(section, "numCores");
  numChannels     = SescConf->getLong(section, "numChannels");
  numRanks        = SescConf->getLong(section, "numRanks");
  numBanks        = SescConf->getLong(section, "numBanks");
  pageSize        = SescConf->getLong(section, "pageSize");
  queueSize       = SescConf->getLong(section, "queueSize");
  tRCD            = SescConf->getLong(section, "tRCD");
  tCL             = SescConf->getLong(section, "tCL");
  tWL             = SescConf->getLong(section, "tWL");
  tCCD            = SescConf->getLong(section, "tCCD");
  tWTR            = SescConf->getLong(section, "tWTR");
  tWR             = SescConf->getLong(section, "tWR");
  tRTP            = SescConf->getLong(section, "tRTP");
  tRP             = SescConf->getLong(section, "tRP");
  tRRD            = SescConf->getLong(section, "tRRD");
  tRAS            = SescConf->getLong(section, "tRAS");
  tRC             = SescConf->getLong(section, "tRC");
  tRTRS           = SescConf->getLong(section, "tRTRS");
  tOST            = SescConf->getLong(section, "tOST");
  BL              = SescConf->getLong(section, "BL");

  tRFC            = SescConf->getLong(section, "tRFC");
  tREFI            = SescConf->getLong(section, "tREFI");  

 
  //Calculate log2
  numChannelsLog2 = log2i(numChannels);
  numRanksLog2    = log2i(numRanks);
  numBanksLog2    = log2i(numBanks);
  pageSizeLog2    = log2i(pageSize);

  //Initialize DRAM clock
  DRAMClock = 0;
  
  //Initialize scheduling queue occupancy
  occupancy=0;
	rdOccupancy=0;
	wrOccupancy=0;

  //Initialize arrival queue occupancy
  arrivals=0;

  //Initialize arrival queue
  arrivalHead = NULL;
  arrivalTail = NULL;

  //Allocate ranks
  ranks = *(new std::vector<DDR2Rank *>(numRanks));
  for(int i=0; i<numRanks; i++){
    ranks[i] = new DDR2Rank(section);
  } 

  //Allocate scheduling queue
  dramQ = *(new std::vector<MemRef *>(queueSize));
  for(int i=0; i<queueSize; i++){
    dramQ[i] = new MemRef();
  }


  //Allocate free-list
#ifndef FASTQUEUE_LIST
  freeList = new std::list<int>();
  for(int i=0; i<queueSize;i++){
    freeList->push_back(i);
  }
#else
  freeList = new FastQueue<int>(queueSize);
  for(int i=0; i<queueSize;i++){
    freeList->push(i);
  }
#endif
  
  //Allocate timing stats
  dramQAvgOccupancy    = new GStatsAvg("dramQ_avg_occupancy(%d)", _channelID);
  arrivalQAvgOccupancy = new GStatsAvg("arrivalQ_avg_occupancy(%d)", _channelID);
  completionRate       = new GStatsAvg("avg_completion_rate(%d)", _channelID);
  numPrecharges        = new GStatsCntr("ddr_precharge_count(%d)", _channelID);
  numActivates         = new GStatsCntr("ddr_activate_count(%d)", _channelID);
  numReads             = new GStatsCntr("ddr_read_count(%d)", _channelID);
  numWrites            = new GStatsCntr("ddr_write_count(%d)", _channelID);
  numNops              = new GStatsCntr("ddr_nop_count(%d)", _channelID);
  numRefreshes         = new GStatsCntr("ddr_refresh_count(%d)", _channelID);

  memoryRAccesTime = new GStatsAvg("avg_memory_Raccestime(%d)", _channelID);
  memoryWAccesTime = new GStatsAvg("avg_memory_Waccestime(%d)", _channelID);  
  //Allocate energy stats
  channelEnergy        = new GStatsCntr("ddr_channel_energy(%d)", _channelID);
}

//A new memory request arrives in arrival queue
void DDR2::arrive(MemRequest *mreq)
{

  //If tail pointer exists
  if(arrivalTail != NULL){
    //Mark the tail's back pointer
    arrivalTail->back = mreq;
    //Mark the arrival's front pointer
    mreq->front = arrivalTail;
  }

  //Make new request the tail
  arrivalTail = mreq;

  //If head is NULL, make new request the head 
  if(arrivalHead == NULL){
    arrivalHead = mreq;
  }

  //Update arrival queue occupancy
  arrivals++;

}

//Enqueue memory request in DRAM queue
void DDR2::enqueue(MemRequest *mreq)
{

  //Update arrival queue
  arrivalHead = mreq->back;
  if(arrivalHead == NULL){
    arrivalTail = NULL;
  }

  occupancy++;
  arrivals--;
  

  //Get index into scheduling queue
#ifndef FASTQUEUE_LIST
  int index = freeList->back();
  freeList->pop_back();
#else
  int index = freeList->top();
  freeList->pop();
#endif
  
  //Set the memory request
  dramQ[index]->setMReq(mreq);
  
  // yanwei
  // page interleaving address mapping:
  // rowid|bank|rank|channel|page size

  //Set the rank ID
  int rankID = (mreq->getPAddr() >> (pageSizeLog2 + numChannelsLog2)) & (numRanks - 1);
  dramQ[index]->setRankID(rankID);

  //Set the bank ID
  int bankID = (mreq->getPAddr() >> (pageSizeLog2 + numChannelsLog2 + numRanksLog2)) & (numBanks - 1);
  dramQ[index]->setBankID(bankID);
	//std::cout << (mreq->getPAddr() >> pageSizeLog2) << std::endl;

  //Set the row ID
  int rowID = mreq->getPAddr() >> (pageSizeLog2 + numChannelsLog2 + numRanksLog2 + numBanksLog2);
  dramQ[index]->setRowID(rowID);

  //Get bank state and open row ID
  STATE bankState = getBankState(rankID, bankID);
  int openRowID = getOpenRowID(rankID, bankID);

  //Set precharge status
  dramQ[index]->setPrechargePending(bankState == ACTIVE && rowID != openRowID);

  //Set activate status
  dramQ[index]->setActivatePending(bankState == IDLE || rowID != openRowID);

  //Set read/write flag
  MemOperation memOp = mreq->getMemOperation();
  dramQ[index]->setRead((memOp != MemWrite) && (memOp != MemPush));

  //Set load miss flag 
  dramQ[index]->setLoadMiss(mreq->isLoadMiss());

  //Set load miss sequence number and coreID
  if(dramQ[index]->isLoadMiss()){
    dramQ[index]->setLoadMissSeqNum(mreq->sequenceNum);
    dramQ[index]->setLoadMissCoreID(mreq->coreID);
  }

  //Set cas status
  dramQ[index]->setReadPending(dramQ[index]->isRead());
  dramQ[index]->setWritePending(!dramQ[index]->isRead());
 
  //Set ready status
  dramQ[index]->setReady(false);

  //Set valid status
  dramQ[index]->setValid(true);

  //Set timestamp
  dramQ[index]->setTimeStamp(DRAMClock);

  //rlavaee, sample think time for writes
	if(dramQ[index]->isRead())
		rdOccupancy++;
	else
		wrOccupancy++;
  if(!wrCompQ.empty() && !dramQ[index]->isRead()){
		
		for(std::deque<Time_t>::iterator it=wrCompQ.begin(); it!=wrCompQ.end(); ++it){
			if(DRAMClock*multiplier > *it){
  			//wrThinkTime->sample(DRAMClock*multiplier - *it);
				wrPopulation->sample(wrCompQ.size()+wrOccupancy);
				wrCompQ.erase(it);
				break;
			}
		}
  }
	
	if(!rdCompQ.empty() && dramQ[index]->isRead()){
		for(std::deque<Time_t>::iterator it=rdCompQ.begin(); it!=rdCompQ.end(); ++it){
			if(DRAMClock*multiplier > *it){
  			//rdThinkTime->sample(DRAMClock*multiplier - *it);
				rdPopulation->sample(rdCompQ.size()+rdOccupancy);
				rdCompQ.erase(it);
				break;
			}
		}
  }

	//don't know if this is correct.
	//if(!wrCompQ.empty() && dramQ[index]->isRead()){
	//	if(wrCompQ.size() > 32){
	//		wrCompQ.pop();
	//	}
	//}
  //~rlavaee

  
}

//Update queue state after a precharge or activate
void DDR2::updateQueueState()
{
  //Go through all references
  for(int i=0; i<queueSize; i++){
    
    //If the reference is valid
    if(dramQ[i]->isValid()){
      
      //Extract rank, bank and row information
      int bankID = dramQ[i]->getBankID();
      int rowID = dramQ[i]->getRowID();
      int rankID = dramQ[i]->getRankID();
      STATE bankState = getBankState(rankID, bankID);
      int openRowID = getOpenRowID(rankID, bankID);
      
      //Does the reference currently need a precharge ?
      dramQ[i]->setPrechargePending(bankState == ACTIVE && rowID != openRowID);
      
      //Does the reference currently need an activate ?
      dramQ[i]->setActivatePending(bankState == IDLE || rowID != openRowID);
      
    }
    
  }
}


//Can a precharge to given rank and bank issue now ?
bool DDR2::canIssuePrecharge(int rankID, int bankID)
{
  //Check intra-rank constraints
  return ranks[rankID]->canIssuePrecharge(bankID);
}

//Can an activate to given rank and bank issue now ?
bool DDR2::canIssueActivate(int rankID, int bankID)
{
  //Check intra-rank constraints
  return ranks[rankID]->canIssueActivate(bankID);
}

//Can a read to given rank, bank, and row issue now
bool DDR2::canIssueRead(int rankID, int bankID, int rowID)
{
  //Check intra-rank constraints
  if(!ranks[rankID]->canIssueRead(bankID, rowID)){  
    return false;
  }
  
  //Enforce tRTRS
  for(int i=0; i<numRanks; i++){
    //Go through all other ranks
    if(i!=rankID){
      //Consecutive reads
      //if(DRAMClock - (ranks[i]->getLastRead() + (BL/2)) < tRTRS){  	 rlavaee fixed
			if(DRAMClock - ranks[i]->getLastRead() < tRTRS + (BL/2)){
	return false;
      }  
      //Write followed by read
      //if(DRAMClock - (ranks[i]->getLastWrite() + tWL + (BL/2) - tCL) < tRTRS){ rlavaee fixed
			if(DRAMClock - (ranks[i]->getLastWrite() + - tCL) < tRTRS + tWL + (BL/2)){
	return false;
      }  
    }
  }
  
  return true;
  
}

//Can a write to given rank, bank, and row issue now
bool DDR2::canIssueWrite(int rankID, int bankID, int rowID)
{
  //Check intra-rank constraints
  if(!ranks[rankID]->canIssueWrite(bankID, rowID)){
    return false;
  }

  //Enforce tRTRS
  for(int i=0; i<numRanks; i++){
    //Read followed by write, any two ranks
    //if(DRAMClock - (lastRead + tCL + (BL/2) - tWL) < tRTRS){ rlavaee fixed
		 if(DRAMClock - (lastRead - tWL) < tRTRS + tCL + BL/2){
      return false;
    }
    //Write followed by write, different ranks
    if(i!=rankID){
      //Enforce tOST
      //if(DRAMClock - (lastWrite + (BL/2)) < tOST){ rlavaee fixed
			if(DRAMClock - lastWrite < tOST + BL/2){

	return false;
      }
    }
  }
  
  return true;

}


//Precharge bank
void DDR2::precharge(int rankID, int bankID)
{
  //Issue precharge
  ranks[rankID]->precharge(bankID);
  lastPrecharge = DRAMClock;
}

//Activate row
void DDR2::activate(int rankID, int bankID, int rowID)
{ 
  //Issue activate
  ranks[rankID]->activate(bankID, rowID); 
  lastActivate = DRAMClock;
}
 
//Read
void DDR2::read(int rankID, int bankID, int rowID)
{
  //Issue read
  ranks[rankID]->read(bankID, rowID);
  lastRead = DRAMClock;
}
 
//Write
void DDR2::write(int rankID, int bankID, int rowID)
{
  //Issue write
  ranks[rankID]->write(bankID, rowID);
  lastWrite = DRAMClock;
}
 
//Schedule a command using fcfs scheduling
void DDR2::scheduleFCFS()
{  
  //Reference to schedule commands for
  MemRef *mRef = NULL;
 
  //Arrival time of oldest reference
  Time_t oldestTime = DRAMClock;
 
  //Index of oldest reference in dramQ
  int index = -1;
 
  //Find the oldest reference in the scheduler's queue
  for(int i=0; i<queueSize; i++){
    if(dramQ[i]->isValid()){
      if(dramQ[i]->getTimeStamp() < oldestTime){
	mRef = dramQ[i];
	oldestTime = dramQ[i]->getTimeStamp();
	index = i;
      }
    }
  }

  //If an outstanding reference exists in the queue
  if(mRef != NULL){
//apareek{
	if(!mRef->servTimeStampSet) {
		mRef->setServTimeStamp(DRAMClock);//apareek
		mRef->servTimeStampSet=true;
		printf("ServTime: %f : Oldest Time: %f\n",DRAMClock,oldestTime);
	}
//apareek}
    //If the reference needs a precharge
    if(mRef->needsPrecharge()){
      //If the precharge can be issued now
      if(canIssuePrecharge(mRef->getRankID(), mRef->getBankID())){
	//Precharge bank
	precharge(mRef->getRankID(), mRef->getBankID());
	mRef->setPrechargePending(false);
	//Update queue state
	updateQueueState();
	//Update stats
	numPrecharges->inc();
      }
      //Update stats
      completionRate->sample(0);
      return;
    }
    
    //If the reference needs an activate
    if(mRef->needsActivate()){
      //If the activate can be issued now
      if(canIssueActivate(mRef->getRankID(), mRef->getBankID())){
	//Activate bank
	activate(mRef->getRankID(), mRef->getBankID(), mRef->getRowID());
	mRef->setActivatePending(false);
	//Update queue state
	updateQueueState();
	//Upddate stats
	numActivates->inc();
      }
      //Update stats
      completionRate->sample(0);
      return;
    }

    //If the reference needs a read
    if(mRef->needsRead()){
      //If the read can be issued now
      if(canIssueRead(mRef->getRankID(), mRef->getBankID(), mRef->getRowID())){
	//Issue read
	read(mRef->getRankID(), mRef->getBankID(), mRef->getRowID());
	mRef->setReadPending(false);
	//This reference is now complete

			Time_t comp_time = globalClock + multiplier * (tCL + (BL/2)+1);

			//rlavaee{
		rdCompQ.push_back(comp_time);
			//rlavaee}
	IntervalMemoryRAccessTime->sample(comp_time - mRef->getTimeStamp()*multiplier); //Omid
			IntervalNReads->inc();
      readServRate->sample(comp_time - mRef->getServTimeStamp()*multiplier);//apareek
	  mRef->servTimeStampSet=false;

	mRef->complete(globalClock + multiplier * (tCL + (BL/2) + 1) );
#ifndef FASTQUEUE_LIST
	freeList->push_back(index);
#else
	freeList->push(index);
#endif
	occupancy--;
	rdOccupancy--;
	//Update stats
	completionRate->sample(1);
	numReads->inc();
      }
      else{
	//Update stats
	completionRate->sample(0);
      }
      return;
    }

    //If the reference needs a write
    if(mRef->needsWrite()){
      //If the write can be issued now
      if(canIssueWrite(mRef->getRankID(), mRef->getBankID(), mRef->getRowID())){
	//Issue write
	write(mRef->getRankID(), mRef->getBankID(), mRef->getRowID());
	mRef->setWritePending(false);
	//This reference is now complete

			//rlavaee: better to reuse completion time
			Time_t comp_time = globalClock+multiplier *(tWL + (BL/2) + 1);
      memoryWAccesTime->sample(comp_time - mRef->getTimeStamp()*multiplier);
      
			// rlavaee{
			IntervalMemoryWAccessTime->sample(comp_time - mRef->getTimeStamp()*multiplier);
			IntervalNWrites->inc();
			//rlavaee}

      writeServRate->sample(comp_time - mRef->getServTimeStamp()*multiplier);//apareek
	  mRef->servTimeStampSet=false;

	  wrCompQ.push_back(comp_time);
	  // ~rlavaee






	mRef->complete(globalClock + multiplier * (tWL + (BL/2) + 1) );
#ifndef FASTQUEUE_LIST
	freeList->push_back(index);
#else
	freeList->push(index);
#endif
	occupancy--;
	wrOccupancy--;
	//Update stats
	completionRate->sample(1);
	numWrites->inc();
      }
      else{
	//Update stats
	completionRate->sample(0);
      }
      return;
    } 
  }
  //Update stats
  completionRate->sample(0);
}

//Schedule a command using fr-fcfs scheduling
void DDR2::scheduleFRFCFS()
{  
  if (freeList->size() != queueSize){
  //Reference to schedule commands for
  MemRef *mRef = NULL;
  
  //Arrival time of oldest reference
  Time_t oldestTime = DRAMClock;
  
  //Index of oldest reference in dramQ
  int index = -1;

  ////////////////////////// original version start //////////////////////////////////////////////////
  //Find oldest ready CAS cmd
  for(int i=0; i<queueSize; i++){
    
    //If the request is valid
    if(dramQ[i]->isValid()){

      //If the request does not need a precharge or activate
      if((!dramQ[i]->needsPrecharge()) && (!dramQ[i]->needsActivate())){
	//If the request needs a read
	if(dramQ[i]->needsRead()){
	  //If the read can issue
	  if(canIssueRead(dramQ[i]->getRankID(), dramQ[i]->getBankID(), dramQ[i]->getRowID())){
	    //If the read is older than the oldest cas cmd found so far
	    if(dramQ[i]->getTimeStamp() < oldestTime){
	      mRef = dramQ[i];
	      oldestTime = dramQ[i]->getTimeStamp();
	      index = i;
	    }
	  }
	}
	//Otherwise, if the request needs a write
	else if(dramQ[i]->needsWrite()){
	  //If the write can issue
	  if(canIssueWrite(dramQ[i]->getRankID(), dramQ[i]->getBankID(), dramQ[i]->getRowID())){
	    //If the write is older than the oldest cas cmd found so far
	    if(dramQ[i]->getTimeStamp() < oldestTime){
	      mRef = dramQ[i];
	      oldestTime = dramQ[i]->getTimeStamp();
	      index = i;
	    }
	  }
	}
	
      }
      
    }
    
  }

  if(mRef != NULL){
	if(!mRef->servTimeStampSet) {
	mRef->setServTimeStamp(DRAMClock);//apareek
	mRef->servTimeStampSet=true;
 //   std::cout << "service time start:\t"<<DRAMClock*multiplier<<"\n";
//    std::cout << "wait time:\t"<<(DRAMClock-mRef->getTimeStamp())*multiplier<<"\n";
	}
	//printf("ServTime: %f : Oldest Time: %f\n",DRAMClock,oldestTime);
  }
  //If no CAS cmd was ready, find oldest ready precharge/activate
  if(mRef == NULL){

    //Go through scheduling queue
    for(int i=0; i<queueSize; i++){
      
      //If the request is valid
      if(dramQ[i]->isValid()){
	//If the request needs a precharge
	if(dramQ[i]->needsPrecharge()){
	  //If the precharge can issue
	  if(canIssuePrecharge(dramQ[i]->getRankID(), dramQ[i]->getBankID())){
	    //If the precharge is older than the oldest pre/act cmd found so far
	    if(dramQ[i]->getTimeStamp() < oldestTime){
	      mRef = dramQ[i];
	      oldestTime = dramQ[i]->getTimeStamp();
	      index = i;
	    }
	  }	  
	}
	//Otherwise, if the request needs an activate
	else if(dramQ[i]->needsActivate()){
	  //If the activate can issue
	  if(canIssueActivate(dramQ[i]->getRankID(), dramQ[i]->getBankID())){ 
	    //If the activate is older than the oldest pre/act cmd found so far
	    if(dramQ[i]->getTimeStamp() < oldestTime){
	      mRef = dramQ[i];
	      oldestTime = dramQ[i]->getTimeStamp();
	      index = i;
	    }
	  }
	}
	
      }
    }
	if(mRef != NULL){
	  if(!mRef->servTimeStampSet) {
		mRef->setServTimeStamp(DRAMClock);//apareek
//		std::cout << "wait time:\t"<<(DRAMClock-mRef->getTimeStamp())*multiplier<<"\n";
		//std::cout << "service time start:\t"<<DRAMClock*multiplier<<"\n";
		mRef->servTimeStampSet=true;
	  }
	}
  }

  //If an outstanding ready cmd exists in the queue
  if(mRef != NULL){
    
    //If the reference needs a precharge
    if(mRef->needsPrecharge()){

      //Precharge bank
      precharge(mRef->getRankID(), mRef->getBankID());
      mRef->setPrechargePending(false);

      //Update queue state
      updateQueueState();

      //Update stats
      completionRate->sample(0);
      numPrecharges->inc();
      return;
    }
    
    //If the reference needs an activate
    if(mRef->needsActivate()){

      //Activate row
      activate(mRef->getRankID(), mRef->getBankID(), mRef->getRowID());
      mRef->setActivatePending(false);

      //Update queue state
      updateQueueState();

      //Update stats
      completionRate->sample(0);
      numActivates->inc();
      return;
    }
    
    //If the reference needs a read
    if(mRef->needsRead()){

      //Issue read
      read(mRef->getRankID(), mRef->getBankID(), mRef->getRowID());
      mRef->setReadPending(false);


      // MemRequest *mreq = mRef->getMReq();
      // if (mreq->dma)
      // 	printf("DRAM read: %p returnaccess@%lld, transactionId: %d\n",
      // 	       (void *) mreq->getPAddr(), (long long) globalClock, mreq->dmaId);
      // else
      // 	printf("DRAM read: %p returnaccess@%lld\n",
      // 	       (void *) mreq->getPAddr(), (long long) globalClock);
			//
			// rlavaee: better to reuse completion time
			Time_t comp_time = globalClock + multiplier * (tCL + (BL/2)+1);

			//rlavaee{
		rdCompQ.push_back(comp_time);
			//rlavaee}
      // yanwei, stats
      memoryRAccesTime->sample(comp_time - mRef->getTimeStamp()*multiplier);
      // ~yanwei
	IntervalMemoryRAccessTime->sample(comp_time - mRef->getTimeStamp()*multiplier); //Omid
			IntervalNReads->inc();
      readServRate->sample(comp_time - mRef->getServTimeStamp()*multiplier);//apareek
			//std::cout << "one read from rank"<< mRef->getRankID() <<", bank " << mRef->getBankID() <<" serviced in "<< comp_time - mRef->getServTimeStamp()*multiplier << std::endl;
	  mRef->servTimeStampSet=false;
    //std::cout << "service time end:\t"<<globalClock + multiplier * (tCL + (BL/2) + 1) <<"\n";

      //This reference is now complete
      mRef->complete(comp_time);
#ifndef FASTQUEUE_LIST
      freeList->push_back(index);
#else
      freeList->push(index);
#endif
      occupancy--;
			rdOccupancy--;


      
      //Update stats
      completionRate->sample(1);
      numReads->inc();

      return;
    }
    
    //If the reference needs a write
    if(mRef->needsWrite()){

      //Issue write
      write(mRef->getRankID(), mRef->getBankID(), mRef->getRowID());
      mRef->setWritePending(false);



      // MemRequest *mreq = mRef->getMReq();
      // if (mreq->dma)
      // 	printf("DRAM write: %p returnaccess@%lld, transactionId: %d\n",
      // 	       (void *) mreq->getPAddr(), (long long) globalClock,  mreq->dmaId);
      // else
      // 	printf("DRAM write: %p returnaccess@%lld\n",
      // 	       (void *) mreq->getPAddr(), (long long) globalClock);

      // yanwei, stats
			//rlavaee: better to reuse completion time
			Time_t comp_time = globalClock+multiplier *(tWL + (BL/2) + 1);
      memoryWAccesTime->sample(comp_time - mRef->getTimeStamp()*multiplier);
      // ~yanwei
      
			// rlavaee{
			IntervalMemoryWAccessTime->sample(comp_time - mRef->getTimeStamp()*multiplier);
			IntervalNWrites->inc();
			//rlavaee}

      writeServRate->sample(comp_time - mRef->getServTimeStamp()*multiplier);//apareek
			//std::cout << "one write from rank"<< mRef->getRankID() <<", bank " << mRef->getBankID() <<" serviced in "<< comp_time - mRef->getServTimeStamp()*multiplier << std::endl;
	  mRef->servTimeStampSet=false;

	  // rlavaee, push this completion time into wrCompQ
	  wrCompQ.push_back(comp_time);
	  // ~rlavaee


      // This reference is now complete
      mRef->complete(comp_time);
#ifndef FASTQUEUE_LIST
      freeList->push_back(index);
#else
      freeList->push(index);
#endif
      occupancy--;
			wrOccupancy--;

      //Update stats
      completionRate->sample(1);
      numWrites->inc();
      return;
    }

  }
  } // if freelist not full
  
  //Update stats
  completionRate->sample(0);

}

/******************************************************************************/

//Clock the DDR2 system
void DDR2::clock()
{
  //If one DRAM clock has passed since last call
	//rlavaee: global clock is the processor clock
  if(globalClock % multiplier == 0){

    //Channel 0 Updates DRAM clock 
    if(channelID == 0){
      DRAMClock++;
    }
    
    //Go through all ranks
    for(int i=0; i<numRanks; i++){
      //Each rank updates its power state
      ranks[i]->updatePowerState();
      
      //Each channel updates DRAM energy with its background power
      channelEnergy->add(ranks[i]->getBackgroundEnergy());
    }
    
    //Update stats
    dramQAvgOccupancy->sample(occupancy);
    arrivalQAvgOccupancy->sample(arrivals);



		//report the stats for interval
		if(globalClock > Interval*IntervalCounter){
			wrJobCountStream << wrPopulation->getDouble() << std::endl;
			wrJobCountStream.flush();

			rdJobCountStream << rdPopulation->getDouble() << std::endl;
			rdJobCountStream.flush();

			wrServTimeStream << writeServRate->getDouble() << std::endl;
			wrServTimeStream.flush();
			rdServTimeStream << readServRate->getDouble() << std::endl;
			rdServTimeStream.flush();

	
			for(int j = 0; j<numRanks;j++){
				ranks[j]->dumpBankStats();
			}
	
			rdAccessTimeStream << IntervalMemoryRAccessTime->getDouble() << std::endl;
			rdAccessTimeStream.flush();

			wrAccessTimeStream << IntervalMemoryWAccessTime->getDouble() << std::endl;
			wrAccessTimeStream.flush();

			rdThinkTimeStream << (rdPopulation->getDouble()*Interval - IntervalMemoryRAccessTime->getSamples() * IntervalMemoryRAccessTime->getDouble())/IntervalMemoryRAccessTime->getSamples() << std::endl;
			rdThinkTimeStream.flush();

			
			wrThinkTimeStream << (wrPopulation->getDouble()*Interval - IntervalMemoryWAccessTime->getSamples() * IntervalMemoryWAccessTime->getDouble())/IntervalMemoryWAccessTime->getSamples() << std::endl;
			wrThinkTimeStream.flush();

			//report number of reads/writes
			rdNumStream << IntervalNReads->getValue() << std::endl;
			wrNumStream << IntervalNWrites->getValue() << std::endl;

			IntervalMemoryRAccessTime -> resetStat();
			IntervalMemoryWAccessTime -> resetStat();

			rdPopulation->resetStat();
			rdCompQ.clear();

			wrPopulation->resetStat();
			wrCompQ.clear();

			writeServRate->resetStat();
			readServRate->resetStat();


			IntervalNReads->resetStat();
			IntervalNWrites->resetStat();

			// increment the interval counter
			IntervalCounter++;
		}
		//~rlavaee
		
		    
    //Enqueue new arrivals
    while( (!freeList->empty()) && (arrivalHead != NULL)){
      enqueue(arrivalHead);
    }
    
    //Schedule next DDR2 command
    scheduleFCFS();

    // updateIdleDelayFuncControl();
    // scheduleElasticRefresh();
    //scheduleDUERefresh();
    //scheduleFRFCFS();
    
  }
  
}

 
//Get DDR2 channel ID
int DDR2::getChannelID(MemRequest *mreq)
{
  return (mreq->getPAddr() >> pageSizeLog2) & (numChannels - 1); 
}

//Get bank ID of a given mreq
int DDR2::getBankID(MemRequest *mreq)
{
  return (mreq->getPAddr() >> (pageSizeLog2 + numChannelsLog2 + numRanksLog2)) & (numBanks - 1);
}

// Added refresh support

//Refresh bank
void DDR2Bank::refresh()
{
  state = IDLE;
  lastRefresh  = DRAMClock;
//  timeRefresh += tREFI;
  casHit = false;
}

//number of needed refreshes ?
int DDR2Rank::neededRefreshes(int bankID)
{
  return (DRAMClock - timeRefresh) / tREFI;
}


//can a refresh issue now ?
bool DDR2Rank::canIssueRefresh(int bankID)
{

  //Enforce tRP
  if(DRAMClock - lastPrecharge < tRP){
    return false;
  }

  //Enforce tRFC
  if(DRAMClock - lastRefresh < tRFC){
    return false;
  }

  return true;
}

//Refresh
void DDR2Rank::refresh(int bankID)
{
  numActive = 0;
  //Timing
  lastRefresh  = DRAMClock;
  timeRefresh += tREFI;

  //Energy
  rankEnergy->add(eRefresh);

  // Refresh all banks
  for(int i=0; i<numBanks; i++){
    banks[i]->refresh();
  }

  //power
  // cke = true;
}

// bool DDR2Rank::consWait(int bankID){
//   return ((DRAMClock - lastRefresh) < consDelay);
// }

// bool DDR2Rank::propWait(int bankID, int backlog){
//   return ((DRAMClock - lastRefresh) < (consDelay - (backlog - 2) * propDelay));
// }

// bool DDR2::consWait(int rankID, int bankID) {
//   return ranks[rankID]->consWait(bankID);
// }

// bool DDR2::propWait(int rankID, int bankID, int backlog) {
//   return ranks[rankID]->propWait(bankID, backlog);
// }

void DDR2::refresh(int rankID, int bankID)
{
  //Issue refresh
  ranks[rankID]->refresh(bankID);
  lastRefresh  = DRAMClock;
  timeRefresh += tREFI;
}

//number of needed refreshes to given rank ?
int DDR2::neededRefreshes(int rankID, int bankID)
{
  return ranks[rankID]->neededRefreshes(bankID);
}

//Can a refresh to given rank issue now ?
bool DDR2::canIssueRefresh(int rankID, int bankID)
{
  return ranks[rankID]->canIssueRefresh(bankID);
}


void DDR2::scheduleDUERefresh()
{
    // check all ranks for refresh
    for(int r=0; r < numRanks; ++r) {
        int backlog = neededRefreshes(r, 0);

        if(ranks[r]->urgentRefresh) {
            if(canIssueRefresh(r, 0)) {
                //refresh the rank
                refresh(r, 0);
                //Update queue state
                updateQueueState();
                //Update stats
                numRefreshes->inc();

                //printf("%10lld\t", globalClock); for(int j=0; j<r; ++j){printf("\t");} printf("%d", backlog); printf("\n");
            }
            ranks[r]->urgentRefresh = (backlog != 0);
        }
        else if(backlog > 0) {
            switch (backlog) {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                case 5:
                case 6:

                    // check for any pending requests in rank buffers
                    for(int i=0; i < queueSize; i++) {
                        if(dramQ[i]->isValid() && (dramQ[i]->getRankID() == r)) {
                            break;
                        }
                    }

                    // issue the refresh command if allowed
                    if(canIssueRefresh(r, 0)) {
                        //refresh the rank
                        refresh(r, 0);
                        //Update queue state
                        updateQueueState();
                        //Update stats
                        numRefreshes->inc();

                        //printf("%10lld\t", globalClock); for(int j=0; j<r; ++j){printf("\t");} printf("%d", backlog); printf("\n");
                    }
                    break;

                case 7:
                    // high priority

                    // issue the refresh command if allowed
                    if(canIssueRefresh(r, 0)) {
                        //refresh the rank
                        refresh(r, 0);
                        //Update queue state
                        updateQueueState();
                        //Update stats
                        numRefreshes->inc();

                        //printf("%10lld\t", globalClock); for(int j=0; j<r; ++j){printf("\t");} printf("%d", backlog); printf("\n");
                    }
                    ranks[r]->urgentRefresh = true;
                    break;

                default:
                    printf("ERROR: number of postponed refresh commands exceeded the limit!\n");
                    exit(0);
            }
        }
    }
}

// void DDR2::scheduleElasticRefresh()
// {
//     // check all ranks for refresh
//     for(int r=0; r < numRanks; ++r) {
//         int backlog = neededRefreshes(r, 0);

//         if(ranks[r]->urgentRefresh) {
//             if(canIssueRefresh(r, 0)) {
//                 //refresh the rank
//                 refresh(r, 0);
//                 //Update queue state
//                 updateQueueState();
//                 //Update stats
//                 numRefreshes->inc();

//                 //printf("%10lld\t", globalClock); for(int j=0; j<r; ++j){printf("\t");} printf("%d", backlog); printf("\n");
//             }
//             ranks[r]->urgentRefresh = (backlog != 0);
//         }
//         else if(backlog > 0) {
//             switch (backlog) {
//                 case 0:
//                 case 1:
//                 case 2:
//                     // constant delay
//                     if(consWait(r, 0)) {
//                         break;
//                     }

//                     // check for any pending requests in rank buffers
//                     for(int i=0; i < queueSize; i++) {
//                         if(dramQ[i]->isValid() && (dramQ[i]->getRankID() == r)) {
//                             break;
//                         }
//                     }

//                     // issue the refresh command if allowed
//                     if(canIssueRefresh(r, 0)) {
//                         //refresh the rank
//                         refresh(r, 0);
//                         //Update queue state
//                         updateQueueState();
//                         //Update stats
//                         numRefreshes->inc();

//                         //printf("%10lld\t", globalClock); for(int j=0; j<r; ++j){printf("\t");} printf("%d", backlog); printf("\n");
//                     }
//                     break;
//                 case 3:
//                 case 4:
//                 case 5:
//                 case 6:
//                     // proportional delay
// 		  if(propWait(r, 0, backlog)) {
//                         break;
//                     }

//                     // check for any pending requests in rank buffers
//                     for(int i=0; i < queueSize; i++) {
//                         if(dramQ[i]->isValid() && (dramQ[i]->getRankID() == r)) {
//                             break;
//                         }
//                     }

//                     // issue the refresh command if allowed
//                     if(canIssueRefresh(r, 0)) {
//                         //refresh the rank
//                         refresh(r, 0);
//                         //Update queue state
//                         updateQueueState();
//                         //Update stats
//                         numRefreshes->inc();

//                         //printf("%10lld\t", globalClock); for(int j=0; j<r; ++j){printf("\t");} printf("%d", backlog); printf("\n");
//                     }
//                     break;

//                 case 7:
//                     // check for any pending requests in rank buffers
//                     for(int i=0; i < queueSize; i++) {
//                         if(dramQ[i]->isValid() && (dramQ[i]->getRankID() == r)) {
//                             break;
//                         }
//                     }

//                     // issue the refresh command if allowed
//                     if(canIssueRefresh(r, 0)) {
//                         //refresh the rank
//                         refresh(r, 0);
//                         //Update queue state
//                         updateQueueState();
//                         //Update stats
//                         numRefreshes->inc();

//                         //printf("%10lld\t", globalClock); for(int j=0; j<r; ++j){printf("\t");} printf("%d", backlog); printf("\n");
//                     }
//                     break;

//                 case 8:
//                     // high priority

//                     // issue the refresh command if allowed
//                     if(canIssueRefresh(r, 0)) {
//                         //refresh the rank
//                         refresh(r, 0);
//                         //Update queue state
//                         updateQueueState();
//                         //Update stats
//                         numRefreshes->inc();

//                         //printf("%10lld\t", globalClock); for(int j=0; j<r; ++j){printf("\t");} printf("%d", backlog); printf("\n");
//                     }
//                     ranks[r]->urgentRefresh = true;
//                     break;

//                 default:
//                     printf("ERROR: number of postponed refresh commands exceeded the limit!\n");
//                     exit(0);
//             }
//         }
//     }
// }

// void DDR2::updateIdleDelayFuncControl() {
//   int consDelay;

//   // update max delay control firstly

//   for(int r=0; r < numRanks; r++) {
//     for(int i=0; i < queueSize; i++) {
//       if(dramQ[i]->isValid() && 
// 	 (dramQ[i]->getRankID() == r) && 
// 	 !ranks[r]->prevStat) {
// 	++ranks[r]->maxDelayCount;
// 	ranks[r]->maxDelayAccum += ranks[r]->idleAccum;
// 	ranks[r]->idleAccum = 0;
// 	ranks[r]->prevStat = true;
// 	break;			// out of fori
//       }
//     }
//     if (i == queueSize) {
//       ranks[r]->idleAccum++;
//       ranks[r]->prevStat = false;
//     }
//     else if (ranks[r]->maxDelayCount == 1024){ // they are exclusive
//       ranks[r]->maxDelayCount = 0;
//       consDelay = ranks[r]->maxDelayAccum >> 10;
//       ranks[r]->consDelay = consDelay > 1024 ? 1024: consDelay;
//       ranks[r]->maxDelayAccum = 0;
//     }
//   }

//   // updae proportional slope control

//   for(int r=0; r < numRanks; r++) {
//     // counter update
//     if (neededRefreshes(r, 0) < ELASTIC_REFRESH_THRESHOLD)
//       ranks[r]->low++;
//     else
//       ranks[r]->high++;
  
//     if (ranks[r]->low > OVERFLOW_16BITS || ranks[r]->high > OVERFLOW_16BITS){
//       ranks[r]->low >>= 1;
//       ranks[r]->high >>= 1;
//     }

//     // propDelay update after 128k memory clocks interval
//     if (!(DRAMClock & MASK_17BITS)){
//       int wp = (ranks[r]->low - ranks[r]->high) & MASK_16BITS;
//       int wi = (ranks[r]->integral + wp) & MASK_16BITS;
//       ranks[r]->integral = wi;
//       wp >>= 11;			// positive or negative effect, FIXME
//       wi >>= 11;
//       ranks[r]->propDelay = wp + wi + ranks[r]->propDelay;
//     }
//   }

// }


