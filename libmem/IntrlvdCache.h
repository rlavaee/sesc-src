#ifndef INTRLVDCACHE_H
#define INTRLVDCACHE_H

#include "Cache.h"
#include <vector>

class IntrlvdCache : public WBCache
{
  static IntrlvdCache** intrlvdCaches;

  class IntrlvdEntry {
  public:
    std::vector<MemRequest *> pendingReqs;
    int  nPendings;
    bool waitPending;
 
    IntrlvdEntry(){
      nPendings   = 0;  
      waitPending = false;
    }
  };
private:
  typedef HASH_MAP<MemRequest*, IntrlvdEntry, MemRequestHashFunc> PendReqTable;

  PendReqTable pendReqTable;

  int id;
  int nIntrlvdBanks;
  int headBankId;
  PAddr addrMask;
  PAddr myBankMask;

public:
   IntrlvdCache(MemorySystem *gms, const char *section, const char *name);
  ~IntrlvdCache();

   void reconfigure(int numBanks);
  
   void access(MemRequest *mreq);
protected:
   int getId() const { return id; }
   void doRead(MemRequest *mreq);
   void readMissHandler(MemRequest *mreq);
   void doReadQueued(MemRequest *mreq);
   void activateOverflow(MemRequest *mreq);
   void sendMiss(MemRequest *mreq);

   void returnAccess(MemRequest *mreq);
   void preReturnAccess(MemRequest *mreq);
   void doReturnAccess(MemRequest *mreq);

   MemRequest * completeRequest(MemRequest *mreq);
   bool canIssue(MemRequest *mreq);
   void retire(MemRequest *mreq);

   typedef CallbackMember1<IntrlvdCache, MemRequest *, &IntrlvdCache::doRead> doReadCB;

   typedef CallbackMember1<IntrlvdCache, MemRequest *, &IntrlvdCache::doReadQueued> doReadQueuedCB;

   typedef CallbackMember1<IntrlvdCache, MemRequest *, &IntrlvdCache::activateOverflow> activateOverflowCB;

  typedef CallbackMember1<IntrlvdCache, MemRequest *,
                         &IntrlvdCache::doReturnAccess> doReturnAccessCB;

  typedef CallbackMember1<IntrlvdCache, MemRequest *,
                         &IntrlvdCache::preReturnAccess> preReturnAccessCB;
};
#endif
