#include "IntrlvdMemRequest.h"
#include "MemObj.h"

pool<IntrlvdMemRequest> IntrlvdMemRequest::rPool(256);

IntrlvdMemRequest::IntrlvdMemRequest()
  : MemRequest()
{
}

IntrlvdMemRequest *IntrlvdMemRequest::create(MemRequest *mreq, PAddr newAddr, MemObj * memObj)
{
  IntrlvdMemRequest *intrlvdReq = rPool.out();

  I(mreq);
  intrlvdReq->oreq = mreq;
  IS(intrlvdReq->acknowledged = false);
  I(intrlvdReq->memStack.empty());

  intrlvdReq->currentClockStamp = globalClock;

  intrlvdReq->pAddr = newAddr;
  intrlvdReq->memOp = mreq->getMemOperation();
  intrlvdReq->dataReq = mreq->isDataReq();
  intrlvdReq->prefetch = mreq->isPrefetch();

  intrlvdReq->currentMemObj = memObj;

  return intrlvdReq;
}

void IntrlvdMemRequest::destroy()
{
  I(memStack.empty());
  rPool.in(this);
}

VAddr IntrlvdMemRequest::getVaddr() const
{
  I(0);
  return oreq->getVaddr();
}

void IntrlvdMemRequest::ack(TimeDelta_t lat)
{
  I(memStack.empty());
  I(acknowledged == false);
  IS(acknowledged = true);
  I(lat == 0);

  destroy();
}

