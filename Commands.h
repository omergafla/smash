#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <stdio.h>
#include <vector>
#include <stack>
#include <map>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)

using namespace std;


bool CheckBackground(const char* cmd_line);
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
    virtual ~BuiltInCommand(){};
};
class ExternalCommand : public Command
{
    pid_t pid;

public:
    ExternalCommand(const char *cmd_line)
    {
        this->cmd_line = cmd_line;
    };
    virtual ~ExternalCommand(){};
    void execute();
};

class PipeCommand : public Command
{
    bool is_amper;

public:
    PipeCommand(const char *cmd_line);
    virtual ~PipeCommand(){};
    void execute() override;
};

class RedirectionCommand : public Command
{

public:
    explicit RedirectionCommand(const char *cmd_line);
    virtual ~RedirectionCommand(){};
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
    virtual ~ChangeDirCommand(){};
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand
{
public:
    GetCurrDirCommand(const char *cmd_line)
    {
        this->cmd_line = cmd_line;
    };
    virtual ~GetCurrDirCommand(){};
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand
{
public:
    ShowPidCommand(const char *cmd_line)
    {
        this->cmd_line = cmd_line;
    };
    virtual ~ShowPidCommand(){};
    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand
{
public:
    JobsList *job_list_ref;
    bool if_kill;

    QuitCommand(const char *cmd_line, JobsList *jobs, bool kill);
    virtual ~QuitCommand(){};
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
    int getMaximalStoppedJobId();
    void killAllJobs();
    bool isEmpty();
    void removeFinishedJobs();
    void setFinished(int jobId);
    JobEntry *getJobById(int jobId);
    JobEntry *getJobByProcessId(int processId);
    void removeJobById(int jobId);
    JobEntry *getLastJob(int *lastJobId);
    JobEntry *getLastStoppedJob(int *jobId);
    void removeJobByProcessIdid(int processId);
};

class JobsCommand : public BuiltInCommand
{
    JobsList *job_list_ref;

public:
    JobsCommand(const char *cmd_line, JobsList *jobs);
    virtual ~JobsCommand(){};
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
    JobsList *joblist;

public:
    BackgroundCommand(const char *cmd_line, JobsList *jobs)
    {
        this->cmd_line = cmd_line;
        this->joblist = jobs;
    };

    virtual ~BackgroundCommand() {}
    void execute() override;
};

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

class TimeoutCommand : public Command
{

public:
    const char *cmd_line;
    TimeoutCommand(const char *cmd_line)
    {
        this->cmd_line = cmd_line;
    }
    virtual ~TimeoutCommand() {}
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

class CpCommand : public BuiltInCommand
{
public:
    string source;
    string destination;
    CpCommand(const char *cmd_line);
    virtual ~CpCommand(){};
    void execute() override;
};

class TimedList
{
public:
    class TimedEntry
    {
    public:
        time_t timestamp;
        int duration;
        string command;
        pid_t process_id;

        bool operator==(TimedEntry &timed)
        {
            if (this->timestamp = timed.timestamp && this->duration == timed.duration && this->command == timed.command && this->process_id == timed.process_id)
                return true;
            return false;
        }
        bool operator!=(TimedEntry &timed)
        {
            if (*this == timed)
                return false;
            else
                return true;
        }
        void operator=(TimedEntry *timed)
        {
            this->duration = timed->duration;
            this->command = timed->command;
            this->timestamp = timed->timestamp;
            this->process_id = timed->process_id;
        }
    };
    // TODO: Add your data members
    //public:
    map<int, TimedEntry *> *timed_list;
    TimedList()
    {
        timed_list = new map<int, TimedEntry *>();
    };
    //JobsList(int process_id);
    ~TimedList()
    {
        // for (auto it = this->timed_list->begin(); it != this->timed_list->end();)
        // {
        //     this->timed_list->erase(it++);
        //     ++it;
        // }
        this->timed_list->clear();
        delete this->timed_list;
    }
    void addTimed(string cmd, pid_t processId, time_t timestap, int duration)
    {
        TimedEntry *entry = new TimedEntry();
        entry->command = cmd;
        entry->timestamp = time(nullptr);
        entry->process_id = processId;
        entry->duration = duration;
        pair<int, TimedEntry *> _pair = make_pair(processId, entry);
        this->timed_list->insert(_pair);
    }
    void removeTimed(pid_t processId)
    {
        this->timed_list->erase(processId);
    }
};


class AlarmsList{
    public:
    class Alarm{
    public:
        time_t time_created;
        int duration;
        int scheduled_fire_time;
        bool operator==(Alarm &alarm)
        {
            if (this->time_created == alarm.time_created && this->duration == alarm.duration)
                return true;
            return false;
        }
        bool operator!=(Alarm &alarm)
        {
            if (*this == alarm)
                return false;
            else
                return true;
        }
        bool operator<(Alarm &alarm){
            return this->scheduled_fire_time < alarm.scheduled_fire_time;   
        }
        void operator=(Alarm *alarm)
        {
            this->duration = alarm->duration;
            this->time_created = alarm->time_created;
        }
    };
    
    vector<Alarm *> *alarms_list;
    AlarmsList()
    {
        alarms_list = new vector<Alarm *>();
    };
    //JobsList(int process_id);
    ~AlarmsList()
    {
        // for (auto it = this->alarms_list->begin(); it != this->alarms_list->end();)
        // {
        //     this->alarms_list->erase(it++);
        //     ++it;
        // }
        this->alarms_list->clear();
        delete this->alarms_list;
    };
    void fireAlarm();
    void addAlarm(time_t timestamp, int duration);
};
class SmallShell
{
private:
    SmallShell();

public:
    string promptName = "smash";
    JobsList *jobList;
    string previousDirectory = "";
    pid_t current_pid;
    pid_t smash_pid;
    Command *command;
    TimedList *timedList;
    AlarmsList *alarmList;
    bool alive;
    bool redirection = false;
    bool append = false;
    bool external = true;
    bool background = false;
    bool forked = false;
    bool piped = false;
    bool timeout = false;

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
