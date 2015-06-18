#include "Agent.h"
#include <string.h>
#include <iostream>

namespace obvious
{

unsigned int Agent::_AgentID = 0;

Agent::Agent(StateBase* initState)
{
  _ID = _AgentID++;
  initState->setAgent(this);
  _currentState = initState;
  _initialized = false;
}

Agent::~Agent()
{
  if(_currentState)
  {
    _currentState->onExit();
    delete _currentState;
  }
}

void Agent::awake()
{
  if(!_initialized)
  {
    // doEntry for the initial state cannot be called in constructor, since constructor of child class has not been passed at that time.
    // Variables set in constructor are not initialized.
    _currentState->onEntry();
    _initialized = true;
  }

  StateBase* nextState = _currentState->onActive();

  if(nextState)
  {
    _currentState->onExit();
    _currentState->onCleanup();
    _currentState = nextState;
    _currentState->onEntry();
  }
}

unsigned int Agent::getID()
{
  return _ID;
}

} // end namespace

