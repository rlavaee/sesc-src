#ifndef SESCOMPAPI_H
#define SESCOMPAPI_H

#include "sescapi.h"

#define fireOffProcs(LoopID)\
    Global->id = 0;\
    for (i=1; i<numthreads; i++) {\
    sesc_spawn(LoopID, NULL, 0);\
    }\
    LoopID();

#define getID(ProcID)\
    {sesc_lock(&(Global->idlock));};\
        ProcID = Global->id - 1;\
    {sesc_unlock(&(Global->idlock));};

#define loopInit(ProcID, TotalThreads, TotalData)\
    {sesc_lock(&(Global->idlock));};\
        ProcID = Global->id;\
        Global->id++;\
    {sesc_unlock(&(Global->idlock));};\
    {sesc_barrier(&(Global->start), TotalThreads);};\
    if (ProcID == 0) {\
        my_start = 0;\ 
        my_stop = (int)(TotalData/TotalThreads) + TotalData%TotalThreads;\
    } else {\
        my_start = ProcID*((int)(TotalData/TotalThreads)) + TotalData%TotalThreads;\
        my_stop = (ProcID+1)*((int)(TotalData/TotalThreads)) + TotalData%TotalThreads;\
    }

#define loopTerm(ProcID)\
    if (ProcID != 0) {\
        sesc_exit(0);\
    }

#endif /* SESCOMPAPI_H */
