#include "StateBase.h"
#include <stddef.h>

namespace obvious
{

StateBase::StateBase()
{
  _agent = NULL;
}

StateBase::~StateBase()
{

}

void StateBase::setAgent(Agent* agent)
{
  _agent = agent;
}

} /* namespace obvious */

