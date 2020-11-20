#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <stdio.h>
#include <vector>
#include <stack>
#include <map>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)

using namespace std;

class Command
{
    // TODO: Add your data members
public:
    const char *cmd_line;
    Command(){};
    Command(const char *cmd_line){};
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
    BuiltInCommand(const char *cmd_line)
    {
        this->cmd_line = cmd_line;
    };
    virtual ~BuiltInCommand() {};
};

class ExternalCommand : public Command
{
public:
    ExternalCommand(const char *cmd_line)
    {
        this->cmd_line = cmd_line;
    };
    virtual ~ExternalCommand(){};
    void execute() override{};
};

class PipeCommand : public Command
{
    // TODO: Add your data members
public:
    PipeCommand(const char *cmd_line)
    {
        this->cmd_line = cmd_line;
    };
    ;
    virtual ~PipeCommand() {};
    void execute() override;
};

class RedirectionCommand : public Command
{
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char *cmd_line)
    {
        this->cmd_line = cmd_line;
    };
    ;
    virtual ~RedirectionCommand() {};
    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand
{
    // TODO: Add your data members
    string err_message;
    bool is_error = false;
    string path;

public:
    ChangeDirCommand(const char *cmd_line, string plastPwd);
    virtual ~ChangeDirCommand() {};
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand
{
public:
    GetCurrDirCommand(const char *cmd_line)
    {
        this->cmd_line = cmd_line;
    };
    virtual ~GetCurrDirCommand() {};
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand
{
public:
    ShowPidCommand(const char *cmd_line)
    {
        this->cmd_line = cmd_line;
    };
    virtual ~ShowPidCommand() {};
    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand
{
public:
    JobsList *job_list_ref;
    bool if_kill;

    QuitCommand(const char *cmd_line, JobsList *jobs, bool kill);
    virtual ~QuitCommand() {};
    void execute() override;
};

class JobsList
{
public:
    class JobEntry
    {
    public:
        bool finished;
        int job_id;
        string command;
        pid_t process_id;
        time_t insertion_time;
        bool stopped;
        bool operator==(JobEntry &job)
        {
            if (job_id == job.job_id && command == job.command &&
                process_id == job.process_id && insertion_time == job.insertion_time && stopped == job.stopped)
                return true;
            else
                return false;
        }
        bool operator!=(JobEntry &job)
        {
            if (*this == job)
                return false;
            else
                return true;
        }
        void operator=(JobEntry *job)
        {
            this->command = job->command;
            this->finished = job->finished;
            this->insertion_time = job->insertion_time;
            this->job_id = job->job_id;
            this->process_id = job->process_id;
            this->stopped = job->stopped;
        }
    };
    // TODO: Add your data members
    //public:
    map<int, JobEntry *> *job_list;
    JobsList()
    {
        job_list = new map<int, JobEntry *>();
    };
    //JobsList(int process_id);
    ~JobsList();
    void addJob(Command *cmd, pid_t processId, bool isStopped = false);
    void printJobsList();
    void printJobsListForQuit();
    int getMaximalJobId();
    void killAllJobs();
    void removeFinishedJobs();
    void setFinished(int jobId);
    JobEntry *getJobById(int jobId);
    JobEntry *getJobByProcessId(int processId);
    void removeJobById(int jobId);
    JobEntry *getLastJob(int *lastJobId);
    JobEntry *getLastStoppedJob(int *jobId);

    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand
{
    JobsList *job_list_ref;

public:
    JobsCommand(const char *cmd_line, JobsList *jobs);
    virtual ~JobsCommand() {};
    void execute() override;
};

class KillCommand : public BuiltInCommand
{
    JobsList *job_list_ref;

public:
    KillCommand(const char *cmd_line, JobsList *jobs);
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand
{
    // TODO: Add your data members
    JobsList *joblist;

public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs)
    {
        this->cmd_line = cmd_line;
        this->joblist = jobs;
    };
    
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand
{
    // TODO: Add your data members
public:
    BackgroundCommand(const char *cmd_line, JobsList *jobs)
    {
        this->cmd_line = cmd_line;
    };
    
    virtual ~BackgroundCommand() {}
    void execute() override;
};

// TODO: add more classes if needed
// maybe ls, timeout ?

class LsCommand : public BuiltInCommand
{
public:
    LsCommand(const char *cmd_line)
    {
        this->cmd_line = cmd_line;
    };
    virtual ~LsCommand() {}
    void execute() override;
};

class ChPrompt : public BuiltInCommand
{
public:
    string newPromptName = "smash";
    ChPrompt(string newPromptName);
    virtual ~ChPrompt(){};
    void execute() override;
};

class SmallShell
{
private:
    SmallShell();

public:
    string promptName = "smash";
    JobsList *jobList;
    string previousDirectory = "";
    //stack<string> dirHistory;
    pid_t current_pid;
    Command *command;
    bool alive;
    Command *CreateCommand(const char *cmd_line);
    SmallShell(SmallShell const &) = delete;     // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance()             // make SmallShell singleton
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
