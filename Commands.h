#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <stdio.h>
#include <vector>
#include <stack>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)

using namespace std;



class Command {
// TODO: Add your data members
 public:
 const char* cmd_line;
 Command(){};
  Command(const char* cmd_line){
  };

  virtual ~Command(){};
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command
{
public:
  BuiltInCommand(){};
  BuiltInCommand(const char *cmd_line);
  virtual ~BuiltInCommand() {}
};


class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line){};
  virtual ~ExternalCommand() {};
  void execute() override {};

};

class PipeCommand : public Command
{
  // TODO: Add your data members
public:
  PipeCommand(const char *cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command
{
  // TODO: Add your data members
public:
  explicit RedirectionCommand(const char *cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand
{
  // TODO: Add your data members
  string err_message;
  bool is_error;
  string path;

public:
  ChangeDirCommand(const char *cmd_line, string plastPwd);
  virtual ~ChangeDirCommand() {}
  void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand
{
public:
  GetCurrDirCommand(const char *cmd_line){};
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand
{
public:
  ShowPidCommand(const char *cmd_line){};
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand
{
  // TODO: Add your data members public:
  QuitCommand(const char *cmd_line, JobsList *jobs);
  virtual ~QuitCommand() {}
  void execute() override;
};

class JobsList
{
public:
  class JobEntry
  {
    public:
      int job_id;
      string command;
      int process_id;
      double seconds_elapsed;
      bool stopped;
      bool operator==(JobEntry &job)
      {
        if (job_id == job.job_id && command == job.command && 
        process_id == job.process_id && seconds_elapsed == job.seconds_elapsed 
        && stopped == job.stopped)
          return true;
        else
          return false;
      }
      bool operator!=(JobEntry &job)
      {
        if (*this == job) return false;
        else return true;
      }
  };
  // TODO: Add your data members
  //public:
  //std::list<JobEntry> *jobList;
  JobsList();
  ~JobsList();
  void addJob(Command *cmd, bool isStopped = false);
  void printJobsList();
  void killAllJobs();
  void removeFinishedJobs();
  JobEntry *getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry *getLastJob(int *lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand
{
  // TODO: Add your data members
public:
  JobsCommand(const char *cmd_line, JobsList *jobs);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand
{
  int pid;
  int job_id;
  int signal;

public:
  KillCommand(const char *cmd_line, JobsList *jobs);
  virtual ~KillCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand
{
  // TODO: Add your data members
public:
  ForegroundCommand(const char *cmd_line, JobsList *jobs);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand
{
  // TODO: Add your data members
public:
  BackgroundCommand(const char *cmd_line, JobsList *jobs);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

// TODO: add more classes if needed
// maybe ls, timeout ?

class LsCommand : public BuiltInCommand
{
public:
  LsCommand(const char *cmd_line){};
  virtual ~LsCommand() {}
  void execute() override;
};

class SmallShell
{
private:
  SmallShell();


 public:
    string promptName = "smash";
    JobsList * jobList;
    // string currentDirectory = ".";
    stack<string> dirHistory;
    class ChPrompt : public BuiltInCommand {
    public:
      std::string newPromptName = "smash";
      //ChPrompt(const char* cmd_line);
      ChPrompt(string newPromptName);
      virtual ~ChPrompt() {}
      void execute() override;
    };
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton

  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void changePrompt(std::string promptName)
  {
    this->promptName = promptName;
  }
  void executeCommand(const char *cmd_line);
  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
