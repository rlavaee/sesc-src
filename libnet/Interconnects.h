#ifndef INTERCONNECTS_H
#define INTERCONNECTS_H

#include <vector>
#include <stdlib.h>
#include "ProtocolBase.h"
#include "InterConn.h"
#include "Message.h"

class DInst;

class RegNetProtocol : public ProtocolBase {

  static std::vector<RegNetProtocol *> protocols;
public:
  RegNetProtocol(InterConnection *net, RouterID_t rID);

  static void destroyProtocols();

  void copyMsgHandler(Message *msg);

  static ProtocolBase * getProtocol(RouterID_t rId) {
    I(rId < protocols.size());
    return protocols[rId];
  }
};

class RegNetMsg:public PMessage {
public:
  static RegNetMsg * createMsg(MessageType msgType, RouterID_t srcRid, RouterID_t dstRid, DInst *dinst) {

    ProtocolBase *srcPB = RegNetProtocol::getProtocol(srcRid);
    ProtocolBase *dstPB = RegNetProtocol::getProtocol(dstRid);

    return static_cast<RegNetMsg *>(PMessage::createMsg(msgType,srcPB,dstPB,0,dinst));
  }
};

class LDSTQNetProtocol : public ProtocolBase {

  static std::vector<LDSTQNetProtocol *> protocols;

public:
  LDSTQNetProtocol(InterConnection *net, RouterID_t rID);  

  static void destroyProtocols();

  void LookUpMsgHandler(Message *msg);
  void RetLookUpMsgHandler(Message * msg);

  static ProtocolBase * getProtocol(RouterID_t rId) {
    I(rId < protocols.size());
    return protocols[rId];
  }
};

class LDSTQNetMsg:public PMessage {
public:
  static LDSTQNetMsg * createMsg(MessageType msgType, RouterID_t srcRid, RouterID_t dstRid, DInst *dinst) {

    ProtocolBase *srcPB = LDSTQNetProtocol::getProtocol(srcRid);
    ProtocolBase *dstPB = LDSTQNetProtocol::getProtocol(dstRid);

    LDSTQNetMsg * msg = static_cast<LDSTQNetMsg *>(PMessage::createMsg(msgType,srcPB,dstPB,0,dinst)); 

    return msg;
  }
};

extern InterConnection * regNet;
extern InterConnection * ldstqNet;
extern void initInterconnects();
#endif //INTERCONNECTS_H
