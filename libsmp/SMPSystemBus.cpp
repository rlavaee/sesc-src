/* 
   SESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Karin Strauss

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

#include "SMPSystemBus.h"
#include "SMemorySystem.h"
#include "SMPCache.h"
#include "SMPDebug.h"

//NM added data bus.

SMPSystemBus::SMPSystemBus(SMemorySystem *dms, const char *section, const char *name)
  : MemObj(section, name)
{
  MemObj *lowerLevel = NULL;

  I(dms);
  lowerLevel = dms->declareMemoryObj(section, "lowerLevel");

  if (lowerLevel != NULL)
    addLowerLevel(lowerLevel);

  SescConf->isLong(section, "numPorts");
  SescConf->isLong(section, "portOccp");
  SescConf->isLong(section, "delay");
  SescConf->isLong(section, "addressDelay");
  SescConf->isLong(section, "snoopDelay");
  SescConf->isLong(section, "dataDelay");
  SescConf->isLong(section, "numDataPorts");
  
  delay = SescConf->getLong(section, "delay");

  char portName[100];
  sprintf(portName, "%s_bus", name);

  addressDelay = SescConf->getLong(section, "addressDelay");
  address_snoopDelay = addressDelay+SescConf->getLong(section, "snoopDelay");
  busPort = PortGeneric::create(portName, 
				SescConf->getLong(section, "numPorts"), 
				SescConf->getLong(section, "portOccp"));

  sprintf(portName,"%s_data",name);

  dataDelay    = SescConf->getLong(section, "dataDelay");
  dataPort = PortGeneric::create(portName, 
                                 SescConf->getLong(section, "numDataPorts"), 
                                 dataDelay);
}

SMPSystemBus::~SMPSystemBus() 
{
  // do nothing
}

Time_t SMPSystemBus::getNextFreeCycle() const
{
  return busPort->calcNextSlot();
}

Time_t SMPSystemBus::nextSlot()
{
  return busPort->nextSlot();
}

Time_t SMPSystemBus::nextDataSlot()
{
  return dataPort->nextSlot();
}

bool SMPSystemBus::canAcceptStore(PAddr addr) const
{
  return true;
}

void SMPSystemBus::access(MemRequest *mreq)
{
  GMSG(mreq->getPAddr() < 1024,
       "mreq dinst=0x%p paddr=0x%x vaddr=0x%x memOp=%d",
       mreq->getDInst(),
       (unsigned int) mreq->getPAddr(),
       (unsigned int) mreq->getVaddr(),
       mreq->getMemOperation());
  
  I(mreq->getPAddr() > 1024); 

  switch(mreq->getMemOperation()){
  case MemRead:     read(mreq);      break;
  case MemReadW:    
  case MemWrite:    write(mreq);     break;
  case MemPush:     push(mreq);      break;
  default:          specialOp(mreq); break;
  }

  // for reqs coming from upper level:
  // MemRead means I need to read the data, but I don't have it
  // MemReadW means I need to write the data, but I don't have it
  // MemWrite means I need to write the data, but I don't have permission
  // MemPush means I don't have space to keep the data, send it to memory
}

void SMPSystemBus::read(MemRequest *mreq)
{
  if(pendReqsTable.find(mreq) == pendReqsTable.end()) {
    doReadCB::scheduleAbs(nextSlot()+address_snoopDelay, this, mreq);
  } else {
    doRead(mreq);
  }
}

void SMPSystemBus::write(MemRequest *mreq)
{
  SMPMemRequest *sreq = static_cast<SMPMemRequest *>(mreq);

  if(pendReqsTable.find(mreq) == pendReqsTable.end()) {
    doWriteCB::scheduleAbs(nextSlot()+address_snoopDelay, this, mreq);
  } else {
    doWrite(mreq);
  }
}

void SMPSystemBus::push(MemRequest *mreq)
{
  doPushCB::scheduleAbs(nextSlot()+addressDelay, this, mreq);
  //Very dangerous!!! busPort->lock4nCycles(delay); //Occupy 1 bus cycle
}

void SMPSystemBus::specialOp(MemRequest *mreq)
{
  I(0);
  mreq->goUp(1); // TODO: implement atomic ops?
}

void SMPSystemBus::doRead(MemRequest *mreq)
{
  SMPMemRequest *sreq = static_cast<SMPMemRequest *>(mreq);

  // no need to snoop, go straight to memory
  if(!sreq->needsSnoop()) {
    goToMem(mreq);
    return;
  }

  if(pendReqsTable.find(mreq) == pendReqsTable.end()) {

    unsigned numSnoops = getNumSnoopCaches(sreq);

    // operation is starting now, add it to the pending requests buffer
    pendReqsTable[mreq] = getNumSnoopCaches(sreq);

    if(!numSnoops) { 
      // nothing to snoop on this chip
      finalizeRead(mreq);
      return;
      // TODO: even if there is only one processor on each chip, 
      // request is doing two rounds: snoop and memory
    }

    // distribute requests to other caches, wait for responses
    for(ulong i = 0; i < upperLevel.size(); i++) {
      if(upperLevel[i] != static_cast<SMPMemRequest *>(mreq)->getRequestor()) {
	upperLevel[i]->returnAccess(mreq);
      }
    }
  } 
  else {
    // operation has already been sent to other caches, receive responses

    I(pendReqsTable[mreq] > 0);
    I(pendReqsTable[mreq] <= (int) upperLevel.size());

    pendReqsTable[mreq]--;
    if(pendReqsTable[mreq] != 0) {
      // this is an intermediate response, request is not serviced yet
      return;
    }

    // this is the final response, request can go up now
    finalizeRead(mreq);
  }
}

void SMPSystemBus::finalizeRead(MemRequest *mreq)
{
  finalizeAccess(mreq);
}

void SMPSystemBus::doWrite(MemRequest *mreq)
{
  SMPMemRequest *sreq = static_cast<SMPMemRequest *>(mreq);

  // no need to snoop, go straight to memory
  if(!sreq->needsSnoop()) {
    goToMem(mreq);
    return;
  }

  if(pendReqsTable.find(mreq) == pendReqsTable.end()) {

    unsigned numSnoops = getNumSnoopCaches(sreq);

    // operation is starting now, add it to the pending requests buffer
    pendReqsTable[mreq] = getNumSnoopCaches(sreq);

    if(!numSnoops) { 
      // nothing to snoop on this chip
      finalizeWrite(mreq);
      return;
      // TODO: even if there is only one processor on each chip, 
      // request is doing two rounds: snoop and memory
    }

    GLOG(SMPDBG_MSGS, "SystemBus::doWrite for addr 0x%x, type = %d, @%lld",
       (uint) mreq->getPAddr(), (uint) mreq->getMemOperation(), globalClock);

    // distribute requests to other caches, wait for responses
    for(ulong i = 0; i < upperLevel.size(); i++) {
      if(upperLevel[i] != static_cast<SMPMemRequest *>(mreq)->getRequestor()) {
	upperLevel[i]->returnAccess(mreq);
      }
    }
  } 
  else {
    // operation has already been sent to other caches, receive responses

    GLOG(SMPDBG_MSGS, "SystemBus::doWrite (one snoop ready) for addr 0x%x, type = %d, @%lld",
       (uint) mreq->getPAddr(), (uint) mreq->getMemOperation(), globalClock);

    I(pendReqsTable[mreq] > 0);
    I(pendReqsTable[mreq] <= (int) upperLevel.size());

    pendReqsTable[mreq]--;
    if(pendReqsTable[mreq] != 0) {
      // this is an intermediate response, request is not serviced yet
      return;
    }

    // this is the final response, request can go up now
    finalizeWrite(mreq);
  }
}

void SMPSystemBus::finalizeWrite(MemRequest *mreq)
{
  finalizeAccess(mreq);
}

void SMPSystemBus::finalizeAccess(MemRequest *mreq)
{
  GLOG(SMPDBG_MSGS, "SystemBus::finalizeAccess for addr 0x%x, type = %d, @%lld",
       (uint) mreq->getPAddr(), (uint) mreq->getMemOperation(), globalClock);

  PAddr addr  = mreq->getPAddr();
  SMPMemRequest *sreq = static_cast<SMPMemRequest *>(mreq);
  
  pendReqsTable.erase(mreq);
 
  // request completed, respond to requestor 
  // (may have to come back later to go to memory)
  if(sreq->needsData() && sreq->isFound()) {
    I(mreq->getMemOperation() != MemWrite);
    sreq->goUpAbs(nextDataSlot() + dataDelay);
  } else
    sreq->goUp(0);  
}

void SMPSystemBus::goToMem(MemRequest *mreq)
{
  mreq->goDown(delay, lowerLevel[0]);
}

void SMPSystemBus::doPush(MemRequest *mreq)
{
  nextDataSlot();
  mreq->goDown(delay, lowerLevel[0]);
}

void SMPSystemBus::invalidate(PAddr addr, ushort size, MemObj *oc)
{
  invUpperLevel(addr, size, oc);
}

void SMPSystemBus::doInvalidate(PAddr addr, ushort size)
{
  I(0);
}

void SMPSystemBus::returnAccess(MemRequest *mreq)
{
  if(mreq->getMemOperation() == MemPush) {
    mreq->goUp(1);
    return;
  }
  mreq->goUpAbs(nextDataSlot() + dataDelay);
}
