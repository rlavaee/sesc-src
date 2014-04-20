#ifndef CMAC_HH
#define CMAC_HH

#include "State.hh"

//CMAC implementation
class CMAC {
  
  //Learning rate
  double alpha;

  //Input float vector 
  float *floatInputs;

  //Number of float inputs
  int numFloatInputs;

  //Input int vector
  int *intInputs;

  //Number of int inputs
  int numIntInputs;

  //Tile indices (after hashing)
  int *tiles;

  //CMAC array 
  double *u;

  //Size of CMAC array
  int memorySize;

  //Number of tilings (after hashing)
  int numTilings;

  //Dimensionality of distinct tables
  int tableDimensionality;
  
 public:
  
  //Predict target for current input vector
  double predict();

  //Predict target for current input vector using the OR relationship
  double predictOR();

  //Update CMAC w/ given target and current input vector
  void update(double target);

  //Update CMAC w/ given target and current input vector using the OR relationship
  void updateOR(double target);

  //Set float input array at given index to given value
  void setFloatInput(int index, float value) { floatInputs[index] = value; }

  //Set int input array at given index to given value
  void setIntInput(int index, int value) { intInputs[index] = value; }

  //Constructor
  CMAC(int _numFloatInputs, int _numIntInputs, int _memorySize, 
       double _alpha, int _numTilings, int _tableDimensionality);

};



#endif

