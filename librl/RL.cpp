#include "RL.hh"

#include <iostream>
#include <cmath>

//Perform a single q-factor update (sarsa)
void QLearning::updateSarsa(Tuple *tuple)
{
  //Get current state
  State *s = tuple->getCurrentState();

  //Get action
  int a = tuple->getCurrentAction();

  //Get reward
  double reward = tuple->getReward();

  //Get next state
  State *s_ = tuple->getNextState();
  
  //Get next action
  int a_ = tuple->getNextAction();

  //Get Q(s',a')
  populateCMAC(s_, a_);
  double nextQ = cmac->predict();
 
  //SARSA update
  populateCMAC(s,a);
  double oldQFactor = cmac->predict();
  cmac->update(reward + delta * nextQ);

}

//Predict Q(s,a)
double QLearning::predictQFactor(State *state, int action)
{
  //Populate CMAC inputs
  populateCMAC(state, action);
  
  //Obtain CMAC's prediction
  double q = cmac->predict();

  //Return CMAC's prediction
  return q;

}


//Initialize CMAC inputs based on given state-action information
void QLearning::populateCMAC(State *s, int a)
{  

  //Set state information
  for(int i=0; i<numFeatures; i++){
    cmac->setFloatInput(i, 1.0 * s->getStat(features[i]));
  }
  
  //Set action information
  cmac->setIntInput(0, a);

}
 

//Constructor
QLearning::QLearning(int _numFeatures, int _numActions, double _delta, double _alpha, 
		     int _memorySize, int _numTilings, int _tableDimensionality)
  : bellmanError(0), 
    numFeatures(_numFeatures),
    numActions(_numActions),
    delta(_delta),
    alpha(_alpha)
  
{
  //Allocate CMAC (2 int inputs for OR relationship, one for AND)
  //cmac = new CMAC(numFeatures, numActions, _memorySize, alpha, _numTilings);
  cmac = new CMAC(numFeatures, 2, _memorySize, alpha, _numTilings, _tableDimensionality);
  //cmac = new CMAC(numFeatures, 1, _memorySize, alpha, _numTilings, _numFeatures);

  //Allocate feature array
  features = new int[numFeatures];
}
 
