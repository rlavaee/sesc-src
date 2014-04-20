#ifndef RL_HH
#define RL_HH

#include "State.hh"
#include "CMAC.hh"

#include <list>
#include <vector>

class Tuple;
class State;

//Q-Learning
class QLearning {

  //Largest Bellman error of the current sweep
  double bellmanError;

  //Number of input features
  int numFeatures;
  //Number of actions
  int numActions;
  //Discount rate
  double delta;
  //Learning rate
  double alpha;

  //CMAC across state-action space
  CMAC *cmac;
  
  //Features selected for tiling 
  int *features;

public:
  
  //Reset bellman error at the beginning of each sweep
  void resetBellmanError() { bellmanError = 0; }

  //A single q-factor update based on <s,a,r,s',a'>
  void updateSarsa(Tuple *tuple);

  //Populate CMAC inputs with given state-action information
  void populateCMAC(State *s, int a);

  //Predict Q(s,a)
  double predictQFactor(State *state, int action);

  //Set feature array entry
  void setFeature(int index, int featureID) { features[index] = featureID; }

  //Constructor
  QLearning(int _numFeatures, int _numActions, double _delta, double _alpha, 
	    int _memorySize, int _numTilings, int _tableDimensionality);

};

#endif
