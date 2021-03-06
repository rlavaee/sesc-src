/* 
   SESC: Super ESCalar simulator
   Copyright (C) 2004 University of Illinois.

   Contributed by Jose Renau
                  Luis Ceze

   This file is part of SESC.

   SESC is free software; you can redistribute it and/or modify it under the terms
   of the GNU General Public License as published by the Free Software Foundation;
   either version 2, or (at your option) any later version.

   SESC is    distributed in the  hope that  it will  be  useful, but  WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
   PARTICULAR PURPOSE.  See the GNU General Public License for more details.21

   You should  have received a copy of  the GNU General  Public License along with
   SESC; see the file COPYING.  If not, write to the  Free Software Foundation, 59
   Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#ifndef MSHR_H
#define MSHR_H

#include <queue>

#include "MemRequest.h"
#include "callback.h"
#include "nanassert.h"
#include "GStats.h"
#include "pool.h"

#include "BloomFilter.h"

#include <iostream>

// This is an extremly fast MSHR implementation.  Given a number of
// outstanding MSHR entries, it does a hash function to find a random
// entry (always the same for a given address).
//
// This can have a little more contention than a fully associative
// structure because it can serialize requests, but it should not
// affect performance much.

enum MSHRAllocPolicy { CONVENTIONAL=0, SPECIAL };

template<class Addr_t>
class OverflowFieldT {
 public:
  Addr_t paddr;
  CallbackBase *cb;
  CallbackBase *ovflwcb;
  MemOperation mo;
};

template<class Addr_t, class Cache_t> class MSHR;

template<class Addr_t, class Cache_t>
  class MSHRStats {
 protected:
  //	HASH_MAP< MSHR<Addr_t, Cache_t> *, int > entries;
  HASH_MAP< long , int > entries;
  int totalEntries;
  int outsRdReqs;

  GStatsTimingHist occupancyHistogram;

  GStatsEventTimingHist rdEntriesOcc;
  GStatsEventTimingHist wrEntriesOcc;
  GStatsTimingHist rdReqsOcc;

 public:
  GStatsHist subEntriesHist;      
  GStatsHist subEntriesReadsHist;      
  GStatsHist subEntriesWritesHist;      
  GStatsHist subEntriesWritesHistComb;      
  GStatsHist subEntriesHistComb;      
  GStatsAvg avgReadSubentriesL2Hit;
  GStatsAvg avgReadSubentriesL2Miss;

  MSHRStats(const char *name) 
    :totalEntries(0)
    ,outsRdReqs(0)
    ,occupancyHistogram("%s_MSHR_occupancy",name)
    ,rdEntriesOcc("%s_MSHR_rdOccupancy",name)
    ,wrEntriesOcc("%s_MSHR_wrOccupancy",name)
    ,rdReqsOcc("%s_MSHR_rdReqsOcc",name)
    ,subEntriesHist("%s_MSHR_usedSubEntries", name)
    ,subEntriesReadsHist("%s_MSHR_usedSubEntriesReads", name)
    ,subEntriesWritesHist("%s_MSHR_usedSubEntriesWrites", name)
    ,subEntriesWritesHistComb("%s_MSHR_usedSubEntriesWritesComb", name)
    ,subEntriesHistComb("%s_MSHR_usedSubEntriesComb", name)
    ,avgReadSubentriesL2Hit("%s_MSHR_avgReadSubentriesL2Hit", name)
    ,avgReadSubentriesL2Miss("%s_MSHR_avgReadSubentriesL2Miss", name)
  {}
	
  void attach( MSHR<Addr_t, Cache_t> *mshr) {
    entries[(long)mshr] = 0;
  }

  void sampleEntryOcc( MSHR<Addr_t, Cache_t> *mshr, int numEntries ) {
    int oldNum = entries[(long)mshr];
    totalEntries += numEntries - oldNum;
    I(totalEntries >= 0);
    entries[(long)mshr] = numEntries;
    occupancyHistogram.sample(totalEntries);
  }

  void sampleEntry( unsigned long long id ) {
    rdEntriesOcc.begin_sample(id);
    wrEntriesOcc.begin_sample(id);
  }

  void retireRdEntry( unsigned long long id ) {
    rdEntriesOcc.commit_sample(id); 
    wrEntriesOcc.remove_sample(id); 
  }
  void retireWrEntry( unsigned long long id ) {
    rdEntriesOcc.remove_sample(id); 
    wrEntriesOcc.commit_sample(id);
  }
  void retireRdWrEntry( unsigned long long id ){
    rdEntriesOcc.commit_sample(id); 
    wrEntriesOcc.commit_sample(id);
  }
  
  void incRdReqs(int nr = 1) {
    outsRdReqs += nr;
    rdReqsOcc.sample(outsRdReqs);
  }

  void decRdReqs(int nr = 1) {
    outsRdReqs -= nr;
    I(outsRdReqs >= 0);
    rdReqsOcc.sample(outsRdReqs);
  }
};

template<class Addr_t> class MSHRentry;

//
// base MSHR class
//

template<class Addr_t,class Cache_t>
  class MSHR {
 private:
 protected:
  const int nEntries;
  int nFreeEntries; 

  const size_t Log2LineSize;

  GStatsCntr nUse;
  GStatsCntr nUseReads;
  GStatsCntr nUseWrites;
  GStatsCntr nOverflows;
  GStatsMax  maxUsedEntries;
  GStatsCntr nCanAccept;
  GStatsCntr nCanNotAccept;
  GStatsCntr nCanNotAcceptConv;
  GStatsTimingHist blockingCycles;

  int allocPolicy;  

  MSHRStats<Addr_t, Cache_t> *occStats;
  bool occStatsAttached;
  
  Cache_t *lowerCache;

 public:
  static MSHR<Addr_t,Cache_t> *create(const char *name, const char *type, 
				      int size, int lineSize, int nse = 16, 
				      int nwr = 16, int aPolicy=SPECIAL);

  static MSHR<Addr_t,Cache_t> *create(const char *name, const char *section);

  static MSHR<Addr_t,Cache_t> *attach(const char *name, 
				      const char *section, 
				      MSHR<Addr_t,Cache_t> *mshr);

 public:
  virtual ~MSHR() { 
    if(!occStatsAttached) delete occStats;
  }
  MSHR(const char *name, int size, int lineSize, int aPolicy);

  void destroy() {
    delete this;
  }

  virtual void attach( MSHR<Addr_t, Cache_t> *mshr ); 
  bool canAcceptRequest(Addr_t paddr, MemOperation mo = MemRead);
  virtual bool isOnlyWrites(Addr_t paddr) { return false; }

  // All derived classes must implement this interface
  
  virtual bool canAcceptRequestSpecial(Addr_t paddr, MemOperation mo = MemRead) = 0;

  virtual bool issue(Addr_t paddr, MemOperation mo = MemRead) = 0;
  virtual void addEntry(Addr_t paddr, CallbackBase *c, 
			CallbackBase *ovflwc = 0, MemOperation mo = MemRead) = 0;
  virtual bool retire(Addr_t paddr) = 0;

  //This function is address independent, and implements conventional
  //assumption that a cache cannot accept a request unless it can be
  //guaranteed cache resources
  virtual bool canAllocateEntry() = 0;
  virtual bool readSEntryFull() { return false; }
  virtual bool writeSEntryFull() { return false; }

  Addr_t calcLineAddr(Addr_t paddr) const { return paddr >> Log2LineSize; }

  int getnEntries() const { return nEntries; }

  virtual int  getnReads() const { return 1; }
  virtual int  getnWrites() const { return 1; }
  
  virtual int  getUsedReads(Addr_t paddr) { return 0; }
  virtual int  getUsedWrites(Addr_t paddr) { return 0; }

  size_t getLineSize() const { return 1 << Log2LineSize; }

  virtual void setLowerCache(Cache_t *lCache) { lowerCache = lCache; } 


  bool hasEntry(Addr_t paddr) { return hasLineReq(calcLineAddr(paddr)); }

  virtual bool hasLineReq(Addr_t paddr) = 0; //Expects a line address only


  // Must pass address because some MSHRs not fully associative and must
  // choose from the correct set for displacement
  virtual MSHRentry<Addr_t> *selectEntryToDrop(Addr_t paddr);
  virtual void dropEntry(Addr_t lineAddr);
  virtual MSHRentry<Addr_t> *getEntry(Addr_t paddr);
  virtual void putEntry(MSHRentry<Addr_t> &me);

  // Statistics generation
  void updateOccHistogram(); 


  // debugging methods
  virtual bool isOverflowing() { return false; }
 
};

//
// MSHR that lets everything go
//
template<class Addr_t, class Cache_t>
  class NoMSHR : public MSHR<Addr_t, Cache_t> {
    using MSHR<Addr_t, Cache_t>::nEntries;
    using MSHR<Addr_t, Cache_t>::nFreeEntries; 
    using MSHR<Addr_t, Cache_t>::Log2LineSize;
    using MSHR<Addr_t, Cache_t>::nUse;
    using MSHR<Addr_t, Cache_t>::nUseReads;
    using MSHR<Addr_t, Cache_t>::nUseWrites;
    using MSHR<Addr_t, Cache_t>::nOverflows;
    using MSHR<Addr_t, Cache_t>::maxUsedEntries;
    using MSHR<Addr_t, Cache_t>::nCanAccept;
    using MSHR<Addr_t, Cache_t>::nCanNotAccept;
    using MSHR<Addr_t, Cache_t>::updateOccHistogram;      
    using MSHR<Addr_t, Cache_t>::lowerCache;

    
 protected:
    friend class MSHR<Addr_t, Cache_t>;
    NoMSHR(const char *name, int size, int lineSize, int aPolicy);

 public:
    virtual ~NoMSHR() { }
    bool canAcceptRequestSpecial(Addr_t paddr, MemOperation mo = MemRead) { return true; }

    bool issue(Addr_t paddr, MemOperation mo = MemRead) { nUse.inc(); return true; }

    void addEntry(Addr_t paddr, CallbackBase *c, 
		  CallbackBase *ovflwc = 0, MemOperation mo = MemRead) { I(0); }

    bool retire(Addr_t paddr) { return true; }

    bool canAllocateEntry() { return true; }

 protected:

    bool hasLineReq(Addr_t paddr) { return false; }
  };

//
// MSHR that just queues the reqs, does NOT enforce address dependencies
//
template<class Addr_t, class Cache_t>
  class NoDepsMSHR : public MSHR<Addr_t, Cache_t> {
    using MSHR<Addr_t, Cache_t>::nEntries;
    using MSHR<Addr_t, Cache_t>::nFreeEntries; 
    using MSHR<Addr_t, Cache_t>::Log2LineSize;
    using MSHR<Addr_t, Cache_t>::nUse;
    using MSHR<Addr_t, Cache_t>::nUseReads;
    using MSHR<Addr_t, Cache_t>::nUseWrites;
    using MSHR<Addr_t, Cache_t>::nOverflows;
    using MSHR<Addr_t, Cache_t>::maxUsedEntries;
    using MSHR<Addr_t, Cache_t>::nCanAccept;
    using MSHR<Addr_t, Cache_t>::nCanNotAccept;
    using MSHR<Addr_t, Cache_t>::updateOccHistogram;      
    using MSHR<Addr_t, Cache_t>::lowerCache;

 private:  
    typedef OverflowFieldT<Addr_t> OverflowField;
    typedef std::deque<OverflowField> Overflow;
    Overflow overflow;
 protected:
    friend class MSHR<Addr_t, Cache_t>;
    NoDepsMSHR(const char *name, int size, int lineSize, int aPolicy);
  
 public:
    virtual ~NoDepsMSHR() { }

    bool canAcceptRequestSpecial(Addr_t paddr, MemOperation mo = MemRead) { 
      return (nFreeEntries > 0); 
    }

    bool issue(Addr_t paddr, MemOperation mo = MemRead); 

    void addEntry(Addr_t paddr, CallbackBase *c, 
		  CallbackBase *ovflwc = 0, MemOperation mo = MemRead);

    bool retire(Addr_t paddr);

    bool canAllocateEntry() { return nFreeEntries > 0; }

    bool hasLineReq(Addr_t paddr);
  };

//
// regular full MSHR, address deps enforcement and overflowing capabilities
//

template<class Addr_t, class Cache_t>
class FullMSHR : public MSHR<Addr_t, Cache_t> {
    using MSHR<Addr_t, Cache_t>::nEntries;
    using MSHR<Addr_t, Cache_t>::nFreeEntries; 
    using MSHR<Addr_t, Cache_t>::Log2LineSize;
    using MSHR<Addr_t, Cache_t>::nUse;
    using MSHR<Addr_t, Cache_t>::nUseReads;
    using MSHR<Addr_t, Cache_t>::nUseWrites;
    using MSHR<Addr_t, Cache_t>::nOverflows;
    using MSHR<Addr_t, Cache_t>::maxUsedEntries;
    using MSHR<Addr_t, Cache_t>::nCanAccept;
    using MSHR<Addr_t, Cache_t>::nCanNotAccept;
    using MSHR<Addr_t, Cache_t>::updateOccHistogram;      
    using MSHR<Addr_t, Cache_t>::lowerCache;

 private:
    GStatsCntr nStallConflict;

    const int    MSHRSize;
    const int    MSHRMask;

    bool overflowing;

    // If a non-integer type is defined, the MSHR template should accept
    // a hash function as a parameter

    // Running crafty, I verified that the current hash function
    // performs pretty well (due to the extra space on MSHRSize). It
    // performs only 5% worse than an oversize prime number (noise).
    int calcEntry(Addr_t paddr) const {
      ulong p = paddr >> Log2LineSize;
      return (p ^ (p>>11)) & MSHRMask;
    }

    class EntryType {
		     public:
      CallbackContainer cc;
      bool inUse;
    };

    EntryType *entry;

    typedef OverflowFieldT<Addr_t> OverflowField;
    typedef std::deque<OverflowField> Overflow;
    Overflow overflow;

 protected:
    friend class MSHR<Addr_t, Cache_t>;
    FullMSHR(const char *name, int size, int lineSize, int aPolicy);

 public:
    virtual ~FullMSHR() { delete entry; }

    bool canAcceptRequestSpecial(Addr_t paddr, MemOperation mo = MemRead) { 
      return (nFreeEntries>0); 
    }

    bool issue(Addr_t paddr, MemOperation mo = MemRead);

    void addEntry(Addr_t paddr, CallbackBase *c, 
		  CallbackBase *ovflwc = 0, MemOperation mo = MemRead);

    bool retire(Addr_t paddr);


    bool canAllocateEntry() { return (nFreeEntries>0); } 
  
    void setLowerCache(Cache_t *lCache) { lowerCache = lCache; }

    bool hasLineReq(Addr_t paddr);
  };


template<class Addr_t> 
class MSHRentry {
 private:
  int nReads;
  int nFreeReads;
  int nWrites;
  bool rdWrSharing;
  int nFreeWrites;
  int nFreeSEntries;  
  CallbackContainer cc;
  Addr_t reqLineAddr;
  bool displaced;
  Time_t whenAllocated;

  bool l2Hit;

  HASH_SET<PAddr> writeSet;
  
 public:
  MSHRentry() {
    reqLineAddr = 0;
    nFreeSEntries = 0; 
    nReads = 0;
    nWrites = 0;
    rdWrSharing = false;
    displaced = false;
    whenAllocated = globalClock;
    l2Hit = false;
    writeSet.clear();
   }

  ~MSHRentry() {
    if(displaced)
      cc.makeEmpty();
  }

  PAddr getLineAddr() { return reqLineAddr; }

  bool isRdWrSharing() { return rdWrSharing; }

  bool addRequest(Addr_t reqAddr, CallbackBase *rcb, MemOperation mo);
  void firstRequest(Addr_t addr, Addr_t lineAddr, int nrd, int nwr, MemOperation mo) { 
    I(nFreeSEntries == 0);
 
    nReads = nrd;

    if(nwr == 0) { // reads and writes share subentries
      nWrites = nrd;
      rdWrSharing = true;
    } else {
      nWrites = nwr;
      rdWrSharing = false;
    }

    nFreeReads = nReads;
    nFreeWrites = nWrites;

    if(nwr == 0)
      nFreeSEntries = nrd - 1;
    else
      nFreeSEntries = nReads + nWrites - 1;
 
    if(mo == MemWrite) {
      nFreeWrites--;
      writeSet.insert(addr & 0xfffffffc);
    } else {
      I(mo == MemRead);
      nFreeReads--;   
    }   
    
    reqLineAddr = lineAddr;
  }

  bool canAcceptRequest(MemOperation mo) const { 
    if(nFreeSEntries == 0)
      return false;

    if(mo == MemWrite)
      return (nFreeWrites > 0);
    
    return (nFreeReads > 0);	
  }

  bool retire();

  int getUsedReads()     const { return (nReads - nFreeReads); }
  int hasFreeReads()     const { return (nFreeSEntries > 0 && nFreeReads > 0); }

  int getNWrittenWords() const { return writeSet.size(); }
  int getUsedWrites()    const { return (nWrites - nFreeWrites); }
  bool hasFreeWrites()   const { return (nFreeSEntries > 0 && nFreeWrites > 0); }

  int getPendingReqs()  const { return nReads+nWrites-nFreeSEntries; }

  void adjustParameters(int nrd, int nwr) {
    // the new entry also shares read/write subentries
    bool rdWrSharingNewEntry = (nwr == 0);

    if(!rdWrSharing && !rdWrSharingNewEntry) {
      // none of the MSHR share subentries
      nFreeSEntries += (nrd - nReads) + (nwr - nWrites);
      
      nFreeReads += (nrd - nReads);
      nReads = nrd;
      
      nFreeWrites += (nwr - nWrites);
      nWrites = nwr;
    } else if (rdWrSharing && !rdWrSharingNewEntry) {
      // the source MSHR shared and the destination does not share
      I(nwr != 0);
      int nUsedWrites = (nWrites - nFreeWrites);
      int nUsedReads = (nReads - nFreeReads);

      nFreeSEntries = (nrd - nUsedReads) + (nwr - nUsedWrites);
      
      nFreeReads = (nrd - nUsedReads);
      nReads = nrd;
      
      nFreeWrites = (nwr - nUsedWrites);
      nWrites = nwr;

      rdWrSharing = false;
    } else {
      MSG("not implemented yet");
      exit(-1);
    }
  }
  
  void displace() { displaced = true; }

  void setL2Hit(bool lh) { l2Hit = lh; }
  bool isL2Hit() const { return l2Hit; }

  Time_t getWhenAllocated() { return whenAllocated; }
};

template<class Addr_t, class Cache_t> class HrMSHR;

//
// SingleMSHR
//

template<class Addr_t, class Cache_t>
  class SingleMSHR : public MSHR<Addr_t, Cache_t> {
    using MSHR<Addr_t, Cache_t>::nEntries;
    using MSHR<Addr_t, Cache_t>::nFreeEntries; 
    using MSHR<Addr_t, Cache_t>::Log2LineSize;
    using MSHR<Addr_t, Cache_t>::nUse;
    using MSHR<Addr_t, Cache_t>::nUseReads;
    using MSHR<Addr_t, Cache_t>::nUseWrites;
    using MSHR<Addr_t, Cache_t>::nOverflows;
    using MSHR<Addr_t, Cache_t>::maxUsedEntries;
    using MSHR<Addr_t, Cache_t>::nCanAccept;
    using MSHR<Addr_t, Cache_t>::nCanNotAccept;
    using MSHR<Addr_t, Cache_t>::updateOccHistogram;      
    using MSHR<Addr_t, Cache_t>::lowerCache;
    using MSHR<Addr_t, Cache_t>::occStats;

 private:  
    const int nReads;
    const int nWrites;
    int nOutsReqs;
    BloomFilter bf;

    bool checkingOverflow;

    int nFullReadEntries;
    int nFullWriteEntries;

    typedef OverflowFieldT<Addr_t> OverflowField;
    typedef std::deque<OverflowField> Overflow;
    Overflow overflow;

    typedef HASH_MAP<Addr_t, MSHRentry<Addr_t> > MSHRstruct;
    typedef typename MSHRstruct::iterator MSHRit;
    typedef typename MSHRstruct::const_iterator const_MSHRit;
  
    MSHRstruct ms;
    GStatsAvg avgOverflowConsumptions;
    GStatsMax maxOutsReqs;
    GStatsAvg avgReqsPerLine;
    GStatsCntr nIssuesNewEntry;
    GStatsCntr nCanNotAcceptSubEntryFull;
    GStatsCntr nCanNotAcceptTooManyWrites;
    GStatsAvg  avgQueueSize;
    GStatsAvg  avgWritesPerLine;
    GStatsAvg  avgWritesPerLineComb;
    GStatsCntr nOnlyWrites;
    GStatsCntr nRetiredEntries;
    GStatsCntr nRetiredEntriesWritten;

 protected:
    friend class MSHR<Addr_t, Cache_t>;
    friend class HrMSHR<Addr_t, Cache_t>;
  
    void toOverflow(Addr_t paddr, CallbackBase *c, CallbackBase *ovflwc, 
		    MemOperation mo);

    void checkOverflow();

    void checkSubEntries(Addr_t paddr, MemOperation mo);

 public:
    SingleMSHR(const char *name, int size, int lineSize, 
	       int nrd = 16, int nwr = 16, int aPolicy = SPECIAL);

    virtual ~SingleMSHR() { }

    bool canAcceptRequestSpecial(Addr_t paddr, MemOperation mo = MemRead);
    bool isOnlyWrites(Addr_t paddr);

    bool issue(Addr_t paddr, MemOperation mo = MemRead); 

    void addEntry(Addr_t paddr, CallbackBase *c, 
		  CallbackBase *ovflwc = 0, MemOperation mo = MemRead);

    bool retire(Addr_t paddr);

    int getnReads() const { return nReads; }
    int getnWrites() const { return nWrites; }

    int getUsedReads(Addr_t paddr) {
      MSHRit it = ms.find(calcLineAddr(paddr));
      I(it != ms.end());
      
      return (*it).second.getUsedReads();
    }

    int getUsedWrites(Addr_t paddr) {
      MSHRit it = ms.find(calcLineAddr(paddr));
      I(it != ms.end());
      
      return (*it).second.getUsedWrites();
    }

    bool canAllocateEntry(); 
    bool readSEntryFull(); 
    bool writeSEntryFull();

    void setLowerCache(Cache_t *lCache) { lowerCache = lCache; }
    bool hasLineReq(Addr_t paddr)  { return (ms.find(paddr) != ms.end()); }

    MSHRentry<Addr_t> *selectEntryToDrop(Addr_t paddr);
    void dropEntry(Addr_t lineAddr);
    virtual MSHRentry<Addr_t> *getEntry(Addr_t paddr);
    void putEntry(MSHRentry<Addr_t> &me);

    bool isOverflowing() { return (overflow.size() > 0); }
  };

//
// BankedMSHR
//

template<class Addr_t, class Cache_t>
  class BankedMSHR : public MSHR<Addr_t, Cache_t> {
    using MSHR<Addr_t, Cache_t>::nEntries;
    using MSHR<Addr_t, Cache_t>::nFreeEntries; 
    using MSHR<Addr_t, Cache_t>::Log2LineSize;
    using MSHR<Addr_t, Cache_t>::nUse;
    using MSHR<Addr_t, Cache_t>::nUseReads;
    using MSHR<Addr_t, Cache_t>::nUseWrites;
    using MSHR<Addr_t, Cache_t>::nOverflows;
    using MSHR<Addr_t, Cache_t>::maxUsedEntries;
    using MSHR<Addr_t, Cache_t>::nCanAccept;
    using MSHR<Addr_t, Cache_t>::nCanNotAccept;
    using MSHR<Addr_t, Cache_t>::updateOccHistogram;      
    using MSHR<Addr_t, Cache_t>::lowerCache;

 private:  
    const int nBanks;
    SingleMSHR<Addr_t, Cache_t> **mshrBank;

    GStatsMax maxOutsReqs;
    GStatsAvg avgOverflowConsumptions;

    int nOutsReqs;
    bool checkingOverflow;
  
    int calcBankIndex(Addr_t paddr) const { 
      paddr = calcLineAddr(paddr);
      Addr_t idx = (paddr >> (16 - Log2LineSize/2)) ^ (paddr & 0x0000ffff);
      return  idx % nBanks;  // TODO:move to a mask
    }

    typedef OverflowFieldT<Addr_t> OverflowField;
    typedef std::deque<OverflowField> Overflow;
    Overflow overflow;

    void toOverflow(Addr_t paddr, CallbackBase *c, CallbackBase *ovflwc, 
		    MemOperation mo);

    void checkOverflow();
  
 protected:
    friend class MSHR<Addr_t,Cache_t>;
    BankedMSHR(const char *name, int size, int lineSize, int nb, 
	       int nrd = 16, int nwr = 16, int aPolicy = SPECIAL);
  
 public:
    virtual ~BankedMSHR() { }

    bool canAcceptRequestSpecial(Addr_t paddr, MemOperation mo = MemRead);

    bool isOnlyWrites(Addr_t paddr) { 
      return mshrBank[calcBankIndex(paddr)]->isOnlyWrites(paddr);
    }

    bool issue(Addr_t paddr, MemOperation mo = MemRead); 

    void addEntry(Addr_t paddr, CallbackBase *c, 
		  CallbackBase *ovflwc = 0, MemOperation mo = MemRead);

    bool retire(Addr_t paddr);

    void attach( MSHR<Addr_t, Cache_t> *mshr ); 

    void setLowerCache(Cache_t *lCache);

    int getnReads() const { I(nBanks); return mshrBank[0]->getnReads(); }
    int getnWrites() const { I(nBanks); return mshrBank[0]->getnWrites(); }

    int getUsedReads(Addr_t paddr) {
      return mshrBank[calcBankIndex(paddr)]->getUsedReads(paddr);
    }

    int getUsedWrites(Addr_t paddr) {
      return mshrBank[calcBankIndex(paddr)]->getUsedWrites(paddr);
    }

 public:

    bool canAllocateEntry();
    bool readSEntryFull(); 
    bool writeSEntryFull();

    bool hasLineReq(Addr_t paddr);
    MSHRentry<Addr_t> *selectEntryToDrop(Addr_t paddr);
    void dropEntry(Addr_t lineAddr);

    MSHRentry<Addr_t> *getEntry(Addr_t paddr);
    void putEntry(MSHRentry<Addr_t> &me);

    bool isOverflowing() { return (overflow.size() > 0); }
  };


#ifndef MSHR_CPP
#include "MSHR.cpp"
#endif
#endif // MSHR_H
