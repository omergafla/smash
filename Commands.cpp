#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <dirent.h>

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY() \
  cerr << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT() \
  cerr << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

#define DEBUG_PRINT cerr << "DEBUG: "

#define EXEC(path, arg) \
  execvp((path), (arg));

string _ltrim(const std::string &s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s)
{
  return _rtrim(_ltrim(s));
}
int _parseCommandLineChar(const char *cmd_line, char **args)
{
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for (std::string s; iss >> s;)
  {
    args[i] = (char *)malloc(s.length() + 1);
    memset(args[i], 0, s.length() + 1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}
int _parseCommandLine(const string cmd_line, vector<string> *args)
{
  //FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(cmd_line);
  for (std::string s; iss >> s;)
  {
    args->push_back(s);
    i++;
  }
  return i;

  //FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line)
{
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line)
{
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos)
  {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&')
  {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h

void ShowPidCommand::execute()
{
  cout << "smash pid is " << getpid() << endl;
}

//-------------------------JobsList----------------------------------
JobsList::JobsList()
{

}
JobsList::~JobsList()
{
}

void JobsList::addJob(Command *cmd, bool isStopped)
{
  JobEntry *job_entry = new JobEntry();
  vector<string> *args = new vector<string>();
  _parseCommandLine(cmd->cmd_line, args);
  job_entry->job_id = this->getMaximalJobId();
  job_entry->command = args->at(0);
  job_entry->process_id = this->processId;
  job_entry->stopped = isStopped;
  time(&(job_entry->insertion_time));
  pair<int, JobEntry *> _pair = make_pair(job_entry->job_id, job_entry);
  this->job_list->insert(_pair);
}

int JobsList::getMaximalJobId()
{
  int max = 0;
  auto it = this->job_list->begin();
  while (it != this->job_list->end())
  {
    if (it->first > max)
    {
      max = it->first;
    }
    it++;
  }
  return max;
}

void JobsList::printJobsList()
{
  int max = getMaximalJobId();
  auto it = this->job_list->begin();
  while (it != this->job_list->end())
  { //example of command: sleep 100&
    cout << "[" << it->second->job_id << "] " << it->second->command << " : "
         << it->second->process_id << difftime(time(nullptr), it->second->insertion_time)
         << " secs ";
    if (it->second->stopped)
    {
      cout << "(stopped)";
    }
    cout << endl;
    it++;
  }
}

void JobsList::killAllJobs()
{
  auto it = this->job_list->begin();
  while (it != this->job_list->end())
  { 
    kill(it->second->process_id, SIGKILL);
  }
}

void JobsList::removeFinishedJobs()
{
  auto it = this->job_list->begin();
  while (it != this->job_list->end())
  { 
    if(it->second->finished == true){
      this->job_list->erase(it);
    }
  }
}

JobsList::JobEntry *JobsList::getJobById(int jobId)
{
  auto it = this->job_list->find(jobId);
  return it->second;
}

void JobsList::removeJobById(int jobId)
{

}
JobsList::JobEntry *JobsList::getLastJob(int *lastJobId) {}
JobsList::JobEntry *JobsList::getLastStoppedJob(int *jobId) {}

//--------------------------- ChangeDirCommand ----------------------------------

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, string plastPwd)
{
  vector<string> *args = new vector<string>();
  int result = _parseCommandLine(cmd_line, args);
  this->path = args->at(1);

  if (result == 2 && args->at(1) == "-" && plastPwd.empty())
  {
    this->is_error = true;
    this->err_message = "smash error: cd: OLDPWD not set";
  }
  else if (result > 2)
  {
    this->is_error = true;
    this->err_message = "smash error: cd: too many arguments";
  }
  else if (args->at(1) == "-")
  {
    this->path = plastPwd;
  }

  delete args;
}

void ChangeDirCommand::execute()
{
  if (this->is_error == true)
  {
    cout << this->err_message << endl;
  }
  else
  {
    int result = chdir(path.c_str());
    if (result == -1)
    {
      perror("smash error: chdir failed");
    }
  }
}

//------------------------------- SmallShell ----------------------------------------------------

SmallShell::SmallShell()
{
  // TODO: add your implementation
  //this->jobList = new JobsList();
}

SmallShell::~SmallShell()
{
  // TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/

Command *SmallShell::CreateCommand(const char *cmd_line)
{
  vector<string> *args = new vector<string>();
  int result = _parseCommandLine(cmd_line, args);
  if(result == 0){
    return nullptr;
  }
  if (args->at(0) == "chprompt")
  {
    std::string name = "smash";
    if (result >= 2)
      name = args->at(1);
    delete args;
    return new ChPrompt(name);
  }

  if (args->at(0) == "ls")
  {
    delete args;
    return new LsCommand(cmd_line);
  }

  if (args->at(0) == "pwd" && result == 1)
  {

    delete args;
    return new GetCurrDirCommand(cmd_line);
  }

  if (args->at(0) == "showpid")
  {
    delete args;
    return new ShowPidCommand(cmd_line);
  }

  if (args->at(0) == "cd")
  {
    ChangeDirCommand *cd;
    char *buffer = new char[1024];
    string curr = getcwd(buffer, COMMAND_ARGS_MAX_LENGTH);
    if (this->dirHistory.empty())
    {
      if (args->at(1) == "-")
      {
        cd = new ChangeDirCommand(cmd_line, "");
      }
      else
      {
        cd = new ChangeDirCommand(cmd_line, curr);
        this->dirHistory.push(curr);
      }
    }
    else
    {
      string lastPwd = this->dirHistory.top();
      cd = new ChangeDirCommand(cmd_line, lastPwd);
      if (args->at(1) == "-")
      {
        this->dirHistory.pop();
      }
      else
      {
        this->dirHistory.push(curr);
      }
    }
    delete buffer;
    delete args;
    return cd;
  }

  else
  {
    delete args;
    return new ExternalCommand(cmd_line);
  }

  return nullptr;
}

const char **convertStringToChar(vector<string> *args)
{
  const char **args_new = new const char *[args->size()];
  for (int i = 0; i < args->size(); i++)
  {
    args_new[i] = args->at(i).c_str();
  }
  return args_new;
}

void SmallShell::executeCommand(const char *cmd_line)
{
  Command *cmd = CreateCommand(cmd_line);
  vector<string> *args = new vector<string>();
  char **args_char = new char *[COMMAND_MAX_ARGS];
  _parseCommandLineChar(cmd_line, args_char);
  int result = _parseCommandLine(cmd_line, args);
  if(result == 0){
    return;
  }
  bool external = true;
  string builtins[] = {"chprompt", "ls", "showpid", "pwd", "cd", "jobs", "kill", "fg", "bg", "quit"};
  for (int i = 0; i < 10; i++)
  {
    if (args->at(0) == builtins[i])
    {
      external = false;
      break;
    }
  }
  bool bg = false;
  if (args->at(result - 1) == "&")
  {
    bg = true;
    args_char[result - 1] = NULL;
  }

  if (cmd == nullptr)
  {
    //Do something?
    return;
  }

  if (external)
  {

    int status;
    pid_t pid = fork();
    if (pid < 0)
      perror("negative fork");
    else
    {
      if (pid == 0)
      { //child
        setpgrp();

        if (execvp(args->at(0).c_str(), args_char) == -1)
        {
          perror("something went wrong");
          kill(getpid(),SIGKILL);
        }
      }
      else
      {
        //parent
        if (!bg)
        {
          waitpid(pid, &status, 0);
        }
      }
    }
  }
  else
  {
    cmd->execute();
  }
}

SmallShell::ChPrompt::ChPrompt(std::string name)
{
  this->newPromptName = name;
}

void SmallShell::ChPrompt::execute()
{
  SmallShell::getInstance().changePrompt(this->newPromptName);
}

void LsCommand::execute()
{
  // DIR *dir;
  // struct dirent *dp;
  // char *file_name;
  // dir = opendir(".");
  // while((dp = readdir(dir)) != NULL){
  //   if ( !strcmp(dp->d_name, ".") || !strcmp(dp->d_name, "..") )

  // }
  struct dirent **namelist;
  int n;
  int i = 0;
  n = scandir(".", &namelist, NULL, alphasort);

  if (n > 0)
  {
    while (i < n)
    {
      char *current = namelist[i]->d_name;
      if (strcmp(current, ".") != 0 && strcmp(current, "..") != 0)
        cout << namelist[i]->d_name << endl;
      free(namelist[i]);
      i++;
    }
    free(namelist);
  }
}

void GetCurrDirCommand::execute()
{
  char *buffer = new char[1024];
  cout << getcwd(buffer, COMMAND_ARGS_MAX_LENGTH) << "\n";
  delete buffer;
}
