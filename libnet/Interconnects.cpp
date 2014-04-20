#include "Interconnects.h"
#include "SescConf.h"
#include "ClusterArchDebug.h"

InterConnection * regNet=0;
InterConnection * ldstqNet=0;

void initInterconnects() {
  
  const char *regNetName   = SescConf->getCharPtr("","regNet");
  const char *ldstqNetName = SescConf->getCharPtr("","ldstqNet");

  GLOG(INIT_NETS_DEBUG, "initInterconnects: Nets initialized. regNet created, ldstqNet created.");
  if(regNet) {
     I(ldstqNet);
     RegNetProtocol::destroyProtocols();
     LDSTQNetProtocol::destroyProtocols();  
     delete regNet;
     delete ldstqNet;
  }
  regNet   = new InterConnection(regNetName);
  ldstqNet = new InterConnection(ldstqNetName);
 
  //should be the same for ldstqNet
  unsigned int nRouters = regNet->getnRouters();
  I(nRouters == ldstqNet->getnRouters());

  for(unsigned int i=0 ; i<nRouters ; i++ ) {
    RegNetProtocol   *p   = new RegNetProtocol(regNet, i);
    LDSTQNetProtocol *lsp = new LDSTQNetProtocol(ldstqNet,i);
  } 
}

//RegNetProtocol ~~~~~~~~~~~~~~~~~~~~~~~~~
std::vector<RegNetProtocol *> RegNetProtocol::protocols;

RegNetProtocol::RegNetProtocol(InterConnection *net, RouterID_t rID)
  : ProtocolBase(net, rID)
{
  ProtocolCBBase *pcb = new ProtocolCB<RegNetProtocol,&RegNetProtocol::copyMsgHandler>(this);
  registerHandler(pcb,CopyMsg);

  protocols.push_back(this);
}

void RegNetProtocol::destroyProtocols()
{
  unsigned int size = protocols.size();
  for(unsigned int i=0 ; i<size ; i++ )
    delete protocols[i];

  protocols.clear();
}

void RegNetProtocol::copyMsgHandler(Message *msg) {
  GLOG(COPY_DEBUG,"RegNet::copyMsgHandler: Copy message processed @%lld\n",globalClock);

  RegNetMsg *regMsg = static_cast<RegNetMsg *>(msg);

  DInst *dinst = regMsg->getDInst();
  dinst->getResource()->executed(dinst);

  regMsg->garbageCollect();
}

//LDSTQNetProtocol ~~~~~~~~~~~~~~~~~~~~~~~~~
std::vector<LDSTQNetProtocol *> LDSTQNetProtocol::protocols;

LDSTQNetProtocol::LDSTQNetProtocol(InterConnection *net, RouterID_t rID)
  : ProtocolBase(net, rID)
{
  ProtocolCBBase *pcb = new ProtocolCB<LDSTQNetProtocol,&LDSTQNetProtocol::LookUpMsgHandler>(this);
  registerHandler(pcb,LookUpMsg);

  pcb = new ProtocolCB<LDSTQNetProtocol,&LDSTQNetProtocol::RetLookUpMsgHandler>(this);
  registerHandler(pcb,RetLookUpMsg);

  protocols.push_back(this);
}

void LDSTQNetProtocol::destroyProtocols()
{
  unsigned int size = protocols.size();
  for(unsigned int i=0 ; i<size ; i++ ) 
    delete protocols[i];
 
  protocols.clear();
}

void LDSTQNetProtocol::LookUpMsgHandler(Message *msg) {
  GLOG(0,"LDSTQNet::LookUpMsgHandler: LookUp message processed @%lld\n",globalClock);

  LDSTQNetMsg *ldstqMsg = static_cast<LDSTQNetMsg *>(msg);
  DInst * dinst = ldstqMsg->getDInst();
 
  I(dinst);

  FULoad * loadRes = static_cast<FULoad *>(dinst->getLSQResource());
  
  if(dinst->getInst()->isLoad())
    loadRes->completeLookUp(dinst, 0); //NM bankNo 0 aslinda yanlis, bu kullanilmamali
  else
    loadRes->completeLookUp(dinst, 0); //NM bankNo 0 aslinda yanlis, bu kullanilmamali

  ldstqMsg->garbageCollect();
}

void LDSTQNetProtocol::RetLookUpMsgHandler(Message *msg) {
  GLOG(0,"LDSTQNet::RetLookUpMsgHandler: RetLookUp message processed @%lld\n",globalClock);

  LDSTQNetMsg *ldstqMsg = static_cast<LDSTQNetMsg *>(msg);
  DInst * dinst = ldstqMsg->getDInst();

  I(dinst);
  I(dinst->getInst()->isLoad());

  dinst->doAtExecuted();
  
  ldstqMsg->garbageCollect();
}
