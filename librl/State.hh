#ifndef STATE_HH
#define STATE_HH

#include "Enum.hh"

#include <list>
#include <vector>

//State information
class State {

  //Array that tracks stats
  double *stats;
  
public:

  //Set given stat to given value
  void setStat(int f, double val){ stats[f] = val; }

  //Get the value of given stats
  double getStat(int f) { return stats[f]; }

  //Constructor
  State();
};

// <s,a,r,s'> tuple for temporal difference methods
class Tuple {
  
  //Current state
  State *currentState;
  //Action taken 
  int currentAction;
  //Reward
  double reward;

  //Next state for sarsa updates
  State *nextState;
  //Next action for sarsa updates
  int nextAction;

public:
  
  //Set current state
  void setCurrentState(State *_currentState) { currentState = _currentState; }
  //Set current action
  void setCurrentAction(int _currentAction) { currentAction = _currentAction; }
  //Set reward
  void setReward(double _reward) { reward = _reward; }
  //Set next state
  void setNextState(State *_nextState) { nextState = _nextState; }
  //Set next action
  void setNextAction(int _nextAction) { nextAction = _nextAction; }

  //Get current state
  State *getCurrentState() { return currentState; }
  //Get current action
  int getCurrentAction() { return currentAction; }
  //Get reward
  double getReward() { return reward; }
  //Get next state
  State *getNextState() { return nextState; }
  //Get next action
  int getNextAction() { return nextAction; }

  //Constructor
  Tuple(State *_currentState, int _currentAction, double _reward, 
	State *_nextState, int _nextAction);
};

#endif
