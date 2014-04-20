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

#ifndef INTRLVDMEMREQUEST_H
#define INTRLVDMEMREQUEST_H

#include "pool.h"
#include "MemRequest.h"

class IntrlvdMemRequest : public MemRequest {
private:

  static pool<IntrlvdMemRequest> rPool;
  friend class pool<IntrlvdMemRequest>;

protected:

  MemRequest *oreq;

public:
  IntrlvdMemRequest(); 
  ~IntrlvdMemRequest(){
  }

  // BEGIN: MemRequest interface

  static IntrlvdMemRequest *create(MemRequest *mreq, PAddr newAddr, MemObj *memObj);

  void destroy();

  VAddr getVaddr() const;
  PAddr getPAddr() const {
    return pAddr;
  }

  void  ack(TimeDelta_t lat);

  // END: MemRequest interface

  MemRequest  *getOriginalRequest() {
    return oreq;
  }
  MemOperation getMemOperation() {
    return memOp;
  }
};

class IntrlvdMemReqHashFunc {
public: 
  size_t operator()(const MemRequest *mreq) const {
    HASH<const char *> H;
    return H((const char *)mreq);
  }
};

#endif // INTRLVDMEMREQUEST_H
