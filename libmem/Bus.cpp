/* 
   SESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Jose Renau
                  Basilio Fraguela
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

#include "SescConf.h"
#include "MemorySystem.h"
#include "Bus.h"
#include "DDR2.h"

#include "OSSim.h"

#include <iostream>

Bus::Bus(MemorySystem* current
	 ,const char *section
	 ,const char *name)
  : MemObj(section, name)
  ,delay(SescConf->getLong(section, "delay"))
{
  MemObj *lower_level = NULL;
  
  SescConf->isLong(section, "numPorts");
  SescConf->isLong(section, "portOccp");
  SescConf->isLong(section, "delay");

  NumUnits_t  num = SescConf->getLong(section, "numPorts");
  TimeDelta_t occ = SescConf->getLong(section, "portOccp");

  //Is this the memory bus
  SescConf->isBool(section, "isMemoryBus");
  isMemoryBus = SescConf->getBool(section, "isMemoryBus");
  
  //Number of independent DDR2 channels
  if(isMemoryBus){
    SescConf->isLong(section, "numChannels");
    numChannels = SescConf->getLong(section, "numChannels");
  }

  char cadena[100];
  sprintf(cadena,"Data%s", name);
  dataPort = PortGeneric::create(cadena, num, occ);
  sprintf(cadena,"Cmd%s", name);
  cmdPort  = PortGeneric::create(cadena, num, 1);

  I(current);
  lower_level = current->declareMemoryObj(section, k_lowerLevel);   
  if (lower_level != NULL)
    addLowerLevel(lower_level);

  opAvgBusTime[MemRead] = new GStatsAvg("%s_AvgTime_MemRead", name);
  opAvgBusTime[MemWrite] = new GStatsAvg("%s_AvgTime_MemWrite", name);
  opAvgBusTime[MemPush] = new GStatsAvg("%s_AvgTime_MemPush", name);
  opAvgBusTime[MemReadW] = new GStatsAvg("%s_AvgTime_MemReadW", name);
  
  //If this is the memory bus
  if(isMemoryBus){
    //DRAM subsystem
    DRAM = new DDR2*[numChannels];
    //Allocate all channels
    for(int i=0; i<numChannels; i++){
      DRAM[i] = new DDR2(section, i);
    } 
    //Link all channels together 
    for(int i=0; i<numChannels; i++){
      DRAM[i]->setNextChannel(DRAM[(i+1) % numChannels]);
    }
    OSSim::DRAM = DRAM;
  }

}

void Bus::access(MemRequest *mreq)
{
  
  //If this is the memory bus, arrive in DRAM queue
  if(isMemoryBus){   
    
    //Get channelID
    int chID = DRAM[0]->getChannelID(mreq);
    //Arrive
    mreq->front = NULL;
    mreq->back = NULL;
    DRAM[chID]->arrive(mreq);
    return;

  }

  Time_t when = 0;
  if (mreq->getMemOperation() == MemWrite) {
    when = dataPort->nextSlot()+delay;
  }else{
    when = cmdPort->nextSlot();
  }
  
  opAvgBusTime[mreq->getMemOperation()]->sample(when-globalClock);
  
  mreq->goDownAbs(when, lowerLevel[0]);
}

void Bus::returnAccess(MemRequest *mreq)
{

  //Check
  if(isMemoryBus){
    std::cout << "Memory bus called return access." << std::endl;
    exit(1);
  }

  Time_t when = 0;

  if (mreq->getMemOperation() == MemWrite) {
    when = cmdPort->nextSlot();
  }else{
    when = dataPort->nextSlot()+delay;
  }

  opAvgBusTime[mreq->getMemOperation()]->sample(when-globalClock);
 
  mreq->goUpAbs(when);
}

bool Bus::canAcceptStore(PAddr addr)
{
  return true;
}

void Bus::invalidate(PAddr addr,ushort size,MemObj *oc)
{ 
  invUpperLevel(addr,size,oc); 
}

Time_t Bus::getNextFreeCycle() const
{ 
  return cmdPort->calcNextSlot();
}
