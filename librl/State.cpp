#include "State.hh"

#include <iostream>
#include <cmath>


//Constructor
State::State()
{
  //Allocate stats array
  stats = new double[NUM_FEATURE_TYPES];

  //Initialize stats array
  for(int i=0; i<NUM_FEATURE_TYPES; i++){
    stats[i] = 0;
  }
  
}

//Constructor
Tuple::Tuple(State *_currentState, int _currentAction, double _reward, 
	     State *_nextState, int _nextAction)
  :currentState(_currentState),
   currentAction(_currentAction),
   reward(_reward),
   nextState(_nextState),
   nextAction(_nextAction)
{
  //Nothing to do
}

