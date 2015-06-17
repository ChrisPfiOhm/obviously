#include <unistd.h>

#include "obcore/statemachine/Agent.h"
#include "obcore/statemachine/states/StatePing.h"

using namespace obvious;

int main(int argc, char* argv[])
{
  Agent* agent = new Agent(new StatePing());

  while(true)
  {
    usleep(100000);
    agent->awake();
  }

  delete agent;

  return 0;
}
