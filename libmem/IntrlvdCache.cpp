#include "IntrlvdCache.h"
#include "IntrlvdMemRequest.h"
#include "SescConf.h"

#define INTRLVD_CACHE_DEBUG 0

IntrlvdCache ** IntrlvdCache::intrlvdCaches = 0;

//Constructor
IntrlvdCache::IntrlvdCache(MemorySystem *gms, const char *section, const char *name)
  :WBCache(gms,section,name)
{
  if(intrlvdCaches == 0)
    intrlvdCaches = new IntrlvdCache*[SescConf->getRecordSize("","cpucore")];

  intrlvdCaches[gms->getId()] = this;
  
  addrMask      = 0;
  myBankMask    = 0; 
  nIntrlvdBanks = 1;
  headBankId    = gms->getId();  
  id            = gms->getId();
}

IntrlvdCache::~IntrlvdCache()
{
  if(intrlvdCaches) {
    delete intrlvdCaches;
    intrlvdCaches = 0;
  }
}

void IntrlvdCache::reconfigure(int numBanks)
{
  int blockOffs = cacheBanks[0]->getLog2AddrLs();

  addrMask   =  ~((numBanks-1) << blockOffs);
  myBankMask = (getId() % numBanks) << blockOffs;
  nIntrlvdBanks = numBanks;
  headBankId = (getId() / numBanks) * numBanks;
  MSG("Id = %d, headBankId = %d, Address mask = %lx, myBankMask = %lx, nIntrlvdBanks = %d", 
      getId(), headBankId, addrMask, myBankMask, nIntrlvdBanks);
}

void IntrlvdCache::access(MemRequest *mreq)
{
  I(mreq->getMemOperation() == MemRead);
  I(mreq->getPAddr() > 1024);

  mreq->setClockStamp((Time_t) -1);

  GLOG(INTRLVD_CACHE_DEBUG,"IntrlvdCache(%d):access addr = %lx", getId(), mreq->getPAddr());

  nAccesses[0]->inc(); //there is only one bank

  I(pendReqTable.find(mreq) == pendReqTable.end());
 
  pendReqTable[mreq].nPendings = nIntrlvdBanks;

  int end = getId() + nIntrlvdBanks;
  IntrlvdMemRequest * intrlvdReq;
  for(int i = getId(); i < end; i++) {
    intrlvdReq = IntrlvdMemRequest::create(mreq, mreq->getPAddr(), intrlvdCaches[i]);
    intrlvdCaches[i]->read(intrlvdReq); 
  }
}

void IntrlvdCache::doRead(MemRequest *mreq)
{
  PAddr addr = (mreq->getPAddr() & addrMask) | myBankMask;
  Line *l = getCacheBank(addr)->readLine(addr);
  
  GLOG(INTRLVD_CACHE_DEBUG, "IntrlvdCache(%d):doRead  memop = %d  addr = %lx  getPAddr = %lx  @%lld",
          getId(), mreq->getMemOperation(), addr,  mreq->getPAddr(), globalClock);

  if (l == 0) { 
    GLOG(INTRLVD_CACHE_DEBUG,"IntrlvdCache(%d):doRead  MISS  @%lld", getId(),globalClock);
    readMissHandler(mreq);
    return;
  }

  readHit.inc();
  l->incReadAccesses();
  
  mreq = completeRequest(mreq);
  if(mreq) {
     GLOG(INTRLVD_CACHE_DEBUG,"IntrlvdCache(%d):doRead HIT  All pending reqs finished @%lld",getId(), globalClock);
    mreq->goUp(hitDelay);
  } else {
    GLOG(INTRLVD_CACHE_DEBUG,"IntrlvdCache(%d):doRead HIT  remainingPendRequests = %d  @%lld", 
           getId(), intrlvdCaches[headBankId]->pendReqTable[mreq].nPendings, globalClock);
  }
}

MemRequest * IntrlvdCache::completeRequest(MemRequest *mreq)
{
  IntrlvdMemRequest * intrlvdReq = static_cast<IntrlvdMemRequest *>(mreq);
  MemRequest *orgReq = intrlvdReq->getOriginalRequest();
  
  intrlvdReq->destroy();

  IntrlvdCache * headCache = intrlvdCaches[headBankId];

  I(headCache->pendReqTable[orgReq].nPendings > 0);

  if(--(headCache->pendReqTable[orgReq].nPendings) == 0) {

    GLOG(INTRLVD_CACHE_DEBUG, "intrlvdCache(%d):completeRequest request for orgAddr %lx completed @%lld",
        getId(), orgReq->getPAddr(),globalClock); 

    headCache->pendReqTable.erase(orgReq);

    return orgReq;
  }

  GLOG(INTRLVD_CACHE_DEBUG,"intrlvdCache(%d):completeRequest request for orgAddr %lx not completed @%lld",
      getId(), orgReq->getPAddr(),globalClock);

  return 0;
}

void IntrlvdCache::readMissHandler(MemRequest *mreq)
{
  PAddr addr = (mreq->getPAddr() & addrMask) | myBankMask;

  mreq->setClockStamp(globalClock);

  nextMSHRSlot(addr); // checking if there is a pending miss

  if(!getBankMSHR(addr)->issue(addr, MemRead)) {
    getBankMSHR(addr)->addEntry(addr, doReadQueuedCB::create(this, mreq),
                                activateOverflowCB::create(this, mreq), MemRead);
    GLOG(INTRLVD_CACHE_DEBUG,"IntrlvdCache(%d):readMissHandler cannot issue addr = %lx orgAddr = %lx ,@%lld", 
        getId(), addr, mreq->getPAddr(), globalClock);
    return;
  }

  GLOG(INTRLVD_CACHE_DEBUG,"IntrlvdCache(%d):readMissHandler can issue addr = %lx orgAddr = %lx ,@%lld", 
      getId(), addr, mreq->getPAddr(), globalClock);
  // Added a new MSHR entry, now send request to lower level
  readMiss.inc();
  sendMiss(mreq);
}

void IntrlvdCache::doReadQueued(MemRequest *mreq)
{
  PAddr paddr = (mreq->getPAddr() & addrMask) | myBankMask;
  readHalfMiss.inc();

  avgMissLat.sample(globalClock - mreq->getClockStamp());

  mreq = completeRequest(mreq);
  if(mreq)
    mreq->goUp(hitDelay);

  // the request came from the MSHR, we need to retire it
  getBankMSHR(paddr)->retire(paddr);
}

void IntrlvdCache::activateOverflow(MemRequest *mreq)
{
  I(mreq->getMemOperation() == MemRead);

  PAddr addr = (mreq->getPAddr() & addrMask) | myBankMask;

  Line *l = getCacheBank(addr)->readLine(addr);

  if (l == 0) {
      // no need to add to the MSHR, it is already there
      // since it came from the overflow
      readMiss.inc();
      sendMiss(mreq);
      return;
  }

  readHit.inc();
  
  mreq = completeRequest(mreq);
  if(mreq)
    mreq->goUp(hitDelay);

  // the request came from the MSHR overflow, we need to retire it
  getBankMSHR(addr)->retire(addr);
}

void IntrlvdCache::sendMiss(MemRequest *mreq)
{
  PAddr addr = (mreq->getPAddr() & addrMask) | myBankMask;

  Time_t when = nextMSHRSlot(addr);

  if(intrlvdCaches[headBankId]->canIssue(mreq)) {

    GLOG(INTRLVD_CACHE_DEBUG,"IntrlvdCache(%d):sendMiss can issue addr = %lx orgAddr = %lx ,@%lld",
         getId(), addr, mreq->getPAddr(),globalClock);

    mreq->goDown(missDelay + when - globalClock, lowerLevel[0]);
  }
}

bool IntrlvdCache::canIssue(MemRequest * mreq)
{
  IntrlvdMemRequest * intrlvdReq = static_cast<IntrlvdMemRequest *>(mreq);
  MemRequest * orgReq = intrlvdReq->getOriginalRequest();
 
  I(pendReqTable.find(orgReq) != pendReqTable.end());

  if(pendReqTable[orgReq].waitPending) {
    pendReqTable[orgReq].pendingReqs.push_back(mreq);
    return false;
  }

  pendReqTable[orgReq].waitPending = true;
  return true;
}

void IntrlvdCache::returnAccess(MemRequest *mreq)
{
  PAddr addr = (mreq->getPAddr() & addrMask) | myBankMask;
  
  GLOG(INTRLVD_CACHE_DEBUG,"IntrlvdCache(%d):returnAccess  memop = %d  addr = %lx  getPAddr = %lx  @%lld", 
 		getId(), mreq->getMemOperation(), addr, mreq->getPAddr(), globalClock);

  intrlvdCaches[headBankId]->retire(mreq); 

  // the MSHR needs to be accesed when the data comes back
  preReturnAccessCB::scheduleAbs(nextMSHRSlot(addr), this, mreq);
}

void IntrlvdCache::retire(MemRequest * mreq)
{
  IntrlvdMemRequest * intrlvdReq = static_cast<IntrlvdMemRequest *>(mreq);
  MemRequest * orgReq = intrlvdReq->getOriginalRequest();
 
  if(pendReqTable[orgReq].waitPending) {
   
    pendReqTable[orgReq].waitPending = false;

    std::vector<MemRequest *> * reqsQ = &pendReqTable[orgReq].pendingReqs;
   
    while(!reqsQ->empty()) {
      MemRequest * pendReq = reqsQ->back();
      reqsQ->pop_back();

      pendReq->getCurrentMemObj()->returnAccess(pendReq);
    };
  }
}

void IntrlvdCache::preReturnAccess(MemRequest *mreq)
{
  PAddr addr = (mreq->getPAddr() & addrMask) | myBankMask;

  Line *l = getCacheBank(addr)->writeLine(addr);

  if (l == 0) {
    nextBankSlot(addr); // had to check the bank if it can accept the new line
    CallbackBase *cb = doReturnAccessCB::create(this, mreq);
    l = allocateLine(addr, cb);

    if(l != 0) {
      // the allocation was successfull, no need for the callback
      cb->destroy();
    } else {
      // not possible to allocate a line, will be called back later
      return;
    }
  }

  int nPendReads = getBankMSHR(addr)->getUsedReads(addr);

  doReturnAccess(mreq);
  l->setReadMisses(nPendReads);
}

void IntrlvdCache::doReturnAccess(MemRequest *mreq)
{
  PAddr addr = (mreq->getPAddr() & addrMask) | myBankMask;

  Line *l = getCacheBank(addr)->findLineTagNoEffect(addr);
  I(l);
  l->validate();

  I(mreq->getMemOperation() == MemRead);
  
  avgMissLat.sample(globalClock - mreq->getClockStamp());

  GLOG(INTRLVD_CACHE_DEBUG,"IntrlvdCache(%d):doReturnAccess addr = %lx orgAddr = %lx ,@%lld", 
       getId(), addr, mreq->getPAddr(),globalClock);
  
  mreq = completeRequest(mreq);
  if(mreq)
    mreq->goUp(0);

  getBankMSHR(addr)->retire(addr);
}
