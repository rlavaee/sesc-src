//Features used as input to RL
enum FEATURE_TYPE {

  //Waiting time of requestor
  REQUESTOR_WAITING_TIME = 0,

  //Number of cas hits to local open page since last activation
  TOTAL_CAS_HITS_LOCAL_OPEN = 1,
 
 //Total number of reads to referenced page
  TOTAL_READS_REFERENCED = 2, 
  //Total number of reads to all open pages
  TOTAL_READS_GLOBAL_OPEN = 3,
  //Total number of reads to all pages
  TOTAL_READS_GLOBAL = 4,

  //Total number of writes to referenced page
  TOTAL_WRITES_REFERENCED = 5,
  //Total number of writes to all open pages
  TOTAL_WRITES_GLOBAL_OPEN = 6,
  //Total number of writes to all pages
  TOTAL_WRITES_GLOBAL = 7,  
  
  //Total number of load misses to referenced page
  TOTAL_LD_MISSES_REFERENCED = 8,
  //Total number of load misses to all open pages
  TOTAL_LD_MISSES_GLOBAL_OPEN = 9,
  //Total number of load misses to all pages
  TOTAL_LD_MISSES_GLOBAL = 10,

  //Requestor's ROB position
  REQUESTOR_ROB_POS = 11,

  //Total number of ROB heads waiting for the referenced page
  TOTAL_ROB_HEADS_REFERENCED = 12,

  //Total ROB space that can be freed by completing the request
  TOTAL_ROB_SPACE_REQUESTOR = 13,
  
  //Total ROB space that can be freed by completing all requests to referenced page
  TOTAL_ROB_SPACE_REFERENCED = 14,

  //Track number of features
  NUM_FEATURE_TYPES = 15 
};

//CMDs used as integer input to RL
enum CMD_TYPE {
  //Read load miss
  READ_LD,
  //Read store miss
  READ_ST,
  //Write
  WRITE, 
  //Precharge bank
  PRECHARGE,
  //Activate row
  ACTIVATE, 
  //Do nothing
  NOP,
  //Preempt open page
  PREEMPT,
  //Track number of cmds
  NUM_CMD_TYPES
};
