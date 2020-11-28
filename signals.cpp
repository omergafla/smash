#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <unistd.h>
#include <sys/wait.h>

using namespace std;
const std::string WHITESPACE = " \n\r\t\f\v";
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
    cout << "smash: process " << pid << " was stopped" << endl;
    smash.jobList->addJob(smash.command, pid, true);
    smash.current_pid = getpid();
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
    cout << "smash: process " << pid << " was killed" << endl;
  }
}

string _ltrim2(const std::string &s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim2(const std::string &s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim2(const std::string &s)
{
  return _rtrim2(_ltrim2(s));
}

void alarmHandler(int sig_num)
{
  SmallShell &smash = SmallShell::getInstance();
  cout << "smash: got an alarm" << endl;
  for (auto it = smash.timedList->timed_list->begin(); it != smash.timedList->timed_list->end();)
  {
    if (smash.timedList->timed_list->empty())
    {
      break;
    }
    int dif = difftime(time(nullptr), it->second->timestamp + it->second->duration);
    bool time_diff = (dif >= 0);
    int pid = it->first;
    if (time_diff)
    {
      int status;
      int returned_pid = waitpid(pid, &status, /*WCONTINUED | WUNTRACED | */WNOHANG);
      // bool is_exited = WIFEXITED(status);
      bool is_exited = WIFEXITED(status);
      // bool is_signaled = WIFSIGNALED(status);
      if (returned_pid != -1)
      {
        kill(it->first, SIGKILL);
        cout << "smash: timeout " << _trim2(it->second->command) << " timed out!" << endl;
        int pid2020 = it->first;
        smash.jobList->removeJobByProcessIdid(pid2020);
        it = smash.timedList->timed_list->erase(it);
        if (smash.timedList->timed_list->empty())
        {
          break;
        }
        if (it != smash.timedList->timed_list->begin())
          it--;
      }
        else
        {
          smash.timedList->timed_list->erase(it);
        }
        
      
    }
    ++it;
  }
  smash.alarmList->alarms_list->erase(smash.alarmList->alarms_list->begin());
  if (!smash.timedList->timed_list->empty())
  {
    AlarmsList::Alarm *next_alarm = smash.alarmList->alarms_list->at(0);
    if (next_alarm != nullptr)
    {
      int delta = next_alarm->scheduled_fire_time - time(nullptr);
      if (delta > 0)
        alarm(delta);
    }
  }
}