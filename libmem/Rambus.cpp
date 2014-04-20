/* 
   SESC: Super ESCalar simulator
   Copyright (C) 2003 University of Illinois.

   Contributed by Basilio Fraguela
                  Jose Renau

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
#include "Rambus.h"
#include "Cache.h"

//******************* RambusMemCtrl **********************
RambusMemCtrl::RambusMemCtrl(MemorySystem* current, const char *device_descr_section, const char *device_name)
  : MemObj(device_descr_section, device_name),
  rowBusOcc(SescConf->getLong(device_descr_section,"rowBusOcc")),
  dataBusOcc(SescConf->getLong(device_descr_section,"dataBusOcc"))
{
  SescConf->isLong(device_descr_section, "numBanks");
  SescConf->isLong(device_descr_section, "rowBusOcc");
  SescConf->isLong(device_descr_section, "dataBusOcc");

  SescConf->isGT(device_descr_section, "numBanks", 0);

  int numBanks    = SescConf->getLong(device_descr_section,"numBanks");
  int blockSize   = SescConf->getLong(device_descr_section,"bsize");

  bankMask   = (1L << log2i(numBanks)) - 1;
  bankOffset = 7 + log2i(blockSize);
  rowOffset  = bankOffset + log2i(numBanks);

  char cadena[100];
  sprintf(cadena,"%s_rowBus",device_name);
  rowBusPort  = PortGeneric::create(cadena, SescConf->getLong(device_descr_section,"numCntrl"), rowBusOcc);
  sprintf(cadena,"%s_dataBus",device_name);
  dataBusPort = PortGeneric::create(cadena, SescConf->getLong(device_descr_section,"numCntrl"), dataBusOcc);
	
  for (short i = 0; i < numBanks; i++) {
    addLowerLevel(current->declareMemoryObj(device_descr_section,"bankType"));
    static_cast<RambusBank *>(lowerLevel[i])->setRowOffset(rowOffset);
  }
}

void RambusMemCtrl::access(MemRequest *mreq)
{
  MemOperation memop = mreq->getMemOperation();

  if(memop==MemWrite || memop == MemPush )
    write(mreq);
  else
    read(mreq);
}

void RambusMemCtrl::read(MemRequest *mreq)
{
  //printf("Read Addr = %lx Bank no = %d\n", mreq->getPAddr(), calcBank(mreq->getPAddr()));
  mreq->goDownAbs(rowBusPort->nextSlot() + rowBusOcc, lowerLevel[calcBank(mreq->getPAddr())]);
}

void RambusMemCtrl::write(MemRequest *mreq)
{
  //printf("Write Addr = %lx Bank no = %d\n", mreq->getPAddr(), calcBank(mreq->getPAddr()));
  mreq->goDownAbs(rowBusPort->nextSlot() + rowBusOcc, lowerLevel[calcBank(mreq->getPAddr())]);
}

void RambusMemCtrl::returnAccess(MemRequest *mreq)
{
  mreq->goUpAbs(dataBusPort->nextSlot() + dataBusOcc);
}

void RambusMemCtrl::specialOp(MemRequest *mreq)
{
  MSG("MemCtrl::specialOp called, and not instrumented");
  exit(-1);
}

Time_t RambusMemCtrl::getNextFreeCycle() const
{ 
  return rowBusPort->calcNextSlot();
}

void RambusMemCtrl::invalidate(PAddr addr,ushort size,MemObj *oc)
{ 
  invUpperLevel(addr,size,oc); 
}

bool RambusMemCtrl::canAcceptStore(PAddr addr)
{
  return true;
}

//NM********** RambusBank *************
RambusBank::RambusBank(MemorySystem *gms, const char *device_descr_section, const char *name)
  : MemObj(device_descr_section, name)
  ,readHit("%s:readHit", name)
  ,writeHit("%s:writeHit", name)
  ,writeMiss("%s:writeMiss", name)
  ,readMiss("%s:readMiss", name)
  ,linePush("%s:linePush",name)
{
  MemObj *lower_level = NULL;
  I(gms);
  lower_level = gms->declareMemoryObj(device_descr_section, k_lowerLevel);
 
  if (lower_level != NULL)
    addLowerLevel(lower_level);

  // OK for a cache that always hits
  GMSG(lower_level == NULL,
    "You are defining a cache with void as lowerLevel\n");
 
  SescConf->isLong(device_descr_section, "numPorts");
  SescConf->isLong(device_descr_section, "portOccp");

  cachePort = PortGeneric::create(name, SescConf->getLong(device_descr_section, "numPorts"),
                                        SescConf->getLong(device_descr_section, "portOccp"));

  hitDelay = SescConf->getLong(device_descr_section, "portOccp");

  rowMask = ((1L<<10) - 1); //Assumes 1024 rows for each bank
  openPage = 0;
}

void RambusBank::access(MemRequest *mreq)
{
  mreq->setClockStamp((Time_t) - 1);

  if(mreq->getPAddr() <= 1024) { // TODO: need to implement support for fences
    mreq->goUp(0);
    return;
  }

  switch(mreq->getMemOperation()){
  case MemReadW:
  case MemRead:    read(mreq);       break;
  case MemWrite:   write(mreq);      break;
  case MemPush:    pushLine(mreq);   break;
  default:         specialOp(mreq);  break;
  }
}

void RambusBank::returnAccess(MemRequest *mreq)
{
  I(0);
}

void RambusBank::read(MemRequest *mreq)
{
  long currRow = getRow(mreq->getPAddr());

  //printf("Read Addr = %lx, currRow %ld, openPage = %ld, rowOffset = %ld\n", mreq->getPAddr(),currRow,openPage,rowOffset);

  if(currRow == openPage) {
    readHit.inc();
    mreq->goUpAbs(cachePort->occupySlots(1) + hitDelay);
  }else {
    readMiss.inc();
    mreq->goUpAbs(cachePort->occupySlots(2) + (hitDelay<<2));
  }
  openPage = currRow;
}

void RambusBank::write(MemRequest *mreq)
{
  long currRow = getRow(mreq->getPAddr());

  if(currRow == openPage) {
    writeHit.inc();
    mreq->goUpAbs(cachePort->occupySlots(1) + hitDelay);
  }else {
    writeMiss.inc();
    mreq->goUpAbs(cachePort->occupySlots(2) + (hitDelay<<2));
  }
  openPage = currRow;
}

void RambusBank::pushLine(MemRequest *mreq)
{
  long currRow = getRow(mreq->getPAddr());

  //printf("Push Addr = %lx, currRow %ld, openPage = %ld, rowOffset = %ld\n", mreq->getPAddr(),currRow,openPage,rowOffset);

  linePush.inc();
  if(currRow == openPage) {
    writeHit.inc();
    mreq->goUpAbs(cachePort->occupySlots(1) + hitDelay);
  }else {
    writeMiss.inc();
    mreq->goUpAbs(cachePort->occupySlots(2) + (hitDelay<<2));
  }
  openPage = currRow;
}

void RambusBank::specialOp(MemRequest *mreq)
{
  mreq->goUp(1);
}

Time_t RambusBank::getNextFreeCycle() const
{
  return cachePort->calcNextSlot();
}

void RambusBank::invalidate(PAddr addr, ushort size, MemObj *oc)
{
  invUpperLevel(addr,size,oc);
}

bool RambusBank::canAcceptStore(PAddr addr)
{
  return true;
}
//************************//
