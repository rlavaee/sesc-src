 
/*
 * This launches the whole SESC simulator environment
 */

#include <stdlib.h>
#include <vector>

#include "nanassert.h"

#include "SMTProcessor.h"
#include "Processor.h"

#include "MemorySystem.h"

#include "OSSim.h"
#include "SescConf.h"

#ifdef TASKSCALAR
#include "TaskHandler.h"
#endif

int main(int argc, char **argv, char **envp)
{ 
#ifdef TASKSCALAR
  taskHandler = new TaskHandler();
#endif
  osSim = new OSSim(argc, argv, envp);

  int nProcs = SescConf->getRecordSize("","cpucore");

  std::vector<GMemorySystem *> ms(nProcs);
  std::vector<GProcessor *>    pr(nProcs);

  for(int i = 0; i < nProcs; i ++) {
    GMemorySystem *gms = new MemorySystem(i);
    gms->buildMemorySystem();
    ms[i] = gms;
    if(SescConf->checkLong("cpucore","smtContexts",i)) {
      if( SescConf->getLong("cpucore","smtContexts",i) > 1 )
	pr[i] =new SMTProcessor(ms[i], i);
    }
    if (pr[i] == 0)
      pr[i] =new Processor(ms[i], i);
  }

  osSim->boot();

  // Reaches this point only when all the active threads have finished.
  
  for(int i = 0; i < nProcs; i ++) {
    GProcessor *gp = pr[i];
    delete gp;
    delete ms[i];
  }

  delete osSim;

  return 0;
}
