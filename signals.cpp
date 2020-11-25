#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <unistd.h>

using namespace std;

void ctrlZHandler(int sig_num)
{
  // TODO: Add your implementation
  SmallShell &smash = SmallShell::getInstance();
  cout << "smash: got ctrl-Z" << endl;
  pid_t pid = smash.current_pid;
  //pid_t pid = SmallShell::getInstance().current_pid;
  if (pid != getpid())
  {
    kill(pid, SIGSTOP);
    cout << "process " << pid << " was stopped" << endl;
    smash.jobList->addJob(smash.command, pid, true);
  }
}

void ctrlCHandler(int sig_num)
{
  // TODO: Add your implementation
  pid_t pid = SmallShell::getInstance().current_pid;
  cout << "smash: got ctrl-C" << endl;
  if (pid != getpid())
  {
    kill(pid, SIGKILL);
    cout << "process " << pid << " was killed" << endl;
  }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
  SmallShell &smash = SmallShell::getInstance();
  cout << "smash: got an alarm" << endl;
  auto endit = smash.timedList->timed_list->end();
  for (auto it = smash.timedList->timed_list->begin(); it != smash.timedList->timed_list->end();)
    {
      if(smash.timedList->timed_list->empty()){
        break;
      }
      bool flag =it != smash.timedList->timed_list->end();
      // cout << it->first <<endl;
      // time_t t1 = time(nullptr);
      // time_t t2 = it->second->timestamp;
      // time_t t3 = t2+ it->second->duration;
      int dif = difftime(time(nullptr), it->second->timestamp+it->second->duration);
      bool time_diff = (dif >= 0);
      
        if (time_diff)
        {
          int pid = it->first;
            kill(it->first, SIGKILL);
            //cout << "smash: " << it->second->command << " timed out! ###pid: " << it->first << endl;
            smash.jobList->removeJobByProcessIdid(it->first);

            // if(smash.background){

            // }
            

        }
        ++it;
    }
}
