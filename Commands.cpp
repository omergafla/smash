#include <unistd.h>
//#include <stdio.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

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

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, char **plastPwd)
{
  char **args = new char *[COMMAND_MAX_ARGS];
  int result = _parseCommandLine(cmd_line, args);
  if (result == 2)
  {
    if (strcmp(args[1], "-") == 0)
    {
      hyphen = true;
    }
    if (strcmp(args[1], "..") == 0)
    {
      hyphen = true;
      string s(*plastPwd);
      //lastPwd = s;
    }
  }
  delete[] args;
}

void ChangeDirCommand::execute()
{
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


 if(strcmp(args[0],"chprompt")==0){
    std::string name = "smash";
    if(result>=2) name = args[1];
    delete [] args;
    return new ChPrompt(name);
 }

 if(strcmp(args[0],"ls")==0){
    return new LsCommand(cmd_line);
 }

 if(strcmp(args[0],"pwd") == 0 && result == 1){
    return new GetCurrDirCommand(cmd_line);
 }


  

  if (strcmp(args[0], "showpid") == 0)
  {
    return new ShowPidCommand(cmd_line);
  }

  if (strcmp(args[0], "cd") == 0)
  {
//    ChangeDirCommand cd = new ChangeDirCommand(cmd_line, )
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



void LsCommand::execute(){
  cout << "hi \n";
}

void GetCurrDirCommand::execute(){
  char * buffer = new char();
 cout << getcwd(buffer, COMMAND_ARGS_MAX_LENGTH) << "\n";
 delete [] buffer;
}
