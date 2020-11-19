#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <unistd.h> 

using namespace std;

void ctrlZHandler(int sig_num) {
	// TODO: Add your implementation
  SmallShell &smash = SmallShell::getInstance();
  cout << "smash: got ctrl-Z" <<endl;
  pid_t pid = smash.current_pid;
  //pid_t pid = SmallShell::getInstance().current_pid;
  if(pid != getpid()){
    kill(pid, SIGSTOP);
    cout << "process " << pid << " was stopped" <<endl;
    smash.jobList->addJob(smash.command, pid, true);
  }
}

void ctrlCHandler(int sig_num) {
  // TODO: Add your implementation
  pid_t pid = SmallShell::getInstance().current_pid;
  cout << "smash: got ctrl-C" <<endl;
  if(pid != getpid()){
    kill(pid, SIGKILL);
    cout << "process " << pid << " was killed" <<endl;
  }

  
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

