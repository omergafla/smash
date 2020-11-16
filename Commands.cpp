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

int _parseCommandLine(const char *cmd_line, char **args)
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

// KillCommand::KillCommand(const char *cmd_line, JobsList* jobs){
//   char **args = new char *[COMMAND_MAX_ARGS];
//   int result = _parseCommandLine(cmd_line, args);
// }

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, string plastPwd)
{
  char **args = new char *[COMMAND_MAX_ARGS];
  int result = _parseCommandLine(cmd_line, args);
  this->path = args[1];

  if (result == 2 && strcmp(args[1], "-") == 0 && plastPwd.empty())
  {
    this->is_error = true;
    this->err_message = "smash error: cd: OLDPWD not set";
  }
  else if (result > 2)
  {
    this->is_error = true;
    this->err_message = "smash error: cd: too many arguments";
  }
  else if (strcmp(args[1], "-") == 0)
  {
    this->path = plastPwd;
  }
  delete[] args;
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


SmallShell::SmallShell()
{
  // TODO: add your implementation
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
  // For example:
  /*
  string cmd_s = string(cmd_line);
  if (cmd_s.find("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  */


  char **args = new char *[COMMAND_MAX_ARGS];
  int result = _parseCommandLine(cmd_line, args);
  
  if (strcmp(args[0], "chprompt") == 0)
  {
    std::string name = "smash";
    if (result >= 2)
      name = args[1];
    delete[] args;
    return new ChPrompt(name);
  }

  if (strcmp(args[0], "ls") == 0)
  {
    delete[] args;
    return new LsCommand(cmd_line);
  }

  if (strcmp(args[0], "pwd") == 0 && result == 1)
  {

    delete[] args;
    return new GetCurrDirCommand(cmd_line);
  }


  if (strcmp(args[0], "showpid") == 0)
  {
    delete[] args;
    return new ShowPidCommand(cmd_line);
  }

  if (strcmp(args[0], "cd") == 0)
  {
    ChangeDirCommand *cd;
    char *buffer = new char();
    string curr = getcwd(buffer, COMMAND_ARGS_MAX_LENGTH);
    if (this->dirHistory.empty()){
      if(strcmp(args[1], "-") == 0){
        cd = new ChangeDirCommand(cmd_line, "");
      }
      else{
        cd = new ChangeDirCommand(cmd_line, curr);
        this->dirHistory.push(curr);
      }
    }
    else
    {
      string lastPwd = this->dirHistory.top();
      cd = new ChangeDirCommand(cmd_line, lastPwd);
      if(strcmp(args[1], "-") == 0){
        this->dirHistory.pop();
      }
      else{
        this->dirHistory.push(curr);
      }
      delete[] buffer;
    }
    delete[] args;
    return cd;
    }
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line)
{
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
  Command *cmd = CreateCommand(cmd_line);
  if (cmd == nullptr)
  {
    //Do something?
    return;
  }
  cmd->execute();

}

SmallShell::ChPrompt::ChPrompt(std::string name)
{
  this->newPromptName = name;
}

void SmallShell::ChPrompt::execute()
{
  SmallShell::getInstance().changePrompt(this->newPromptName);
}

}


void LsCommand::execute()
{
  cout << "hi \n";
}


void LsCommand::execute()
{
  DIR *dir;
  struct dirent *dp;
  char *file_name;
  dir = opendir(".");
  while((dp = readdir(dir)) != NULL){
    cout << dp->d_name <<endl;
  }
}

void GetCurrDirCommand::execute(){
  char * buffer = new char();
  cout << getcwd(buffer, COMMAND_ARGS_MAX_LENGTH) << "\n";
  delete[] buffer;
}